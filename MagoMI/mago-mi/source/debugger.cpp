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
	, _pThread(NULL)
	, _quitRequested(false)
	, _verbose(false)
	, _loadCalled(false)
	, _loaded(false)
	, _started(false)
	, _paused(false)
	, _stopped(false)
	, _entryPointContinuePending(false)
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
void Debugger::writeErrorMessage(ulong requestId, std::wstring msg, const wchar_t * errorCode) {
	if (params.miMode) {
		WstringBuffer buf;
		buf.appendUlongIfNonEmpty(requestId);
		buf.append('^');
		buf.append(L"error");
		buf.appendStringParam(L"msg", msg);
		if (errorCode && errorCode[0])
			buf.appendStringParam(L"msg", std::wstring(errorCode));
		writeStdout(buf.wstr());
	}
	else
		writeStdout(msg);
}

static const wchar_t * HELP_MSGS[] = {
	L"mago-mi: GDB and GDB-MI compatible interfaces for MAGO debugger.",
	L"",
	L"run                     - start program execution",
	L"continue                - continue program execution",
	L"interrupt               - interrupt program execution",
	L"next                    - step over",
	L"nexti                   - step over by instruction",
	L"step                    - step into",
	L"stepi                   - step into by instruction",
	L"finish                  - step out of current function",
	L"break                   - add breakpoint",
	L"info break              - list breakpoints",
	L"enable N                - enable breakpoint",
	L"disable N               - disable breakpoint",
	L"delete N                - delete breakpoint",
	L"info thread             - thread list with properties",
	L"info threads            - list of thread ids",
	L"",
	L"Type quit to exit.",
	NULL
};

static const wchar_t * HELP_MSGS_MI[] = {
	L"mago-mi: GDB and GDB-MI compatible interfaces for MAGO debugger.",
	L"",
	L"-exec-run [--start]     - start program execution",
	L"-exec-continue          - continue program execution",
	L"-exec-interrupt         - interrupt program which is being running",
	L"-exec-next              - step over",
	L"-exec-next-instruction  - step over by instruction",
	L"-exec-step              - step into",
	L"-exec-step-instruction  - step into by instruction",
	L"-exec-finish            - exit function",
	L"-break-insert           - add breakpoint",
	L"-break-list             - list breakpoints",
	L"-break-enable           - enable breakpoint",
	L"-break-disable          - disable breakpoint",
	L"-break-delete           - delete breakpoint",
	L"-thread-info            - thread list with properties",
	L"-thread-list-ids        - list of thread ids",
	L"",
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
		writeErrorMessage(cmd.requestId, std::wstring(L"invalid command syntax: ") + s, L"undefined-command");
		return;
	}
	if (cmd.commandName == L"quit") {
		_quitRequested = true;
		writeStringMessage('^', std::wstring(L"exit"));
		Sleep(1);
	}
	else if (cmd.commandName == L"help") {
		showHelp();
	}
	else if (cmd.commandName == L"run" || cmd.commandName == L"-exec-run") {
		if (cmd.hasParam(std::wstring(L"--start")))
			params.stopOnEntry = true;
		run(cmd.requestId);
	}
	else if (cmd.commandName == L"continue" || cmd.commandName == L"-exec-continue") {
		resume(cmd.requestId);
	}
	else if (cmd.commandName == L"interrupt" || cmd.commandName == L"-exec-interrupt") {
		causeBreak(cmd.requestId);
	}
	else if (cmd.commandName == L"finish" || cmd.commandName == L"-exec-finish") {
		step(STEP_OUT, STEP_LINE, cmd.getThreadIdParam(), cmd.requestId);
	}
	else if (cmd.commandName == L"next" || cmd.commandName == L"-exec-next") {
		step(STEP_OVER, STEP_LINE, cmd.getThreadIdParam(), cmd.requestId);
	}
	else if (cmd.commandName == L"nexti" || cmd.commandName == L"-exec-next-instruction") {
		step(STEP_OVER, STEP_INSTRUCTION, cmd.getThreadIdParam(), cmd.requestId);
	}
	else if (cmd.commandName == L"step" || cmd.commandName == L"-exec-step") {
		step(STEP_INTO, STEP_LINE, cmd.getThreadIdParam(), cmd.requestId);
	}
	else if (cmd.commandName == L"stepi" || cmd.commandName == L"-exec-step-instruction") {
		step(STEP_INTO, STEP_INSTRUCTION, cmd.getThreadIdParam(), cmd.requestId);
	}
	else if (cmd.commandName == L"break" || cmd.commandName == L"-break-insert") {
		handleBreakpointInsertCommand(cmd);
	}
	else if (cmd.commandName == L"delete" || cmd.commandName == L"-break-delete") {
		handleBreakpointDeleteCommand(cmd);
	}
	else if (cmd.commandName == L"enable" || cmd.commandName == L"-break-enable") {
		handleBreakpointEnableCommand(cmd, true);
	}
	else if (cmd.commandName == L"disable" || cmd.commandName == L"-break-disable") {
		handleBreakpointEnableCommand(cmd, false);
	}
	else if ((cmd.commandName == L"info" && cmd.tail == L"break") || cmd.commandName == L"-break-list") {
		handleBreakpointListCommand(cmd);
	}
	else if ((cmd.commandName == L"info" && cmd.tail == L"thread") || cmd.commandName == L"-thread-info") {
		handleThreadInfoCommand(cmd, false);
	}
	else if ((cmd.commandName == L"info" && cmd.tail == L"threads") || cmd.commandName == L"-thread-list-ids") {
		handleThreadInfoCommand(cmd, true);
	}
	else
	{
		if (cmd.miCommand)
			writeErrorMessage(cmd.requestId, std::wstring(L"Undefined MI command: ") + s, L"undefined-command");
		else
			writeErrorMessage(cmd.requestId, std::wstring(L"unknown command: ") + s, L"undefined-command");
	}
}

