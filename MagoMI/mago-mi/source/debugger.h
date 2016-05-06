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

class Debugger : public IEventCallback, public CmdInputCallback {
	MagoCore::DebuggerProxy _debuggerProxy;
	RefPtr<IProcess> _proc;
	//Exec*       _exec;
	IModule*    _mod;
	CmdInput _cmdinput;
	bool _quitRequested;
public:
	MagoCore::DebuggerProxy & GetProxy() { return _debuggerProxy; }
	Debugger();
	virtual ~Debugger();

	// IEventCallback methods
	virtual void AddRef();
	virtual void Release();

	virtual void OnProcessStart(IProcess* process);
	virtual void OnProcessExit(IProcess* process, DWORD exitCode);
	virtual void OnThreadStart(IProcess* process, Thread* thread);
	virtual void OnThreadExit(IProcess* process, DWORD threadId, DWORD exitCode);
	virtual void OnModuleLoad(IProcess* process, IModule* module);
	virtual void OnModuleUnload(IProcess* process, Address baseAddr);
	virtual void OnOutputString(IProcess* process, const wchar_t* outputString);
	virtual void OnLoadComplete(IProcess* process, DWORD threadId);

	virtual RunMode OnException(IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec);
	virtual RunMode OnBreakpoint(IProcess* process, uint32_t threadId, Address address, bool embedded);
	virtual void OnStepComplete(IProcess* process, uint32_t threadId);
	virtual void OnAsyncBreakComplete(IProcess* process, uint32_t threadId);
	virtual void OnError(IProcess* process, HRESULT hrErr, EventCode event);

	virtual ProbeRunMode OnCallProbe(
		IProcess* process, uint32_t threadId, Address address, AddressRange& thunkRange);

	virtual void writeOutput(std::wstring msg);
	virtual void writeOutput(std::string msg);
	virtual void writeOutput(const char * msg);
	virtual void writeOutput(const wchar_t * msg);

	// CmdInputCallback interface handlers

	/// called on new input line
	virtual void onInputLine(std::wstring &s);
	/// called when ctrl+c or ctrl+break is called
	virtual void onCtrlBreak();

	virtual int enterCommandLoop();
};
