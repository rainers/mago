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
	: _pProcess(NULL)
	, _pProgram(NULL)
	, _quitRequested(false)
	, _verbose(false)
	, _loadCalled(false)
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
	if (!params.miMode)
		writeStdout(msg);
	else
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

// MI interface stdout output: [##requestId##]^error[,"msg"]
void Debugger::writeErrorMessage(ulong requestId, std::wstring msg) {
	if (params.miMode)
		writeResultMessage(requestId, L"error", msg);
	else
		writeStdout(msg);
}

static const wchar_t * HELP_MSGS[] = {
	L"mago-mi tries to implement GDB and GDB-MI interfaces for MAGO debugger.",
	L"",
	L"run                     - start program execution",
	L"continue                - continue program execution",
	L"Type quit to exit.",
	NULL
};

static const wchar_t * HELP_MSGS_MI[] = {
	L"mago-mi tries to implement GDB and GDB-MI interfaces for MAGO debugger.",
	L"",
	L"-exec-run               - start program execution",
	L"-exec-continue          - continue program execution",
	L"Type quit to exit.",
	NULL
};

void Debugger::showHelp() {
	const wchar_t **msgs = params.miMode ? HELP_MSGS_MI : HELP_MSGS;
	for (int i = 0; msgs[i]; i++)
	    writeDebuggerMessage(std::wstring(msgs[i]));
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
	else if (cmd.commandName == L"help") {
		showHelp();
	}
	else if (cmd.commandName == L"run" || cmd.commandName == L"-exec-run") {
		run(cmd.requestId);
	}
	else if (cmd.commandName == L"continue" || cmd.commandName == L"-exec-continue") {
		resume(cmd.requestId);
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

// load executable
bool Debugger::load() {
	if (!params.hasExecutableSpecified()) {
		writeErrorMessage(UNSPECIFIED_REQUEST_ID, std::wstring(L"Executable file not specified. Use file or exec-file"));
	}
	writeDebuggerMessage(std::wstring(L"Starting program: ") + params.exename);
	if (!fileExists(params.exename)) {
		writeErrorMessage(UNSPECIFIED_REQUEST_ID, std::wstring(L"Executable file not found: ") + params.exename);
		return false;
	}
	HRESULT hr = _engine->Launch(
		params.exename.c_str(), //LPCOLESTR             pszExe,
		NULL, //LPCOLESTR             pszArgs,
		params.dir.c_str() //LPCOLESTR             pszDir,
		);
	if (FAILED(hr)) {
		writeErrorMessage(UNSPECIFIED_REQUEST_ID, std::wstring(L"Failed to start debugging of ") + params.exename);
		return false;
	}
	else {
		_loadCalled = true;
	}
	return true;
}

int Debugger::enterCommandLoop() {
	CRLog::info("Entering command loop");
	if (params.hasExecutableSpecified()) {
		load();
	}

	while (!_cmdinput.isClosed() && !_quitRequested) {
		_cmdinput.poll();
	}
	CRLog::info("Debugger shutdown");
	CRLog::info("Exiting");
	return 0;
}


// start execution
bool Debugger::run(uint64_t requestId) {
	if (!_loadCalled) {
		writeErrorMessage(requestId, std::wstring(L"Executable is not specified. Use file or exec-file command."));
		return false;
	}
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

// resume paused execution (continue)
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
	if (!_pProgram) {
		writeErrorMessage(requestId, std::wstring(L"Process is not started"));
		return false;
	}
	if (FAILED(_pProgram->Continue(NULL))) {
		writeErrorMessage(requestId, std::wstring(L"Failed to continue process"));
		return false;
	}
	writeResultMessage(requestId, L"running", NULL);
	_paused = false;
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
	UNUSED_EVENT_PARAMS;
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
	UNUSED_EVENT_PARAMS;
	_pProcess = pProcess;
	_pProgram = pProgram;
	writeStdout(L"=thread-group-added,id=\"i1\"");
	CRLog::info("Program created");
	return S_OK;
}

HRESULT Debugger::OnDebugProgramDestroy(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugProgramDestroyEvent2 * pEvent) 
{
	UNUSED_EVENT_PARAMS;
	DWORD exitCode = 0;
	pEvent->GetExitCode(&exitCode);
	writeStdout(L"=thread-group-exited,id=\"i1\",exit-code=\"%d\"", exitCode);
	writeStdout(L"*stopped,reason=\"exited-normally\"", exitCode);
	_stopped = true;
	CRLog::info("Program destroyed");
	return S_OK;
}
HRESULT Debugger::OnDebugLoadComplete(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugLoadCompleteEvent2 * pEvent) 
{
	UNUSED_EVENT_PARAMS;
	_loaded = true;
	_started = true;
	_paused = true;
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
	UNUSED_EVENT_PARAMS;
	_loaded = true;
	_paused = true;
	if (_verbose)
		writeDebuggerMessage(std::wstring(L"Entry point"));
	else
		CRLog::info("Entry point");
	return S_OK;
}

// reads thread id from thread
DWORD getThreadId(IDebugThread2 * pThread) {
	if (!pThread)
		return 0;
	DWORD res = 0;
	if (FAILED(pThread->GetThreadId(&res)))
		return 0;
	return res;
}

HRESULT Debugger::OnDebugThreadCreate(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugThreadCreateEvent2 * pEvent) 
{
	UNUSED_EVENT_PARAMS;
	writeStdout(L"=thread-created,id=\"%d\",group-id=\"i1\"", getThreadId(pThread));
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
	UNUSED_EVENT_PARAMS;
	writeStdout(L"=thread-exited,id=\"%d\",group-id=\"i1\"", getThreadId(pThread));
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
	UNUSED_EVENT_PARAMS;
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
	UNUSED_EVENT_PARAMS;
	DUMP_EVENT(OnDebugBreak);
	return S_OK;
}

HRESULT Debugger::OnDebugOutputString(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugOutputStringEvent2 * pEvent) 
{
	UNUSED_EVENT_PARAMS;
	DUMP_EVENT(OnDebugOutputString);
	return S_OK;
}

std::wstring getModuleName(IDebugModule2 * pModule) {
	if (!pModule)
		return std::wstring();
	MODULE_INFO info;
	pModule->GetInfo(MIF_NAME, &info);
	std::wstring res = std::wstring(info.m_bstrName);
	SysFreeString(info.m_bstrName);
	return res;
}

HRESULT Debugger::OnDebugModuleLoad(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugModuleLoadEvent2 * pEvent) 
{
	UNUSED_EVENT_PARAMS;
	//DUMP_EVENT(OnDebugModuleLoad);
	IDebugModule2 * pModule = NULL;
	BOOL pbLoad = FALSE;
	pEvent->GetModule(&pModule, NULL, &pbLoad);
	std::wstring moduleName = getModuleName(pModule);
	//writeStdout(L"=library-loaded,id=\"%s\",target-name=\"%s\",host-name=\"%s\",symbols-loaded=\"0\",thread-group-id=\"i1\"", 
	//	moduleName.c_str(), moduleName.c_str(), moduleName.c_str());
	pModule->Release();
	return S_OK;
}

HRESULT Debugger::OnDebugSymbolSearch(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugSymbolSearchEvent2 * pEvent) 
{
	UNUSED_EVENT_PARAMS;
	//DUMP_EVENT(OnDebugSymbolSearch);

	IDebugModule3 * pModule = NULL;
	MODULE_INFO_FLAGS dwModuleInfoFlags = 0;
	pEvent->GetSymbolSearchInfo(&pModule, NULL, &dwModuleInfoFlags);
	bool loaded = dwModuleInfoFlags & MIF_SYMBOLS_LOADED;
	std::wstring moduleName = getModuleName(pModule);
	writeStdout(L"=library-loaded,id=\"%s\",target-name=\"%s\",host-name=\"%s\",symbols-loaded=\"%d\",thread-group-id=\"i1\"",
		moduleName.c_str(), moduleName.c_str(), moduleName.c_str(),
		loaded ? 1 : 0
		);
	pModule->Release();
	return S_OK;
}
HRESULT Debugger::OnDebugBreakpoint(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugBreakpointEvent2 * pEvent) 
{
	UNUSED_EVENT_PARAMS;
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
	UNUSED_EVENT_PARAMS;
	DUMP_EVENT(OnDebugBreakpointBound);
	return S_OK;
}

HRESULT Debugger::OnDebugBreakpointError(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugBreakpointErrorEvent2 * pEvent) 
{
	UNUSED_EVENT_PARAMS;
	DUMP_EVENT(OnDebugBreakpointError);
	return S_OK;
}

HRESULT Debugger::OnDebugBreakpointUnbound(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugBreakpointUnboundEvent2 * pEvent) 
{
	UNUSED_EVENT_PARAMS;
	DUMP_EVENT(OnDebugBreakpointUnbound);
	return S_OK;
}

HRESULT Debugger::OnDebugException(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugExceptionEvent2 * pEvent) 
{
	UNUSED_EVENT_PARAMS;
	DUMP_EVENT(OnDebugException);
	return S_OK;
}

HRESULT Debugger::OnDebugMessage(IDebugEngine2 *pEngine,
	IDebugProcess2 *pProcess,
	IDebugProgram2 *pProgram,
	IDebugThread2 *pThread,
	IDebugMessageEvent2 * pEvent) 
{
	UNUSED_EVENT_PARAMS;
	DUMP_EVENT(OnDebugMessage);
	return S_OK;
}
