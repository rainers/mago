// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// _MagoNatCCService.cpp : Implementation of CMagoNatCCService

#include "stdafx.h"
#include "MagoNatCCService.h"
#include <dia2.h>
#include <msdbg.h>

#include "Common.h"
#include "EED.h"
#include "UniAlpha.h"
#include "Guard.h"

#include "../../CVSym/CVSym/CVSymPublic.h"
#include "../../CVSym/CVSym/Error.h"
#include "../../CVSym/CVSym/ISymbolInfo.h"

#include "../../CVSym/CVSTI/CVSTIPublic.h"
#include "../../CVSym/CVSTI/IDataSource.h"
#include "../../CVSym/CVSTI/ISession.h"
#include "../../CVSym/CVSTI/ImageAddrMap.h"

#include "../../DebugEngine/Include/MagoDECommon.h"
#include "../../DebugEngine/Include/WinPlat.h"
#include "../../DebugEngine/MagoNatDE/Utility.h"
#include "../../DebugEngine/MagoNatDE/ExprContext.h"
#include "../../DebugEngine/MagoNatDE/Property.h"
#include "../../DebugEngine/MagoNatDE/DRuntime.h"
#include "../../DebugEngine/MagoNatDE/RegisterSet.h"
#include "../../DebugEngine/MagoNatDE/ArchDataX64.h"
#include "../../DebugEngine/MagoNatDE/ArchDataX86.h"
#include "../../DebugEngine/MagoNatDE/EnumPropertyInfo.h"
#include "../../DebugEngine/MagoNatDE/FrameProperty.h"
#include "../../DebugEngine/MagoNatDE/CodeContext.h"
#include "../../DebugEngine/MagoNatDE/Module.h"
#include "../../DebugEngine/Exec/Types.h"
#include "../../DebugEngine/Exec/Exec.h"
#include "../../DebugEngine/Exec/Thread.h"
#include "../../DebugEngine/Exec/IProcess.h"
#include "../../DebugEngine/Exec/IModule.h"
#include "../../DebugEngine/MagoNatDE/IDebuggerProxy.h"
#include "../../DebugEngine/MagoNatDE/Thread.h"
#include "../../DebugEngine/MagoNatDE/Program.h"
#include "../../DebugEngine/MagoNatDE/LocalProcess.h"

///////////////////////////////////////////////////////////////////////////////
#define NOT_IMPL(x) virtual HRESULT x override { return E_NOTIMPL; }

#define tryHR(x) do { HRESULT _hr = (x); if(FAILED(_hr)) return _hr; } while(false)

Evaluation::IL::DkmILCallingConvention::e toCallingConvention(uint8_t callConv);
CComPtr<DkmString> toDkmString(const wchar_t* str);

// stub to read/write memory from process, all other functions supposed to not be called
class CCDebuggerProxy : public Mago::IDebuggerProxy
{
    RefPtr<DkmProcess> mProcess;

public:
    CCDebuggerProxy(DkmProcess* process)
        : mProcess(process)
    {
    }

    NOT_IMPL( Launch( LaunchInfo* launchInfo, Mago::ICoreProcess*& process ) );
    NOT_IMPL( Attach( uint32_t id, Mago::ICoreProcess*& process ) );

    NOT_IMPL( Terminate( Mago::ICoreProcess* process ) );
    NOT_IMPL( Detach( Mago::ICoreProcess* process ) );

    NOT_IMPL( ResumeLaunchedProcess( Mago::ICoreProcess* process ) );

    virtual HRESULT ReadMemory( 
        Mago::ICoreProcess* process, 
        Mago::Address64 address,
        uint32_t length, 
        uint32_t& lengthRead, 
        uint32_t& lengthUnreadable, 
        uint8_t* buffer )
    {
        // assume single process for now
        tryHR(mProcess->ReadMemory(address, DkmReadMemoryFlags::None, buffer, length, &lengthRead));
        lengthUnreadable = length - lengthRead;
        return S_OK;
    }

    virtual HRESULT WriteMemory( 
        Mago::ICoreProcess* process, 
        Mago::Address64 address,
        uint32_t length, 
        uint32_t& lengthWritten, 
        uint8_t* buffer )
    {
        // assume single process for now
        DkmArray<BYTE> arr = { buffer, length };
        tryHR(mProcess->WriteMemory(address, arr));
        lengthWritten = length;
        return S_OK;
    }

    NOT_IMPL( SetBreakpoint( Mago::ICoreProcess* process, Mago::Address64 address ) );
    NOT_IMPL( RemoveBreakpoint( Mago::ICoreProcess* process, Mago::Address64 address ) );

    NOT_IMPL( StepOut( Mago::ICoreProcess* process, Mago::Address64 targetAddr, bool handleException ) );
    NOT_IMPL( StepInstruction( Mago::ICoreProcess* process, bool stepIn, bool handleException ) );
    NOT_IMPL( StepRange( 
        Mago::ICoreProcess* process, bool stepIn, Mago::AddressRange64 range, bool handleException ) );

    NOT_IMPL( Continue( Mago::ICoreProcess* process, bool handleException ) );
    NOT_IMPL( Execute( Mago::ICoreProcess* process, bool handleException ) );

    NOT_IMPL( AsyncBreak( Mago::ICoreProcess* process ) );

    NOT_IMPL( GetThreadContext( Mago::ICoreProcess* process, Mago::ICoreThread* thread, Mago::IRegisterSet*& regSet ) );
    NOT_IMPL( SetThreadContext( Mago::ICoreProcess* process, Mago::ICoreThread* thread, Mago::IRegisterSet* regSet ) );

    NOT_IMPL( GetPData( 
        Mago::ICoreProcess* process, 
        Mago::Address64 address, 
        Mago::Address64 imageBase, 
        uint32_t size, 
        uint32_t& sizeRead, 
        uint8_t* pdata ) );
};

class CCCoreModule : public Mago::ICoreModule
{
public:
    CCCoreModule(DkmModuleInstance* modInst) : mModule(modInst) { }

    virtual void AddRef()
    {
        InterlockedIncrement(&mRefCount);
    }
    virtual void Release()
    {
        long newRef = InterlockedDecrement(&mRefCount);
        _ASSERT(newRef >= 0);
        if (newRef == 0)
            delete this;
    }