// called to handle -thread-info command
void Debugger::handleThreadInfoCommand(MICommand & cmd, bool idsOnly) {
	if (!_paused || _stopped || !_pThread || !_pProgram) {
		writeErrorMessage(cmd.requestId, std::wstring(L"Execution is not paused"));
		return;
	}
	uint64_t threadId = 0;
	if (!idsOnly)
		toUlong(cmd.tail, threadId);
	WstringBuffer buf;
	buf.append(!idsOnly ? L"^done,threads=[" : L"^done,thread-ids={");


	IEnumDebugThreads2* pThreadList = NULL;
	ULONG count = 0;
	if (SUCCEEDED(_pProgram->EnumThreads(&pThreadList)) && pThreadList && SUCCEEDED(pThreadList->GetCount(&count))) {
		IDebugThread2 * threads[1];
		bool firstThread = true;
		for (ULONG i = 0; i < count; i++) {
			IDebugThread2 * thread = NULL;
			ULONG fetched = 0;
			if (FAILED(pThreadList->Next(1, &thread, &fetched)) || fetched != 1 || !threads[0]) {
				break;
			}
			DWORD id = 0;
			if (SUCCEEDED(thread->GetThreadId(&id))) {
				if (idsOnly) {
					//if (!firstThread)
					//	buf.append(L",");
					buf.appendUlongParamAsString(L"thread-id", id);
					//firstThread = false;
				} else if (id == threadId || threadId == 0) {
					StackFrameInfo frameInfo;
					if (getThreadFrameContext(thread, frameInfo)) {
						// append thread information
						if (!firstThread)
							buf.append(L",");
						buf.append(L"{");
						buf.appendUlongParamAsString(L"id", id);
						buf.appendUlongParamAsString(L"target-id", id);
						buf.append(L",frame=");
						frameInfo.dumpMIFrame(buf);
						buf.append(L",state=\"stopped\"");
						buf.append(L"}");
						thread->Release();
						firstThread = false;
					}
				}
			}
			thread->Release();
		}
		pThreadList->Release();
	}

	buf.append(idsOnly ? L"}" : L"]");
	DWORD currentThreadId = 0;
	_pThread->GetThreadId(&currentThreadId);
	buf.appendUlongParamAsString(L"current-thread-id", currentThreadId);
	if (idsOnly)
		buf.appendUlongParamAsString(L"number-of-threads", count);
	writeStdout(buf.wstr());
}

