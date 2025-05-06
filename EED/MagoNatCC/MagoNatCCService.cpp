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
#include "../../CVSym/CVSym/Util.h"

#include "../../CVSym/CVSTI/CVSTIPublic.h"
#include "../../CVSym/CVSTI/IDataSource.h"
#include "../../CVSym/CVSTI/ISession.h"
#include "../../CVSym/CVSTI/ImageAddrMap.h"

#include "../../DebugEngine/Include/MagoDECommon.h"
#include "../../DebugEngine/Include/WinPlat.h"
#include "../../DebugEngine/MagoNatDE/Config.h"
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

#include "../../DebugEngine/MagoNatDE/EnumPropertyInfo.h"

#include <atomic>
#include <chrono>

///////////////////////////////////////////////////////////////////////////////
#define NOT_IMPL(x) virtual HRESULT x override { return E_NOTIMPL; }

#define tryHR(x) do { HRESULT _hr = (x); if(FAILED(_hr)) return _hr; } while(false)

static_assert(E_OPERATIONCANCELED == COR_E_OPERATIONCANCELED, "bad E_OPERATIONCANCELED");

Evaluation::IL::DkmILCallingConvention::e toCallingConvention(uint8_t callConv);
CComPtr<DkmString> toDkmString(const wchar_t* str);

extern "C" char* dlang_demangle(const char* mangled, int options);

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
        HRESULT hr = mProcess->ReadMemory(address, DkmReadMemoryFlags::None, buffer, length, &lengthRead);
        if (hr == E_INVALID_MEMORY_ADDRESS)
            return E_MAGOEE_INVALID_ADDRESS;
        if (FAILED(hr))
            return hr;
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

EXTERN_C const GUID DECLSPEC_SELECTANY guidIDiaSession2013 =
{ 0x6FC5D63F, 0x011E, 0x40C2, { 0x8D, 0xD2, 0xE6, 0x48, 0x6E, 0x9D, 0x6B, 0x68 } };

class DECLSPEC_UUID("598DECC9-CF79-4E90-A408-5E1433B4DBFF") CCModule : public CCDataItem<CCModule>
{
    friend class CCExprContext;

public:
    CCModule() : mDebuggerProxy(nullptr) {}
    ~CCModule() 
    {
        delete mDebuggerProxy;
    }

protected:
    RefPtr<MagoST::IDataSource> mDataSource;
    RefPtr<MagoST::ISession> mSession;
    RefPtr<MagoST::ImageAddrMap> mAddrMap;
    RefPtr<Mago::ArchData> mArchData;
    RefPtr<Mago::IRegisterSet> mRegSet;
    RefPtr<Mago::Module> mModule;
    RefPtr<Mago::Program> mProgram;

