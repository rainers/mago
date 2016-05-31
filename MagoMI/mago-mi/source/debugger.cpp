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

// MI interface stdout output: [##requestId##]^result[,msg] -- same as writeResultMessage but msg is raw string
void Debugger::writeResultMessageRaw(ulong requestId, const wchar_t * status, std::wstring msg, wchar_t typeChar) {
	WstringBuffer buf;
	buf.appendUlongIfNonEmpty(requestId);
	buf.append(typeChar);
	buf.append(status);
	if (!msg.empty()) {
		buf.append(L",");
		buf.append(msg.c_str());
	}
	writeStdout(buf.wstr());
}

// MI interface stdout output: [##requestId##]^result[,"msg"]
void Debugger::writeResultMessage(ulong requestId, const wchar_t * status, std::wstring msg, wchar_t typeChar) {
	WstringBuffer buf;
	buf.appendUlongIfNonEmpty(requestId);
	buf.append(typeChar);
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
			buf.appendStringParam(L"code", std::wstring(errorCode));
		writeStdout(buf.wstr());
	}
	else
		writeStdout(msg);
}

void Debugger::showHelp() {
	wstring_vector res;
	getCommandsHelp(res, params.miMode);
	for (unsigned i = 0; i < res.size(); i++)
	    writeDebuggerMessage(res[i]);
}

