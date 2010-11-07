/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Events.h"
#include "Program.h"


static const GUID   GuidWin32ExceptionType = {0x3B476D35,0xA401,0x11D2,{0xAA,0xD4,0x00,0xC0,0x4F,0x99,0x01,0x71}};
static const DWORD  DExceptionCode = 0xE0440001;


namespace Mago
{
//----------------------------------------------------------------------------
//  EngineCreateEvent
//----------------------------------------------------------------------------

    void EngineCreateEvent::Init( IDebugEngine2* engine )
    {
        _ASSERT( engine != NULL );
        _ASSERT( mEngine == NULL );
        mEngine = engine;
    }

    HRESULT EngineCreateEvent::GetEngine( IDebugEngine2** pEngine )
    {
        if ( pEngine == NULL )
            return E_INVALIDARG;

        _ASSERT( mEngine != NULL );
        *pEngine = mEngine;
        (*pEngine)->AddRef();
        return S_OK;
    }


//----------------------------------------------------------------------------
//  ProgramDestroyEvent
//----------------------------------------------------------------------------

    ProgramDestroyEvent::ProgramDestroyEvent()
        :   mExitCode( 0 )
    {
    }

    void ProgramDestroyEvent::Init( DWORD exitCode )
    {
        _ASSERT( mExitCode == 0 );
        mExitCode = exitCode;
    }

    HRESULT ProgramDestroyEvent::GetExitCode( DWORD* pdwExit )
    {
        if ( pdwExit == NULL )
            return E_INVALIDARG;

        *pdwExit = mExitCode;
        return S_OK;
    }


//----------------------------------------------------------------------------
//  ThreadDestroyEvent
//----------------------------------------------------------------------------

    ThreadDestroyEvent::ThreadDestroyEvent()
        :   mExitCode( 0 )
    {
    }

    void ThreadDestroyEvent::Init( DWORD exitCode )
    {
        _ASSERT( mExitCode == 0 );
        mExitCode = exitCode;
    }

    HRESULT ThreadDestroyEvent::GetExitCode( DWORD* pdwExit )
    {
        if ( pdwExit == NULL )
            return E_INVALIDARG;

        *pdwExit = mExitCode;
        return S_OK;
    }


//----------------------------------------------------------------------------
//  OutputStringEvent
//----------------------------------------------------------------------------

    void OutputStringEvent::Init( const wchar_t* str )
    {
        _ASSERT( str != NULL );
        _ASSERT( mStr == NULL );
        mStr = str;
    }

    HRESULT OutputStringEvent::GetString( BSTR* pbstrString )
    {
        if ( pbstrString == NULL )
            return E_INVALIDARG;

        *pbstrString = SysAllocString( mStr );
        return *pbstrString != NULL ? S_OK : E_OUTOFMEMORY;
    }


//----------------------------------------------------------------------------
//  ModuleLoadEvent
//----------------------------------------------------------------------------

    ModuleLoadEvent::ModuleLoadEvent()
        :   mLoad( false )
    {
    }

    void ModuleLoadEvent::Init(
       IDebugModule2*   module,
       const wchar_t*   debugMessage,
       bool             load )
    {
        _ASSERT( module != NULL );
        _ASSERT( mMod == NULL );
        _ASSERT( mMsg == NULL );

        mMod = module;
        mLoad = load;

        // don't worry if we can't allocate the string or if it's NULL
        mMsg.Attach( SysAllocString( debugMessage ) );
    }

