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
	CmdInput _cmdinput;
	bool _quitRequested;
public:
	Debugger();
	virtual ~Debugger();

	// IEventCallback methods

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