	// Program needs the image base
    virtual Mago::Address64 GetImageBase() { return mModule->BaseAddress(); }
    virtual Mago::Address64 GetPreferredImageBase() { return mModule->BaseAddress(); }
    virtual uint32_t        GetSize() { return mModule->Size(); }
    virtual uint16_t        GetMachine() { return 0; }
    virtual const wchar_t*  GetPath() { return mModule->FullName()->Value(); }
    virtual const wchar_t*  GetSymbolSearchPath() { return mModule->FullName()->Value(); }

private:
    long mRefCount = 0;

    RefPtr<DkmModuleInstance> mModule;
};

///////////////////////////////////////////////////////////////////////////////
template<typename I>
I* GetEnumTable(IDiaSession *pSession)
{
    I*              pUnknown    = NULL;
    REFIID          iid         = __uuidof(I);
    IDiaEnumTables* pEnumTables = NULL;
    IDiaTable*      pTable      = NULL;
    ULONG           celt        = 0;

    if (pSession->getEnumTables(&pEnumTables) != S_OK)
    {
        // wprintf(L"ERROR - GetTable() getEnumTables\n");
        return NULL;
    }
    while (pEnumTables->Next(1, &pTable, &celt) == S_OK && celt == 1)
    {
        // There is only one table that matches the given iid
        HRESULT hr = pTable->QueryInterface(iid, (void**)&pUnknown);
        pTable->Release();
        if (hr == S_OK)
            break;
    }
    pEnumTables->Release();
    return pUnknown;
}

///////////////////////////////////////////////////////////////////////////////
template<class T>
class CCDataItem : public IUnknown
{
public:
    virtual ~CCDataItem() {}

    // COM like ref counting
    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
        long newRef = InterlockedIncrement(&mRefCount);
        return newRef;
    }
    virtual ULONG STDMETHODCALLTYPE Release()
    {
        long newRef = InterlockedDecrement(&mRefCount);
        _ASSERT(newRef >= 0);
        if (newRef == 0)
            delete this;
        return newRef;
    }
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv)
    {
        if (riid == __uuidof(IUnknown))
        {
            *ppv = static_cast<IUnknown*>(this);
            AddRef();
            return S_OK;
        }
        else if (riid == __uuidof(T))
        {
            *ppv = static_cast<T*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }

private:
    long mRefCount = 0;
};

///////////////////////////////////////////////////////////////////////////////
static Mago::ProcFeaturesX86 toMago(DefaultPort::DkmProcessorFeatures::e features)
{
    int flags = Mago::PF_X86_None;
    if (features & DefaultPort::DkmProcessorFeatures::MMX)
        flags |= Mago::PF_X86_MMX;
    if (features & DefaultPort::DkmProcessorFeatures::SSE)
        flags |= Mago::PF_X86_SSE;
    if (features & DefaultPort::DkmProcessorFeatures::SSE2)
        flags |= Mago::PF_X86_SSE2;
    if (features & DefaultPort::DkmProcessorFeatures::AMD3DNow)
        flags |= Mago::PF_X86_3DNow;
    if (features & DefaultPort::DkmProcessorFeatures::AVX)
        flags |= Mago::PF_X86_AVX;
    if (features & DefaultPort::DkmProcessorFeatures::VFP32)
        flags |= Mago::PF_X86_None; // not an x86 feature

    return Mago::ProcFeaturesX86(flags);
}

// dia2.h has the wrong IID?
// {2F609EE1-D1C8-4E24-8288-3326BADCD211}
EXTERN_C const GUID DECLSPEC_SELECTANY guidIDiaSession =
{ 0x2F609EE1, 0xD1C8, 0x4E24, { 0x82, 0x88, 0x33, 0x26, 0xBA, 0xDC, 0xD2, 0x11 } };

class DECLSPEC_UUID("598DECC9-CF79-4E90-A408-5E1433B4DBFF") CCModule : public CCDataItem<CCModule>
{
    friend class CCExprContext;

protected:
    CCModule() : mDRuntime(nullptr), mDebuggerProxy(nullptr) {}
    ~CCModule() 
    {
        delete mDRuntime;
        delete mDebuggerProxy;
    }

    RefPtr<MagoST::IDataSource> mDataSource;
    RefPtr<MagoST::ISession> mSession;
    RefPtr<MagoST::ImageAddrMap> mAddrMap;
    RefPtr<Mago::ArchData> mArchData;
    RefPtr<Mago::IRegisterSet> mRegSet;
    RefPtr<Mago::Module> mModule;
    RefPtr<Mago::Program> mProgram;

    CCDebuggerProxy* mDebuggerProxy;
    Mago::DRuntime* mDRuntime;
    RefPtr<DkmProcess> mProcess;

    HRESULT Init(DkmProcess* process, Symbols::DkmModule* module)
    {
        auto system = process->SystemInformation();
        auto arch = system->ProcessorArchitecture();
        int ptrSize = arch == PROCESSOR_ARCHITECTURE_AMD64 ? 8 : 4;

        CComPtr<IDiaSession> diasession;
        if (module->GetSymbolInterface(guidIDiaSession, (IUnknown**)&diasession) != S_OK) // VS 2015
            if (HRESULT hr = module->GetSymbolInterface(__uuidof(IDiaSession), (IUnknown**)&diasession) != S_OK) // VS2013
                return hr;

        auto features = toMago(system->ProcessorFeatures());
        if (arch == PROCESSOR_ARCHITECTURE_AMD64)
            mArchData = new Mago::ArchDataX64(features);
        else
            mArchData = new Mago::ArchDataX86(features);

        CONTEXT_X64 context;
        tryHR(mArchData->BuildRegisterSet(&context, sizeof(context), mRegSet.Ref()));

        mDebuggerProxy = new CCDebuggerProxy(process);
        mProcess = process;
        mAddrMap = new MagoST::ImageAddrMap;
        tryHR(mAddrMap->LoadFromDiaSession(diasession));

        tryHR(MakeCComObject(mModule));
        tryHR(MagoST::MakeDataSource(mDataSource.Ref()));
        tryHR(mDataSource->InitDebugInfo(diasession, mAddrMap));
        tryHR(mDataSource->OpenSession(mSession.Ref()));
        mModule->SetSession(mSession);
        DkmArray<Microsoft::VisualStudio::Debugger::DkmModuleInstance*> pModules = { 0 };
        module->GetModuleInstances(&pModules);
        if(pModules.Length > 0)
        {
            mSession->SetLoadAddress(pModules.Members[0]->BaseAddress());
            mModule->SetCoreModule(new CCCoreModule(pModules.Members[0]));
        }
        DkmFreeArray(pModules);

        tryHR(MakeCComObject(mProgram));

        // any process to pass architecture info
        RefPtr<Mago::LocalProcess> localprocess = new Mago::LocalProcess(mArchData);
        mProgram->SetCoreProcess(localprocess);
        mProgram->SetDebuggerProxy(mDebuggerProxy);
        mProgram->AddModule(mModule);
        UniquePtr<Mago::DRuntime> druntime(new Mago::DRuntime(mDebuggerProxy, localprocess));
        mProgram->SetDRuntime(druntime);

        return S_OK;
    }