    CCDebuggerProxy* mDebuggerProxy;
    RefPtr<DkmProcess> mProcess;

public:
    HRESULT Init(DkmProcess* process, Symbols::DkmModule* module)
    {
        auto system = process->SystemInformation();
        auto arch = system->ProcessorArchitecture();
        int ptrSize = arch == PROCESSOR_ARCHITECTURE_AMD64 ? 8 : 4;

        CComPtr<IDiaSession> diasession;
        if (module->GetSymbolInterface(guidIDiaSession, (IUnknown**)&diasession) != S_OK) // VS 2015
            if (HRESULT hr = module->GetSymbolInterface(guidIDiaSession2013, (IUnknown**)&diasession)) // VS2013
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

    HRESULT InitRuntime(DkmProcess* process)
    {
        auto system = process->SystemInformation();
        auto arch = system->ProcessorArchitecture();
        int ptrSize = arch == PROCESSOR_ARCHITECTURE_AMD64 ? 8 : 4;

        auto features = toMago(system->ProcessorFeatures());
        if (arch == PROCESSOR_ARCHITECTURE_AMD64)
            mArchData = new Mago::ArchDataX64(features);
        else
            mArchData = new Mago::ArchDataX86(features);

        mDebuggerProxy = new CCDebuggerProxy(process);
        mProcess = process;

        tryHR(MakeCComObject(mProgram));

        // any process to pass architecture info
        RefPtr<Mago::LocalProcess> localprocess = new Mago::LocalProcess(mArchData);
        mProgram->SetCoreProcess(localprocess);
        mProgram->SetDebuggerProxy(mDebuggerProxy);
        UniquePtr<Mago::DRuntime> druntime(new Mago::DRuntime(mDebuggerProxy, localprocess));
        mProgram->SetDRuntime(druntime);

        return S_OK;
    }

    HRESULT FindFunction(uint64_t va, MagoST::SymHandle& funcSH, std::vector<MagoST::SymHandle>& blockSH)
    {
        uint32_t    offset = 0;
        uint16_t    sec = mSession->GetSecOffsetFromVA( va, offset );
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

    HRESULT GetSymbolName(const MagoST::SymHandle& symHandle, std::wstring& symName)
    {
        MagoST::SymInfoData infoData = { 0 };
        MagoST::ISymbolInfo* symInfo;
        HRESULT hr = mSession->GetSymbolInfo(symHandle, infoData, symInfo);
        if (FAILED(hr))
            return hr;

        SymString pstrName;
        if (!symInfo->GetName(pstrName))
            return E_FAIL;

        symName = MagoEE::to_wstring(pstrName.GetName(), pstrName.GetLength());
        return S_OK;
    }

    HRESULT GetExceptionInfo(MagoEE::Address addr, MagoEE::Address ip, std::wstring& exceptionInfo)
    {
        if (auto druntime = mProgram->GetDRuntime())
        {
            BSTR bstrClassname = NULL;
            if (SUCCEEDED(druntime->GetClassName(addr, &bstrClassname)))
            {
                wchar_t buf[20] = L"";
                swprintf_s(buf, L"0x%I64x", ip);
                exceptionInfo = L"Unhandled " + std::wstring(bstrClassname) + L" at " + buf;
                SysFreeString(bstrClassname);

                BSTR bstrInfo = NULL, bstrLine;
                if (SUCCEEDED(druntime->GetExceptionInfo(addr, &bstrInfo, &bstrLine)))
                {
                    if (bstrInfo && bstrLine)
                        exceptionInfo = bstrLine + (L": " + exceptionInfo) + L" - " + bstrInfo;
                    else if (bstrInfo)
                        exceptionInfo = exceptionInfo + L" - " + bstrInfo;
                    else if (bstrLine)
                        exceptionInfo = bstrLine + (L": " + exceptionInfo);

                    if (bstrInfo)
                        SysFreeString(bstrInfo);
                    if (bstrLine)
                        SysFreeString(bstrLine);
                }
                return S_OK;
            }
        }
        return E_FAIL;
    }

    virtual HRESULT ReadMemory(MagoEE::Address addr, uint32_t sizeToRead, uint32_t& sizeRead, uint8_t* buffer)
    {
        return mProcess->ReadMemory(addr, DkmReadMemoryFlags::None, buffer, sizeToRead, &sizeRead);
    }
};

///////////////////////////////////////////////////////////////////////////////

// struct Slice { size_t length; void* ptr; }
// alias func = void[] function();
// 
// extern(C++) Slice cppFun(void* vfn)
// {
//     auto fn = cast(func) vfn;
//     void[] s = fn();
//     return Slice(s.length, s.ptr);
// }

unsigned char sliceRetTrampoline64[] =
{
    /* 000 : */ 0x55,                         // push        rbp
    /* 001 : */ 0x48, 0x8B, 0xEC,             // mov         rbp, rsp
    /* 004 : */ 0x48, 0x83, 0xEC, 0x30,       // sub         rsp, 30h
    /* 008 : */ 0x48, 0x89, 0x4D, 0x10,       // mov         qword ptr[rbp + 10h], rcx
    /* 00C : */ 0x48, 0x89, 0x55, 0x18,       // mov         qword ptr[rbp + 18h], rdx
    /* 010 : */ 0x48, 0xFF, 0x55, 0x18,       // call        qword ptr[rbp + 18h]
    /* 014 : */ 0x48, 0x89, 0x45, 0xF0,       // mov         qword ptr[rbp - 10h], rax
    /* 018 : */ 0x48, 0x89, 0x55, 0xF8,       // mov         qword ptr[rbp - 8], rdx
    /* 01C : */ 0x48, 0x8B, 0x45, 0xF0,       // mov         rax, qword ptr[rbp - 10h]
    /* 020 : */ 0x48, 0x8B, 0x4D, 0x10,       // mov         rcx, qword ptr[rbp + 10h]
    /* 024 : */ 0x48, 0x89, 0x01,             // mov         qword ptr[rcx], rax
    /* 027 : */ 0x48, 0x8B, 0x55, 0xF8,       // mov         rdx, qword ptr[rbp - 8]
    /* 02B : */ 0x48, 0x89, 0x51, 0x08,       // mov         qword ptr[rcx + 8], rdx
    /* 02F : */ 0x48, 0x89, 0xC8,             // mov         rax, rcx
    /* 032 : */ 0x48, 0x8B, 0xE5,             // mov         rsp, rbp
    /* 035 : */ 0x5D,                         // pop         rbp
    /* 036 : */ 0xC3,                         // ret
};

unsigned char sliceRetTrampoline32[] =
{
    /* 000 : */ 0xC8, 0x10, 0x00, 0x00, //  enter       10h, 0
    /* 004 : */ 0x8B, 0x45, 0x08,       //  mov         eax, dword ptr[ebp + 8]
    /* 007 : */ 0xFF, 0xD0,             //  call        eax
    /* 009 : */ 0x89, 0x45, 0xF0,       //  mov         dword ptr[ebp - 10h], eax
    /* 00C : */ 0x89, 0x55, 0xF4,       //  mov         dword ptr[ebp - 0Ch], edx
    /* 00F : */ 0x8B, 0x4D, 0xF0,       //  mov         ecx, dword ptr[ebp - 10h]
    /* 012 : */ 0x89, 0x4D, 0xF8,       //  mov         dword ptr[ebp - 8], ecx
    /* 015 : */ 0x8B, 0x55, 0xF4,       //  mov         edx, dword ptr[ebp - 0Ch]
    /* 018 : */ 0x89, 0x55, 0xFC,       //  mov         dword ptr[ebp - 4], edx
    /* 01B : */ 0x8B, 0x55, 0xFC,       //  mov         edx, dword ptr[ebp - 4]
    /* 01E : */ 0x8B, 0x45, 0xF8,       //  mov         eax, dword ptr[ebp - 8]
    /* 021 : */ 0xC9,                   //  leave
    /* 022 : */ 0xC3,                   //  ret
};

// alias sdeleg = void[] delegate();
// extern(C++) Slice cppSliceDelegate(void* vfn, void* ctx)
// {
//     sdeleg dg;
//     dg.funcptr = cast(sfunc) vfn;
//     dg.ptr = ctx;
//     void[] s = dg();
//     return Slice(s.length, s.ptr);
// }
unsigned char sliceRetTrampolineDelegate64[] =
{
    /* 0A0 : */ 0x55,                           // push        rbp
    /* 0A1 : */ 0x48, 0x8B, 0xEC,               // mov         rbp, rsp
    /* 0A4 : */ 0x48, 0x83, 0xEC, 0x28,         // sub         rsp, 28h
    /* 0A8 : */ 0x53,                           // push        rbx
    /* 0A9 : */ 0x56,                           // push        rsi
    /* 0AA : */ 0x57,                           // push        rdi
    /* 0AB : */ 0x48, 0x89, 0x4D, 0x10,         // mov         qword ptr[rbp + 10h], rcx
    /* 0AF : */ 0x48, 0x89, 0x55, 0x18,         // mov         qword ptr[rbp + 18h], rdx
    /* 0B3 : */ 0x4C, 0x89, 0x45, 0x20,         // mov         qword ptr[rbp + 20h], r8
    /* 0B7 : */ 0x48, 0xC7, 0x45, 0xE0,0,0,0,0, // mov         qword ptr[rbp - 20h], 0
    /* 0BF : */ 0x48, 0xC7, 0x45, 0xE8,0,0,0,0, // mov         qword ptr[rbp - 18h], 0
    /* 0C7 : */ 0x48, 0x8B, 0x45, 0x18,         // mov         rax, qword ptr[rbp + 18h]
    /* 0CB : */ 0x48, 0x89, 0x45, 0xE8,         // mov         qword ptr[rbp - 18h], rax
    /* 0CF : */ 0x48, 0x8B, 0x4D, 0x20,         // mov         rcx, qword ptr[rbp + 20h]
    /* 0D3 : */ 0x48, 0x89, 0x4D, 0xE0,         // mov         qword ptr[rbp - 20h], rcx
    /* 0D7 : */ 0x48, 0x83, 0xEC, 0x20,         // sub         rsp, 20h
    /* 0DB : */ 0x48, 0x8B, 0x55, 0xE8,         // mov         rdx, qword ptr[rbp - 18h]
    /* 0DF : */ 0x48, 0x8B, 0x45, 0xE0,         // mov         rax, qword ptr[rbp - 20h]
    /* 0E3 : */ 0x48, 0xFF, 0xD2,               // call        rdx
    /* 0E6 : */ 0x48, 0x83, 0xC4, 0x20,         // add         rsp, 20h
    /* 0EA : */ 0x48, 0x89, 0x45, 0xF0,         // mov         qword ptr[rbp - 10h], rax
    /* 0EE : */ 0x48, 0x89, 0x55, 0xF8,         // mov         qword ptr[rbp - 8], rdx
    /* 0F2 : */ 0x48, 0x8B, 0x5D, 0xF0,         // mov         rbx, qword ptr[rbp - 10h]
    /* 0F6 : */ 0x48, 0x8B, 0x75, 0x10,         // mov         rsi, qword ptr[rbp + 10h]
    /* 0FA : */ 0x48, 0x89, 0x1E,               // mov         qword ptr[rsi], rbx
    /* 0FD : */ 0x48, 0x8B, 0x7D, 0xF8,         // mov         rdi, qword ptr[rbp - 8]
    /* 101 : */ 0x48, 0x89, 0x7E, 0x08,         // mov         qword ptr[rsi + 8], rdi
    /* 105 : */ 0x48, 0x89, 0xF0,               // mov         rax, rsi
    /* 108 : */ 0x5F,                           // pop         rdi
    /* 109 : */ 0x5E,                           // pop         rsi
    /* 10A : */ 0x5B,                           // pop         rbx
    /* 10B : */ 0x48, 0x8B, 0xE5,               // mov         rsp, rbp
    /* 10E : */ 0x5D,                           // pop         rbp
    /* 10F : */ 0xC3,                           // ret
};

// forward larger return value pointer from stack to EAX
//
// alias func = S16 function();
// extern(C++) S16 cppFun(void* vfn)
// {
//     auto fn = cast(func) vfn;
//     return fn();
// }

unsigned char structRetTrampoline32[] =
{
    /* 000 : */ 0x8B, 0x44, 0x24, 0x04,  //  mov         eax,dword ptr[esp + 4]
    /* 004 : */ 0xFF, 0x54, 0x24, 0x08,  //  call        dword ptr[esp + 8]
    /* 008 : */ 0xC3,                    //  ret
#if 0
    /* 038 : */ 0xC8, 0x04, 0x00, 0x00,  //  enter       4, 0
    /* 03C : */ 0x8B, 0x45, 0x0C,        //  mov         eax, dword ptr[ebp + 0Ch]
    /* 03F : */ 0x89, 0x45, 0xFC,        //  mov         dword ptr[ebp - 4], eax
    /* 042 : */ 0x8B, 0x45, 0x08,        //  mov         eax, dword ptr[ebp + 8]
    /* 045 : */ 0xFF, 0x55, 0xFC,        //  call        dword ptr[ebp - 4]
    /* 048 : */ 0xC9,                    //  leave
    /* 049 : */ 0xC3,                    //  ret
#endif
};

// alias deleg = S16 delegate();
// extern(C++) S16 cppDelegate(void* vfn, void* ctx)
// {
//     deleg dg;
//     dg.funcptr = cast(func) vfn;
//     dg.ptr = ctx;
//     return dg();
// }

unsigned char structRetTrampolineDelegate32[] =
{
    /* 0D0 : */ 0xFF, 0x74, 0x24, 0x04,  //  push        dword ptr[esp + 4]
    /* 0D4 : */ 0x8B, 0x44, 0x24, 0x10,  //  mov         eax, dword ptr[esp + 10h]
    /* 0D8 : */ 0xFF, 0x54, 0x24, 0x0C,  //  call        dword ptr[esp + 0Ch]
    /* 0DC : */ 0xC3,                    //  ret
};

// alias ifunc = size_t function();
// alias ideleg = size_t delegate();
// extern(C++) size_t cppIntDelegate(void* vfn, void* ctx)
// {
//     ideleg dg;
//     dg.funcptr = cast(ifunc) vfn;
//     dg.ptr = ctx;
//     return dg();
// }
unsigned char intRetTrampolineDelegate32[] =
{
    /* 000 : */ 0x8B, 0x44, 0x24, 0x08,  //  mov         eax, dword ptr[esp + 8]
    /* 004 : */ 0xFF, 0x54, 0x24, 0x04,  //  call        dword ptr[esp + 4]
    /* 008 : */ 0xC3,                    //  ret
};

struct ProcessHelper
{
    MagoEE::Address baseAddr;
    MagoEE::Address sliceRetTrampoline;
    MagoEE::Address sliceRetTrampolineDelegate;
    MagoEE::Address structRetTrampoline;
    MagoEE::Address structRetTrampolineDelegate;
    MagoEE::Address intRetTrampolineDelegate;
    MagoEE::Address tmpBuf;
    MagoEE::Address tmpLen;
    MagoEE::Address tmpPos;

    MagoEE::Address fnInitGC = 0;
    MagoEE::Address fnTermGC = 0;
    MagoEE::Address magoGC = 0;
    MagoEE::Address adrInstanceGC = 0;

    MagoEE::Address getTmpBuffer(uint32_t size)
    {
        size = (size + 15) & ~0xf;
        if (size > tmpLen)
            return 0;
        if (tmpPos + size > tmpLen)
            tmpPos = 0;
        MagoEE::Address adr = tmpBuf + tmpPos;
        tmpPos += size;
        return adr;
    }
};

std::map<DkmProcess*, ProcessHelper*> processHelpers;
struct cleanUpProcessHelpers
{
    ~cleanUpProcessHelpers()
    {
        for (auto ph : processHelpers)
            delete ph.second;
    }
} _cleanUpProcessHelpers;

ProcessHelper* getProcessHelper(DkmProcess* process)
{
    auto it = processHelpers.find(process);
    if (it != processHelpers.end())
        return  it->second;

    MagoEE::Address addr;
    uint32_t size = 0x10000;
    HRESULT hr = process->AllocateVirtualMemory(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE, &addr);
    if (FAILED(hr))
        return nullptr;

    auto helper = new ProcessHelper;
    helper->baseAddr = addr;

    bool isX64 = process->SystemInformation()->ProcessorArchitecture() == PROCESSOR_ARCHITECTURE_AMD64;
    DkmArray<BYTE> code;
    DWORD szCode = 0;
    uint32_t used = 0;
    if (isX64)
    {
        szCode = sizeof(sliceRetTrampoline64);
        code = { sliceRetTrampoline64, szCode };
        hr = process->WriteMemory(addr + used, code);
        if (FAILED(hr))
        {
            delete helper;
            return nullptr;
        }
        helper->sliceRetTrampoline = addr + used;
        used += (szCode + 15) & ~0xf;

        szCode = sizeof(sliceRetTrampolineDelegate64);
        code = { sliceRetTrampolineDelegate64, szCode };
        hr = process->WriteMemory(addr + used, code);
        if (FAILED(hr))
        {
            delete helper;
            return nullptr;
        }
        helper->sliceRetTrampolineDelegate = addr + used;
        used += (szCode + 15) & ~0xf;
    }
    else
    {
        szCode = sizeof(structRetTrampoline32);
        code = { structRetTrampoline32, szCode };
        hr = process->WriteMemory(addr + used, code);
        if (FAILED(hr))
        {
            delete helper;
            return nullptr;
        }
        helper->structRetTrampoline = addr + used;
        used += (szCode + 15) & ~0xf;

        szCode = sizeof(structRetTrampolineDelegate32);
        code = { structRetTrampolineDelegate32, szCode };
        hr = process->WriteMemory(addr + used, code);
        if (FAILED(hr))
        {
            delete helper;
            return nullptr;
        }
        helper->structRetTrampolineDelegate = addr + used;
        used += (szCode + 15) & ~0xf;

        szCode = sizeof(intRetTrampolineDelegate32);
        code = { intRetTrampolineDelegate32, szCode };
        hr = process->WriteMemory(addr + used, code);
        if (FAILED(hr))
        {
            delete helper;
            return nullptr;
        }
        helper->intRetTrampolineDelegate = addr + used;
        used += (szCode + 15) & ~0xf;
    }
    helper->tmpBuf = addr + used;
    helper->tmpLen = size - used;
    helper->tmpPos = 0;

    processHelpers[process] = helper;
    return helper;
}

///////////////////////////////////////////////////////////////////////////////
class DECLSPEC_UUID("98F84FA4-E9E5-458C-81B7-F04352727B97") CCExprContext : public Mago::ExprContext
{
public:
    RefPtr<DkmWorkList> mWorkList;
    RefPtr<CallStack::DkmStackWalkFrame> mStackFrame;
    RefPtr<CCModule> mModule;
    RefPtr<Mago::Thread> mThread;
    std::atomic<uint32_t> mQueuedCalls;
    Mago::Address64 mSEH = 0;
    Mago::Address64 mInstanceGC = 0;

    static std::atomic<uint32_t> sNumInstances;

    CCExprContext()
    {
        sNumInstances++;
    }
    ~CCExprContext() 
    {
        sNumInstances--;
    }
    
    HRESULT Init(DkmProcess* process, DkmWorkList* workList, CallStack::DkmStackWalkFrame* frame)
    {
        HRESULT hr;
        CComPtr<Symbols::DkmInstructionSymbol> pInstruction;
        hr = frame->GetInstructionSymbol(&pInstruction);
        if (hr != S_OK)
          return hr;

        auto nativeInst = Native::DkmNativeInstructionSymbol::TryCast(pInstruction);
        if (!nativeInst)
          return E_FAIL;

        Symbols::DkmModule* module = pInstruction->Module();
        if (!module)
          return E_FAIL;

        // Restore/Create CCModule in DkmModule
        hr = module->GetDataItem(&mModule.Ref());
        if (!mModule)
        {
            mModule = new CCModule; 
            tryHR(mModule->Init(process, module));
            tryHR(module->SetDataItem(DkmDataCreationDisposition::CreateAlways, mModule.Get()));
        }

        mWorkList = workList;
        mStackFrame = frame;

        uint64_t va = frame->InstructionAddress()->CPUInstructionPart()->InstructionPointer;
        MagoST::SymHandle funcSH;
        std::vector<MagoST::SymHandle> blockSH;
        tryHR(mModule->FindFunction(va, funcSH, blockSH));

        // setup a fake environment for Mago
        tryHR(MakeCComObject(mThread));
        mThread->SetProgram(mModule->mProgram, mModule->mDebuggerProxy);

        return ExprContext::Init(mModule->mModule, mThread, funcSH, blockSH, va, mModule->mRegSet);
    }

    HRESULT SetWorkList(DkmWorkList* pWorkList)
    {
        mWorkList = pWorkList;
        return S_OK;
    }

    HRESULT SetStackFrame(CallStack::DkmStackWalkFrame* frame)
    {
        if (mStackFrame == frame)
            return S_OK;

        uint64_t va = frame->InstructionAddress()->CPUInstructionPart()->InstructionPointer;
        MagoST::SymHandle funcSH;
        std::vector<MagoST::SymHandle> blockSH;
        tryHR(mModule->FindFunction(va, funcSH, blockSH));

        return ExprContext::Init(mModule->mModule, mThread, funcSH, blockSH, va, mModule->mRegSet);
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

    static bool demangleDSymbol(std::wstring& symName)
    {
        size_t len = symName.length();
        if (len < 1 || symName[0] != '_')
            return false;
        size_t pos = 1;
        while (pos < len && symName[pos] == '_')
            pos++;
        if (pos >= len || symName[pos] != 'D')
            return false;

        CAutoVectorPtr<char> u8Name;
        size_t               u8NameLen = 0;
        HRESULT hr = Utf16To8(symName.data() + pos - 1, len - pos + 1, u8Name.m_p, u8NameLen);
        if (FAILED(hr))
            return false;

        char* demangled = dlang_demangle(u8Name, 0);
        if (!demangled)
            return false;

        CAutoVectorPtr<char> u16Name;
        size_t               u16NameLen = 0;
        BSTR u16Str = NULL;
        hr = Utf8To16(demangled, strlen(demangled), u16Str);
        free(demangled);
        if (FAILED(hr))
            return false;

        symName = u16Str;
        SysFreeString(u16Str);
        return true;
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
                    if (module->UndecorateName(toDkmString(symName.c_str()), 0x7ff, &undec.Ref()) == S_OK)
                    {
                        symName = undec->Value();
                    }
                }
            }
        }
        else
            demangleDSymbol(symName);
        return S_OK;
    }

