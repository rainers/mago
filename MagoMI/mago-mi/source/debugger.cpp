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

Debugger::Debugger() 
	: _quitRequested(false)
	, _verbose(false)
	, _loaded(false)
	, _started(false)
	, _paused(false)
	, _stopped(false)
{
	Log::Enable(false);
	_verbose = params.verbose;
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

// MI interface stdout output: ch"msg_text"
void Debugger::writeStringMessage(wchar_t ch, std::wstring msg) {
	WstringBuffer buf;
	buf.append(ch);
	buf.appendStringLiteral(msg);
	writeStdout(buf.wstr());
}

// MI interface stdout output: ~"msg_text"
void Debugger::writeDebuggerMessage(std::wstring msg) {
	writeStringMessage('~', msg);
}

// MI interface stdout output: [##requestId##]^result[,"msg"]
void Debugger::writeResultMessage(ulong requestId, const wchar_t * status, std::wstring msg) {
	WstringBuffer buf;
	buf.appendUlongIfNonEmpty(requestId);
	buf.append('^');
	buf.append(status);
	if (!msg.empty()) {
		buf.append(L",");
		buf.appendStringLiteral(msg);
	}
	writeStdout(buf.wstr());
}

/// called on new input line
void Debugger::onInputLine(std::wstring &s) {
	CRLog::debug("Input line: %s", toUtf8(s).c_str());
	if (s.empty())
		return;
	MICommand cmd;
	if (!cmd.parse(s)) {
		writeErrorMessage(cmd.requestId, std::wstring(L"invalid command syntax: ") + s);
		return;
	}
	if (cmd.commandName == L"quit") {
		_quitRequested = true;
		writeOutput("Quit requested");
	}
	else if (cmd.commandName == L"run" || cmd.commandName == L"-exec-run") {
		run(cmd.requestId);
	}
	else
	{
		if (cmd.miCommand)
			writeErrorMessage(cmd.requestId, std::wstring(L"Undefined MI command: ") + s);
		else
			writeErrorMessage(cmd.requestId, std::wstring(L"unknown command: ") + s);
	}
}
/// called when ctrl+c or ctrl+break is called
void Debugger::onCtrlBreak() {
	writeOutput("Ctrl+Break is pressed");
}

int Debugger::enterCommandLoop() {
	writeOutput("Entering command loop");
	if (!params.exename.empty()) {
		if (!fileExists(params.exename)) {
			fprintf(stderr, "%s: no such file or directory", params.exename);
			return 4;
		}

		HRESULT hr = _engine->Launch(
			params.exename.c_str(), //LPCOLESTR             pszExe,
			NULL, //LPCOLESTR             pszArgs,
			params.dir.c_str() //LPCOLESTR             pszDir,
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


// start execution
bool Debugger::run(uint64_t requestId) {
	if (_started) {
		writeErrorMessage(requestId, std::wstring(L"Process is already started"));
		return false;
	}
	if (_stopped) {
		writeErrorMessage(requestId, std::wstring(L"Process is finished"));
		return false;
	}
	if (FAILED(_engine->ResumeProcess())) {
		writeErrorMessage(requestId, std::wstring(L"Failed to start process"));
		_stopped = true;
		return false;
	}
	writeResultMessage(requestId, L"running", NULL);
	return true;
}

// resume paused execution
bool Debugger::resume(uint64_t requestId) {
	if (!_started) {
		writeErrorMessage(requestId, std::wstring(L"Process is not started"));
		return false;
	}
	if (_stopped) {
		writeErrorMessage(requestId, std::wstring(L"Process is finished"));
		return false;
	}
	if (!_paused) {
		writeErrorMessage(requestId, std::wstring(L"Process is already running"));
		return false;
	}
	if (FAILED(_engine->ResumeProcess())) {
		writeErrorMessage(requestId, std::wstring(L"Failed to resume process"));
		_stopped = true;
		return false;
	}
	writeResultMessage(requestId, L"running", NULL);
	return true;
}

#undef DUMP_EVENT
#define DUMP_EVENT(x) \
	if (_verbose) \
		writeDebuggerMessage(std::wstring(L#x)); \
	else \
		CRLog::info(#x);



HRESULT Debugger::OnDebugEngineCreated(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugEngineCreateEvent2 * pEvent) 
{
	if (_verbose)
		writeDebuggerMessage(std::wstring(L"Debug engine created"));
	else
		CRLog::info("Debug engine created");
	return S_OK;
}
HRESULT Debugger::OnDebugProgramCreated(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugProgramCreateEvent2 * pEvent) 
{
	if (_verbose)
		writeDebuggerMessage(std::wstring(L"Program created"));
	else
		CRLog::info("Program created");
	return S_OK;
}
HRESULT Debugger::OnDebugProgramDestroy(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugProgramDestroyEvent2 * pEvent) 
{
	_stopped = true;
	if (_verbose)
		writeDebuggerMessage(std::wstring(L"Program destroyed"));
	else
		CRLog::info("Program destroyed");
	return S_OK;
}
HRESULT Debugger::OnDebugLoadComplete(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugLoadCompleteEvent2 * pEvent) 
{
	_loaded = true;
	if (_verbose)
		writeDebuggerMessage(std::wstring(L"Load complete"));
	else
		CRLog::info("Load complete");
	return S_OK;
}
HRESULT Debugger::OnDebugEntryPoint(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugEntryPointEvent2 * pEvent) 
{
	_loaded = true;
	if (_verbose)
		writeDebuggerMessage(std::wstring(L"Entry point"));
	else
		CRLog::info("Entry point");
	return S_OK;
}
HRESULT Debugger::OnDebugThreadCreate(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugThreadCreateEvent2 * pEvent) 
{
	if (_verbose)
		writeDebuggerMessage(std::wstring(L"Thread created"));
	else
		CRLog::info("Thread created");
	return S_OK;
}


HRESULT Debugger::OnDebugThreadDestroy(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugThreadDestroyEvent2 * pEvent)
{
	if (_verbose)
		writeDebuggerMessage(std::wstring(L"Thread destroyed"));
	else
		CRLog::info("Thread destroyed");
	return S_OK;
}

HRESULT Debugger::OnDebugStepComplete(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugStepCompleteEvent2 * pEvent) 
{
	_paused = true;
	DUMP_EVENT(OnDebugStepComplete);
	return S_OK;
}
HRESULT Debugger::OnDebugBreak(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugBreakEvent2 * pEvent) 
{
	DUMP_EVENT(OnDebugBreak);
	return S_OK;
}

HRESULT Debugger::OnDebugOutputString(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugOutputStringEvent2 * pEvent) 
{
	DUMP_EVENT(OnDebugOutputString);
	return S_OK;
}

HRESULT Debugger::OnDebugModuleLoad(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugModuleLoadEvent2 * pEvent) 
{
	DUMP_EVENT(OnDebugModuleLoad);
	return S_OK;
}

HRESULT Debugger::OnDebugSymbolSearch(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugSymbolSearchEvent2 * pEvent) {
	DUMP_EVENT(OnDebugSymbolSearch);
	return S_OK;
}
HRESULT Debugger::OnDebugBreakpoint(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugBreakpointEvent2 * pEvent) 
{
	_paused = true;
	DUMP_EVENT(OnDebugBreakpoint);
	return S_OK;
}

HRESULT Debugger::OnDebugBreakpointBound(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugBreakpointBoundEvent2 * pEvent) 
{
	DUMP_EVENT(OnDebugBreakpointBound);
	return S_OK;
}

HRESULT Debugger::OnDebugBreakpointError(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugBreakpointErrorEvent2 * pEvent) {
	DUMP_EVENT(OnDebugBreakpointError);
	return S_OK;
}

HRESULT Debugger::OnDebugBreakpointUnbound(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugBreakpointUnboundEvent2 * pEvent) {
	DUMP_EVENT(OnDebugBreakpointUnbound);
	return S_OK;
}

HRESULT Debugger::OnDebugException(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugExceptionEvent2 * pEvent) {
	DUMP_EVENT(OnDebugException);
	return S_OK;
}

HRESULT Debugger::OnDebugMessage(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugMessageEvent2 * pEvent) {

	DUMP_EVENT(OnDebugMessage);
	return S_OK;
}
