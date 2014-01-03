/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Program.h"
#include "IDebuggerProxy.h"
#include "Thread.h"
#include "Module.h"
#include "ComEnumWithCount.h"

#include "Engine.h"
#include "PendingBreakpoint.h"

#include "MemoryBytes.h"
#include "CodeContext.h"
#include "DisassemblyStream.h"
#include "DRuntime.h"
#include "ICoreProcess.h"
#include <algorithm>


typedef CComEnumWithCount< 
    IEnumDebugThreads2, 
    &IID_IEnumDebugThreads2, 
    IDebugThread2*, 
    _CopyInterface<IDebugThread2>,
    CComMultiThreadModel
> EnumDebugThreads;

typedef CComEnumWithCount< 
    IEnumDebugModules2, 
    &IID_IEnumDebugModules2, 
    IDebugModule2*, 
    _CopyInterface<IDebugModule2>,
    CComMultiThreadModel
> EnumDebugModules;

typedef CComEnumWithCount< 
    IEnumDebugCodeContexts2, 
    &IID_IEnumDebugCodeContexts2, 
    IDebugCodeContext2*, 
    _CopyInterface<IDebugCodeContext2>, 
    CComMultiThreadModel
> EnumDebugCodeContexts;


namespace Mago
{
    // Program

    Program::Program()
    :   mProgId( GUID_NULL ),
        mAttached( false ),
        mPassExceptionToDebuggee( true ),
        mCanPassExceptionToDebuggee( true ),
        mDebugger( NULL ),
        mNextModLoadIndex( 0 ),
        mEntryPoint( 0 )
    {
    }

    Program::~Program()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugProgram2 methods

    HRESULT Program::EnumThreads( IEnumDebugThreads2** ppEnum )
    {
        GuardedArea guard( mThreadGuard );
        return MakeEnumWithCount<
            EnumDebugThreads, 
            IEnumDebugThreads2, 
            ThreadMap, 
            IDebugThread2>( mThreadMap, ppEnum );
    }

    HRESULT Program::GetName( BSTR* pbstrName )
    {
        if ( pbstrName == NULL )
            return E_INVALIDARG;

        *pbstrName = mName.Copy();
        return *pbstrName != NULL ? S_OK : E_OUTOFMEMORY;
    }

    HRESULT Program::GetProcess( IDebugProcess2** ppProcess )
    {
        return mProcess.CopyTo( ppProcess );
    }

    HRESULT Program::Terminate()
    {
        HRESULT hr = S_OK;

        hr = mDebugger->Terminate( mCoreProc.Get() );
        _ASSERT( hr == S_OK );

        return hr;
    }

    HRESULT Program::Attach( IDebugEventCallback2* pCallback )
    { return E_NOTIMPL; } 
    HRESULT Program::Detach()
    { return E_NOTIMPL; } 
    HRESULT Program::GetDebugProperty( IDebugProperty2** ppProperty )
    { return E_NOTIMPL; } 

    HRESULT Program::CauseBreak()
    {
        HRESULT hr = S_OK;

        hr = mDebugger->AsyncBreak( mCoreProc.Get() );

        return hr;
    }

    HRESULT Program::GetEngineInfo( BSTR* pbstrEngine, GUID* pguidEngine )
    {
        if ( (pbstrEngine == NULL) || (pguidEngine == NULL) )
            return E_INVALIDARG;

        *pbstrEngine = SysAllocString( ::GetEngineName() );
        *pguidEngine = ::GetEngineId();
        return *pbstrEngine != NULL ? S_OK : E_OUTOFMEMORY;
    }