// called to handle breakpoint list command
void Debugger::handleBreakpointListCommand(MICommand & cmd) {
	WstringBuffer buf;
	buf.appendUlongIfNonEmpty(cmd.requestId);
	buf.append(L"^done,");
	buf.append(L"BreakpointTable={");
	buf.appendUlongParamAsString(L"nr_rows", _breakpointList.size());
	buf.appendUlongParamAsString(L"nr_cols", 6);
	buf.append(L",");
	buf.append(L"hdr = [{width = \"3\", alignment = \"-1\", col_name = \"number\", colhdr = \"Num\"},");
	buf.append(L"{ width = \"14\",alignment = \"-1\",col_name = \"type\",colhdr = \"Type\" },");
	buf.append(L"{ width = \"4\",alignment = \"-1\",col_name = \"disp\",colhdr = \"Disp\" },");
	buf.append(L"{ width = \"3\",alignment = \"-1\",col_name = \"enabled\",colhdr = \"Enb\" },");
	buf.append(L"{ width = \"10\",alignment = \"-1\",col_name = \"addr\",colhdr = \"Address\" },");
	buf.append(L"{ width = \"40\",alignment = \"2\",col_name = \"what\",colhdr = \"What\" }],");
	buf.append(L",body=[");
	for (unsigned i = 0; i < _breakpointList.size(); i++) {
		if (i > 0)
			buf.append(L",");
		_breakpointList[i]->printBreakpointInfo(buf);
	}
	buf.append(L"]}");
	writeStdout(buf.wstr());
}

// called to handle breakpoint enable/disable commands
void Debugger::handleBreakpointEnableCommand(MICommand & cmd, bool enable) {
	// check all ids
	bool foundValidIds = false;
	bool foundErrors = false;
	for (unsigned i = 0; i < cmd.unnamedValues.size(); i++) {
		uint64_t id;
		BreakpointInfoRef bp;
		if (toUlong(cmd.unnamedValues[i], id)) {
			bp = _breakpointList.findById(id);
			if (bp.Get()) {
				// OK
				foundValidIds = true;
			}
			else {
				writeErrorMessage(cmd.requestId, std::wstring(L"Breakpoint not found: ") + cmd.unnamedValues[i]);
				foundErrors = true;
			}
		}
		else {
			writeErrorMessage(cmd.requestId, std::wstring(L"Invalid breakpoint number: ") + cmd.unnamedValues[i]);
			foundErrors = true;
		}
	}
	if (foundErrors)
		return;
	if (!foundValidIds) {
		writeErrorMessage(cmd.requestId, std::wstring(enable ? L"No breakpoints to enable" : L"No breakpoints to disable"));
		return;
	}

	for (unsigned i = 0; i < cmd.unnamedValues.size(); i++) {
		uint64_t id;
		BreakpointInfoRef bp;
		if (toUlong(cmd.unnamedValues[i], id)) {
			bp = _breakpointList.findById(id);
			if (bp.Get() && bp->getBoundBreakpoint()) {
				if (FAILED(bp->getBoundBreakpoint()->Enable(enable ? TRUE : FALSE))) {
					CRLog::error("Failed to enable/disable breakpoint");
				}
				bp->enabled = enable;
				WstringBuffer buf;
				buf.append(L"=breakpoint-modified,bkpt=");
				bp->printBreakpointInfo(buf);
				writeStdout(buf.wstr());
			}
		}
	}

	writeResultMessage(cmd.requestId, L"done");
}

// called to handle breakpoint delete command
void Debugger::handleBreakpointDeleteCommand(MICommand & cmd) {
	// check all ids
	bool foundValidIds = false;
	bool foundErrors = false;
	for (unsigned i = 0; i < cmd.unnamedValues.size(); i++) {
		uint64_t id;
		BreakpointInfoRef bp;
		if (toUlong(cmd.unnamedValues[i], id)) {
			bp = _breakpointList.findById(id);
			if (bp.Get()) {
				// OK
				foundValidIds = true;
			}
			else {
				writeErrorMessage(cmd.requestId, std::wstring(L"Breakpoint not found: ") + cmd.unnamedValues[i]);
				foundErrors = true;
			}
		}
		else {
			writeErrorMessage(cmd.requestId, std::wstring(L"Invalid breakpoint number: ") + cmd.unnamedValues[i]);
			foundErrors = true;
		}
	}
	if (foundErrors)
		return;
	if (!foundValidIds) {
		writeErrorMessage(cmd.requestId, std::wstring(L"No breakpoints to delete"));
		return;
	}

	for (unsigned i = 0; i < cmd.unnamedValues.size(); i++) {
		uint64_t id;
		BreakpointInfoRef bp;
		if (toUlong(cmd.unnamedValues[i], id)) {
			bp = _breakpointList.findById(id);
			if (bp.Get()) {
				if (FAILED(bp->getBoundBreakpoint()->Delete())) {
					CRLog::error("Failed to delete breakpoint");
				}
				_breakpointList.removeItem(bp);
			}
		}
	}
	writeResultMessage(cmd.requestId, L"done");
}