    HRESULT FindFunction(uint32_t rva, MagoST::SymHandle& funcSH, std::vector<MagoST::SymHandle>& blockSH)
    {
        uint32_t    offset = 0;
        uint16_t    sec = mSession->GetSecOffsetFromRVA( rva, offset );
        if ( sec == 0 )
            return E_NOT_FOUND;

        // TODO: verify that it's a function or public symbol (or something else?)
        DWORD symOff = 0;
        HRESULT hr = mSession->FindOuterSymbolByAddr( MagoST::SymHeap_GlobalSymbols, sec, offset, funcSH, symOff );
        if ( FAILED( hr ) )
            hr = mSession->FindOuterSymbolByAddr( MagoST::SymHeap_StaticSymbols, sec, offset, funcSH, symOff);
        if ( FAILED( hr ) )
            hr = mSession->FindOuterSymbolByAddr( MagoST::SymHeap_PublicSymbols, sec, offset, funcSH, symOff);
        if ( FAILED( hr ) )
            return hr;

        hr = mSession->FindInnermostSymbol( funcSH, sec, offset, blockSH );
        // it might be a public symbol, which doesn't have anything: blocks, args, or locals

        return S_OK;
    }

public:
    virtual HRESULT ReadMemory(MagoEE::Address addr, uint32_t sizeToRead, uint32_t& sizeRead, uint8_t* buffer)
    {
        return mProcess->ReadMemory(addr, DkmReadMemoryFlags::None, buffer, sizeToRead, &sizeRead);
    }
};

///////////////////////////////////////////////////////////////////////////////
class CCExprContext : public Mago::ExprContext
{
    RefPtr<CallStack::DkmStackWalkFrame> mStackFrame;
    RefPtr<CCModule> mModule;
    RefPtr<Mago::Thread> mThread;

public:
    CCExprContext() {}
    ~CCExprContext() 
    {
    }
    
    HRESULT Init(DkmProcess* process, CallStack::DkmStackWalkFrame* frame)
    {
        HRESULT hr;
        CComPtr<Symbols::DkmInstructionSymbol> pInstruction;
        hr = frame->GetInstructionSymbol(&pInstruction);
        if (hr != S_OK)
          return hr;

        auto nativeInst = Native::DkmNativeInstructionSymbol::TryCast(pInstruction);
        if (!nativeInst)
          return S_FALSE;

        Symbols::DkmModule* module = pInstruction->Module();
        if (!module)
          return S_FALSE;

        // Restore/Create CCModule in DkmModule
        hr = module->GetDataItem(&mModule.Ref());
        if (!mModule)
        {
            mModule = new CCModule; 
            tryHR(mModule->Init(process, module));
            tryHR(module->SetDataItem(DkmDataCreationDisposition::CreateAlways, mModule.Get()));
        }

        mStackFrame = frame;

        uint32_t rva = nativeInst->RVA();
        MagoST::SymHandle funcSH;
        std::vector<MagoST::SymHandle> blockSH;
        tryHR(mModule->FindFunction(rva, funcSH, blockSH));

        // setup a fake environment for Mago
        tryHR(MakeCComObject(mThread));
        mThread->SetProgram(mModule->mProgram, mModule->mDebuggerProxy);

        return ExprContext::Init(mModule->mModule, mThread, funcSH, blockSH, /*image-base+*/rva, mModule->mRegSet);
    }

    virtual HRESULT GetSession(MagoST::ISession*& session)
    {
        _ASSERT(session == NULL);
        session = mModule->mSession;
        session->AddRef();
        return S_OK;
    }

    virtual HRESULT GetRegValue(DWORD reg, MagoEE::DataValueKind& kind, MagoEE::DataValue& value)
    {
        if (auto regs = mStackFrame->Registers())
        {
            UINT32 read;
            Mago::RegisterValue regVal = { 0 };
            if(regs->GetRegisterValue(reg, &regVal.Value, sizeof (regVal.Value), &read) != S_OK)
                return E_MAGOEE_BADREGISTER;

            int archRegId = mModule->mArchData->GetArchRegId( reg );
            if ( archRegId < 0 )
                return E_MAGOEE_BADREGISTER;

            regVal.Type = mModule->mRegSet->GetRegisterType( archRegId );
            switch ( regVal.Type )
            {
            case Mago::RegType_Int8:  value.UInt64Value = regVal.Value.I8;    kind = MagoEE::DataValueKind_UInt64;    break;
            case Mago::RegType_Int16: value.UInt64Value = regVal.Value.I16;   kind = MagoEE::DataValueKind_UInt64;    break;
            case Mago::RegType_Int32: value.UInt64Value = regVal.Value.I32;   kind = MagoEE::DataValueKind_UInt64;    break;
            case Mago::RegType_Int64: value.UInt64Value = regVal.Value.I64;   kind = MagoEE::DataValueKind_UInt64;    break;

            case Mago::RegType_Float32:
                value.Float80Value.FromFloat( regVal.Value.F32 );
                kind = MagoEE::DataValueKind_Float80;
                break;

            case Mago::RegType_Float64:
                value.Float80Value.FromDouble( regVal.Value.F64 );
                kind = MagoEE::DataValueKind_Float80;
                break;

            case Mago::RegType_Float80:   
                memcpy( &value.Float80Value, regVal.Value.F80Bytes, sizeof value.Float80Value );
                kind = MagoEE::DataValueKind_Float80;
                break;

            default:
                return E_MAGOEE_BADREGISTER;
            }
            return S_OK;
        }
        return E_FAIL;
    }

    virtual Mago::Address64 GetTebBase()
    {
        return mStackFrame->Thread()->TebAddress();
    }