    HRESULT Program::EnumCodeContexts( IDebugDocumentPosition2* pDocPos, 
                                       IEnumDebugCodeContexts2** ppEnum )
    {
        if ( (pDocPos == NULL) || (ppEnum == NULL) )
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        CComBSTR                bstrFileName;
        CAutoVectorPtr<char>    u8FileName;
        size_t                  u8FileNameLen = 0;
        TEXT_POSITION           startPos = { 0 };
        TEXT_POSITION           endPos = { 0 };
        std::list<AddressBinding>   bindings;

        hr = pDocPos->GetFileName( &bstrFileName );
        if ( FAILED( hr ) )
            return hr;

        hr = pDocPos->GetRange( &startPos, &endPos );
        if ( FAILED( hr ) )
            return hr;

        // AD7 lines are 0-based, DIA/CV ones are 1-based
        startPos.dwLine++;
        endPos.dwLine++;

        hr = Utf16To8( bstrFileName, bstrFileName.Length(), u8FileName.m_p, u8FileNameLen );
        if ( FAILED( hr ) )
            return hr;

        bool    foundExact = BindCodeContextsToFile( 
            true, 
            u8FileName, 
            u8FileNameLen, 
            (uint16_t) startPos.dwLine, 
            (uint16_t) endPos.dwLine, 
            bindings );

        if ( !foundExact )
            foundExact = BindCodeContextsToFile( 
            false, 
            u8FileName, 
            u8FileNameLen, 
            (uint16_t) startPos.dwLine, 
            (uint16_t) endPos.dwLine, 
            bindings );

        InterfaceArray<IDebugCodeContext2>  codeContextArray( bindings.size() );
        int i = 0;

        if ( codeContextArray.Get() == NULL )
            return E_OUTOFMEMORY;

        for ( std::list<AddressBinding>::iterator it = bindings.begin();
            it != bindings.end();
            it++, i++ )
        {
            RefPtr<CodeContext> codeContext;

            hr = MakeCComObject( codeContext );
            if ( FAILED( hr ) )
                return hr;

            hr = codeContext->Init( (Address) it->Addr, it->Mod, NULL );
            if ( FAILED( hr ) )
                return hr;

            codeContext->QueryInterface( __uuidof( IDebugCodeContext2 ), (void**) &codeContextArray[i] );
        }

        return MakeEnumWithCount<EnumDebugCodeContexts>( codeContextArray, ppEnum );
    }

    bool Program::BindCodeContextsToFile( 
        bool exactMatch, 
        const char* fileName, 
        size_t fileNameLen, 
        uint16_t reqLineStart, 
        uint16_t reqLineEnd,
        std::list<AddressBinding>& bindings )
    {
        GuardedArea guard( mModGuard );

        HRESULT hr = S_OK;
        bool    foundMatch = false;

        for ( ModuleMap::iterator it = mModMap.begin();
            it != mModMap.end();
            it++ )
        {
            RefPtr<MagoST::ISession>    session;
            Module*                     mod = it->second;
            uint32_t                    compCount = 0;

            if ( !mod->GetSymbolSession( session ) )
                continue;

            hr = session->GetCompilandCount( compCount );
            if ( FAILED( hr ) )
                continue;

            for ( uint16_t compIx = 1; compIx <= compCount; compIx++ )
            {
                MagoST::CompilandInfo   compInfo = { 0 };

                hr = session->GetCompilandInfo( compIx, compInfo );
                if ( FAILED( hr ) )
                    continue;

                for ( uint16_t fileIx = 0; fileIx < compInfo.FileCount; fileIx++ )
                {
                    MagoST::FileInfo    fileInfo = { 0 };
                    bool                matches = false;

                    hr = session->GetFileInfo( compIx, fileIx, fileInfo );
                    if ( FAILED( hr ) )
                        continue;

                    if ( exactMatch )
                        matches = ExactFileNameMatch( fileName, fileNameLen, fileInfo.Name.ptr, fileInfo.Name.length );
                    else
                        matches = PartialFileNameMatch( fileName, fileNameLen, fileInfo.Name.ptr, fileInfo.Name.length );

                    if ( !matches )
                        continue;

                    foundMatch = true;

                    MagoST::LineNumber  line = { 0 };
                    if ( !session->FindLineByNum( compIx, fileIx, (uint16_t) reqLineStart, line ) )
                        continue;

                    // do the line ranges overlap?
                    if ( ((line.Number <= reqLineEnd) && (line.NumberEnd >= reqLineStart)) )
                    {
                        line.NumberEnd = line.Number;

                        do
                        {
                            bindings.push_back( AddressBinding() );
                            bindings.back().Addr = session->GetVAFromSecOffset( line.Section, line.Offset );
                            bindings.back().Mod = mod;
                        }
                        while( session->FindNextLineByNum( compIx, fileIx, (uint16_t) reqLineStart, line ) );
                    }
                }
            }
        }

        return foundMatch;
    }

    HRESULT Program::GetMemoryBytes( IDebugMemoryBytes2** ppMemoryBytes )
    {
        if ( ppMemoryBytes == NULL )
            return E_INVALIDARG;
        if ( mProgMod == NULL )
            return E_FAIL;

        HRESULT hr = S_OK;
        Address addr = mProgMod->GetAddress();
        DWORD   size = mProgMod->GetSize();
        RefPtr<MemoryBytes> memBytes;

        hr = MakeCComObject( memBytes );
        if ( FAILED( hr ) )
            return hr;

        memBytes->Init( addr, size, mDebugger, mCoreProc );

        *ppMemoryBytes = memBytes.Detach();
        return S_OK;
    }