// called to handle breakpoint command
void Debugger::handleBreakpointInsertCommand(MICommand & cmd) {
	if (_stopped)
		return;
	//writeDebuggerMessage(cmd.dumpCommand());
	BreakpointInfoRef bp = new BreakpointInfo();
	bp->fromCommand(cmd);
	//writeDebuggerMessage(bp->dumpParams());
	if (bp->validateParameters()) {
		HRESULT hr = _engine->CreatePendingBreakpoint(bp);
		if (FAILED(hr)) {
			writeErrorMessage(cmd.requestId, std::wstring(L"Failed to add breakpoint"));
			return;
		}
		_breakpointList.addItem(bp);
		if (!bp->bind()) {
			if (!bp->pending) {
				writeErrorMessage(cmd.requestId, std::wstring(L"Failed to bind breakpoint"));
				return;
			}
		}
		if ((!bp->bound && !bp->pending) || bp->error) {
			writeErrorMessage(cmd.requestId, bp->errorMessage.empty() ? std::wstring(L"Failed to bind breakpoint") : bp->errorMessage);
		}
		bp->assignId();
		WstringBuffer buf;
		buf.append(L"^done,bkpt=");
		bp->printBreakpointInfo(buf);
		writeStdout(buf.wstr());
		//writeDebuggerMessage(std::wstring(L"added pending breakpoint"));
	}
}

/// called when ctrl+c or ctrl+break is called
void Debugger::onCtrlBreak() {
	CRLog::info("Ctrl+Break is pressed");
	causeBreak();
}

// load executable
bool Debugger::load(uint64_t requestId, bool synchronous) {
	if (!params.hasExecutableSpecified()) {
		writeErrorMessage(requestId, std::wstring(L"Executable file not specified. Use file or exec-file"));
		return false;
	}
	writeDebuggerMessage(std::wstring(L"Starting program: ") + params.exename);
	if (!fileExists(params.exename)) {
		writeErrorMessage(requestId, std::wstring(L"Executable file not found: ") + params.exename);
		return false;
	}
	HRESULT hr = _engine->Launch(
		params.exename.c_str(), //LPCOLESTR             pszExe,
		NULL, //LPCOLESTR             pszArgs,
		params.dir.c_str() //LPCOLESTR             pszDir,
		);
	if (FAILED(hr)) {
		writeErrorMessage(requestId, std::wstring(L"Failed to start debugging of ") + params.exename);
		return false;
	}
	else {
		_loadCalled = true;
		if (synchronous) {
			CRLog::info("Waiting for load completion");
			for (;;) {
				Sleep(10);
				if (_loaded || _stopped)
					break;
			}
			if (_loaded) {
				writeDebuggerMessage(std::wstring(L"Loaded."));
			}
		}
	}
	return true;
}