/// called on new input line
void Debugger::onInputLine(std::wstring &s) {
	
	CRLog::trace("Input line: %s", toUtf8(s).c_str());
	if (s.empty())
		return;

	//if (_entryPointContinuePending) {
	//	_entryPointContinuePending = false;
	//	resume();
	//}

	MICommand cmd;
	if (!cmd.parse(s)) {
		writeErrorMessage(cmd.requestId, std::wstring(L"invalid command syntax: ") + s, L"undefined-command");
		return;
	}
	switch (cmd.commandId) {
	case CMD_UNKNOWN:
	default:
		if (cmd.miCommand)
			writeErrorMessage(cmd.requestId, std::wstring(L"Undefined MI command: ") + s, L"undefined-command");
		else
			writeErrorMessage(cmd.requestId, std::wstring(L"unknown command: ") + s, L"undefined-command");
		break;
	case CMD_GDB_EXIT:
		CRLog::info("quit requested");
		_quitRequested = true;
		if (params.miMode) {
			writeStdout(L"^exit");
			Sleep(10);
		}
		break;
	case CMD_HELP:
		showHelp();
		break;
	case CMD_EXEC_RUN:
		if (cmd.hasParam(std::wstring(L"--start")))
			params.stopOnEntry = true;
		run(cmd.requestId);
		break;
	case CMD_EXEC_CONTINUE:
		resume(cmd.requestId);
		break;
	case CMD_EXEC_INTERRUPT:
		causeBreak(cmd.requestId);
		break;
	case CMD_EXEC_FINISH:
		step(STEP_OUT, STEP_LINE, cmd.threadId, cmd.requestId);
		break;
	case CMD_EXEC_NEXT:
		step(STEP_OVER, STEP_LINE, cmd.threadId, cmd.requestId);
		break;
	case CMD_EXEC_NEXT_INSTRUCTION:
		step(STEP_OVER, STEP_INSTRUCTION, cmd.threadId, cmd.requestId);
		break;
	case CMD_EXEC_STEP:
		step(STEP_INTO, STEP_LINE, cmd.threadId, cmd.requestId);
		break;
	case CMD_EXEC_STEP_INSTRUCTION:
		step(STEP_INTO, STEP_INSTRUCTION, cmd.threadId, cmd.requestId);
		break;
	case CMD_BREAK_INSERT:
		handleBreakpointInsertCommand(cmd);
		break;
	case CMD_BREAK_DELETE:
		handleBreakpointDeleteCommand(cmd);
		break;
	case CMD_BREAK_ENABLE:
		handleBreakpointEnableCommand(cmd, true);
		break;
	case CMD_BREAK_DISABLE:
		handleBreakpointEnableCommand(cmd, false);
		break;
	case CMD_LIST_THREAD_GROUPS:
		{
			if (!_paused && (cmd.unnamedValue(0) == L"i1" || cmd.hasParam(L"--available"))) {
				writeErrorMessage(cmd.requestId, L"Can not fetch data now.\n");
				return;
			}
			if (cmd.unnamedValue(0) == L"i1") {
				handleThreadInfoCommand(cmd, false);
				return;
			}
			WstringBuffer buf;
			buf.appendUlongIfNonEmpty(cmd.requestId);
			buf.append(L"^done");
			buf.append(L",groups=[{");
			buf.appendStringParam(L"id", L"i1");
			buf.appendStringParam(L"type", L"process");
			buf.appendStringParam(L"pid", L"123"); // todo: add real PID
			buf.appendStringParamIfNonEmpty(L"executable", params.exename);
			buf.append(L"}]");
			writeStdout(buf.wstr());
		}
		break;
	case CMD_BREAK_LIST:
		handleBreakpointListCommand(cmd);
		break;
	case CMD_THREAD_INFO:
		handleThreadInfoCommand(cmd, false);
		break;
	case CMD_THREAD_LIST_IDS:
		handleThreadInfoCommand(cmd, true);
		break;
	case CMD_STACK_LIST_FRAMES:
		handleStackListFramesCommand(cmd, false);
		break;
	case CMD_STACK_INFO_DEPTH:
		handleStackListFramesCommand(cmd, true);
		break;
	//case CMD_STACK_LIST_ARGUMENTS:
	//	handleStackListVariablesCommand(cmd, false, true);
	//	break;
	case CMD_STACK_LIST_VARIABLES:
		handleStackListVariablesCommand(cmd, false, false);
		break;
	case CMD_STACK_LIST_LOCALS:
		handleStackListVariablesCommand(cmd, true, false);
		break;
	case CMD_VAR_CREATE:
	case CMD_VAR_UPDATE:
	case CMD_VAR_DELETE:
		handleVariableCommand(cmd);
		break;
	case CMD_VAR_SET_FORMAT:
		handleVariableCommand(cmd);
		break;
	case CMD_LIST_FEATURES:
		{
			WstringBuffer buf;
			buf.appendUlongIfNonEmpty(cmd.requestId);
			buf.append(L"^done,features=[\"frozen - varobjs\",\"pending - breakpoints\",\"thread-info\"]");
			//,\"breakpoint-notifications\",\"undefined-command-error-code\",\"exec-run-start-option\"
			writeStdout(buf.wstr());
		}
		break;
	case CMD_GDB_VERSION:
		writeStdout(L"~\"" VERSION_STRING L"\\n\"");
		writeStdout(L"~\"" VERSION_EXPLANATION_STRING L"\\n\"");
		writeResultMessage(cmd.requestId, L"done");
		break;
	case CMD_ENVIRONMENT_CD:
		{
			if (cmd.unnamedValues.size() != 1) {
				writeErrorMessage(cmd.requestId, L"directory name parameter required");
				return;
			}
			std::wstring dir = unquoteString(cmd.unnamedValues[0]);
			params.setDir(dir);
			CRLog::info("Changing current directory to %s", toUtf8z(params.dir));
			if (SetCurrentDirectoryW(dir.c_str()) != TRUE)
				CRLog::error("Cannot change current directory to %s", toUtf8z(params.dir));
			writeResultMessage(cmd.requestId, L"done");
		}
		break;
	case CMD_GDB_SHOW:
		if (cmd.unnamedValue(0) == L"language") {
			writeResultMessageRaw(cmd.requestId, L"done", L"value=\"auto\"");
			return;
		}
		CRLog::warn("command -gdb-show is not implemented");
		writeResultMessage(cmd.requestId, L"done");
		break;
	case CMD_INTERPRETER_EXEC:
		if (cmd.unnamedValue(0) == L"console") {
			if (unquoteString(cmd.unnamedValue(1)) == L"show endian") {
				writeDebuggerMessage(L"The target endianness is set automatically (currently little endian)\n");
				writeResultMessage(cmd.requestId, L"done");
				return;
			}
			else if (unquoteString(cmd.unnamedValue(1)) == L"p/x (char)-1") {
				writeDebuggerMessage(L"$1 = 0xff\n");
				writeResultMessage(cmd.requestId, L"done");
				return;
			}
			else if (unquoteString(cmd.unnamedValue(1)) == L"kill") {
				stop(cmd.requestId);
				return;
			}
		}
		CRLog::warn("command -interpreter-exec is not implemented");
		writeResultMessage(cmd.requestId, L"done");
		break;
	case CMD_DATA_EVALUATE_EXPRESSION:
		handleDataEvaluateExpressionCommand(cmd);
		break;
	case CMD_GDB_SET:
		CRLog::warn("command -gdb-set is not implemented");
		writeResultMessage(cmd.requestId, L"done");
		break;
	case CMD_MAINTENANCE:
		CRLog::warn("command maintenance is not implemented");
		writeStdout(L"&\"%s\\n\"", cmd.commandText.c_str());
		writeResultMessage(cmd.requestId, L"done");
		break;
	case CMD_ENABLE_PRETTY_PRINTING:
		CRLog::warn("command -enable-pretty-printing is not implemented");
		writeResultMessage(cmd.requestId, L"done");
		break;
	case CMD_HANDLE:
		// ignore, reply done
		writeResultMessage(cmd.requestId, L"done");
		break;
	case CMD_SOURCE:
		CRLog::warn("command source is not implemented");
		writeStdout(L"&\"source.gdbinit\\n\"");
		writeStdout(L"&\".gdbinit: No such file or directory.\\n\"");
		writeErrorMessage(cmd.requestId, L".gdbinit: No such file or directory.");
		break;
	case CMD_FILE_EXEC_AND_SYMBOLS:
		{
			if (cmd.unnamedValues.size() != 1) {
				writeErrorMessage(cmd.requestId, L"directory name parameter required");
				return;
			}
			std::wstring fn = unquoteString(cmd.unnamedValues[0]);
			params.setExecutable(fn);
			load(cmd.requestId, true);
			writeResultMessage(cmd.requestId, L"done");

		}
		break;
	}
}

