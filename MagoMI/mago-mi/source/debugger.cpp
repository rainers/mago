#include "debugger.h"
#include "cmdline.h"
#include "../../DebugEngine/Exec/Log.h"

void InitDebug()
{
	int f = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	f |= _CRTDBG_LEAK_CHECK_DF;     // should always use in debug build
	f |= _CRTDBG_CHECK_ALWAYS_DF;   // check on free AND alloc
	_CrtSetDbgFlag(f);

	//_CrtSetAllocHook( LocalMemAllocHook );
	//SetLocalMemWorkingSetLimit( 550 );
}

Debugger::Debugger() : _quitRequested(false) {
	Log::Enable(false);
	_engine = new MIEngine();
	//InitDebug();
	_cmdinput.setCallback(this);
	if (FAILED(_engine->Init(this))) {
		fprintf(stderr, "Cannot initialize debug engine\n");
		exit(-1);
	}
}

Debugger::~Debugger() {
}

void Debugger::writeOutput(std::wstring msg) {
	writeStdout(msg);
}

void Debugger::writeOutput(std::string msg) {
	writeStdout(toUtf16(msg));
}

void Debugger::writeOutput(const char * msg) {
	writeStdout(toUtf16(std::string(msg)));
}

void Debugger::writeOutput(const wchar_t * msg) {
	writeStdout(std::wstring(msg));
}

/// called on new input line
void Debugger::onInputLine(std::wstring &s) {
	writeStdout(L"Input line: %s\n", s.c_str());
	if (s == L"quit") {
		_quitRequested = true;
		writeOutput("Quit requested");
	}
}
/// called when ctrl+c or ctrl+break is called
void Debugger::onCtrlBreak() {
	writeOutput("Ctrl+Break is pressed");
}

int Debugger::enterCommandLoop() {
	writeOutput("Entering command loop");
	if (!executableInfo.exename.empty()) {
		if (!fileExists(executableInfo.exename)) {
			fprintf(stderr, "%s: no such file or directory", executableInfo.exename);
			return 4;
		}

		HRESULT hr = _engine->Launch(
			executableInfo.exename.c_str(), //LPCOLESTR             pszExe,
			NULL, //LPCOLESTR             pszArgs,
			executableInfo.dir.c_str() //LPCOLESTR             pszDir,
			);
		if (FAILED(hr)) {
			writeOutput("Failed to load debuggee\n");
		}
	}

	while (!_cmdinput.isClosed() && !_quitRequested) {
		_cmdinput.poll();
	}
	writeOutput("Debugger shutdown");
	writeOutput("Exiting");
	return 0;
}