    virtual HRESULT SymbolFromAddr(MagoEE::Address addr, std::wstring& symName, MagoEE::Type** pType)
    {
        HRESULT hr = ExprContext::SymbolFromAddr(addr, symName, pType);
        if (FAILED(hr))
            return hr;
        if (symName.length() > 0 && symName[0] == '?')
        {
            // Undecorate C++ symbols
            CComPtr<Symbols::DkmInstructionSymbol> pInstruction;
            if (mStackFrame->GetInstructionSymbol(&pInstruction) == S_OK)
            {
                if (Symbols::DkmModule* module = pInstruction->Module())
                {
                    RefPtr<DkmString> undec;
                    if (module->UndecorateName(toDkmString (symName.c_str()), 0x7ff, &undec.Ref()) == S_OK)
                    {
                        symName = undec->Value();
                    }
                }
            }
        }
        return S_OK;
    }

    virtual HRESULT CallFunction(MagoEE::Address addr, MagoEE::ITypeFunction* func, MagoEE::Address arg, MagoEE::DataObject& obj)
    {
        using namespace Evaluation::IL;

        int ptrSize = mModule->mArchData->GetPointerSize();
        DkmILInstruction* instructions[8];
        int cntInstr = 0;

        // push function address
        RefPtr<DkmReadOnlyCollection<BYTE>> paddrfn;
        tryHR(DkmReadOnlyCollection<BYTE>::Create((BYTE*)&addr, ptrSize, &paddrfn.Ref()));
        RefPtr<DkmILPushConstant> ppushfn;
        tryHR(DkmILPushConstant::Create(paddrfn, &ppushfn.Ref()));
        instructions[cntInstr++] = ppushfn;

        // push argument (context pointer)
        RefPtr<DkmReadOnlyCollection<BYTE>> parg;
        tryHR(DkmReadOnlyCollection<BYTE>::Create((BYTE*)&arg, ptrSize, &parg.Ref()));
        RefPtr<DkmILPushConstant> ppusharg;
        tryHR(DkmILPushConstant::Create(parg, &ppusharg.Ref()));
        instructions[cntInstr++] = ppusharg;

        // call function
        UINT32 ArgumentCount = 1;
        UINT32 ReturnValueSize = obj._Type->GetSize();
        bool returnsDXAX = obj._Type->IsDArray() || obj._Type->IsDelegate();
        if (returnsDXAX)
            ReturnValueSize = ptrSize;
        uint8_t callConv = func->GetCallConv();
        DkmILCallingConvention::e CallingConvention = ptrSize == 4 ? toCallingConvention(callConv) : DkmILCallingConvention::StdCall;
        if (CallingConvention == DkmILCallingConvention::e(-1))
            return E_MAGOEE_BADCALLCONV;
        DkmILFunctionEvaluationFlags::e Flags = DkmILFunctionEvaluationFlags::HasThisPointer;
        if (obj._Type->IsFloatingPoint())
            Flags |= DkmILFunctionEvaluationFlags::FloatingPointReturn;

        DkmILFunctionEvaluationArgumentFlags::e argFlag = DkmILFunctionEvaluationArgumentFlags::ThisPointer;
        RefPtr<DkmReadOnlyCollection<DkmILFunctionEvaluationArgumentFlags::e>> argFlags;
        tryHR(DkmReadOnlyCollection<DkmILFunctionEvaluationArgumentFlags::e>::Create(&argFlag, 1, &argFlags.Ref()));
        UINT32 UniformComplexReturnElementSize = 0;

        RefPtr<DkmILExecuteFunction> pcall;
        tryHR(DkmILExecuteFunction::Create(ArgumentCount, ReturnValueSize, CallingConvention, Flags, argFlags, 0, &pcall.Ref()));
        instructions[cntInstr++] = pcall;

        RefPtr<DkmILRegisterRead> pregreaddx;
        RefPtr<DkmILReturnTop> pregretax;
        if (returnsDXAX)
        {
            // TODO: does not work yet, EDX/RDX not the function return value
            tryHR(DkmILReturnTop::Create(&pregretax.Ref()));
            instructions[cntInstr++] = pregretax;

            tryHR(DkmILRegisterRead::Create(ptrSize == 4 ? CV_REG_EDX : CV_AMD64_RDX, &pregreaddx.Ref()));
            instructions[cntInstr++] = pregreaddx;
        }

        // return top of stack
        RefPtr<DkmILReturnTop> preturn;
        tryHR(DkmILReturnTop::Create(&preturn.Ref()));
        instructions[cntInstr++] = preturn;

        // build instruction list
        RefPtr<DkmReadOnlyCollection<DkmILInstruction*>> pinstr;
        tryHR(DkmReadOnlyCollection<DkmILInstruction*>::Create(instructions, cntInstr, &pinstr.Ref()));

        // run instructions
        RefPtr<DkmCompiledILInspectionQuery> pquery;
        tryHR(DkmCompiledILInspectionQuery::Create(mStackFrame->RuntimeInstance(), pinstr, &pquery.Ref()));

        RefPtr<Evaluation::DkmILContext> pcontext;
        tryHR(Evaluation::DkmILContext::Create(mStackFrame, nullptr, &pcontext.Ref()));

        DkmILEvaluationResult* results[1] = { nullptr };
        DkmArray<DkmILEvaluationResult*> arrResults;
        DkmILFailureReason::e failureReason;
        HRESULT hr = pquery->Execute(nullptr, pcontext, 1000, Evaluation::DkmFuncEvalFlags::None, &arrResults, &failureReason);

        if (!FAILED(hr))
        {
            if (arrResults.Length > 0 && arrResults.Members[0])
            {
                DkmReadOnlyCollection<BYTE>* res = arrResults.Members[0]->ResultBytes();
                if (returnsDXAX)
                {
                    if (returnsDXAX && res && res->Count() == ptrSize)
                    {
                        uint8_t buf[16]; // enough for two pointers
                        memcpy(buf, res->Items(), ptrSize);
                        if (arrResults.Length > 1 && arrResults.Members[1])
                        {
                            res = arrResults.Members[1]->ResultBytes();
                            if (res && res->Count() == ptrSize)
                            {
                                memcpy(buf + ptrSize, res->Items(), ptrSize);
                                hr = FromRawValue(buf, obj._Type, obj.Value);
                                hr = S_OK;
                            }
                        }
                    }
                }
                else if (res && res->Count() == ReturnValueSize)
                {
                    if (ReturnValueSize <= sizeof(MagoEE::DataValue))
                    {
                        hr = FromRawValue(res->Items(), obj._Type, obj.Value);
                        hr = S_OK;
                    }
                }
            }
            else
                hr = ReturnValueSize == 0 ? S_OK : E_FAIL;
        }
        DkmFreeArray(arrResults);
        return hr;
    }