    HRESULT ModuleLoadEvent::GetModule( 
       IDebugModule2** pModule,
       BSTR*           pbstrDebugMessage,
       BOOL*           pbLoad )
    {
        _ASSERT( mMod != NULL );

        if ( pbstrDebugMessage != NULL )
        {
            *pbstrDebugMessage = SysAllocString( mMsg );
        }

        if ( pbLoad != NULL )
            *pbLoad = mLoad;

        *pModule = mMod;
        (*pModule)->AddRef();

        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  SymbolSearchEvent
    //----------------------------------------------------------------------------

    SymbolSearchEvent::SymbolSearchEvent()
        :   mInfoFlags( 0 )
    {
    }

    void SymbolSearchEvent::Init(
       IDebugModule3*       module,
       const wchar_t*       debugMessage,
       MODULE_INFO_FLAGS    infoFlags )
    {
        _ASSERT( module != NULL );
        _ASSERT( mMod == NULL );
        _ASSERT( mMsg == NULL );

        mMod = module;
        mInfoFlags = infoFlags;
        // don't worry if we can't allocate the string or if it's NULL
        mMsg.Attach( SysAllocString( debugMessage ) );
    }

    HRESULT SymbolSearchEvent::GetSymbolSearchInfo( 
       IDebugModule3**      pModule,
       BSTR*                pbstrDebugMessage,
       MODULE_INFO_FLAGS*   pdwModuleInfoFlags )
    {
        _ASSERT( mMod != NULL );

        if ( pbstrDebugMessage != NULL )
        {
            *pbstrDebugMessage = SysAllocString( mMsg );
        }

        if ( pdwModuleInfoFlags != NULL )
            *pdwModuleInfoFlags = mInfoFlags;

        *pModule = mMod;
        (*pModule)->AddRef();

        return S_OK;
    }


//----------------------------------------------------------------------------
//  BreakpointEvent
//----------------------------------------------------------------------------

    void BreakpointEvent::Init( 
        IEnumDebugBoundBreakpoints2* pEnum )
    {
        _ASSERT( pEnum != NULL );
        _ASSERT( mEnumBP == NULL );
        mEnumBP = pEnum;
    }

    HRESULT BreakpointEvent::EnumBreakpoints( IEnumDebugBoundBreakpoints2** ppEnum )
    {
        if ( ppEnum == NULL )
            return E_INVALIDARG;

        _ASSERT( mEnumBP != NULL );
        *ppEnum = mEnumBP;
        (*ppEnum)->AddRef();
        return S_OK;
    }

//----------------------------------------------------------------------------
//  BreakpointBoundEvent
//----------------------------------------------------------------------------

    void BreakpointBoundEvent::Init( 
        IEnumDebugBoundBreakpoints2* pEnum, 
        IDebugPendingBreakpoint2* pPending )
    {
        _ASSERT( pEnum != NULL );
        _ASSERT( pPending != NULL );
        _ASSERT( mEnumBoundBP == NULL );
        _ASSERT( mPendingBP == NULL );
        mEnumBoundBP = pEnum;
        mPendingBP = pPending;
    }

    HRESULT BreakpointBoundEvent::GetPendingBreakpoint( IDebugPendingBreakpoint2** ppPendingBP )
    {
        if ( ppPendingBP == NULL )
            return E_INVALIDARG;

        _ASSERT( mPendingBP != NULL );
        *ppPendingBP = mPendingBP;
        (*ppPendingBP)->AddRef();
        return S_OK;
    }

    HRESULT BreakpointBoundEvent::EnumBoundBreakpoints( IEnumDebugBoundBreakpoints2** ppEnum )
    {
        if ( ppEnum == NULL )
            return E_INVALIDARG;

        _ASSERT( mEnumBoundBP != NULL );
        *ppEnum = mEnumBoundBP;
        (*ppEnum)->AddRef();
        return S_OK;
    }


//----------------------------------------------------------------------------
//  BreakpointErrorEvent
//----------------------------------------------------------------------------

    void BreakpointErrorEvent::Init( 
        IDebugErrorBreakpoint2* pError )
    {
        _ASSERT( pError != NULL );
        _ASSERT( mErrorBP == NULL );
        mErrorBP = pError;
    }

    HRESULT BreakpointErrorEvent::GetErrorBreakpoint( IDebugErrorBreakpoint2** ppErrorBP )
    {
        if ( ppErrorBP == NULL )
            return E_INVALIDARG;

        _ASSERT( mErrorBP != NULL );
        *ppErrorBP = mErrorBP;
        (*ppErrorBP)->AddRef();
        return S_OK;
    }


//----------------------------------------------------------------------------
//  BreakpointUnboundEvent
//----------------------------------------------------------------------------

    BreakpointUnboundEvent::BreakpointUnboundEvent()
        :   mReason( BPUR_UNKNOWN )
    {
    }

    void BreakpointUnboundEvent::Init(
        IDebugBoundBreakpoint2* pBound, BP_UNBOUND_REASON reason )
    {
        _ASSERT( pBound != NULL );
        _ASSERT( reason != NULL );
        _ASSERT( mBoundBP == NULL );
        mBoundBP = pBound;
        mReason = reason;
    }

    HRESULT BreakpointUnboundEvent::GetBreakpoint( IDebugBoundBreakpoint2** ppBoundBP )
    {
        if ( ppBoundBP == NULL )
            return E_INVALIDARG;

        _ASSERT( mBoundBP != NULL );
        *ppBoundBP = mBoundBP;
        (*ppBoundBP)->AddRef();
        return S_OK;
    }

    HRESULT BreakpointUnboundEvent::GetReason( BP_UNBOUND_REASON* pdwUnboundReason )
    {
        if ( pdwUnboundReason == NULL )
            return E_INVALIDARG;

        *pdwUnboundReason = mReason;
        return S_OK;
    }


//----------------------------------------------------------------------------
//  ExceptionEvent
//----------------------------------------------------------------------------

    ExceptionEvent::ExceptionEvent()
        :   mCode( 0 ),
            mState( EXCEPTION_NONE ),
            mGuidType( GUID_NULL )
    {
    }

    void ExceptionEvent::Init( 
        Program* prog, 
        bool firstChance, 
        const EXCEPTION_RECORD* record, 
        bool canPassToDebuggee )
    {
        mProg = prog;
        mState = firstChance ? EXCEPTION_STOP_FIRST_CHANCE : EXCEPTION_STOP_SECOND_CHANCE;
        mCode = record->ExceptionCode;
        mCanPassToDebuggee = canPassToDebuggee;

        wchar_t name[100] = L"";
        if ( record->ExceptionCode == DExceptionCode )
        {
            mGuidType = GetEngineId();
            if ( IProcess* process = prog->GetCoreProcess() )
            {
                GetClassName( process, record->ExceptionInformation[0], &mExceptionName );
                GetExceptionInfo( process, record->ExceptionInformation[0], &mExceptionInfo );
            }

            if ( mExceptionName == NULL )
                mExceptionName = L"D Exception";
        }
        else
        {
            // make it a Win32 exception
            mGuidType = GuidWin32ExceptionType;
            swprintf_s( name, L"%08x", record->ExceptionCode );
            mExceptionName = name;
        }
    }

    HRESULT ExceptionEvent::GetException( EXCEPTION_INFO* pExceptionInfo )
    {
        if ( pExceptionInfo == NULL )
            return E_INVALIDARG;

        memset( pExceptionInfo, 0, sizeof *pExceptionInfo );

        _ASSERT( mProg.Get() != NULL );
        mProg->QueryInterface( __uuidof( IDebugProgram2 ), (void**) &pExceptionInfo->pProgram );
        
        mProg->GetName( &pExceptionInfo->bstrProgramName );
        
        pExceptionInfo->guidType = mGuidType;
        pExceptionInfo->dwState = mState;
        pExceptionInfo->dwCode = mCode;
        pExceptionInfo->bstrExceptionName = mExceptionName.Copy();

        return S_OK;
    }

    HRESULT ExceptionEvent::GetExceptionDescription( BSTR* pbstrDescription )
    {
        if ( pbstrDescription == NULL )
            return E_INVALIDARG;

        bool    firstChance = (mState & EXCEPTION_STOP_FIRST_CHANCE) != 0;
        wchar_t msg[256] = L"";
        swprintf_s( msg, L"%ls exception: %s ", (firstChance ? L"First-chance" : L"Unhandled"), mExceptionName.m_str );

        if ( mExceptionInfo )
        {
            CComBSTR bmsg( msg );
            bmsg.Append( mExceptionInfo );
            *pbstrDescription = bmsg.Detach();
        }
        else
            *pbstrDescription = SysAllocString( msg );
        return S_OK;
    }

    void ExceptionEvent::SetExceptionName( LPCOLESTR name )
    {
        mExceptionName = name;
    }

    HRESULT ExceptionEvent::CanPassToDebuggee()
    {
        return mCanPassToDebuggee ? S_OK : S_FALSE;
    }

    HRESULT ExceptionEvent::PassToDebuggee( BOOL fPass )
    {
        mProg->SetPassExceptionToDebuggee( fPass ? true : false );
        return S_OK;
    }


//----------------------------------------------------------------------------
//  MessageTextEvent
//----------------------------------------------------------------------------

    MessageTextEvent::MessageTextEvent()
        :   mMessageType( MT_OUTPUTSTRING )
    {
    }

    void MessageTextEvent::Init( 
        MESSAGETYPE reason, const wchar_t* msg )
    {
        _ASSERT( msg != NULL );
        mMessageType = (reason & MT_REASON_MASK) | MT_OUTPUTSTRING;
        mMessage = msg;
    }

    HRESULT MessageTextEvent::GetMessageW( 
            MESSAGETYPE*    pMessageType,
            BSTR*           pbstrMessage,
            DWORD*          pdwType,
            BSTR*           pbstrHelpFileName,
            DWORD*          pdwHelpId )
    {
        if ( (pMessageType == NULL)
            || (pbstrMessage == NULL)
            || (pdwType == NULL)
            || (pbstrHelpFileName == NULL)
            || (pdwHelpId == NULL) )
            return E_INVALIDARG;

        *pbstrMessage = mMessage.Copy();
        if ( *pbstrMessage == NULL )
            return E_OUTOFMEMORY;

        *pMessageType = mMessageType;
        *pdwType = 0;
        *pbstrHelpFileName = NULL;
        *pdwHelpId = 0;
        return S_OK;
    }

    HRESULT MessageTextEvent::SetResponse( DWORD dwResponse )
    {
        return S_OK;
    }
}
