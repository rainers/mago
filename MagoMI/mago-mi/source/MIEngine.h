#pragma once

#include "../../DebugEngine/MagoNatDE/Common.h"
#include "cmdinput.h"
#include "miutils.h"

namespace Mago {
	class Engine;
};

//class IDebugProcess2;

class MIDebugPort;

//#define DUMP_EVENT(x) printf("%s\n", #x)
#define DUMP_EVENT(x) writeStdout("Event: %s\n", #x); CRLog::debug("Event: %s", #x)
#define UNUSED_EVENT_PARAMS UNREFERENCED_PARAMETER(pEngine); UNREFERENCED_PARAMETER(pProcess); UNREFERENCED_PARAMETER(pProgram); UNREFERENCED_PARAMETER(pThread); UNREFERENCED_PARAMETER(pEvent)

class MIEventCallback {
public:
	virtual HRESULT OnDebugEngineCreated(IDebugEngine2 *pEngine,
			IDebugProcess2 *pProcess,
			IDebugProgram2 *pProgram,
			IDebugThread2 *pThread,
			IDebugEngineCreateEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugEngineCreated);
		return S_OK; 
	}
	virtual HRESULT OnDebugProgramCreated(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugProgramCreateEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugProgramCreated);
		return S_OK;
	}
	virtual HRESULT OnDebugProgramDestroy(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugProgramDestroyEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugProgramDestroy);
		return S_OK;
	}
	virtual HRESULT OnDebugLoadComplete(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugLoadCompleteEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugLoadComplete);
		return S_OK;
	}
	virtual HRESULT OnDebugEntryPoint(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugEntryPointEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugEntryPoint);
		return S_OK;
	}
	virtual HRESULT OnDebugThreadCreate(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugThreadCreateEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugThreadCreate);
		return S_OK;
	}


	virtual HRESULT OnDebugThreadDestroy(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugThreadDestroyEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugThreadDestroy);
		return S_OK;
	}

	virtual HRESULT OnDebugStepComplete(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugStepCompleteEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugStepComplete);
		return S_OK;
	}
	virtual HRESULT OnDebugBreak(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugBreakEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugBreak);
		return S_OK;
	}
	virtual HRESULT OnDebugOutputString(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugOutputStringEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugOutputString);
		return S_OK;
	}
	virtual HRESULT OnDebugModuleLoad(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugModuleLoadEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugModuleLoad);
		return S_OK;
	}
	virtual HRESULT OnDebugSymbolSearch(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugSymbolSearchEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugSymbolSearch);
		return S_OK;
	}
	virtual HRESULT OnDebugBreakpoint(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugBreakpointEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugBreakpoint);
		return S_OK;
	}
	virtual HRESULT OnDebugBreakpointBound(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugBreakpointBoundEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugBreakpointBound);
		return S_OK;
	}
	virtual HRESULT OnDebugBreakpointError(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugBreakpointErrorEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugBreakpointError);
		return S_OK;
	}
	virtual HRESULT OnDebugBreakpointUnbound(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugBreakpointUnboundEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugBreakpointUnbound);
		return S_OK;
	}
	virtual HRESULT OnDebugException(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugExceptionEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugException);
		return S_OK;
	}
	virtual HRESULT OnDebugMessage(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugMessageEvent2 * pEvent) {
		UNUSED_EVENT_PARAMS;
		DUMP_EVENT(OnDebugMessage);
		return S_OK;
	}
};

class MIEngine : public IDebugEventCallback2 {
protected:
	CComObject<Mago::Engine> * engine;
	IDebugProcess2* debugProcess;
	MIEventCallback * callback;
	MIDebugPort * debugPort;
public:
	MIEngine();
	~MIEngine();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);
	virtual HRESULT STDMETHODCALLTYPE Event(
		/* [in] */ __RPC__in_opt IDebugEngine2 *pEngine,
		/* [in] */ __RPC__in_opt IDebugProcess2 *pProcess,
		/* [in] */ __RPC__in_opt IDebugProgram2 *pProgram,
		/* [in] */ __RPC__in_opt IDebugThread2 *pThread,
		/* [in] */ __RPC__in_opt IDebugEvent2 *pEvent,
		/* [in] */ __RPC__in REFIID riidEvent,
		/* [in] */ DWORD dwAttrib);

	HRESULT Init(MIEventCallback * callback);

	HRESULT Launch(const wchar_t * pszExe,
		const wchar_t * pszArgs,
		const wchar_t * pszDir
		);
	HRESULT ResumeProcess();
	HRESULT CreatePendingBreakpoint(BreakpointInfoRef & bp);
};