	bool returnInRegister(MagoEE::Type* type)
	{
		if (type->IsSArray())
			return false;

		int ptrSize = mModule->mArchData->GetPointerSize();
		if (type->GetSize() > ptrSize)
			return false;

		MagoEE::ITypeStruct* struc = type->AsTypeStruct();
		if (!struc)
			return true;

		return struc->IsPOD();
	}

	HRESULT EvalReturnValue(Evaluation::DkmInspectionContext* pInspectionContext, 
	                        Evaluation::DkmNativeRawReturnValue* pNativeRetValue, DEBUG_PROPERTY_INFO& info)
	{
		std::wstring funcName;
		DkmInstructionAddress* funcAddr = pNativeRetValue->ReturnFrom();
		auto cpuinfo = funcAddr->CPUInstructionPart();
		if (!cpuinfo)
			return E_INVALIDARG;

		MagoST::SymHandle funcSH;
		std::vector<MagoST::SymHandle> blockSH;
		uint64_t rva = cpuinfo->InstructionPointer;
		tryHR(mModule->FindFunction(rva, funcSH, blockSH));

		RefPtr<MagoEE::Type> type;
		tryHR(SymbolFromAddr(rva, funcName, &type.Ref()));
		funcName.append(L"()");

		MagoEE::ITypeFunction* func = type->AsTypeFunction();
		if (!func)
			return E_INVALIDARG;
		auto retType = func->GetReturnType();
		if (!retType || retType->GetBackingTy() == MagoEE::Tvoid)
			return E_INVALIDARG; // do not show void function return

		std::wstring funcType;
		retType->ToString(funcType);

		MagoEE::EvalResult value = { 0 };
		uint8_t buf[16]; // enough for two pointers
		const uint8_t* pbuf = buf;
		UINT32 ReturnValueSize = retType->GetSize();
		bool returnsDXAX = retType->IsDArray() || retType->IsDelegate();
		int ptrSize = mModule->mArchData->GetPointerSize();
		if (returnsDXAX || returnInRegister(retType))
		{
			auto regs = pNativeRetValue->Registers();
			auto getReg = [regs](CV_HREG_e reg, uint8_t* pbuf) -> bool
			{
				DWORD cnt = regs->Count();
				for (DWORD i = 0; i < cnt; i++)
					if (regs->Items()[i]->Identifier() == reg)
						if (auto bytes = regs->Items()[i]->Value())
						{
							memcpy(pbuf, bytes->Items(), bytes->Count());
							return true;
						}
				return false;
			};
			if (retType->IsFloatingPoint())
			{
				if (!getReg(ptrSize > 4 ? CV_REG_XMM0 : CV_REG_ST0, buf))
					return E_INVALIDARG;
			}
			else
			{
				if (!getReg(ptrSize > 4 ? CV_AMD64_RAX : CV_REG_EAX, buf))
					return E_INVALIDARG;
			}
			if (returnsDXAX && !getReg(ptrSize > 4 ? CV_AMD64_RDX : CV_REG_EDX, buf + ptrSize))
				return E_INVALIDARG;
		}
		else if (auto mem = pNativeRetValue->Memory())
		{
			if (mem->Count() < ReturnValueSize)
				return E_INVALIDARG;
			pbuf = mem->Items();
		}
		else
			return E_INVALIDARG;
		value.ObjVal._Type = retType;

		MagoEE::FormatOptions fmtopt;
		fmtopt.radix = pInspectionContext->Radix();
		uint32_t maxLength = MagoEE::kMaxFormatValueLength;
		std::wstring valStr;

		if (retType->AsTypeStruct())
		{
			tryHR(FormatRawStructValue(this, pbuf, retType, fmtopt, valStr, maxLength));
		}
		else
		{
			tryHR(FromRawValue(pbuf, retType, value.ObjVal.Value));
			tryHR(FormatValue(this, value.ObjVal, fmtopt, valStr, maxLength));
		}

		MagoEE::FillValueTraits(value, nullptr);

		RefPtr<Mago::Property> pProperty;
		tryHR(MakeCComObject(pProperty));
		tryHR(pProperty->Init(funcName.c_str(), funcName.c_str(), value, this, fmtopt));

		info.bstrName = SysAllocString(funcName.c_str());
		info.bstrFullName = SysAllocString(funcName.c_str());
		info.bstrValue = SysAllocString(valStr.c_str());
		info.bstrType = SysAllocString(funcType.c_str());
		return S_OK;
	}

    Mago::IRegisterSet* getRegSet() { return mModule->mRegSet; }
};

///////////////////////////////////////////////////////////////////////////////
CComPtr<DkmString> toDkmString(const wchar_t* str)
{
    CComPtr<DkmString> pString;
    HRESULT hr = DkmString::Create(str, &pString);
    _ASSERT(hr == S_OK);
    return pString;
}

///////////////////////////////////////////////////////////////////////////////
Evaluation::DkmEvaluationResultFlags::e toResultFlags(DBG_ATTRIB_FLAGS attrib)
{
    Evaluation::DkmEvaluationResultFlags::e resultFlags = Evaluation::DkmEvaluationResultFlags::None;
    if ( attrib & DBG_ATTRIB_VALUE_RAW_STRING )
        resultFlags |= Evaluation::DkmEvaluationResultFlags::RawString;
    if ( attrib & DBG_ATTRIB_VALUE_READONLY )
        resultFlags |= Evaluation::DkmEvaluationResultFlags::ReadOnly;
    if ( attrib & DBG_ATTRIB_OBJ_IS_EXPANDABLE )
        resultFlags |= Evaluation::DkmEvaluationResultFlags::Expandable;
    return resultFlags;
}

Evaluation::DkmEvaluationResultAccessType::e toResultAccess(DBG_ATTRIB_FLAGS attrib)
{
    Evaluation::DkmEvaluationResultAccessType::e resultAccess = Evaluation::DkmEvaluationResultAccessType::None;
    if ( attrib & DBG_ATTRIB_ACCESS_PUBLIC )
        resultAccess = Evaluation::DkmEvaluationResultAccessType::Public;
    else if ( attrib & DBG_ATTRIB_ACCESS_PRIVATE )
        resultAccess = Evaluation::DkmEvaluationResultAccessType::Private;
    else if ( attrib & DBG_ATTRIB_ACCESS_PROTECTED )
        resultAccess = Evaluation::DkmEvaluationResultAccessType::Protected;
    else if ( attrib & DBG_ATTRIB_ACCESS_FINAL )
        resultAccess = Evaluation::DkmEvaluationResultAccessType::Final;
    return resultAccess;
}