    HRESULT FindFunctionAddress(const wchar_t* dllname, const wchar_t* funcname, MagoEE::Address& fnaddr)
    {
        auto nativeRuntime = Native::DkmNativeRuntimeInstance::TryCast(mStackFrame->RuntimeInstance());
        if (!nativeRuntime)
            return E_FAIL;
        DkmArray<DkmModuleInstance*> modules;
        nativeRuntime->FindModulesByName(toDkmString(dllname), &modules); // GetModuleInstances(&modules); //  

        fnaddr = 0;
        for (DWORD i = 0; i < modules.Length; i++)
        {
            if (auto nativeModule = Native::DkmNativeModuleInstance::TryCast(modules.Members[i]))
            {
                RefPtr<Native::DkmNativeInstructionAddress> pAddress = nullptr;
                if (nativeModule->FindExportName(toDkmString(funcname), true, &pAddress.Ref()) == S_OK)
                {
                    fnaddr = nativeModule->BaseAddress() + pAddress->RVA();
                    break;
                }
            }
        }
        DkmFreeArray(modules);
        return fnaddr == 0 ? E_FAIL : S_OK;
    }

    // if no GC found in symbols, return S_FALSE
    // else load magogc32/64.dll, and setup addresses and GC
    HRESULT LoadMagoGCDLL()
    {
        auto helper = getProcessHelper(mModule->mProcess);
        if (!helper)
            return E_MAGOEE_CANNOTALLOCTRAMP;
        if (helper->fnInitGC)
            return S_OK;
        int ptrSize = mModule->mArchData->GetPointerSize();
        bool x64 = ptrSize == 8;

        MagoEE::Address instaddr;
        auto tryFindSymbol = [&](const wchar_t* sym, MagoEE::Address& addr)
        {
            return FindGlobalSymbolAddr(x64 ? sym + 1 : sym, addr);
        };
        int druntime_version = 2'097;
        if (FAILED(tryFindSymbol(L"__D4core8internal2gc5proxy9_instanceCQBiQx11gcinterface2GC", instaddr)))
        {
            // gc.proxy not yet moved to internal?
            tryHR(tryFindSymbol(L"__D2gc5proxy9_instanceC4coreQz11gcinterface2GC", instaddr));
            druntime_version = 2'096;
        }
        else
        {
            MagoEE::Address cnsaddr; // collectNoStack removed?
            if (FAILED(tryFindSymbol(L"__D4core8internal2gc4impl5protoQo7ProtoGC14collectNoStackMFNbZv", cnsaddr)))
                druntime_version = 2'109;
        }
        MagoEE::Address isProxiedAddr; // collectNoStack removed?
        bool ldc = SUCCEEDED(tryFindSymbol(L"_gc_isProxied", isProxiedAddr));

        auto nativeRuntime = Native::DkmNativeRuntimeInstance::TryCast(mStackFrame->RuntimeInstance());
        if (!nativeRuntime)
            return E_FAIL;
        DkmArray<DkmModuleInstance*> modules;
        nativeRuntime->FindModulesByName(toDkmString(L"kernel32.dll"), &modules); // GetModuleInstances(&modules); //  

        MagoEE::Address fnaddr;
        tryHR(FindFunctionAddress(L"kernel32.dll", L"LoadLibraryW", fnaddr));

        HKEY    hKey = NULL;
        LSTATUS ret = OpenRootRegKey(false, false, hKey);
        if (ret != ERROR_SUCCESS)
            return HRESULT_FROM_WIN32(ret);

        wchar_t magogcPath[MAX_PATH] = L"";
        int     magogcPathLen = _countof(magogcPath);
        auto    regValue = x64 ? L"magogc64.dll" : L"magogc32.dll";
        ret = GetRegString(hKey, regValue, magogcPath, magogcPathLen);
        RegCloseKey(hKey);
        if (ret != ERROR_SUCCESS)
            return HRESULT_FROM_WIN32(ret);
        
        magogcPathLen = (int)wcslen(magogcPath);
        if (ldc && magogcPathLen > 4 && magogcPath[magogcPathLen - 4] == '.')
        {
            wcscpy(magogcPath + magogcPathLen - 4, L"_LDC.dll");
            magogcPathLen += 4;
        }

        MagoEE::Address argbuf = helper->getTmpBuffer(2 * magogcPathLen + 2);
        if (argbuf == 0)
            return E_MAGOEE_CANNOTALLOCTRAMP;
        DkmArray<BYTE> dllname;
        dllname.Members = (BYTE*)magogcPath;
        dllname.Length = 2 * magogcPathLen + 2;
        tryHR(mModule->mProcess->WriteMemory(argbuf, dllname));

        using namespace Evaluation::IL;

        MagoEE::Address retval;
        tryHR(ExecFunc(fnaddr, DkmILCallingConvention::StdCall, retval, ptrSize, argbuf, ptrSize));

        std::wstring initFn = druntime_version < 2'109 ? L"initGC_2_108" : L"initGC_2_109";
        if (!x64)
            initFn = L"_" + initFn + L"@0";
        MagoEE::Address initGC;
        tryHR(FindFunctionAddress(regValue, initFn.c_str(), initGC));

        MagoEE::Address termGC;
        tryHR(FindFunctionAddress(regValue, x64 ? L"termGC" : L"_termGC@0", termGC));

        MagoEE::Address gcaddr;
        tryHR(ExecFunc(initGC, DkmILCallingConvention::StdCall, gcaddr, ptrSize));

        if (gcaddr == 0)
            return E_MAGOEE_CALLFAILED;

        helper->fnInitGC = initGC;
        helper->fnTermGC = termGC;
        helper->magoGC = gcaddr;
        helper->adrInstanceGC = instaddr;
        return S_OK;
    }