static uint64_t nextVarId = 1;

// called to handle -data-evaluate-expression command
void Debugger::handleDataEvaluateExpressionCommand(MICommand & cmd) {
	WstringBuffer buf;
	buf.appendUlongIfNonEmpty(cmd.requestId);
	if (cmd.unnamedValue(0) == L"sizeof (void*)" || cmd.unnamedValue(0) == L"\"sizeof (void*)\"") {
		writeResultMessageRaw(cmd.requestId, L"done", L"value=\"4\"");
		return;
	}
	std::wstring expr = cmd.unnamedValue(0);
	DWORD threadId = cmd.threadId;
	DWORD frameIndex = cmd.frameId;
	// returns stack frame if found
	IDebugStackFrame2 * frame = getStackFrame(threadId, frameIndex);
	if (!frame) {
		writeErrorMessage(cmd.requestId, L"cannot find specified thread or stack frame");
		return;
	}
	StackFrameInfo frameInfo;
	bool hasContext = getThreadFrameContext(findThreadById(threadId), &frameInfo) == 1;
	LocalVariableList list;
	if (frame && getLocalVariables(frame, list, true)) {
		for (unsigned i = 0; i < list.size(); i++) {
			if (list[i]->varName != expr)
				continue;
			buf.append(L"^done");
			buf.appendStringParam(L"value", list[i]->varValue);
			writeStdout(buf.wstr());
			return;
		}
	}
	writeErrorMessage(cmd.requestId, std::wstring(L"Cannot evaluate ") + quoteString(expr));
}