int Debugger::enterCommandLoop() {
	CRLog::info("Entering command loop");
	if (params.hasExecutableSpecified()) {
		load();
	}

	while (!_cmdinput.isClosed() && !_quitRequested) {
		if (_entryPointContinuePending) {
			_entryPointContinuePending = false;
			resume();
		}
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
	//if (FAILED(_engine->ResumeProcess())) {
	//	writeErrorMessage(requestId, std::wstring(L"Failed to start process"));
	//	_stopped = true;
	//	return false;
	//}
	_started = true;
	resume(requestId);
	writeResultMessage(requestId, L"running", NULL);
	return true;
}

// resume paused execution (continue)
bool Debugger::resume(uint64_t requestId, DWORD threadId) {
	if (!_loaded) {
		writeErrorMessage(requestId, std::wstring(L"Process is not loaded"));
		return false;
	}
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
	IDebugThread2 * pThread = findThreadById(threadId);
	if (FAILED(_pProgram->Continue(pThread))) {
		writeErrorMessage(requestId, std::wstring(L"Failed to continue process"));
		return false;
	}
	//writeResultMessage(requestId, L"running", NULL);
	_paused = false;
	return true;
}

// break program if running
bool Debugger::causeBreak(uint64_t requestId) {
	if (!_started || !_loaded || _paused || !_pProgram) {
		if (requestId != UNSPECIFIED_REQUEST_ID)
			writeErrorMessage(requestId, std::wstring(L"Cannot break: program is not running"));
		return false;
	}
	if (FAILED(_pProgram->CauseBreak())) {
		writeErrorMessage(requestId, std::wstring(L"Failed to break program"));
		return false;
	}
	_paused = true;
	return true;
}

// find current program's thread by id
IDebugThread2 * Debugger::findThreadById(DWORD threadId) {
	IDebugThread2 * res = NULL;
	if (!_pProgram) {
		CRLog::warn("Cannot find thread: no current program");
		return NULL;
	}
	IEnumDebugThreads2* pThreadList = NULL;
	if (FAILED(_pProgram->EnumThreads(&pThreadList)) || !pThreadList) {
		CRLog::error("Cannot find thread: cannot enum threads");
		return NULL;
	}
	ULONG count = 0;
	if (FAILED(pThreadList->GetCount(&count))) {
		CRLog::error("Cannot find thread: cannot enum threads");
		pThreadList->Release();
		return NULL;
	}
	IDebugThread2 * threads[1];
	for (ULONG i = 0; i < count; i++) {
		IDebugThread2 * thread = NULL;
		ULONG fetched = 0;
		if (FAILED(pThreadList->Next(1, &thread, &fetched)) || fetched != 1 || !threads[0]) {
			break;
		}
		DWORD id = 0;
		if (SUCCEEDED(thread->GetThreadId(&id))) {
			if (id == threadId || (threadId == 0 && count == 1)) {
				thread->Release();
				res = thread;
				break;
			}
		}
		thread->Release();
	}
	pThreadList->Release();
	return res;
}

void Debugger::paused(IDebugThread2 * pThread, PauseReason reason, uint64_t requestId, BreakpointInfo * bp) {
	_paused = true;
	_pThread = pThread;
	StackFrameInfo frameInfo;
	bool hasContext = getThreadFrameContext(pThread, frameInfo);
	WstringBuffer buf;
	buf.appendUlongIfNonEmpty(requestId);
	buf.append(L"*stopped");
	std::wstring reasonName;
	switch (reason) {
	case PAUSED_BY_BREAKPOINT:
		reasonName = L"breakpoint-hit";
		break;
	case PAUSED_BY_STEP_COMPLETED:
		reasonName = L"end-stepping-range";
		break;
	case PAUSED_BY_EXCEPTION:
	case PAUSED_BY_BREAK:
		reasonName = L"signal-received";
		break;
	}
	if (!reasonName.empty()) {
		buf.append(L",reason=");
		buf.appendStringLiteral(reasonName);
	}
	if (reason == PAUSED_BY_BREAKPOINT && bp) {
		buf.appendStringParam(L"disp", std::wstring(bp->temporary ? L"del" : L"keep"));
		buf.appendUlongParamAsString(L"bkptno", bp->id);
	}
	if (hasContext) {
		buf.append(L",frame=");
		frameInfo.dumpMIFrame(buf);
	}

	buf.appendUlongParamAsString(L"thread-id", getThreadId(pThread), ',');
	buf.appendStringParam(L"stopped-threads", std::wstring(L"all"), ',');
	buf.appendStringParam(L"core", std::wstring(L"1"), ',');
	writeStdout(buf.wstr());
}

// gets thread frame contexts
bool Debugger::getThreadFrameContext(IDebugThread2 * pThread, StackFrameInfo & frameInfo) {
	if (!_paused || _stopped || !pThread) {
		return false;
	}
	IEnumDebugFrameInfo2* pFrames = NULL;
	if (FAILED(pThread->EnumFrameInfo(FIF_FUNCNAME | FIF_RETURNTYPE | FIF_ARGS | FIF_DEBUG_MODULEP | FIF_DEBUGINFO | FIF_MODULE | FIF_FRAME, 10, &pFrames))) {
		CRLog::error("cannot get thread frame enum");
		return false;
	}
	ULONG count = 0;
	pFrames->GetCount(&count);
	for (ULONG i = 0; i < count; i++) {
		FRAMEINFO frame;
		memset(&frame, 0, sizeof(FRAMEINFO));
		ULONG fetched = 0;
		if (FAILED(pFrames->Next(1, &frame, &fetched)) || fetched != 1)
			break;
		if (frame.m_pFrame) {
			IDebugCodeContext2 * pCodeContext = NULL;
			IDebugDocumentContext2 * pDocumentContext = NULL;
			//IDebugMemoryContext2 * pMemoryContext = NULL;
			frame.m_pFrame->GetCodeContext(&pCodeContext);
			frame.m_pFrame->GetDocumentContext(&pDocumentContext);
			CONTEXT_INFO contextInfo;
			memset(&contextInfo, 0, sizeof(CONTEXT_INFO));
			if (pCodeContext)
				pCodeContext->GetInfo(CIF_ALLFIELDS, &contextInfo);
			if (contextInfo.bstrAddress)
				frameInfo.address = contextInfo.bstrAddress;
			if (contextInfo.bstrFunction && contextInfo.bstrAddressOffset)
				frameInfo.functionName = std::wstring(contextInfo.bstrFunction) + std::wstring(contextInfo.bstrAddressOffset);
			else if (contextInfo.bstrFunction)
				frameInfo.functionName = contextInfo.bstrFunction;
			if (contextInfo.bstrModuleUrl)
				frameInfo.moduleName = contextInfo.bstrModuleUrl;
			frameInfo.sourceLine = contextInfo.posFunctionOffset.dwLine;
			frameInfo.sourceColumn = contextInfo.posFunctionOffset.dwColumn;
			TEXT_POSITION srcBegin, srcEnd;
			memset(&srcBegin, 0, sizeof(srcBegin));
			memset(&srcEnd, 0, sizeof(srcEnd));
			if (pDocumentContext) {
				if (SUCCEEDED(pDocumentContext->GetSourceRange(&srcBegin, &srcEnd))) {
					//srcBegin.dwLine;
					//srcBegin.dwColumn;
				}
				BSTR pFileName = NULL;
				BSTR pBaseName = NULL;
				pDocumentContext->GetName(GN_FILENAME, &pFileName);
				pDocumentContext->GetName(GN_BASENAME, &pBaseName);
				if (pFileName) { frameInfo.sourceFileName = pFileName; SysFreeString(pFileName); }
				if (pBaseName) { frameInfo.sourceBaseName = pBaseName; SysFreeString(pBaseName); }
			}
			if (pDocumentContext)
				pDocumentContext->Release();
			if (pCodeContext)
				pCodeContext->Release();
		}
		break;
	}
	pFrames->Release();
	return true;
}

// step paused program
bool Debugger::step(STEPKIND stepKind, STEPUNIT stepUnit, DWORD threadId, uint64_t requestId) {
	if (!_started || !_loaded || !_pProgram) {
		if (requestId != UNSPECIFIED_REQUEST_ID)
			writeErrorMessage(requestId, std::wstring(L"Cannot step: program is not running"));
		return false;
	}
	if (!_paused) {
		writeErrorMessage(requestId, std::wstring(L"Cannot step: program is not stopped"));
		return false;
	}
	IDebugThread2 * pThread = findThreadById(threadId);
	return stepInternal(stepKind, stepUnit, pThread, requestId);
}

// step paused program
bool Debugger::stepInternal(STEPKIND stepKind, STEPUNIT stepUnit, IDebugThread2 * pThread, uint64_t requestId) {
	CRLog::trace("Debugger::step(kind=%d, unit=%d", stepKind, stepUnit);
	if (!_started || !_loaded || !_pProgram) {
		if (requestId != UNSPECIFIED_REQUEST_ID)
			writeErrorMessage(requestId, std::wstring(L"Cannot step: program is not running"));
		return false;
	}
	if (!_paused) {
		writeErrorMessage(requestId, std::wstring(L"Cannot step: program is not stopped"));
		return false;
	}
	if (!pThread)
		pThread = _pThread;
	if (!pThread) {
		writeErrorMessage(requestId, std::wstring(L"Cannot step: thread is not specified"));
		return false;
	}
	if (FAILED(_pProgram->Step(pThread, stepKind, stepUnit))) {
		writeErrorMessage(requestId, std::wstring(L"Step failed"));
		return false;
	}
	_paused = false;
	if (params.miMode)
		writeStdout(L"^running");
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
	paused(pThread, PAUSED_BY_LOAD_COMPLETED);
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
	_pThread = pThread;
	_loaded = true;
	if (_verbose)
		writeDebuggerMessage(std::wstring(L"Entry point"));
	else
		CRLog::info("Entry point");
	if (params.stopOnEntry) {
		paused(pThread, PAUSED_BY_ENTRY_POINT_REACHED);
	}
	else {
		_paused = true;
		_entryPointContinuePending = true;
		//resume();
	}
	return S_OK;
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
	_pThread = pThread;
	paused(pThread, PAUSED_BY_STEP_COMPLETED);
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
	_pThread = pThread;
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
	DUMP_EVENT(OnDebugBreakpoint);
	_pThread = pThread;
	BreakpointInfoRef bp;
	IEnumDebugBoundBreakpoints2 * pEnum = NULL;
	if (SUCCEEDED(pEvent->EnumBreakpoints(&pEnum))) {
		ULONG count = 0;
		if (SUCCEEDED(pEnum->GetCount(&count)) && count > 0) {
			IDebugBoundBreakpoint2 * pBoundBP = NULL;
			ULONG fetched = 0;
			if (SUCCEEDED(pEnum->Next(1, &pBoundBP, &fetched)) && fetched == 1) {
				bp = _breakpointList.findByBoundBreakpoint(pBoundBP);
				pBoundBP->Release();
			}
		}
		pEnum->Release();
	}
	if (!bp.Get()) {
		CRLog::warn("OnDebugBreakpoint: Breakpoint not found");
	}
	paused(pThread, PAUSED_BY_BREAKPOINT, UNSPECIFIED_REQUEST_ID, bp.Get());
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
	IDebugPendingBreakpoint2 * pPendingBP = NULL;
	if (FAILED(pEvent->GetPendingBreakpoint(&pPendingBP))) {
		CRLog::error("Cannot get pending breakpoint in OnDebugBreakpointBound");
		return E_FAIL;
	}
	BreakpointInfoRef bp = _breakpointList.findByPendingBreakpoint(pPendingBP);
	pPendingBP->Release();
	if (!bp.Get()) {
		// unknown breakpoint
		CRLog::error("Unknown breakpoint request in OnDebugBreakpointBound");
		return E_FAIL;
	}
	IEnumDebugBoundBreakpoints2 * pEnum = NULL;
	if (FAILED(pEvent->EnumBoundBreakpoints(&pEnum))) {
		CRLog::error("Failed to get enum of bound breakpoints");
		return E_FAIL;
	}
	ULONG count = 0;
	if (FAILED(pEnum->GetCount(&count)) || count < 1) {
		pEnum->Release();
		CRLog::error("Failed to get enum size for bound breakpoints");
		return E_FAIL;
	}
	IDebugBoundBreakpoint2 * boundBp = NULL;
	ULONG fetched = 0;
	if (FAILED(pEnum->Next(1, &boundBp, &fetched))) {
		pEnum->Release();
		CRLog::error("Failed to get enum size for bound breakpoints");
		return E_FAIL;
	}
	pEnum->Release();
	IDebugBreakpointResolution2 * pBPResolution = NULL;
	if (FAILED(boundBp->GetBreakpointResolution(&pBPResolution))) {
		CRLog::error("no breakpoint resolution");
		return E_FAIL;
	}
	BP_TYPE bpType;
	pBPResolution->GetBreakpointType(&bpType);
	BP_RESOLUTION_INFO bpResolutionInfo;
	memset(&bpResolutionInfo, 0, sizeof(bpResolutionInfo));
	if (FAILED(pBPResolution->GetResolutionInfo(BPRESI_BPRESLOCATION, &bpResolutionInfo))) {
		CRLog::error("failed to get breakpoint resolution info");
		pBPResolution->Release();
		return E_FAIL;
	}
	bp->setBound(boundBp);
	IDebugCodeContext2* context = bpResolutionInfo.bpResLocation.bpResLocation.bpresCode.pCodeContext;
	if (context) {
		IDebugDocumentContext2 * pSrcCtxt = NULL;
		if (SUCCEEDED(context->GetDocumentContext(&pSrcCtxt))) {
			IDebugDocument2 * pDoc = NULL;
			if (SUCCEEDED(pSrcCtxt->GetDocument(&pDoc))) {
				BSTR fname = NULL;
				BSTR fullfname = NULL;
				pDoc->GetName(GN_FILENAME, &fullfname);
				pDoc->GetName(GN_BASENAME, &fname);
				if (fullfname) SysFreeString(fullfname);
				if (fname) SysFreeString(fname);
				pDoc->Release();
			}
			TEXT_POSITION beginPos, endPos;
			if (SUCCEEDED(pSrcCtxt->GetSourceRange(&beginPos, &endPos))) {
				bp->boundLine = beginPos.dwLine;
				CRLog::debug("Breakpoint bound to line %d", beginPos.dwLine);
			}
			BSTR debugCodeContextName = NULL;
			context->GetName(&debugCodeContextName);
			if (debugCodeContextName) {
				SysFreeString(debugCodeContextName);
			}
			BSTR pSrcName = NULL;
			context->GetName(&pSrcName);
			if (pSrcName) {
				SysFreeString(pSrcName);
			}
			pSrcCtxt->Release();
		}
		CONTEXT_INFO info;
		memset(&info, 0, sizeof(info));
		if (SUCCEEDED(context->GetInfo(CIF_ALLFIELDS, &info))) {
			int fnLine = info.posFunctionOffset.dwLine;
			if (info.bstrFunction && info.bstrAddressOffset)
				bp->functionName = std::wstring(info.bstrFunction) + std::wstring(info.bstrAddressOffset);
			else if (info.bstrFunction)
				bp->functionName = info.bstrFunction;
			if (info.bstrAddress)
				bp->address = info.bstrAddress;
			if (info.bstrModuleUrl)
				bp->moduleName = info.bstrModuleUrl;
			if (info.bstrModuleUrl) SysFreeString(info.bstrModuleUrl);
			if (info.bstrAddress) SysFreeString(info.bstrAddress);
			if (info.bstrAddressAbsolute) SysFreeString(info.bstrAddressAbsolute);
			if (info.bstrAddressOffset) SysFreeString(info.bstrAddressOffset);
			if (info.bstrFunction) SysFreeString(info.bstrFunction);
		}
		context->Release();
	}
	pBPResolution->Release();
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
	IDebugErrorBreakpoint2 * pErrorBp = NULL;
	if (FAILED(pEvent->GetErrorBreakpoint(&pErrorBp))) {
		CRLog::error("pEvent->GetErrorBreakpoint failed");
		return E_FAIL;
	}

	IDebugPendingBreakpoint2 * pPendingBP = NULL;
	if (FAILED(pErrorBp->GetPendingBreakpoint(&pPendingBP))) {
		CRLog::error("Cannot get pending breakpoint in OnDebugBreakpointError");
		pErrorBp->Release();
		return E_FAIL;
	}
	BreakpointInfoRef bp = _breakpointList.findByPendingBreakpoint(pPendingBP);
	pPendingBP->Release();
	bp->error = true;
	if (!bp.Get()) {
		// unknown breakpoint
		CRLog::error("Unknown breakpoint request in OnDebugBreakpointBound");
		pErrorBp->Release();
		return E_FAIL;
	}
	//
	IDebugErrorBreakpointResolution2 * pResolution = NULL;
	if (FAILED(pErrorBp->GetBreakpointResolution(&pResolution))) {
		pErrorBp->Release();
		return E_FAIL;
	}
	pErrorBp->Release();
	BP_ERROR_RESOLUTION_INFO info;
	memset(&info, 0, sizeof(info));
	if (SUCCEEDED(pResolution->GetResolutionInfo(BPERESI_TYPE | BPERESI_MESSAGE, &info))) {
		std::wstring msg = info.bstrMessage;
		SysFreeString(info.bstrMessage);
		bp->errorMessage = msg;
		writeErrorMessage(bp->requestId, msg);
		writeDebuggerMessage(std::wstring(L"Breakpoint binding error: ") + msg);
	}
	pResolution->Release();
	_breakpointList.removeItem(bp);
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
	paused(pThread, PAUSED_BY_EXCEPTION);
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


// helper functions


// reads thread id from thread
DWORD getThreadId(IDebugThread2 * pThread) {
	if (!pThread)
		return 0;
	DWORD res = 0;
	if (FAILED(pThread->GetThreadId(&res)))
		return 0;
	return res;
}