    HRESULT SwitchToMagoGC()
    {
        if (mInstanceGC)
            return S_FALSE;

        auto helper = getProcessHelper(mModule->mProcess);
        if (!helper)
            return E_MAGOEE_CANNOTALLOCTRAMP;

        if (!helper->adrInstanceGC)
            if (auto hr = LoadMagoGCDLL())
                return E_MAGOEE_INVALID_MAGOGC;

        UINT32 ptrSize = mModule->mArchData->GetPointerSize();
        MagoEE::Address gcinst;
        UINT32 read;
        tryHR(mModule->mProcess->ReadMemory(helper->adrInstanceGC, DkmReadMemoryFlags::None,
                                            &gcinst, ptrSize, &read));
        if (read != ptrSize)
            return E_FAIL;

        DkmArray<BYTE> arr = { (BYTE*)&(helper->magoGC), ptrSize };
        tryHR(mModule->mProcess->WriteMemory(helper->adrInstanceGC, arr));
        mInstanceGC = gcinst;
        return S_OK;
    }

    HRESULT ResetMagoGC()
    {
        auto helper = getProcessHelper(mModule->mProcess);
        if (!helper)
            return E_MAGOEE_CANNOTALLOCTRAMP;
        if (!helper->magoGC || !helper->fnInitGC || !helper->fnTermGC)
            return S_FALSE;

        int ptrSize = mModule->mArchData->GetPointerSize();
        MagoEE::Address retval;
        tryHR(ExecFunc(helper->fnTermGC, Evaluation::IL::DkmILCallingConvention::StdCall, retval, 1));
        helper->magoGC = 0;

        MagoEE::Address gcaddr;
        tryHR(ExecFunc(helper->fnInitGC, Evaluation::IL::DkmILCallingConvention::StdCall, gcaddr, ptrSize));

        helper->magoGC = gcaddr;
        return S_OK;
    }

    HRESULT RestoreInstanceGC()
    {
        if (mQueuedCalls > 0 || mInstanceGC == 0)
            return S_FALSE;
        auto helper = getProcessHelper(mModule->mProcess);
        if (!helper)
            return E_MAGOEE_CANNOTALLOCTRAMP;

        UINT32 ptrSize = mModule->mArchData->GetPointerSize();
        DkmArray<BYTE> arr = { (BYTE*)&(mInstanceGC), ptrSize };
        tryHR(mModule->mProcess->WriteMemory(helper->adrInstanceGC, arr));
        mInstanceGC = 0;
        return S_OK;
    }

    HRESULT DisableSEH()
    {
        if (mSEH != 0)
            return S_FALSE;

        Mago::Address64 teb = GetTebBase();
        UINT32 ptrSize = mModule->mArchData->GetPointerSize();
        UINT32 read;
        tryHR(mModule->mProcess->ReadMemory(teb, DkmReadMemoryFlags::None, &mSEH, ptrSize, &read));
        if (read != ptrSize)
            return E_MAGOEE_CALLFAILED;
        if (mSEH == 0)
            return S_FALSE; // nothing todo
        Mago::Address64 zero = 0;
        DkmArray<BYTE> arr = { (BYTE*)&zero, ptrSize };
        tryHR(mModule->mProcess->WriteMemory(teb, arr));
        return S_OK;
    }

    HRESULT RestoreSEH()
    {
        if (mSEH == 0 || mQueuedCalls > 0)
            return S_FALSE;
        Mago::Address64 teb = GetTebBase();
        UINT32 ptrSize = mModule->mArchData->GetPointerSize();
        DkmArray<BYTE> arr = { (BYTE*)&mSEH, ptrSize };
        tryHR(mModule->mProcess->WriteMemory(teb, arr));
        mSEH = 0;
        return S_OK;
    }