// called to handle variable commands
void Debugger::handleVariableCommand(MICommand & cmd) {
	WstringBuffer buf;
	buf.appendUlongIfNonEmpty(cmd.requestId);
	VariableObjectRef var;
	std::wstring name = cmd.unnamedValue(0);
	std::wstring addr = L"*";
	std::wstring expr;

	int varIndex = -1;
	if (cmd.commandName == L"-var-delete") {
		var = _varList.find(name, &varIndex);
		if (varIndex >= 0) {
			_varList.erase(_varList.begin() + varIndex);
			buf.append(L"^done");
			writeStdout(buf.wstr());
			return;
		}
		writeErrorMessage(cmd.requestId, std::wstring(L"No variable ") + quoteString(name));
		return;
	}
	bool isVarCreate = cmd.commandName == L"-var-create";
	bool isVarUpdate = cmd.commandName == L"-var-update";
	VariableObjectList updatedVarsList;

	if (isVarCreate) {
		name = cmd.unnamedValue(0);
		addr = cmd.unnamedValue(1);
		expr = cmd.unnamedValue(2);
		if (name == L"-")
			name = std::wstring(L"var") + toWstring(nextVarId++);
	}

	DWORD threadId = cmd.threadId;
	DWORD frameIndex = cmd.frameId;
	// returns stack frame if found
	IDebugStackFrame2 * frame = getStackFrame(threadId, frameIndex);
	if (!frame) {
		writeErrorMessage(cmd.requestId, L"cannot find specified thread or stack frame");
		return;
	}
	StackFrameInfo frameInfo;
	bool hasContext = getThreadFrameContext(findThreadById(threadId), &frameInfo) == 1;
	if (addr == L"*") {
		addr = frameInfo.address;
	}

	if (isVarUpdate) {
		name = cmd.unnamedValue(1);
	}
	bool allVars = isVarUpdate && name == L"*";
	if (isVarUpdate) {
		if (!allVars) {
			var = _varList.find(name, &varIndex);
			if (varIndex == -1) {
				writeErrorMessage(cmd.requestId, std::wstring(L"No variable ") + quoteString(name));
				return;
			}
			expr = var->expr;
		}
	}


	LocalVariableList list;
	bool found = false;
	if (isVarCreate)
		var = new VariableObject();
	if (frame && getLocalVariables(frame, list, true)) {
		for (unsigned i = 0; i < list.size(); i++) {
			if (!allVars && list[i]->varName != expr)
				continue;
			found = true;
			if (allVars)
				var = _varList.find(addr, list[i]->varName);
			if (var.Get()) {
				var->type = list[i]->varType;
				var->value = list[i]->varValue;
				if (isVarUpdate)
					var->inScope = var->frame == addr;
				updatedVarsList.push_back(var);
				if (!allVars)
					break;
			}
		}
	}

	if (!found) {
		writeErrorMessage(cmd.requestId, std::wstring(L"No symbol ") + quoteString(expr));
		return;
	}

	if (name.empty() || addr.empty() || expr.empty()) {
		writeErrorMessage(cmd.requestId, L"Invalid -var-create command");
		return;
	}
	buf.append(L"^done");
	if (isVarCreate) {
		var->name = name;
		var->frame = addr;
		var->expr = expr;
		_varList.push_back(var);
		var->dumpVariableInfo(buf, false);
	}
	else if (isVarUpdate) {
		buf.append(L",changelist=[");
		for (unsigned i = 0; i < updatedVarsList.size(); i++) {
			var = updatedVarsList[i];
			if (i > 0)
				buf.append(L",");
			buf.append(L"{");
			var->dumpVariableInfo(buf, true);
			buf.append(L"}");
		}
		buf.append(L"]");
	}
	writeStdout(buf.wstr());
}

// called to handle -stack-list-variables command
void Debugger::handleStackListVariablesCommand(MICommand & cmd, bool localsOnly, bool argsOnly) {
	if (!_paused || _stopped) {
		writeErrorMessage(cmd.requestId, L"Cannot get variables for running or terminated process");
		return;
	}

	WstringBuffer buf;
	buf.appendUlongIfNonEmpty(cmd.requestId);
	// start list
	if (localsOnly)
		buf.append(L"^done,locals=[");
	else
		buf.append(L"^done,variables=[");

	DWORD threadId = cmd.threadId;
	DWORD frameIndex = cmd.frameId;

	int level = cmd.printLevel;

	// returns stack frame if found
	IDebugStackFrame2 * frame = getStackFrame(threadId, frameIndex);
	if (!frame) {
		writeErrorMessage(cmd.requestId, L"cannot find specified thread or stack frame");
		return;
	}
	bool includeArgs = !localsOnly;
	LocalVariableList list;
	if (frame && getLocalVariables(frame, list, includeArgs)) {
		for (unsigned i = 0; i < list.size(); i++) {
			if (i != 0)
				buf.append(L",");
			list[i]->dumpMiVariable(buf, level != PRINT_NO_VALUES ? true : false, level != PRINT_NO_VALUES ? true : false);
		}
	}
	
	//// fake output for testing
	//buf.append(L"{name=\"x\",value=\"11\"},{name=\"s\",value=\"{a = 1, b = 2}\"}");

	// end of list
	buf.append(L"]");
	writeStdout(buf.wstr());
}