Evaluation::DkmEvaluationResultStorageType::e toResultStorage(DBG_ATTRIB_FLAGS attrib)
{
    Evaluation::DkmEvaluationResultStorageType::e resultStorage = Evaluation::DkmEvaluationResultStorageType::None;
    if ( attrib & DBG_ATTRIB_STORAGE_GLOBAL )
        resultStorage = Evaluation::DkmEvaluationResultStorageType::Global;
    else if ( attrib & DBG_ATTRIB_STORAGE_STATIC )
        resultStorage = Evaluation::DkmEvaluationResultStorageType::Static;
    else if ( attrib & DBG_ATTRIB_STORAGE_REGISTER )
        resultStorage = Evaluation::DkmEvaluationResultStorageType::Register;
    return resultStorage;
}

Evaluation::DkmEvaluationResultCategory::e toResultCategory(DBG_ATTRIB_FLAGS attrib)
{
    Evaluation::DkmEvaluationResultCategory::e cat = Evaluation::DkmEvaluationResultCategory::Other;
    if (attrib & DBG_ATTRIB_DATA)
        cat = Evaluation::DkmEvaluationResultCategory::Data;
    else if (attrib & DBG_ATTRIB_METHOD)
        cat = Evaluation::DkmEvaluationResultCategory::Method;
    else if (attrib & DBG_ATTRIB_PROPERTY)
        cat = Evaluation::DkmEvaluationResultCategory::Property;
    else if (attrib & DBG_ATTRIB_CLASS)
        cat = Evaluation::DkmEvaluationResultCategory::Class;
    else if (attrib & DBG_ATTRIB_BASECLASS)
        cat = Evaluation::DkmEvaluationResultCategory::BaseClass;
    else if (attrib & DBG_ATTRIB_INTERFACE)
        cat = Evaluation::DkmEvaluationResultCategory::Interface;
    else if (attrib & DBG_ATTRIB_INNERCLASS)
        cat = Evaluation::DkmEvaluationResultCategory::InnerClass;
    else if (attrib & DBG_ATTRIB_MOSTDERIVEDCLASS)
        cat = Evaluation::DkmEvaluationResultCategory::MostDerivedClass;
    return cat;
}

Evaluation::DkmEvaluationResultTypeModifierFlags::e toResultModifierFlags(DBG_ATTRIB_FLAGS attrib)
{
    Evaluation::DkmEvaluationResultTypeModifierFlags::e resultFlags = Evaluation::DkmEvaluationResultTypeModifierFlags::None;
    if ( attrib & DBG_ATTRIB_TYPE_VIRTUAL )
        resultFlags |= Evaluation::DkmEvaluationResultTypeModifierFlags::Virtual;
    if ( attrib & DBG_ATTRIB_TYPE_CONSTANT )
        resultFlags |= Evaluation::DkmEvaluationResultTypeModifierFlags::Constant;
    if ( attrib & DBG_ATTRIB_TYPE_SYNCHRONIZED )
        resultFlags |= Evaluation::DkmEvaluationResultTypeModifierFlags::Synchronized;
    if ( attrib & DBG_ATTRIB_TYPE_VOLATILE )
        resultFlags |= Evaluation::DkmEvaluationResultTypeModifierFlags::Volatile;
    return resultFlags;
}

Evaluation::IL::DkmILCallingConvention::e toCallingConvention(uint8_t callConv)
{
    switch (callConv)
    {
    case CV_CALL_NEAR_C:
        return Evaluation::IL::DkmILCallingConvention::CDecl;
    case CV_CALL_NEAR_PASCAL: // reverse order of args, so ok up to 1 arg
    case CV_CALL_NEAR_STD:
        return Evaluation::IL::DkmILCallingConvention::StdCall;
    case CV_CALL_THISCALL:
        return Evaluation::IL::DkmILCallingConvention::ThisCall;
    case CV_CALL_NEAR_FAST:
        return Evaluation::IL::DkmILCallingConvention::FastCall;
    }
    return Evaluation::IL::DkmILCallingConvention::e (-1);
}

HRESULT createEvaluationResult(Evaluation::DkmInspectionContext* pInspectionContext, CallStack::DkmStackWalkFrame* pStackFrame,
                               DEBUG_PROPERTY_INFO& info, Evaluation::DkmSuccessEvaluationResult** ppResultObject)
{
    HRESULT hr = E_FAIL;
    CComPtr<Evaluation::DkmDataAddress> dataAddr;
    CComPtr<IDebugMemoryContext2> memctx;
    if (info.pProperty)
    {
        hr = info.pProperty->GetMemoryContext(&memctx);
        if(hr == S_OK) // doesn't fail for properties without context
        {
            CComPtr<Mago::IMagoMemoryContext> magoCtx;
            hr = memctx->QueryInterface(&magoCtx);
            if(SUCCEEDED(hr))
            {
                UINT64 addr = 0;
                tryHR(magoCtx->GetAddress(addr));
                // always create an instruction address, too
                CComPtr<DkmInstructionAddress> instrAddr;
                pStackFrame->Process()->CreateNativeInstructionAddress(addr, &instrAddr);
                Evaluation::DkmDataAddress::Create(pInspectionContext->RuntimeInstance(), addr, instrAddr, &dataAddr);
            }
        }
    }
    Evaluation::DkmEvaluationResultFlags::e resultFlags = toResultFlags(info.dwAttrib);
    if (dataAddr)
        resultFlags |= Evaluation::DkmEvaluationResultFlags::Address;

    hr = Evaluation::DkmSuccessEvaluationResult::Create(
        pInspectionContext, pStackFrame, 
        toDkmString(info.bstrName), 
        toDkmString(info.bstrFullName),
        resultFlags,
        toDkmString(info.bstrValue), // display value
        toDkmString(info.bstrValue), // editable value
        toDkmString(info.bstrType),
        toResultCategory(info.dwAttrib), 
        toResultAccess(info.dwAttrib), 
        toResultStorage(info.dwAttrib),
        toResultModifierFlags(info.dwAttrib),
        dataAddr, // address
        nullptr, // UI visiualizers
        nullptr, // external modules
        info.pProperty ? DkmDataItem(info.pProperty, __uuidof(Mago::Property)) : DkmDataItem::Null(),
        ppResultObject);
    return hr;
}

