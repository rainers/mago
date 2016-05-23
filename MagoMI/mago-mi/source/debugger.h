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
	BreakpointInfoList _breakpointList;
	VariableObjectList _varList;
	bool _quitRequested;
	bool _verbose;
	bool _loadCalled;
	bool _loaded;
	bool _started;
	bool _paused;
	bool _stopped;
	bool _entryPointContinuePending;
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
	virtual void writeResultMessage(ulong requestId, const wchar_t * status, std::wstring msg = std::wstring(), wchar_t typeChar = '^');
	// MI interface stdout output: [##requestId##]^result[,msg] -- same as writeResultMessage but msg is raw string
	virtual void writeResultMessageRaw(ulong requestId, const wchar_t * status, std::wstring msg = std::wstring(), wchar_t typeChar = '^');
	// MI interface stdout output: [##requestId##]^result[,"msg"]
	virtual void writeResultMessage(ulong requestId, const wchar_t * status, const wchar_t * msg, wchar_t typeChar = '^') { 
		writeResultMessage(requestId, status, std::wstring(msg ? msg : L""), typeChar); 
	}
	// MI interface stdout output: [##requestId##]^error[,"msg"]
	virtual void writeErrorMessage(ulong requestId, std::wstring msg, const wchar_t * errorCode = NULL);

	// CmdInputCallback interface handlers

	// called to handle -stack-list-variables command
	virtual void handleStackListVariablesCommand(MICommand & cmd, bool localsOnly = false);
	// called to handle -stack-list-frames command
	virtual void handleStackListFramesCommand(MICommand & cmd, bool depthOnly = false);
	// called to handle -thread-info and -thread-list-ids commands
	virtual void handleThreadInfoCommand(MICommand & cmd, bool idsOnly);
	// called to handle breakpoint list command
	virtual void handleBreakpointListCommand(MICommand & cmd);
	// called to handle breakpoint delete command
	virtual void handleBreakpointDeleteCommand(MICommand & cmd);
	// called to handle breakpoint insert command
	virtual void handleBreakpointInsertCommand(MICommand & cmd);
	// called to handle breakpoint enable/disable commands
	virtual void handleBreakpointEnableCommand(MICommand & cmd, bool enable);
	// called to handle variable commands
	virtual void handleVariableCommand(MICommand & cmd);
	/// called on new input line
	virtual void onInputLine(std::wstring &s);
	/// called when ctrl+c or ctrl+break is called
	virtual void onCtrlBreak();

	virtual void showHelp();
	virtual int enterCommandLoop();

	// load executable
	virtual bool load(uint64_t requestId = UNSPECIFIED_REQUEST_ID, bool synchronous = true);
	// start execution
	virtual bool run(uint64_t requestId = UNSPECIFIED_REQUEST_ID);
	// resume paused execution
	virtual bool resume(uint64_t requestId = UNSPECIFIED_REQUEST_ID, DWORD threadId = 0);
	// break program if running
	virtual bool causeBreak(uint64_t requestId = UNSPECIFIED_REQUEST_ID);
	/// stop program execution
	virtual bool stop(uint64_t requestId);
	// step paused program
	virtual bool step(STEPKIND stepKind, STEPUNIT stepUnit, DWORD threadId = 0, uint64_t requestId = UNSPECIFIED_REQUEST_ID);
	// step paused program
	virtual bool stepInternal(STEPKIND stepKind, STEPUNIT stepUnit, IDebugThread2 * pThread, uint64_t requestId = UNSPECIFIED_REQUEST_ID);
	// find current program's thread by id
	IDebugThread2 * findThreadById(DWORD threadId);


	// returns stack frame if found
	virtual IDebugStackFrame2 * getStackFrame(DWORD threadId, unsigned frameIndex);
	// returns stack frame if found
	virtual IDebugStackFrame2 * getStackFrame(IDebugThread2 * pThread, unsigned frameIndex);
	// retrieves list of local variables from debug frame
	virtual bool getLocalVariables(IDebugStackFrame2 * frame, LocalVariableList &list, bool includeArgs);
	// gets thread frame contexts, return count of frames read
	unsigned getThreadFrameContext(IDebugThread2 * pThread, StackFrameInfo * frameInfo, unsigned minFrame = 0, unsigned maxFrame = 0);
	enum PauseReason {
		PAUSED_BY_LOAD_COMPLETED,
		PAUSED_BY_ENTRY_POINT_REACHED,
		PAUSED_BY_STEP_COMPLETED,
		PAUSED_BY_BREAKPOINT,
		PAUSED_BY_EXCEPTION,
		PAUSED_BY_BREAK,
	};
	void paused(IDebugThread2 * pThread, PauseReason reason, uint64_t requestId = UNSPECIFIED_REQUEST_ID, BreakpointInfo * bp = NULL);

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


// helper functions


// returns thread id or 0 if cannot get it
DWORD getThreadId(IDebugThread2 * pThread);