    HRESULT ExecFunc(MagoEE::Address fnaddr, Evaluation::IL::DkmILCallingConvention::e CallingConvention,
                     MagoEE::Address& retval, int ReturnValueSize,
                     MagoEE::Address arg1 = 0, int arg1size = 0,
                     MagoEE::Address arg2 = 0, int arg2size = 0)
    {
        using namespace Evaluation::IL;
        auto evalFlags = DkmILFunctionEvaluationFlags::Default;
        int ptrSize = mModule->mArchData->GetPointerSize();

        DkmILInstruction* instructions[8];
        int cntInstr = 0;

        // push function address
        RefPtr<DkmReadOnlyCollection<BYTE>> paddrfn;
        tryHR(DkmReadOnlyCollection<BYTE>::Create((BYTE*)&fnaddr, ptrSize, &paddrfn.Ref()));
        RefPtr<DkmILPushConstant> ppushfn;
        tryHR(DkmILPushConstant::Create(paddrfn, &ppushfn.Ref()));
        instructions[cntInstr++] = ppushfn;

        static const int kMaxArgs = 2;
        UINT32 ArgumentCount = 0;
        DkmILFunctionEvaluationArgumentFlags::e argFlag[kMaxArgs];
        RefPtr<DkmReadOnlyCollection<BYTE>> argValue[kMaxArgs];
        RefPtr<DkmILPushConstant> argInstr[kMaxArgs];

        auto pushArg = [&](MagoEE::Address argument, int argsize, DkmILFunctionEvaluationArgumentFlags::e flags)
        {
            tryHR(DkmReadOnlyCollection<BYTE>::Create((BYTE*)&argument, argsize, &(argValue[ArgumentCount].Ref())));
            tryHR(DkmILPushConstant::Create(argValue[ArgumentCount], &argInstr[ArgumentCount].Ref()));
            instructions[cntInstr++] = argInstr[ArgumentCount];
            argFlag[ArgumentCount++] = flags;
            return S_OK;
        };

        if (arg1size > 0)
            tryHR(pushArg(arg1, arg1size, DkmILFunctionEvaluationArgumentFlags::Default));
        if (arg2size > 0)
            tryHR(pushArg(arg2, arg2size, DkmILFunctionEvaluationArgumentFlags::Default));

        // call function
        RefPtr<DkmReadOnlyCollection<DkmILFunctionEvaluationArgumentFlags::e>> argFlags;
        tryHR(DkmReadOnlyCollection<DkmILFunctionEvaluationArgumentFlags::e>::Create(argFlag, ArgumentCount, &argFlags.Ref()));
        UINT32 UniformComplexReturnElementSize = 0;

        RefPtr<DkmILExecuteFunction> pcall;
        tryHR(DkmILExecuteFunction::Create(ArgumentCount, ReturnValueSize, CallingConvention, evalFlags, argFlags, 0, &pcall.Ref()));
        instructions[cntInstr++] = pcall;

        RefPtr<DkmILRegisterRead> pregreaddx;
        RefPtr<DkmILReturnTop> pregretax;

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

        DkmArray<DkmILEvaluationResult*> arrResults;
        DkmILFailureReason::e failureReason;
        HRESULT hr = pquery->Execute(nullptr, pcontext, 1000, Evaluation::DkmFuncEvalFlags::None, &arrResults, &failureReason);

        retval = 0;
        if (!FAILED(hr))
        {
            if (failureReason != DkmILFailureReason::None)
                hr = E_MAGOEE_CALLFAILED + failureReason;
            else if (arrResults.Length > 0 && arrResults.Members[0])
            {
                DkmReadOnlyCollection<BYTE>* res = arrResults.Members[0]->ResultBytes();
                if (res && res->Count() == ReturnValueSize)
                {
                    memcpy(&retval, res->Items(), ReturnValueSize);
                    hr = S_OK;
                }
            }
            else if (ReturnValueSize > 0)
                hr = E_MAGOEE_CALLFAILED;
        }
        DkmFreeArray(arrResults);
        return hr;
    }

    struct CallFunctionComplete : IDkmCompletionRoutine<Evaluation::DkmExecuteQueryAsyncResult>
    {
        long mRefCount = 0;
        RefPtr<CCExprContext> exprContext;
        std::function<HRESULT(HRESULT, MagoEE::DataObject)> complete;
        MagoEE::Address retbuf;
        UINT32 retSize;
        bool passRetbuf;
        MagoEE::DataObject obj;

        ~CallFunctionComplete()
        {
        }

        virtual void STDMETHODCALLTYPE OnComplete(const Evaluation::DkmExecuteQueryAsyncResult& res)
        {
            HRESULT hr = res.ErrorCode;
            hr = exprContext->processCallFunctionResult(hr, res.FailureReason, res.Results, retbuf, retSize, passRetbuf, obj);
            complete(hr, obj);
            exprContext->mQueuedCalls--;
            exprContext->RestoreSEH();
            exprContext->RestoreInstanceGC();
            Release();
        }

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
            *ppv = NULL;
            return E_NOINTERFACE;
        }
    };

    virtual HRESULT CallFunction(MagoEE::Address addr, MagoEE::ITypeFunction* func, MagoEE::Address arg,
                                 MagoEE::DataObject& obj, bool saveGC, std::function<HRESULT(HRESULT, MagoEE::DataObject)> complete)
    {
        HRESULT hr_gc = S_FALSE;
        if (saveGC && MagoEE::gCallDebuggerUseMagoGC)
            hr_gc = SwitchToMagoGC();
        struct Exit { ~Exit() { fn(); } std::function<void()> fn; };
        Exit ex{ [&]() { RestoreInstanceGC(); } };

        using namespace Evaluation::IL;

        uint8_t callConv = func->GetCallConv();
        bool returnsDXAX = obj._Type->IsDArray() || obj._Type->IsDelegate();
        UINT32 ReturnValueSize = obj._Type->GetSize();
        UINT32 ptrSize = mModule->mArchData->GetPointerSize();
        bool isX64 = ptrSize == 8;
        bool passRetbuf = false;
        UINT32 returnInRegisterLimit = 8; // EDX:EAX on x86, RAX on x64

        DkmILCallingConvention::e CallingConvention = toCallingConvention(callConv);
        if (CallingConvention == DkmILCallingConvention::e(-1))
            return E_MAGOEE_BADCALLCONV;

        if (!isX64)
            tryHR( DisableSEH() );
        Exit ex2{ [&]() { RestoreSEH(); } };

        DkmILFunctionEvaluationArgumentFlags::e thisArgflags = DkmILFunctionEvaluationArgumentFlags::ThisPointer;
        MagoEE::Address tramp = 0;
        MagoEE::Address retbuf = 0;
        if (returnsDXAX && isX64)
        {
            auto helper = getProcessHelper(mModule->mProcess);
            if (!helper)
                return E_MAGOEE_CANNOTALLOCTRAMP;
            tramp = arg ? helper->sliceRetTrampolineDelegate : helper->sliceRetTrampoline;
            passRetbuf = true;
        }
        if (CallingConvention == DkmILCallingConvention::StdCall && ReturnValueSize > returnInRegisterLimit && !isX64)
        {
            auto helper = getProcessHelper(mModule->mProcess);
            if (!helper)
                return E_MAGOEE_CANNOTALLOCTRAMP;
            tramp = arg ? helper->structRetTrampolineDelegate : helper->structRetTrampoline;
            thisArgflags = DkmILFunctionEvaluationArgumentFlags::Default;
        }
        else if (CallingConvention == DkmILCallingConvention::StdCall && arg != 0 && !isX64)
        {
            auto helper = getProcessHelper(mModule->mProcess);
            if (!helper)
                return E_MAGOEE_CANNOTALLOCTRAMP;
            tramp = helper->intRetTrampolineDelegate;
            thisArgflags = DkmILFunctionEvaluationArgumentFlags::Default;
        }
        if (tramp && isX64)
            CallingConvention = DkmILCallingConvention::StdCall;

        if (ReturnValueSize > returnInRegisterLimit || tramp || obj._Type->AsTypeStruct())
        {
            auto helper = getProcessHelper(mModule->mProcess);
            if (!helper)
                return E_MAGOEE_CANNOTALLOCTRAMP;
            retbuf = helper->getTmpBuffer(ReturnValueSize);
            if (retbuf == 0)
                return E_MAGOEE_CALLFAILED;
        }

        DkmILFunctionEvaluationFlags::e evalFlags = arg != 0 && tramp == 0 ? DkmILFunctionEvaluationFlags::HasThisPointer
            : DkmILFunctionEvaluationFlags::Default;
        if (obj._Type->IsFloatingPoint())
            evalFlags |= DkmILFunctionEvaluationFlags::FloatingPointReturn;

        if (CallingConvention == DkmILCallingConvention::CDecl && isX64
            && ReturnValueSize <= returnInRegisterLimit && !tramp && arg != 0 && obj._Type->AsTypeStruct())
        {
            // structs never passed through registers?
            evalFlags |= DkmILFunctionEvaluationFlags::NoEnregisteredReturn;
            passRetbuf = false;
        }

        DkmILInstruction* instructions[8];
        int cntInstr = 0;

        // push function address
        MagoEE::Address funcaddr = tramp != 0 ? tramp : addr;
        RefPtr<DkmReadOnlyCollection<BYTE>> paddrfn;
        tryHR(DkmReadOnlyCollection<BYTE>::Create((BYTE*)&funcaddr, ptrSize, &paddrfn.Ref()));
        RefPtr<DkmILPushConstant> ppushfn;
        tryHR(DkmILPushConstant::Create(paddrfn, &ppushfn.Ref()));
        instructions[cntInstr++] = ppushfn;

        static const int kMaxArgs = 3;
        UINT32 ArgumentCount = 0;
        DkmILFunctionEvaluationArgumentFlags::e argFlag[kMaxArgs];
        RefPtr<DkmReadOnlyCollection<BYTE>> argValue[kMaxArgs];
        RefPtr<DkmILPushConstant> argInstr[kMaxArgs];

        auto pushArg = [&](MagoEE::Address argument, DkmILFunctionEvaluationArgumentFlags::e flags)
        {
            tryHR(DkmReadOnlyCollection<BYTE>::Create((BYTE*)& argument, ptrSize, &(argValue[ArgumentCount].Ref())));
            tryHR(DkmILPushConstant::Create(argValue[ArgumentCount], &argInstr[ArgumentCount].Ref()));
            instructions[cntInstr++] = argInstr[ArgumentCount];
            argFlag[ArgumentCount++] = flags;
            return S_OK;
        };

        if (tramp != 0)
        {
            // push argument (function pointer)
            tryHR(pushArg(addr, DkmILFunctionEvaluationArgumentFlags::Default));
        }

        if (arg != 0)
        {
            // push argument (context pointer)
            tryHR(pushArg(arg, thisArgflags));
        }

        if (passRetbuf)
        {
            // push argument (return buffer pointer)
            tryHR(pushArg(retbuf, DkmILFunctionEvaluationArgumentFlags::Default));
        }

        // call function
        RefPtr<DkmReadOnlyCollection<DkmILFunctionEvaluationArgumentFlags::e>> argFlags;
        tryHR(DkmReadOnlyCollection<DkmILFunctionEvaluationArgumentFlags::e>::Create(argFlag, ArgumentCount, &argFlags.Ref()));
        UINT32 retSize = ReturnValueSize;
        if (retSize < 4 && !(evalFlags & DkmILFunctionEvaluationFlags::NoEnregisteredReturn))
            retSize = 4; // bool and short not properly returned
        RefPtr<DkmILExecuteFunction> pcall;
        tryHR(DkmILExecuteFunction::Create(ArgumentCount, retSize, CallingConvention, evalFlags, argFlags, 0, &pcall.Ref()));
        instructions[cntInstr++] = pcall;

        RefPtr<DkmILRegisterRead> pregreaddx;
        RefPtr<DkmILReturnTop> pregretax;

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

        HRESULT hr;

        auto worklist = complete ? mWorkList : nullptr;
        if (worklist)
        {
            auto pCompletionRoutine = new CallFunctionComplete;
            pCompletionRoutine->exprContext = this;
            pCompletionRoutine->complete = complete;
            pCompletionRoutine->retbuf = retbuf;
            pCompletionRoutine->retSize = retSize;
            pCompletionRoutine->passRetbuf = passRetbuf;
            pCompletionRoutine->obj = obj;
            pCompletionRoutine->AddRef();
            hr = pquery->Execute(worklist, nullptr, pcontext, 1000, Evaluation::DkmFuncEvalFlags::None,
                pCompletionRoutine);
            if (SUCCEEDED(hr))
            {
                mQueuedCalls++;
                return S_QUEUED;
            }
            // OutputDebugString(L"Execute failed\n");
            pCompletionRoutine->Release();
        }
        else
        {
            DkmArray<DkmILEvaluationResult*> arrResults;
            DkmILFailureReason::e failureReason;
            hr = pquery->Execute(nullptr, pcontext, 1000, Evaluation::DkmFuncEvalFlags::None, &arrResults, &failureReason);
            hr = processCallFunctionResult(hr, failureReason, arrResults, retbuf, retSize, passRetbuf, obj);
            DkmFreeArray(arrResults);
        }
        return hr;
    }

    HRESULT processCallFunctionResult(HRESULT hr, Evaluation::IL::DkmILFailureReason::e failureReason,
        const DkmArray<Evaluation::IL::DkmILEvaluationResult*>& arrResults,
        MagoEE::Address retbuf, UINT32 retSize, bool passRetbuf, MagoEE::DataObject& obj)
    {
        using namespace Evaluation::IL;

        if (!FAILED(hr))
        {
            if (failureReason != DkmILFailureReason::None)
                hr = E_MAGOEE_CALLFAILED + failureReason;
            else if (arrResults.Length > 0 && arrResults.Members[0])
            {
                DkmReadOnlyCollection<BYTE>* res = arrResults.Members[0]->ResultBytes();
                if (res && res->Count() == retSize)
                {
                    if (retbuf)
                    {
                        if (!passRetbuf)
                        {
                            DkmArray<BYTE> arr = { const_cast<BYTE*>(res->Items()), retSize };
                            tryHR(mModule->mProcess->WriteMemory(retbuf, arr));
                        }
                        obj.Addr = retbuf;
                    }
                    if (retSize <= sizeof(MagoEE::DataValue))
                    {
                        hr = FromRawValue(res->Items(), obj._Type, obj.Value);
                        hr = S_OK;
                    }
                }
            }
            else
                hr = retSize == 0 ? S_OK : E_FAIL;
        }
        return hr;
    }

    bool returnInRegister(MagoEE::Type* type)
    {
        if (type->IsSArray())
            return false;

        uint32_t ptrSize = mModule->mArchData->GetPointerSize();
        if (type->GetSize() > ptrSize)
            return false;

        MagoEE::ITypeStruct* struc = type->AsTypeStruct();
        if (!struc)
            return true;

        return struc->IsPOD();
    }

    HRESULT EvalReturnValue(Evaluation::DkmInspectionContext* pInspectionContext, 
                            Evaluation::DkmNativeRawReturnValue* pNativeRetValue, Mago::PropertyInfo& info)
    {
        std::wstring funcName;
        DkmInstructionAddress* funcAddr = pNativeRetValue->ReturnFrom();
        auto cpuinfo = funcAddr->CPUInstructionPart();
        auto modInst = funcAddr->ModuleInstance();
        if (!cpuinfo || !modInst)
            return E_INVALIDARG;

        MagoST::SymHandle funcSH;
        std::vector<MagoST::SymHandle> blockSH;
        uint64_t va = cpuinfo->InstructionPointer;
        tryHR(mModule->FindFunction(va, funcSH, blockSH));

        RefPtr<MagoEE::Type> type;
        tryHR(SymbolFromAddr(va, funcName, &type.Ref()));
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
        MagoEE::FormatData fmtdata(fmtopt);

        if (retType->AsTypeStruct())
        {
            tryHR(FormatRawStructValue(this, pbuf, retType, fmtdata));
        }
        else
        {
            tryHR(FromRawValue(pbuf, retType, value.ObjVal.Value));
            tryHR(FormatValue(this, value.ObjVal, fmtdata, {}));
        }

        tryHR(MagoEE::FillValueTraits(this, value, nullptr, {}));

        RefPtr<Mago::Property> pProperty;
        tryHR(MakeCComObject(pProperty));
        tryHR(pProperty->Init(funcName.c_str(), funcName.c_str(), value, this, fmtopt));

        info.bstrName = SysAllocString(funcName.c_str());
        info.bstrFullName = SysAllocString(funcName.c_str());
        info.bstrValue = SysAllocString(fmtdata.outStr.c_str());
        info.bstrType = SysAllocString(funcType.c_str());
        return S_OK;
    }

    Mago::IRegisterSet* getRegSet() { return mModule->mRegSet; }
};

