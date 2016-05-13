#pragma once

#include <windows.h>
#include <stdint.h>
#include <string>
#include <list>
#include "SmartPtr.h"
#include "../../DebugEngine/Exec/Types.h"
#include "../../DebugEngine/Exec/Error.h"
#include "../../DebugEngine/Exec/Module.h"
#include "../../DebugEngine/Exec/Process.h"
#include "../../DebugEngine/Exec/EventCallback.h"
#include "../../DebugEngine/Exec/DebuggerProxy.h"
#include "../../DebugEngine/Exec/IModule.h"
#include "cmdinput.h"
#include "MIEngine.h"

class Debugger : public MIEventCallback, public CmdInputCallback {
	RefPtr<MIEngine> _engine;
	IDebugProcess2 * _pProcess;
	IDebugProgram2 *_pProgram;
	IDebugThread2 *_pThread;
	bool _quitRequested;
	bool _verbose;
	bool _loadCalled;
	bool _loaded;
	bool _started;
	bool _paused;
	bool _stopped;
public:
	Debugger();
	virtual ~Debugger();

	// IEventCallback methods

	virtual void writeOutput(std::wstring msg);
	virtual void writeOutput(std::string msg);
	virtual void writeOutput(const char * msg);
	virtual void writeOutput(const wchar_t * msg);
	// MI interface stdout output: ch"msg_text"
	virtual void writeStringMessage(wchar_t ch, std::wstring msg);
	// MI interface stdout output: ~"msg_text"
	virtual void writeDebuggerMessage(std::wstring msg);
	// MI interface stdout output: [##requestId##]^result[,"msg"]
	virtual void writeResultMessage(ulong requestId, const wchar_t * status, std::wstring msg);
	// MI interface stdout output: [##requestId##]^result[,"msg"]
	virtual void writeResultMessage(ulong requestId, const wchar_t * status, const wchar_t * msg) { writeResultMessage(requestId, status, std::wstring(msg ? msg : L"")); }
	// MI interface stdout output: [##requestId##]^error[,"msg"]
	virtual void writeErrorMessage(ulong requestId, std::wstring msg);

	// CmdInputCallback interface handlers

	/// called on new input line
	virtual void onInputLine(std::wstring &s);
	/// called when ctrl+c or ctrl+break is called
	virtual void onCtrlBreak();

	virtual void showHelp();
	virtual int enterCommandLoop();

	// load executable
	virtual bool load();
	// start execution
	virtual bool run(uint64_t requestId = UNSPECIFIED_REQUEST_ID);
	// resume paused execution
	virtual bool resume(uint64_t requestId = UNSPECIFIED_REQUEST_ID);
	// break program if running
	virtual bool causeBreak(uint64_t requestId = UNSPECIFIED_REQUEST_ID);
	// step paused program
	virtual bool step(STEPKIND stepKind, STEPUNIT stepUnit, DWORD threadId = 0, uint64_t requestId = UNSPECIFIED_REQUEST_ID);
	// step paused program
	virtual bool stepInternal(STEPKIND stepKind, STEPUNIT stepUnit, IDebugThread2 * pThread, uint64_t requestId = UNSPECIFIED_REQUEST_ID);
	// find current program's thread by id
	IDebugThread2 * findThreadById(DWORD threadId);

	// MIEventCallback interface
	virtual HRESULT OnDebugEngineCreated(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugEngineCreateEvent2 * pEvent);
	virtual HRESULT OnDebugProgramCreated(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugProgramCreateEvent2 * pEvent);
	virtual HRESULT OnDebugProgramDestroy(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugProgramDestroyEvent2 * pEvent);
	virtual HRESULT OnDebugLoadComplete(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugLoadCompleteEvent2 * pEvent);
	virtual HRESULT OnDebugEntryPoint(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugEntryPointEvent2 * pEvent);
	virtual HRESULT OnDebugThreadCreate(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugThreadCreateEvent2 * pEvent);
	virtual HRESULT OnDebugThreadDestroy(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugThreadDestroyEvent2 * pEvent);
	virtual HRESULT OnDebugStepComplete(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugStepCompleteEvent2 * pEvent);
	virtual HRESULT OnDebugBreak(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugBreakEvent2 * pEvent);
	virtual HRESULT OnDebugOutputString(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugOutputStringEvent2 * pEvent);
	virtual HRESULT OnDebugModuleLoad(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugModuleLoadEvent2 * pEvent);
	virtual HRESULT OnDebugSymbolSearch(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugSymbolSearchEvent2 * pEvent);
	virtual HRESULT OnDebugBreakpoint(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugBreakpointEvent2 * pEvent);
	virtual HRESULT OnDebugBreakpointBound(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugBreakpointBoundEvent2 * pEvent);
	virtual HRESULT OnDebugBreakpointError(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugBreakpointErrorEvent2 * pEvent);
	virtual HRESULT OnDebugBreakpointUnbound(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugBreakpointUnboundEvent2 * pEvent);
	virtual HRESULT OnDebugException(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugExceptionEvent2 * pEvent);
	virtual HRESULT OnDebugMessage(IDebugEngine2 *pEngine,
		IDebugProcess2 *pProcess,
		IDebugProgram2 *pProgram,
		IDebugThread2 *pThread,
		IDebugMessageEvent2 * pEvent);
};