HRESULT createEvaluationError(Evaluation::DkmInspectionContext* pInspectionContext, CallStack::DkmStackWalkFrame* pStackFrame,
                              HRESULT hrErr, Evaluation::DkmLanguageExpression* expr,
                              IDkmCompletionRoutine<Evaluation::DkmEvaluateExpressionAsyncResult>* pCompletionRoutine)
{
    std::wstring errStr;
    Evaluation::DkmFailedEvaluationResult* pResultObject = nullptr;

    if (MagoEE::GetErrorString(hrErr, errStr) != S_OK)
        MagoEE::GetErrorString(E_MAGOEE_BASE, errStr);

    tryHR(Evaluation::DkmFailedEvaluationResult::Create(
        pInspectionContext, pStackFrame, 
        expr->Text(), expr->Text(), toDkmString(errStr.c_str()), 
        Evaluation::DkmEvaluationResultFlags::None, nullptr, 
        DkmDataItem::Null(), &pResultObject));

    Evaluation::DkmEvaluateExpressionAsyncResult result;
    result.ErrorCode = hrErr;
    result.pResultObject = pResultObject;
    pCompletionRoutine->OnComplete(result);
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE CMagoNatCCService::EvaluateExpression(
    _In_ Evaluation::DkmInspectionContext* pInspectionContext,
    _In_ DkmWorkList* pWorkList,
    _In_ Evaluation::DkmLanguageExpression* pExpression,
    _In_ CallStack::DkmStackWalkFrame* pStackFrame,
    _In_ IDkmCompletionRoutine<Evaluation::DkmEvaluateExpressionAsyncResult>* pCompletionRoutine)
{
    HRESULT hr;
    auto process = pInspectionContext->RuntimeInstance()->Process();

    RefPtr<CCExprContext> exprContext;
    tryHR(MakeCComObject(exprContext));
    tryHR(exprContext->Init(process, pStackFrame));

    std::wstring exprText = pExpression->Text()->Value();
    MagoEE::FormatOptions fmtopt;
    tryHR(MagoEE::StripFormatSpecifier(exprText, fmtopt));

    RefPtr<MagoEE::IEEDParsedExpr> pExpr;
    hr = MagoEE::ParseText(exprText.c_str(), exprContext->GetTypeEnv(), exprContext->GetStringTable(), pExpr.Ref());
    if (FAILED(hr))
        return createEvaluationError(pInspectionContext, pStackFrame, hr, pExpression, pCompletionRoutine);

    MagoEE::EvalOptions options = MagoEE::EvalOptions::defaults;
    options.Radix = pInspectionContext->Radix();
    options.Timeout = pInspectionContext->Timeout();
    Evaluation::DkmEvaluationFlags::e evalFlags = pInspectionContext->EvaluationFlags();
    if ((evalFlags & Evaluation::DkmEvaluationFlags::NoSideEffects) == 0)
        options.AllowAssignment = true;
    if ((evalFlags & Evaluation::DkmEvaluationFlags::NoFuncEval) == 0)
        options.AllowFuncExec = true;

    hr = pExpr->Bind(options, exprContext);
    if (FAILED(hr))
        return createEvaluationError(pInspectionContext, pStackFrame, hr, pExpression, pCompletionRoutine);

    MagoEE::EvalResult value = { 0 };
    hr = pExpr->Evaluate(options, exprContext, value);
    if (FAILED(hr))
        return createEvaluationError(pInspectionContext, pStackFrame, hr, pExpression, pCompletionRoutine);

    RefPtr<Mago::Property> pProperty;
    tryHR(MakeCComObject(pProperty));
    tryHR(pProperty->Init(exprText.c_str(), exprText.c_str(), value, exprContext, fmtopt));

    ScopedStruct<DEBUG_PROPERTY_INFO, Mago::_CopyPropertyInfo> info;
    tryHR(pProperty->GetPropertyInfo(DEBUGPROP_INFO_ALL, options.Radix, options.Timeout, nullptr, 0, &info));

    Evaluation::DkmSuccessEvaluationResult* pResultObject = nullptr;
    tryHR(createEvaluationResult(pInspectionContext, pStackFrame, info, &pResultObject));

    Evaluation::DkmEvaluateExpressionAsyncResult result;
    result.ErrorCode = S_OK;
    result.pResultObject = pResultObject;
    pCompletionRoutine->OnComplete(result);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMagoNatCCService::GetChildren(
    _In_ Evaluation::DkmEvaluationResult* pResult,
    _In_ DkmWorkList* pWorkList,
    _In_ UINT32 InitialRequestSize,
    _In_ Evaluation::DkmInspectionContext* pInspectionContext,
    _In_ IDkmCompletionRoutine<Evaluation::DkmGetChildrenAsyncResult>* pCompletionRoutine)
{
    auto successResult = Evaluation::DkmSuccessEvaluationResult::TryCast(pResult);
    if (!successResult)
        return S_FALSE;

    RefPtr<Mago::Property> pProperty;
    tryHR(pResult->GetDataItem(&pProperty.Ref()));

    int radix = pInspectionContext->Radix();
    int timeout = pInspectionContext->Timeout();
    RefPtr<IEnumDebugPropertyInfo2> pEnum;
    tryHR(pProperty->EnumChildren(DEBUGPROP_INFO_ALL, radix, GUID(), DBG_ATTRIB_ALL, NULL, timeout, &pEnum.Ref()));
    ULONG count;
    tryHR(pEnum->GetCount(&count));

    CComPtr<Evaluation::DkmEvaluationResultEnumContext> pEnumContext;
    tryHR(Evaluation::DkmEvaluationResultEnumContext::Create(count, successResult->StackFrame(),
                                                             pInspectionContext, pEnum.Get(), &pEnumContext));

    Evaluation::DkmGetChildrenAsyncResult result;
    result.ErrorCode = S_OK;
    result.InitialChildren.Length = 0; // TODO
    result.InitialChildren.Members = nullptr;
    result.pEnumContext = pEnumContext;
    pCompletionRoutine->OnComplete(result);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMagoNatCCService::GetFrameLocals(
    _In_ Evaluation::DkmInspectionContext* pInspectionContext,
    _In_ DkmWorkList* pWorkList,
    _In_ CallStack::DkmStackWalkFrame* pStackFrame,
    _In_ IDkmCompletionRoutine<Evaluation::DkmGetFrameLocalsAsyncResult>* pCompletionRoutine)
{
    RefPtr<CCExprContext> exprContext;
    tryHR(MakeCComObject(exprContext));
    auto process = pInspectionContext->RuntimeInstance()->Process();
    tryHR(exprContext->Init(process, pStackFrame));

    RefPtr<Mago::FrameProperty> frameProp;
    tryHR(MakeCComObject(frameProp));
    tryHR(frameProp->Init(exprContext->getRegSet(), exprContext));

    RefPtr<IEnumDebugPropertyInfo2> pEnum;
    tryHR(frameProp->EnumChildren(DEBUGPROP_INFO_ALL, pInspectionContext->Radix(),
                                  guidFilterLocalsPlusArgs, DBG_ATTRIB_ALL, NULL, pInspectionContext->Timeout(),
                                  &pEnum.Ref()));

    ULONG count;
    tryHR(pEnum->GetCount(&count));

    CComPtr<Evaluation::DkmEvaluationResultEnumContext> pEnumContext;
    tryHR(Evaluation::DkmEvaluationResultEnumContext::Create(count, pStackFrame,
                                                             pInspectionContext, pEnum.Get(), &pEnumContext));

    Evaluation::DkmGetFrameLocalsAsyncResult result;
    result.ErrorCode = S_OK;
    result.pEnumContext = pEnumContext;
    pCompletionRoutine->OnComplete(result);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMagoNatCCService::GetFrameArguments(
    _In_ Evaluation::DkmInspectionContext* pInspectionContext,
    _In_ DkmWorkList* pWorkList,
    _In_ CallStack::DkmStackWalkFrame* pFrame,
    _In_ IDkmCompletionRoutine<Evaluation::DkmGetFrameArgumentsAsyncResult>* pCompletionRoutine)
{
//    return E_NOTIMPL;
    HRESULT hr = pInspectionContext->GetFrameArguments(pWorkList, pFrame, pCompletionRoutine);
    return hr;
}


HRESULT STDMETHODCALLTYPE CMagoNatCCService::GetItems(
    _In_ Evaluation::DkmEvaluationResultEnumContext* pEnumContext,
    _In_ DkmWorkList* pWorkList,
    _In_ UINT32 StartIndex,
    _In_ UINT32 Count,
    _In_ IDkmCompletionRoutine<Evaluation::DkmEvaluationEnumAsyncResult>* pCompletionRoutine)
{
    RefPtr<IEnumDebugPropertyInfo2> pEnum;
    tryHR(pEnumContext->GetDataItem(&pEnum.Ref()));

    ULONG children;
    tryHR(pEnum->GetCount(&children));

    if (StartIndex >= children)
        Count = 0;
    else
    {
        if (StartIndex + Count > children)
            Count = children - StartIndex;
        tryHR(pEnum->Reset());
        tryHR(pEnum->Skip(StartIndex));
    }
    Evaluation::DkmEvaluationEnumAsyncResult result;
    result.ErrorCode = S_OK;
    tryHR(DkmAllocArray(Count, &result.Items));
    for (ULONG i = 0; i < Count; i++)
    {
        ULONG fetched;
        ScopedStruct<DEBUG_PROPERTY_INFO, Mago::_CopyPropertyInfo> info;
        Mago::_CopyPropertyInfo::init(&info);
        HRESULT hr = pEnum->Next(1, &info, &fetched);
        if (SUCCEEDED(hr) && fetched == 1 && info.pProperty)
        {
            Evaluation::DkmSuccessEvaluationResult* pResultObject = nullptr;
            hr = createEvaluationResult(pEnumContext->InspectionContext(), pEnumContext->StackFrame(), info, &pResultObject);
            if (SUCCEEDED(hr))
                result.Items.Members[i] = pResultObject;
        }
    }
    pCompletionRoutine->OnComplete(result);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMagoNatCCService::SetValueAsString(
    _In_ Evaluation::DkmEvaluationResult* pResult,
    _In_ DkmString* pValue,
    _In_ UINT32 Timeout,
    _Deref_out_opt_ DkmString** ppErrorText)
{
    RefPtr<Mago::Property> prop;
    tryHR(pResult->GetDataItem(&prop.Ref()));

    int radix = 10;
    tryHR(prop->SetValueAsString(pValue->Value(), radix, Timeout));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMagoNatCCService::GetUnderlyingString(
    _In_ Evaluation::DkmEvaluationResult* pResult,
    _Deref_out_opt_ DkmString** ppStringValue)
{
    RefPtr<Mago::Property> prop;
    tryHR(pResult->GetDataItem(&prop.Ref()));

    ULONG len, fetched;
    tryHR(prop->GetStringCharLength(&len));
    std::unique_ptr<WCHAR> str (new WCHAR[len + 1]);
    tryHR(prop->GetStringChars(len + 1, str.get(), &fetched));
    if (fetched <= len)
        str.get()[fetched] = 0;
    *ppStringValue = toDkmString(str.get()).Detach();
    return S_OK;
}

// IDkmLanguageReturnValueEvaluator
HRESULT STDMETHODCALLTYPE CMagoNatCCService::EvaluateReturnValue(
    _In_ Evaluation::DkmInspectionContext* pInspectionContext,
    _In_ DkmWorkList* pWorkList,
    _In_ CallStack::DkmStackWalkFrame* pStackFrame,
    _In_ Evaluation::DkmRawReturnValue* pRawReturnValue,
    _In_ IDkmCompletionRoutine<Evaluation::DkmEvaluateReturnValueAsyncResult>* pCompletionRoutine)
{
	auto pNativeRetValue = Evaluation::DkmNativeRawReturnValue::TryCast(pRawReturnValue);
	if (!pNativeRetValue)
		return E_NOTIMPL;

	RefPtr<CCExprContext> exprContext;
	tryHR(MakeCComObject(exprContext));
	auto process = pInspectionContext->RuntimeInstance()->Process();
	tryHR(exprContext->Init(process, pStackFrame));

	ScopedStruct<DEBUG_PROPERTY_INFO, Mago::_CopyPropertyInfo> info;
	tryHR(exprContext->EvalReturnValue(pInspectionContext, pNativeRetValue, info));

	Evaluation::DkmSuccessEvaluationResult* pResultObject = nullptr;
	tryHR(createEvaluationResult(pInspectionContext, pStackFrame, info, &pResultObject));

	Evaluation::DkmEvaluateReturnValueAsyncResult result;
	result.ErrorCode = S_OK;
	result.pResultObject = pResultObject;
	pCompletionRoutine->OnComplete(result);
	return S_OK;
}