std::atomic<uint32_t> CCExprContext::sNumInstances;

///////////////////////////////////////////////////////////////////////////////
CComPtr<DkmString> toDkmString(const wchar_t* str)
{
    CComPtr<DkmString> pString;
    HRESULT hr = DkmString::Create(str, &pString);
    _ASSERT(hr == S_OK);
    return pString;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT InitExprContext(Evaluation::DkmInspectionContext* pInspectionContext,
                        DkmWorkList* pWorkList,
                        CallStack::DkmStackWalkFrame* pStackFrame,
                        bool forceReinit,
                        RefPtr<CCExprContext>& exprContext)
{
    static UINT64 lastFrameBase = 0;
    if (pStackFrame->FrameBase() != lastFrameBase)
    {
        lastFrameBase = pStackFrame->FrameBase();
        forceReinit = true; 
    }
    auto session = pInspectionContext->InspectionSession();
    if (!forceReinit)
        if (session->GetDataItem<CCExprContext>(&exprContext.Ref()) == S_OK)
            return exprContext->SetStackFrame(pStackFrame);
    auto process = pInspectionContext->RuntimeInstance()->Process();

    tryHR(MakeCComObject(exprContext));
    tryHR(exprContext->Init(process, pWorkList, pStackFrame));

#if 1 // disable if caching causes too much trouble
    tryHR(session->SetDataItem(DkmDataCreationDisposition::CreateAlways, exprContext.Get()));
#endif
    return S_OK;
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
                               const DEBUG_PROPERTY_INFO& info, Evaluation::DkmSuccessEvaluationResult** ppResultObject)
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
                              HRESULT hrErr, const std::wstring& expr,
                              IDkmCompletionRoutine<Evaluation::DkmEvaluateExpressionAsyncResult>* pCompletionRoutine)
{
    std::wstring errStr;
    Evaluation::DkmFailedEvaluationResult* pResultObject = nullptr;

    MagoEE::GetErrorString(hrErr, errStr);
    auto text = toDkmString(expr.c_str());
    tryHR(Evaluation::DkmFailedEvaluationResult::Create(
        pInspectionContext, pStackFrame, 
        text, text, toDkmString(errStr.c_str()), 
        Evaluation::DkmEvaluationResultFlags::None, nullptr, 
        DkmDataItem::Null(), &pResultObject));

    Evaluation::DkmEvaluateExpressionAsyncResult result;
    result.ErrorCode = hrErr == COR_E_OPERATIONCANCELED ? hrErr : S_OK; // display error message
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
    tryHR(InitExprContext(pInspectionContext, pWorkList, pStackFrame, false, exprContext));
    exprContext->SetWorkList(pWorkList);

    std::wstring exprText = pExpression->Text()->Value();
    MagoEE::FormatOptions fmtopt;
    tryHR(MagoEE::StripFormatSpecifier(exprText, fmtopt));

    RefPtr<MagoEE::IEEDParsedExpr> pExpr;
    hr = MagoEE::ParseText(exprText.c_str(), exprContext->GetTypeEnv(), exprContext->GetStringTable(), pExpr.Ref());
    if (FAILED(hr))
        return createEvaluationError(pInspectionContext, pStackFrame, hr, exprText, pCompletionRoutine);

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
        return createEvaluationError(pInspectionContext, pStackFrame, hr, exprText, pCompletionRoutine);

    MagoEE::EvalResult value = { 0 };
    hr = pExpr->Evaluate(options, exprContext, value,
        [exprContext, exprText, fmtopt, options,
        inspectionContext = RefPtr<Evaluation::DkmInspectionContext>(pInspectionContext),
        completionRoutine = RefPtr<IDkmCompletionRoutine<Evaluation::DkmEvaluateExpressionAsyncResult>>(pCompletionRoutine)]
        (HRESULT hr, MagoEE::EvalResult value)
        {
            RefPtr<Mago::Property> pProperty;
            if(SUCCEEDED(hr))
                hr = MakeCComObject(pProperty);
            if (SUCCEEDED(hr))
                hr = pProperty->Init(exprText.c_str(), exprText.c_str(), value, exprContext, fmtopt);

            Mago::PropertyInfo info;
            auto completeInfo = [inspectionContext, exprContext, exprText, completionRoutine]
                (HRESULT hr, const DEBUG_PROPERTY_INFO& info)
                {
                    Evaluation::DkmSuccessEvaluationResult* pResultObject = nullptr;
                    if (SUCCEEDED(hr))
                        hr = createEvaluationResult(inspectionContext, exprContext->mStackFrame, info, &pResultObject);
                    if (FAILED(hr))
                        return createEvaluationError(inspectionContext, exprContext->mStackFrame, hr, exprText, completionRoutine);

                    Evaluation::DkmEvaluateExpressionAsyncResult result;
                    result.ErrorCode = S_OK;
                    result.pResultObject = pResultObject;
                    completionRoutine->OnComplete(result);
                    return hr;
                };
            if (SUCCEEDED(hr))
                hr = pProperty->GetPropertyInfoAsync(DEBUGPROP_INFO_ALL, options.Radix, options.Timeout,
                                                     nullptr, 0, &info, completeInfo);
            if (FAILED(hr))
                return createEvaluationError(inspectionContext, exprContext->mStackFrame, hr, exprText, completionRoutine);
            return hr;
        });
    if (FAILED(hr))
        return createEvaluationError(pInspectionContext, pStackFrame, hr, exprText, pCompletionRoutine);

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

    RefPtr<CCExprContext> exprContext;
    if (auto session = pInspectionContext->InspectionSession())
        if (session->GetDataItem<CCExprContext>(&exprContext.Ref()) == S_OK)
            exprContext->SetWorkList(pWorkList);

    int radix = pInspectionContext->Radix();
    int timeout = pInspectionContext->Timeout();
    RefPtr<IEnumDebugPropertyInfo2> pEnum;
    tryHR(pProperty->EnumChildren(DEBUGPROP_INFO_ALL, radix, GUID(), DBG_ATTRIB_ALL, NULL, timeout, &pEnum.Ref()));
    ULONG count;
    tryHR(pEnum->GetCount(&count));
    if (count == 0)
        return S_FALSE;

    CComPtr<Evaluation::DkmEvaluationResultEnumContext> pEnumContext;
    tryHR(Evaluation::DkmEvaluationResultEnumContext::Create(count, successResult->StackFrame(),
                                                             pInspectionContext, pEnum.Get(), &pEnumContext));

    if (InitialRequestSize > count)
        InitialRequestSize = count;

    RefPtr<IEnumDebugPropertyInfoAsync> pEnumAsync;
    if (InitialRequestSize > 0)
        pEnum->QueryInterface(__uuidof(IEnumDebugPropertyInfoAsync), (void**)&pEnumAsync.Ref());
    if (pEnumAsync)
    {
        tryHR(_GetItemsAsync(pEnumContext, pEnumAsync, InitialRequestSize, pCompletionRoutine, nullptr));
    }
    else
    {
        Evaluation::DkmGetChildrenAsyncResult result = { 0 };
        result.ErrorCode = S_OK;
        tryHR(_GetItems(pEnumContext, pEnum, 0, InitialRequestSize, result.InitialChildren));
        result.pEnumContext = pEnumContext;
        pCompletionRoutine->OnComplete(result);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMagoNatCCService::GetFrameLocals(
    _In_ Evaluation::DkmInspectionContext* pInspectionContext,
    _In_ DkmWorkList* pWorkList,
    _In_ CallStack::DkmStackWalkFrame* pStackFrame,
    _In_ IDkmCompletionRoutine<Evaluation::DkmGetFrameLocalsAsyncResult>* pCompletionRoutine)
{
    auto session = pInspectionContext->InspectionSession();

    RefPtr<CCExprContext> exprContext;
    tryHR(InitExprContext(pInspectionContext, pWorkList, pStackFrame, true, exprContext));

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
#if 1
    return E_NOTIMPL;
#else
    HRESULT hr = pInspectionContext->GetFrameArguments(pWorkList, pFrame, pCompletionRoutine);
    return hr;
#endif
}


HRESULT STDMETHODCALLTYPE CMagoNatCCService::GetItems(
    _In_ Evaluation::DkmEvaluationResultEnumContext* pEnumContext,
    _In_ DkmWorkList* pWorkList,
    _In_ UINT32 StartIndex,
    _In_ UINT32 Count,
    _In_ IDkmCompletionRoutine<Evaluation::DkmEvaluationEnumAsyncResult>* pCompletionRoutine)
{
    RefPtr<CCExprContext> exprContext;
    if (auto session = pEnumContext->InspectionSession())
        if (session->GetDataItem<CCExprContext>(&exprContext.Ref()) == S_OK)
            exprContext->SetWorkList(pWorkList);

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
    RefPtr<IEnumDebugPropertyInfoAsync> pEnumAsync;
    if (Count > 0)
        pEnum->QueryInterface(__uuidof(IEnumDebugPropertyInfoAsync), (void**) &pEnumAsync.Ref());
    if (pEnumAsync)
    {
        tryHR(_GetItemsAsync(pEnumContext, pEnumAsync, Count, nullptr, pCompletionRoutine));
    }
    else
    {
        Evaluation::DkmEvaluationEnumAsyncResult result = { 0 };
        result.ErrorCode = S_OK;
        tryHR(_GetItems(pEnumContext, pEnum, StartIndex, Count, result.Items));
        pCompletionRoutine->OnComplete(result);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMagoNatCCService::_GetItems(
    _In_ Evaluation::DkmEvaluationResultEnumContext* pEnumContext,
    _In_ IEnumDebugPropertyInfo2* pEnum,
    _In_ UINT32 StartIndex,
    _In_ UINT32 Count,
    _Out_ DkmArray<Evaluation::DkmEvaluationResult*>& Items)
{
    tryHR(DkmAllocArray(Count, &Items));
    for (ULONG i = 0; i < Count; i++)
    {
        ULONG fetched;
        Mago::PropertyInfo info;
        HRESULT hr = pEnum->Next(1, &info, &fetched);
        if (SUCCEEDED(hr) && fetched == 1 && info.pProperty)
        {
            Evaluation::DkmSuccessEvaluationResult* pResultObject = nullptr;
            hr = createEvaluationResult(pEnumContext->InspectionContext(), pEnumContext->StackFrame(), info, &pResultObject);
            if (SUCCEEDED(hr))
                Items.Members[i] = pResultObject;
        }
    }
    return S_OK;
}

struct GetItemsAsyncResult
{
    RefPtr<Evaluation::DkmEvaluationResultEnumContext> enumContext;
    RefPtr<IDkmCompletionRoutine<Evaluation::DkmGetChildrenAsyncResult>> completionGetChildren;
    RefPtr<IDkmCompletionRoutine<Evaluation::DkmEvaluationEnumAsyncResult>> completionGetItems;
};

int gNumRequestScheduled = 0;
int gNumRequestCompleted = 0;
int gNumRequestFailed = 0;

HRESULT STDMETHODCALLTYPE CMagoNatCCService::_GetItemsAsync(
    _In_ Evaluation::DkmEvaluationResultEnumContext* pEnumContext,
    _In_ IEnumDebugPropertyInfoAsync* pEnum,
    _In_ UINT32 Count,
    _In_ IDkmCompletionRoutine<Evaluation::DkmGetChildrenAsyncResult>* pCompletionGetChildren,
    _In_ IDkmCompletionRoutine<Evaluation::DkmEvaluationEnumAsyncResult>* pCompletionGetItems)
{
    auto closure = std::make_shared<GetItemsAsyncResult>();
    closure->completionGetChildren = pCompletionGetChildren;
    closure->completionGetItems = pCompletionGetItems;
    closure->enumContext = pEnumContext;

    HRESULT hr = pEnum->NextAsync(Count,
        [closure, pEnumContext](HRESULT status, const std::vector<Mago::PropertyInfo>& infos)
        {
            Evaluation::DkmEvaluationEnumAsyncResult res;
            tryHR(DkmAllocArray(infos.size(), &res.Items));
            res.ErrorCode = status;
            for (ULONG i = 0; i < infos.size(); i++)
            {
                Evaluation::DkmSuccessEvaluationResult* pResultObject = nullptr;
                HRESULT hr = createEvaluationResult(closure->enumContext->InspectionContext(),
                    closure->enumContext->StackFrame(), infos[i], &pResultObject);
                if (SUCCEEDED(hr))
                    res.Items.Members[i] = pResultObject;
            }
            gNumRequestCompleted++;
            if (status != S_OK)
                gNumRequestFailed++;

            if (closure->completionGetItems)
                closure->completionGetItems->OnComplete(res);
            else
            {
                Evaluation::DkmGetChildrenAsyncResult res2;
                res2.ErrorCode = res.ErrorCode;
                res2.InitialChildren = std::move(res.Items);
                res2.pEnumContext = pEnumContext;
                closure->completionGetChildren->OnComplete(res2);
            }
            return status;
        });
    if (hr == S_QUEUED)
        gNumRequestScheduled++;
    return hr;
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

    std::wstring text;
    tryHR(prop->GetStringViewerText(text));
    *ppStringValue = toDkmString(text.data()).Detach();
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
    tryHR(InitExprContext(pInspectionContext, pWorkList, pStackFrame, false, exprContext));

    Mago::PropertyInfo info;
    tryHR(exprContext->EvalReturnValue(pInspectionContext, pNativeRetValue, info));

    Evaluation::DkmSuccessEvaluationResult* pResultObject = nullptr;
    tryHR(createEvaluationResult(pInspectionContext, pStackFrame, info, &pResultObject));

    Evaluation::DkmEvaluateReturnValueAsyncResult result;
    result.ErrorCode = S_OK;
    result.pResultObject = pResultObject;
    pCompletionRoutine->OnComplete(result);
    return S_OK;
}

CComPtr<CCModule> GetExceptionModule(Native::DkmWin32ExceptionInformation* ex)
{
    UINT64 hr = 0;
    auto instr = ex->InstructionAddress();
    if (!instr)
        return nullptr;
    auto modInst = instr->ModuleInstance();
    if (!modInst)
        return nullptr;
    auto process = modInst->Process();
    if (!process)
        return nullptr;

    CComPtr<Symbols::DkmModule> mod = nullptr;
    if (modInst->GetModule(&mod) != S_OK || !mod)
        return nullptr;

    CComPtr<CCModule> ccmod = new CCModule();
    ccmod->Init(process, mod);

    return ccmod;
}

UINT64 GetExceptionObjectWin64(Native::DkmWin32ExceptionInformation* ex, CCModule* ccmod)
{
    UINT64 hr = 0;

    MagoST::SymHandle funcSH;
    std::vector<MagoST::SymHandle> blockSH;
    if (ccmod->FindFunction(ex->Address(), funcSH, blockSH) != S_OK)
        return hr;
    std::wstring symName;
    if (ccmod->GetSymbolName(funcSH, symName) != S_OK)
        return hr;
    if (symName != L"_D2rt15deh_win64_posix9terminateFZv")
        return hr;

    auto thread = ex->Thread();
    if (!thread)
        return hr;
    CComPtr<CallStack::DkmFrameRegisters> regs;
    DkmArray<CallStack::DkmUnwoundRegister*> specialRegs = { 0, 0 };
    if (thread->GetCurrentRegisters(specialRegs, &regs) != S_OK || !regs)
        return hr;

    UINT64 rsp;
    if (regs->GetStackPointer(&rsp) != S_OK)
        return hr;
    // assume hlt instruction with standard stack frame
    UINT64 prevRBP;
    UINT32 read;
    auto process = ex->Process();
    if (process->ReadMemory(rsp, DkmReadMemoryFlags::None, &prevRBP, sizeof(prevRBP), &read) != S_OK || read != sizeof(prevRBP))
        return hr;

    // prevRBP now frame pointer of _d_throwc
    UINT64 excObj;
    if (process->ReadMemory(prevRBP + 16, DkmReadMemoryFlags::None, &excObj, sizeof(excObj), &read) != S_OK || read != sizeof(prevRBP))
        return hr;

    return excObj;
}

// IDkmExceptionTriggerHitNotification
HRESULT STDMETHODCALLTYPE CMagoNatCCService::OnExceptionTriggerHit(
    _In_ Exceptions::DkmExceptionTriggerHit* pHit,
    _In_ DkmEventDescriptorS* pEventDescriptor)
{
    HRESULT hr = S_OK;
    auto ex = Native::DkmWin32ExceptionInformation::TryCast(pHit->Exception());
    if (!ex)
        return hr;

    auto code = ex->Code();
    if (code != 0xC0000096 && code != 0xE0440001) // Privileged instruction or D exception
        return hr;

    auto process = ex->Process();
    CComPtr<CCModule> ccmod;
    UINT64 excObj = 0;
    if (code == 0xC0000096)
    {
        // Win64: only handle unhandled exceptions for now
        ccmod = GetExceptionModule(ex);
        if (ccmod)
            excObj = GetExceptionObjectWin64(ex, ccmod);
    }
    else if (ex->ExceptionParameters() && ex->ExceptionParameters()->Count() > 0)
    {
        // Win32 only
        excObj = ex->ExceptionParameters()->Items()[0];
        ccmod = new CCModule();
        ccmod->InitRuntime(process);
    }
    if (!ccmod || excObj == 0)
        return hr;

    std::wstring exInfo;
    if (ccmod->GetExceptionInfo(excObj, ex->Address(), exInfo) != S_OK)
        return hr;

    exInfo.append(L"\n");
    CComPtr<DkmString> outputMessage = toDkmString(exInfo.data());
    CComPtr<DkmUserMessage> message;
    DkmUserMessage::Create(process->Connection(), process, DkmUserMessageOutputKind::UnfilteredOutputWindowMessage,
                           outputMessage, 0, S_OK, &message);
    if (message)
        message->Post();
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMagoNatCCService::GetSteppingCallSites(
    _In_ Native::DkmNativeInstructionAddress* pNativeAddress,
    _In_ const DkmArray<Symbols::DkmSteppingRange>& SteppingRanges,
    _Out_ DkmArray<Stepping::DkmNativeSteppingCallSite*>* pCallSites)
{
    // for "Step into specific"
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMagoNatCCService::IsUserCodeExtended(
    _In_ Native::DkmNativeInstructionAddress* pNativeAddress,
    _In_ DkmWorkList* pWorkList,
    _In_ IDkmCompletionRoutine<Native::DkmIsUserCodeExtendedAsyncResult>* pCompletionRoutine)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMagoNatCCService::GetFrameName(
    _In_ Evaluation::DkmInspectionContext* pInspectionContext,
    _In_ DkmWorkList* pWorkList,
    _In_ CallStack::DkmStackWalkFrame* pFrame,
    _In_ Evaluation::DkmVariableInfoFlags_t ArgumentFlags,
    _In_ IDkmCompletionRoutine<Evaluation::DkmGetFrameNameAsyncResult>* pCompletionRoutine)
{
    using namespace Evaluation;
    RefPtr<CCExprContext> exprContext;
    tryHR(InitExprContext(pInspectionContext, pWorkList, pFrame, false, exprContext));
    exprContext->SetWorkList(pWorkList);

    bool types = ArgumentFlags & DkmVariableInfoFlags::Types;
    bool names = ArgumentFlags & DkmVariableInfoFlags::Names;
    bool values = ArgumentFlags & DkmVariableInfoFlags::Values;
    auto completeFunc = [complete = RefPtr<IDkmCompletionRoutine<DkmGetFrameNameAsyncResult>>(pCompletionRoutine),
        exprContext](HRESULT hr, const std::string& funcName)
        {
            DkmGetFrameNameAsyncResult res;
            res.ErrorCode = DkmString::Create(CP_UTF8, funcName.data(), funcName.size(), &res.pFrameName);
            complete->OnComplete(res);
            return hr;
        };
    std::string funcName;
    HRESULT hr = exprContext->GetFunctionName(names, types, values,
        pInspectionContext->Radix(), funcName, completeFunc);
    return hr == S_QUEUED ? S_OK : hr;
}

HRESULT STDMETHODCALLTYPE CMagoNatCCService::GetFrameReturnType(
    _In_ Evaluation::DkmInspectionContext* pInspectionContext,
    _In_ DkmWorkList* pWorkList,
    _In_ CallStack::DkmStackWalkFrame* pFrame,
    _In_ IDkmCompletionRoutine<Evaluation::DkmGetFrameReturnTypeAsyncResult>* pCompletionRoutine)
{
    using namespace Evaluation;
    RefPtr<CCExprContext> exprContext;
    tryHR(InitExprContext(pInspectionContext, pWorkList, pFrame, false, exprContext));
    exprContext->SetWorkList(pWorkList);

    std::wstring retType = exprContext->GetFunctionReturnType();

    DkmGetFrameReturnTypeAsyncResult res;
    res.ErrorCode = retType.empty() ? E_FAIL : S_OK;
    res.pReturnType = toDkmString(retType.c_str());
    pCompletionRoutine->OnComplete(res);
    return S_OK;
}