#define MAX_FRAMES 100
// called to handle -stack-list-frames command and -stack-info-depth
void Debugger::handleStackListFramesCommand(MICommand & cmd, bool depthOnly) {
	if (!_paused || _stopped) {
		writeErrorMessage(cmd.requestId, L"Cannot get frames info for running or terminated process");
		return;
	}
	int maxDepth = 0;
	int depth = 0;
	if (depthOnly) {
		uint64_t n = 0;
		if (toUlong(cmd.unnamedValue(0), n))
			maxDepth = (int)n;
	}
	DWORD tid = cmd.threadId;
	IDebugThread2 * pThread = findThreadById(tid);
	StackFrameInfo frameInfos[MAX_FRAMES];
	unsigned minIndex = 0;
	unsigned maxIndex = MAX_FRAMES - 1;
	if (cmd.unnamedValues.size() >= 2) {
		uint64_t n1 = 0;
		uint64_t n2 = 0;
		if (toUlong(cmd.unnamedValues[0], n1) && toUlong(cmd.unnamedValues[0], n2)) {
			minIndex = (int)n1;
			maxIndex = (int)n2;
			if (maxIndex >= minIndex + MAX_FRAMES)
				maxIndex = minIndex + MAX_FRAMES - 1;
		}
	}
	unsigned frameCount = getThreadFrameContext(pThread, frameInfos, minIndex, maxIndex);
	if (!frameCount) {
		writeErrorMessage(cmd.requestId, L"Cannot get frames info");
		return;
	}
	WstringBuffer buf;
	buf.appendUlongIfNonEmpty(cmd.requestId);
	buf.append(L"^done");
	if (depthOnly) {
		depth = frameCount;
		if (maxDepth && depth > maxDepth)
			depth = maxDepth;
		buf.appendUlongParamAsString(L"depth", depth);
	}
	else {
		buf.append(L",stack=[");
		for (unsigned i = 0; i < frameCount; i++) {
			if (i > 0)
				buf.append(L",");
			buf.append(L"frame=");
			frameInfos[i].dumpMIFrame(buf, true);
		}
		buf.append(L"]");
	}
	writeStdout(buf.wstr());
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
	buf.appendUlongIfNonEmpty(cmd.requestId);
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
					if (getThreadFrameContext(thread, &frameInfo)) {
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
	if (_stopped) {
		writeErrorMessage(cmd.requestId, L"Application is stopped");
		return;
	}
	//writeDebuggerMessage(cmd.dumpCommand());
	BreakpointInfoRef bp = new BreakpointInfo();
	bp->fromCommand(cmd);
	//writeDebuggerMessage(bp->dumpParams());
	CRLog::debug("new breakpoint cmd: %s", toUtf8z(cmd.dumpCommand()));
	CRLog::debug("new breakpoint params: %s", toUtf8z(bp->dumpParams()));
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
		buf.appendUlongIfNonEmpty(cmd.requestId);
		buf.append(L"^done,bkpt=");
		bp->printBreakpointInfo(buf);
		writeStdout(buf.wstr());
		//writeDebuggerMessage(std::wstring(L"added pending breakpoint"));
	}
	else {
		writeErrorMessage(cmd.requestId, L"Invalid breakpoint parameters");
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
	CRLog::info("Mode: %s", _cmdinput.inConsole() ? "Console" : "Stream");
	writeStdout(L"=thread-group-added,id=\"i1\"");
	if (params.miMode && !params.silent) {
		writeStdout(L"~\"" VERSION_STRING L"\\n\"");
		writeStdout(L"~\"" VERSION_EXPLANATION_STRING L"\\n\"");
	}
	if (!params.miMode && !params.silent) {

		// some software detect GDB by "GNU gdb..." line
		writeStdout(VERSION_STRING);
		writeStdout(VERSION_EXPLANATION_STRING);
		writeStdout(L"This is text (gdb MI) interface for Mago debugger: https://github.com/rainers/mago");
		writeStdout(L"Mago-MI project page: https://github.com/buggins/mago");

		writeStdout(L"Developer: Vadim Lopatin <" L"coolreader.org" /* anti */ L"@" /* spam */ L"gmail.com>");
		writeStdout(L"Supports subset of GDB commands. Type help to see list of available commands.");

	}
	else if (params.miMode && !params.silent) {
		// some software detect GDB by "GNU gdb..." line
		//writeDebuggerMessage(L"GNU gdb compatible debugger mago-mi " MAGO_MI_VERSION L"\n");

	}
	if (params.hasExecutableSpecified()) {
		load();
	}

	while (!_cmdinput.isClosed() && !_quitRequested) {
		//if (_entryPointContinuePending) {
		//	CRLog::debug("_entryPointContinuePending: calling resume()");
		//	_entryPointContinuePending = false;
		//	resume();
		//}
		//CRLog::trace("before poll");
		_cmdinput.poll();
		//CRLog::trace("after poll");
		if (_entryPointContinuePending) {
			CRLog::debug("_entryPointContinuePending: calling resume()");
			_entryPointContinuePending = false;
			resume();
		}
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


	writeStdout(L"=thread-group-started,id=\"i1\",pid=\"123\""); // TODO: proper PID
	writeStdout(L"=thread-created,id=\"%d\",group-id=\"i1\"", getThreadId(NULL));

	writeResultMessage(requestId, L"running", NULL, '^');
	writeResultMessage(UNSPECIFIED_REQUEST_ID, L"running,thread-id=\"all\"", NULL, '*');
	writeStdout(L"(gdb) "); // TODO: proper PID
	return true;
}

// resume paused execution (continue)
bool Debugger::resume(uint64_t requestId, DWORD threadId) {
	CRLog::trace("Debugger::resume()");
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
	CRLog::trace("Debugger::resume() : calling pProgram->Continue");
	if (FAILED(_pProgram->Continue(pThread))) {
		writeErrorMessage(requestId, std::wstring(L"Failed to continue process"));
		return false;
	}
	//writeResultMessage(requestId, L"running", NULL);
	_paused = false;
	_cmdinput.enable(false);
	return true;
}

/// stop program execution
bool Debugger::stop(uint64_t requestId) {
	if (FAILED(_pProgram->Terminate())) {
		writeErrorMessage(requestId, std::wstring(L"Failed to terminate process"));
		return false;
	}
	writeResultMessage(requestId, L"done");
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
	if (!threadId) {
		if (_paused)
			return _pThread;
		return NULL;
	}

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
	DWORD threadId = getThreadId(pThread);
	bool hasContext = getThreadFrameContext(pThread, &frameInfo) == 1;


	WstringBuffer buf;
	buf.append(L"=thread-selected");
	buf.appendUlongParamAsString(L"id", threadId);
	writeStdout(buf.wstr());

	buf.reset();
	buf.appendUlongIfNonEmpty(requestId);
	buf.append(L"*stopped");
	std::wstring reasonName;
	switch (reason) {
	case PAUSED_BY_BREAKPOINT:
	case PAUSED_BY_ENTRY_POINT_REACHED:
	case PAUSED_BY_LOAD_COMPLETED:
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

	buf.appendUlongParamAsString(L"thread-id", threadId, ',');
	buf.appendStringParam(L"stopped-threads", std::wstring(L"all"), ',');
	buf.appendStringParam(L"core", std::wstring(L"1"), ',');
	writeStdout(buf.wstr());
	_cmdinput.enable(true);
}


// returns stack frame if found
IDebugStackFrame2 * Debugger::getStackFrame(DWORD threadId, unsigned frameIndex) {
	if (!_paused || _stopped) {
		return NULL;
	}
	IDebugThread2 * pThread = findThreadById(threadId);
	if (!pThread)
		return NULL;
	return getStackFrame(pThread, frameIndex);
}

// returns stack frame if found
IDebugStackFrame2 * Debugger::getStackFrame(IDebugThread2 * pThread, unsigned frameIndex) {
	if (!_paused || _stopped || !pThread) {
		return NULL;
	}
	IEnumDebugFrameInfo2* pFrames = NULL;
	if (FAILED(pThread->EnumFrameInfo(FIF_FUNCNAME | FIF_RETURNTYPE | FIF_ARGS | FIF_DEBUG_MODULEP | FIF_DEBUGINFO | FIF_MODULE | FIF_FRAME, 10, &pFrames))) {
		CRLog::error("cannot get thread frame enum");
		return false;
	}
	unsigned outIndex = 0;
	ULONG count = 0;
	pFrames->GetCount(&count);
	for (ULONG i = 0; i < count; i++) {
		FRAMEINFO frame;
		memset(&frame, 0, sizeof(FRAMEINFO));
		ULONG fetched = 0;
		if (FAILED(pFrames->Next(1, &frame, &fetched)) || fetched != 1)
			break;
		if (i != frameIndex) {
			if (frame.m_pFrame)
				frame.m_pFrame->Release();
			continue;
		}
		if (frame.m_pFrame) {
			pFrames->Release();
			return frame.m_pFrame;
		}
	}
	pFrames->Release();
	return NULL;
}

// retrieves list of local variables from debug frame
bool Debugger::getLocalVariables(IDebugStackFrame2 * frame, LocalVariableList &list, bool includeArgs) {
	if (!frame)
		return NULL;
	ULONG count = 0;
	IEnumDebugPropertyInfo2* pEnum = NULL;
	if (SUCCEEDED(frame->EnumProperties(DEBUGPROP_INFO_FULLNAME | DEBUGPROP_INFO_NAME | DEBUGPROP_INFO_TYPE | DEBUGPROP_INFO_VALUE | DEBUGPROP_INFO_ATTRIB | DEBUGPROP_INFO_VALUE_AUTOEXPAND,
		10, guidFilterAllLocalsPlusArgs, 1000, &count, &pEnum)) && pEnum) { //guidFilterAllLocals
		// get info
		for (unsigned i = 0; i < count; i++) {
			ULONG fetched = 0;
			DEBUG_PROPERTY_INFO prop;
			memset(&prop, 0, sizeof(prop));
			if (SUCCEEDED(pEnum->Next(1, &prop, &fetched)) && fetched == 1) {
				LocalVariableInfoRef item = new LocalVariableInfo();
				if (prop.dwFields & DEBUGPROP_INFO_FULLNAME)
					item->varFullName = fromBSTR(prop.bstrFullName);
				if (prop.dwFields & DEBUGPROP_INFO_NAME)
					item->varName = fromBSTR(prop.bstrName);
				if (prop.dwFields & DEBUGPROP_INFO_TYPE)
					item->varType = fromBSTR(prop.bstrType);
				if (prop.dwFields & DEBUGPROP_INFO_VALUE)
					item->varValue = fromBSTR(prop.bstrValue);
				if (prop.dwFields & DEBUGPROP_INFO_ATTRIB) {
					if (prop.dwAttrib & DBG_ATTRIB_OBJ_IS_EXPANDABLE)
						item->expandable = true;
					if (prop.dwAttrib & DBG_ATTRIB_VALUE_READONLY)
						item->readonly = true;
				}
				list.push_back(item);
			}
		}
	}
	if (pEnum)
		pEnum->Release();
	frame->Release();
	return true;
}

// gets thread frame contexts
unsigned Debugger::getThreadFrameContext(IDebugThread2 * pThread, StackFrameInfo * frameInfo, unsigned minFrame, unsigned maxFrame) {
	if (!_paused || _stopped || !pThread) {
		return false;
	}
	if (maxFrame < minFrame) {
		CRLog::error("getThreadFrameContext -- invalid frame range");
		return false;
	}
	IEnumDebugFrameInfo2* pFrames = NULL;
	if (FAILED(pThread->EnumFrameInfo(FIF_FUNCNAME | FIF_RETURNTYPE | FIF_ARGS | FIF_DEBUG_MODULEP | FIF_DEBUGINFO | FIF_MODULE | FIF_FRAME, 10, &pFrames))) {
		CRLog::error("cannot get thread frame enum");
		return false;
	}
	unsigned outIndex = 0;
	ULONG count = 0;
	pFrames->GetCount(&count);
	for (ULONG i = 0; i < count; i++) {
		FRAMEINFO frame;
		memset(&frame, 0, sizeof(FRAMEINFO));
		ULONG fetched = 0;
		if (FAILED(pFrames->Next(1, &frame, &fetched)) || fetched != 1)
			break;
		if (i < minFrame)
			continue;
		if (i > maxFrame)
			break;
		if (frame.m_pFrame) {
			IDebugCodeContext2 * pCodeContext = NULL;
			IDebugDocumentContext2 * pDocumentContext = NULL;
			//IDebugMemoryContext2 * pMemoryContext = NULL;
			frame.m_pFrame->GetCodeContext(&pCodeContext);
			frame.m_pFrame->GetDocumentContext(&pDocumentContext);
			CONTEXT_INFO contextInfo;
			memset(&contextInfo, 0, sizeof(CONTEXT_INFO));
			frameInfo[outIndex].frameIndex = i;
			if (pCodeContext)
				pCodeContext->GetInfo(CIF_ALLFIELDS, &contextInfo);
			if (contextInfo.bstrAddress)
				frameInfo[outIndex].address = contextInfo.bstrAddress;
			if (contextInfo.bstrFunction && contextInfo.bstrAddressOffset)
				frameInfo[outIndex].functionName = std::wstring(contextInfo.bstrFunction) + std::wstring(contextInfo.bstrAddressOffset);
			else if (contextInfo.bstrFunction)
				frameInfo[outIndex].functionName = contextInfo.bstrFunction;
			if (contextInfo.bstrModuleUrl)
				frameInfo[outIndex].moduleName = contextInfo.bstrModuleUrl;
			frameInfo[outIndex].sourceLine = contextInfo.posFunctionOffset.dwLine;
			frameInfo[outIndex].sourceColumn = contextInfo.posFunctionOffset.dwColumn;
			TEXT_POSITION srcBegin, srcEnd;
			memset(&srcBegin, 0, sizeof(srcBegin));
			memset(&srcEnd, 0, sizeof(srcEnd));
			TEXT_POSITION stmtBegin, stmtEnd;
			memset(&stmtBegin, 0, sizeof(stmtBegin));
			memset(&stmtEnd, 0, sizeof(stmtEnd));
			if (pDocumentContext) {
				if (SUCCEEDED(pDocumentContext->GetSourceRange(&srcBegin, &srcEnd))) {
					if (srcBegin.dwLine)
						frameInfo[outIndex].sourceLine = srcBegin.dwLine + 1;
					//srcBegin.dwLine;
					//srcBegin.dwColumn;
				}
				if (SUCCEEDED(pDocumentContext->GetStatementRange(&stmtBegin, &stmtEnd))) {
					if (stmtBegin.dwLine)
						frameInfo[outIndex].sourceLine = stmtBegin.dwLine + 1;

					//srcBegin.dwLine;
					//srcBegin.dwColumn;
				}
				BSTR pFileName = NULL;
				BSTR pBaseName = NULL;
				pDocumentContext->GetName(GN_FILENAME, &pFileName);
				pDocumentContext->GetName(GN_BASENAME, &pBaseName);
				if (pFileName) { frameInfo[outIndex].sourceFileName = pFileName; SysFreeString(pFileName); }
				if (pBaseName) { frameInfo[outIndex].sourceBaseName = pBaseName; SysFreeString(pBaseName); }
			}
			if (pDocumentContext)
				pDocumentContext->Release();
			if (pCodeContext)
				pCodeContext->Release();
			outIndex++;
		}
	}
	pFrames->Release();
	return outIndex;
}

// step paused program
bool Debugger::step(STEPKIND stepKind, STEPUNIT stepUnit, DWORD threadId, uint64_t requestId) {
	if (!_started || !_loaded || !_pProgram) {
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

	if (params.miMode) {
		writeResultMessage(requestId, L"running", NULL, '^');
		writeResultMessage(UNSPECIFIED_REQUEST_ID, L"running,thread-id=\"all\"", NULL, '*');
	}
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
	//writeStdout(L"=thread-group-added,id=\"i1\"");
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
	_cmdinput.enable(true);
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
	_pThread = pThread;
	_paused = true;
	//paused(pThread, PAUSED_BY_LOAD_COMPLETED);
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
		CRLog::info("Will continue on next poll - setting _entryPointContinuePending");
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
	//writeStdout(L"=thread-created,id=\"%d\",group-id=\"i1\"", getThreadId(pThread));
	if (_verbose)
		writeDebuggerMessage(std::wstring(L"Thread created"));
	else
		CRLog::info("Thread created");
	DWORD threadId = getThreadId(pThread);
	if (_started)
		writeStdout(L"=thread-created,id=\"%d\",group-id=\"i1\"", threadId);
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
				bp->boundLine = beginPos.dwLine + 1;
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
	_cmdinput.enable(true);
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