    HRESULT Program::GetDisassemblyStream( DISASSEMBLY_STREAM_SCOPE dwScope, 
                                           IDebugCodeContext2* pCodeContext, 
                                           IDebugDisassemblyStream2** ppDisassemblyStream )
    {
        if ( (pCodeContext == NULL) || (ppDisassemblyStream == NULL) )
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        Address addr = 0;
        RefPtr<DisassemblyStream>       stream;
        RefPtr<Module>                  mod;
        CComQIPtr<IMagoMemoryContext>   magoMem = pCodeContext;

        if ( magoMem == NULL )
            return E_INVALIDARG;

        magoMem->GetAddress( addr );

        _RPT2( _CRT_WARN, "Program::GetDisassemblyStream: addr=%08X scope=%X\n", addr, dwScope );

        if ( !FindModuleContainingAddress( addr, mod ) )
            return HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND );

        hr = MakeCComObject( stream );
        if ( FAILED( hr ) )
            return hr;

        hr = stream->Init( dwScope, addr, this, mDebugger );
        if ( FAILED( hr ) )
            return hr;

        *ppDisassemblyStream = stream.Detach();
        return S_OK;
    }

    HRESULT Program::EnumModules( IEnumDebugModules2** ppEnum )
    {
        GuardedArea guard( mModGuard );
        return MakeEnumWithCount<
            EnumDebugModules, 
            IEnumDebugModules2, 
            ModuleMap, 
            IDebugModule2>( mModMap, ppEnum );
    }

    HRESULT Program::GetENCUpdate( IDebugENCUpdate** ppUpdate )
    { return E_NOTIMPL; } 
    HRESULT Program::EnumCodePaths( LPCOLESTR pszHint, 
                                    IDebugCodeContext2* pStart, 
                                    IDebugStackFrame2* pFrame, 
                                    BOOL fSource, 
                                    IEnumCodePaths2** ppEnum, 
                                    IDebugCodeContext2** ppSafety )
    { return E_NOTIMPL; } 
    HRESULT Program::WriteDump( DUMPTYPE DumpType,LPCOLESTR pszCrashDumpUrl )
    { return E_NOTIMPL; } 
    HRESULT Program::CanDetach()
    { return E_NOTIMPL; } 

    HRESULT Program::GetProgramId( GUID* pguidProgramId )
    {
        if ( pguidProgramId == NULL )
            return E_INVALIDARG;

        *pguidProgramId = mProgId;
        return S_OK; 
    } 

    HRESULT Program::Execute()
    {
        return mDebugger->Execute( GetCoreProcess(), !mPassExceptionToDebuggee );
    }

    HRESULT Program::Continue( IDebugThread2 *pThread )
    {
        return mDebugger->Continue( GetCoreProcess(), !mPassExceptionToDebuggee );
    }

    HRESULT Program::Step( IDebugThread2 *pThread, STEPKIND sk, STEPUNIT step )
    {
        if ( pThread == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        hr = StepInternal( pThread, sk, step );
        if ( FAILED( hr ) )
        {
            hr = mDebugger->Execute( GetCoreProcess(), !mPassExceptionToDebuggee );
        }

        return hr;
    }

    HRESULT Program::StepInternal( IDebugThread2* pThread, STEPKIND sk, STEPUNIT step )
    {
        _ASSERT( pThread != NULL );

        HRESULT         hr = S_OK;
        DWORD           threadId = 0;
        RefPtr<Thread>  thread;

        // another way to do this is to use a private interface
        hr = pThread->GetThreadId( &threadId );
        if ( FAILED( hr ) )
            return hr;

        if ( !FindThread( threadId, thread ) )
            return E_NOT_FOUND;

        hr = thread->Step( mCoreProc.Get(), sk, step, !mPassExceptionToDebuggee );

        return hr;
    }


    //----------------------------------------------------------------------------

    void Program::Dispose()
    {
        mThreadMap.clear();

        for ( ModuleMap::iterator it = mModMap.begin(); it != mModMap.end(); it++ )
        {
            it->second->Dispose();
        }

        mModMap.clear();

        mProgMod.Release();
        mEngine.Release();
    }

    void        Program::SetEngine( Engine* engine )
    {
        mEngine = engine;
    }

    ICoreProcess*   Program::GetCoreProcess()
    {
        return mCoreProc.Get();
    }

    void        Program::GetCoreProcess( ICoreProcess*& proc )
    {
        proc = mCoreProc.Get();
        proc->AddRef();
    }

    void Program::SetCoreProcess( ICoreProcess* proc )
    {
        mCoreProc = proc;
    }

    void Program::SetProcess( IDebugProcess2* proc )
    {
        mProcess = proc;

        proc->GetName( GN_NAME, &mName );
    }

    IDebugEventCallback2*   Program::GetCallback()
    {
        return mCallback;
    }

    void Program::SetCallback( IDebugEventCallback2* callback )
    {
        mCallback = callback;
    }

    void Program::SetPortSettings( IDebugProgram2* portProgram )
    {
        HRESULT hr = S_OK;

        hr = portProgram->GetProgramId( &mProgId );
        _ASSERT( hr == S_OK );
    }

    void Program::SetDebuggerProxy( IDebuggerProxy* debugger )
    {
        mDebugger = debugger;
    }

    DRuntime* Program::GetDRuntime()
    {
        return mDRuntime.get();
    }

    void Program::SetDRuntime( std::unique_ptr<DRuntime>& druntime )
    {
        mDRuntime.reset( NULL );
        mDRuntime.swap( druntime );
    }

    bool Program::GetAttached()
    {
        return mAttached;
    }

    void Program::SetAttached()
    {
        mAttached = true;
    }

    void Program::SetPassExceptionToDebuggee( bool value )
    {
        if ( mCanPassExceptionToDebuggee )
            mPassExceptionToDebuggee = value;
    }

    bool Program::CanPassExceptionToDebuggee()
    {
        return mCanPassExceptionToDebuggee;
    }

    void Program::NotifyException( bool firstChance, const EXCEPTION_RECORD* exceptRec )
    {
        if ( exceptRec->ExceptionCode == EXCEPTION_BREAKPOINT )
        {
            mCanPassExceptionToDebuggee = false;
            mPassExceptionToDebuggee = false;
        }
        else
        {
            mCanPassExceptionToDebuggee = firstChance;
            mPassExceptionToDebuggee = firstChance;
        }
    }

    HRESULT Program::CreateThread( ICoreThread* coreThread, RefPtr<Thread>& thread )
    {
        HRESULT hr = S_OK;

        hr = MakeCComObject( thread );
        if ( FAILED( hr ) )
            return hr;

        thread->SetCoreThread( coreThread );

        return hr;
    }

    HRESULT Program::AddThread( Thread* thread )
    {
        GuardedArea guard( mThreadGuard );
        DWORD   id = 0;

        thread->GetThreadId( &id );
        if ( id == 0 )
            return E_FAIL;

        ThreadMap::iterator it = mThreadMap.find( id );

        if ( it != mThreadMap.end() )
            return E_FAIL;

        thread->SetProgram( this, mDebugger );

        mThreadMap.insert( ThreadMap::value_type( id, thread ) );

        return S_OK;
    }

    bool    Program::FindThread( DWORD threadId, RefPtr<Thread>& thread )
    {
        GuardedArea guard( mThreadGuard );
        ThreadMap::iterator it = mThreadMap.find( threadId );

        if ( it == mThreadMap.end() )
            return false;

        thread = it->second;
        return true;
    }

    void Program::DeleteThread( Thread* thread )
    {
        GuardedArea guard( mThreadGuard );
        mThreadMap.erase( thread->GetCoreThread()->GetTid() );
    }

    HRESULT Program::CreateModule( ICoreModule* coreModule, RefPtr<Module>& mod )
    {
        HRESULT hr = S_OK;

        hr = MakeCComObject( mod );
        if ( FAILED( hr ) )
            return hr;

        mod->SetId( mEngine->GetNextModuleId() );
        mod->SetCoreModule( coreModule );

        return hr;
    }

    HRESULT Program::AddModule( Module* mod )
    {
        GuardedArea guard( mModGuard );
        Address addr = 0;

        addr = mod->GetAddress();
        if ( addr == 0 )
            return E_FAIL;

        if ( mProgMod == NULL )
            mProgMod = mod;

        ModuleMap::iterator it = mModMap.find( addr );

        if ( it != mModMap.end() )
            return E_FAIL;

        mModMap.insert( ModuleMap::value_type( addr, mod ) );

        DWORD   index = mNextModLoadIndex++;

        mod->SetLoadIndex( index );

        return S_OK;
    }

    bool    Program::FindModule( Address addr, RefPtr<Module>& mod )
    {
        GuardedArea guard( mModGuard );
        ModuleMap::iterator it = mModMap.find( addr );

        if ( it == mModMap.end() )
            return false;

        mod = it->second;
        return true;
    }

    bool    Program::FindModuleContainingAddress( Address address, RefPtr<Module>& refMod )
    {
        GuardedArea guard( mModGuard );

        for ( ModuleMap::iterator it = mModMap.begin();
            it != mModMap.end();
            it++ )
        {
            Module*         mod = it->second.Get();
            Address         base = mod->GetAddress();
            Address         limit = base + mod->GetSize();

            if ( (base <= address) && (limit > address) )
            {
                refMod = mod;
                return true;
            }
        }

        return false;
    }

    void Program::DeleteModule( Module* mod )
    {
        GuardedArea guard( mModGuard );

        // no need to decrement the load index of all modules after that deleted one

        mModMap.erase( mod->GetAddress() );

        mod->Dispose();
    }

    void Program::ForeachModule( ModuleCallback* callback )
    {
        GuardedArea guard( mModGuard );

        for ( ModuleMap::iterator it = mModMap.begin();
            it != mModMap.end();
            it++ )
        {
            if ( !callback->AcceptModule( it->second.Get() ) )
                break;
        }
    }


    HRESULT Program::SetInternalBreakpoint( Address address, BPCookie cookie )
    {
        HRESULT hr = S_OK;

        {
            GuardedArea guard( mBPGuard );

            BPMap::iterator itVec = mBPMap.find( address );

            if ( itVec != mBPMap.end() )
            {
                // There's at least one cookie for this address already.
                // So, add this one if needed, and leave, because the BP is already set.
                CookieVec& vec = itVec->second;
                CookieVec::iterator itCookie = std::find( vec.begin(), vec.end(), cookie );
                if ( itCookie == vec.end() )
                    vec.push_back( cookie );

                return S_OK;
            }
        }

        // You can deadlock with the event callback, if you set a BP while the BP table is locked.
        hr = mDebugger->SetBreakpoint( mCoreProc, address );
        if ( FAILED( hr ) )
            return hr;

        // check everything again, in case anything changed since the last time we locked the table
        {
            GuardedArea guard( mBPGuard );

            BPMap::iterator itVec = mBPMap.find( address );

            if ( itVec == mBPMap.end() )
            {
                std::pair<BPMap::iterator, bool> pair =
                    mBPMap.insert( BPMap::value_type( address, std::vector<BPCookie>() ) );

                itVec = pair.first;
                itVec->second.push_back( cookie );
            }
            else
            {
                CookieVec& vec = itVec->second;
                CookieVec::iterator itCookie = std::find( vec.begin(), vec.end(), cookie );
                if ( itCookie == vec.end() )
                    vec.push_back( cookie );
            }
        }

        return S_OK;
    }

    HRESULT Program::RemoveInternalBreakpoint( Address address, BPCookie cookie )
    {
        HRESULT hr = S_OK;

        {
            GuardedArea guard( mBPGuard );

            BPMap::iterator itVec = mBPMap.find( address );
            if ( itVec == mBPMap.end() )
                return S_OK;

            CookieVec& vec = itVec->second;
            CookieVec::iterator itCookie = std::find( vec.begin(), vec.end(), cookie );
            if ( itCookie == vec.end() )
                return S_OK;

            // Clear the BP only when all cookies are gone. There are others, so remove this one only.
            if ( vec.size() > 1 )
            {
                vec.erase( itCookie );
                return S_OK;
            }
        }

        // You can deadlock with the event callback, if you clear a BP while the BP table is locked.
        hr = mDebugger->RemoveBreakpoint( mCoreProc, address );
        if ( FAILED( hr ) )
            return hr;

        // check everything again, in case anything changed since the last time we locked the table
        {
            GuardedArea guard( mBPGuard );

            BPMap::iterator itVec = mBPMap.find( address );
            if ( itVec == mBPMap.end() )
                return S_OK;

            CookieVec& vec = itVec->second;
            CookieVec::iterator itCookie = std::find( vec.begin(), vec.end(), cookie );
            if ( itCookie == vec.end() )
                return S_OK;

            vec.erase( itCookie );

            if ( vec.size() == 0 )
            {
                mBPMap.erase( itVec );
            }
        }

        return S_OK;
    }

    HRESULT Program::EnumBPCookies( Address address, std::vector< BPCookie >& iter )
    {
        GuardedArea guard( mBPGuard );

        iter.clear();

        BPMap::iterator itVec = mBPMap.find( address );
        if ( itVec == mBPMap.end() )
            return S_OK;

        CookieVec& vec = itVec->second;
        iter.reserve( vec.size() );

        for ( CookieVec::iterator it = vec.begin(); it != vec.end(); it++ )
        {
            iter.push_back( *it );
        }

        return S_OK;
    }

    Address Program::GetEntryPoint()
    {
        return mEntryPoint;
    }

    void Program::SetEntryPoint( Address address )
    {
        mEntryPoint = address;
    }
}
