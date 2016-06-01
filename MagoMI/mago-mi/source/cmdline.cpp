#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "miutils.h"
#include "cmdline.h"
#include "logger.h"
#include "miutils.h"

// global object
ExecutableInfo params;

static void fatalError(const char * msg, int errCode) {
	fprintf(stderr, "%s\n", msg);
	CRLog::error("%s", msg);
	exit(errCode);
}

static void fatalError(std::wstring msg, int errCode) {
	fwprintf(stderr, L"%s\n", msg.c_str());
	CRLog::error("%s", toUtf8(msg).c_str());
	exit(errCode);
}

ExecutableInfo::ExecutableInfo()
	: verbose(false) 
	, miMode(false)
	, silent(false)
	, stopOnEntry(false)
{
	setDir(getCurrentDirectory());
}

void ExecutableInfo::clear() {
	exename.clear();
	dir.clear();
	args.clear();
}

ExecutableInfo::~ExecutableInfo() {
	clear();
}

bool ExecutableInfo::hasExecutableSpecified() {
	return !exename.empty();
}

void ExecutableInfo::setExecutable(std::wstring  exe) {
	if (exe.empty()) {
		exename.clear();
		return;
	}
	std::wstring tmp = fixPathDelimiters(unquoteString(exe));
	exename = relativeToAbsolutePath(tmp);
	CRLog::info("exename is set: %s", toUtf8z(exename));
}

void ExecutableInfo::setDir(std::wstring directory) {
	if (directory.empty()) {
		dir.clear();
		return;
	}
	std::wstring tmp = fixPathDelimiters(unquoteString(directory));
	dir = relativeToAbsolutePath(tmp);
	CRLog::info("dir is set: %s", toUtf8z(dir));
}

void ExecutableInfo::setTty(std::wstring tty) {
	this->tty = tty;
}

void ExecutableInfo::addArg(std::wstring param) {
	args.push_back(param);
}

void ExecutableInfo::dumpParams() {
	CRLog::info("%s", toUtf8(VERSION_STRING).c_str());
	if (!exename.empty()) {
		CRLog::info("Executable file: %s\n", toUtf8(exename).c_str());
		if (argCount()) {
			CRLog::info("Inferior arguments:");
			for (int i = 0; i < argCount(); i++) {
				CRLog::info("[%d] %s", i, toUtf8(args[i]).c_str());
			}
		}
	}
	if (!dir.empty())
		CRLog::info("Directory: %s\n", toUtf8(dir).c_str());
}

enum CmdLineParamType {
	NO_PARAMS,
	STRING_PARAM,
};

struct CmdLineParamDef;
typedef void(*parameterHandler)(CmdLineParamDef * param, const wchar_t * value);

struct CmdLineParamDef {
	const char * shortName;
	const char * longName;
	CmdLineParamType paramType;
	const char * description;
	const char * defValue;
	parameterHandler handler;
	bool isHelpSection() {
		return !shortName && !longName && description;
	}
	bool isLast() {
		return !shortName && !longName && !description;
	}

	// for normal parameter
	CmdLineParamDef(const char * _shortName, const char * _longName, CmdLineParamType _paramType, const char * _description, const char * _defValue, parameterHandler _handler)
		: shortName(_shortName), longName(_longName), paramType(_paramType), description(_description), defValue(_defValue), handler(_handler) {
	}
	// for help section
	CmdLineParamDef(const char * _description)
		: shortName(NULL), longName(NULL), paramType(NO_PARAMS), description(_description), defValue(NULL), handler(NULL) {
	}
	// for last item
	CmdLineParamDef()
		: shortName(NULL), longName(NULL), paramType(NO_PARAMS), description(NULL), defValue(NULL), handler(NULL) {
	}
};

void dumpParameterHelp();

static void showHelp(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	UNREFERENCED_PARAMETER(value);
	printf("This is Mago debugger command line interface. Usage:\n");
	printf("    mago-mi [options] [executable-file]\n");
	printf("    mago-mi [options] --args executable-file [inferior-arguments ...]\n");
	dumpParameterHelp();
	exit(0);
}

static void showVersion(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	UNREFERENCED_PARAMETER(value);
	//if (params.miMode)
	//	printf("~\"%s\\n%s\\n\"", toUtf8(VERSION_STRING).c_str(), toUtf8(VERSION_EXPLANATION_STRING).c_str());
	//else
	printf("%s\n%s\n", toUtf8(VERSION_STRING).c_str(), toUtf8(VERSION_EXPLANATION_STRING).c_str());
	exit(0);
}

void defParamHandler(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	UNREFERENCED_PARAMETER(value);
	// TODO
}

static void handleInterpreter(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	if (!wcscmp(value, L"mi2") || !wcscmp(value, L"mi") || !wcscmp(value, L"mi1")) {
		params.miMode = true;
		return;
	}
	if (!wcscmp(value, L"console")) {
		params.miMode = false;
		return;
	}
	fatalError("Unknown interpreter is specified", -3);
}

static bool argsFound = false;

static void handleArgs(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	if (params.hasExecutableSpecified()) {
		fatalError("Executable file already specified", 3);
	}
	params.setExecutable(value);
	argsFound = true;
}

static void handleExec(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	if (params.hasExecutableSpecified()) {
		fatalError("Executable file already specified", 3);
	}
	params.setExecutable(std::wstring(value));
}

static void handleDir(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	params.setDir(value);
}

static void handleNx(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	UNREFERENCED_PARAMETER(value);
	params.setDir(value);
}

static void handleLogFile(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	params.logFile = value;
}

static void handleLogLevel(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	params.logLevel = value;
}

static void handleVerbose(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	UNREFERENCED_PARAMETER(value);
	params.verbose = true;
}

static void handleSilent(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	UNREFERENCED_PARAMETER(value);
	params.silent = true;
}

void nonParamHandler(const wchar_t * value) {
	if (params.exename.empty()) {
		params.setExecutable(std::wstring(value));
		return;
	}
	if (!argsFound) {
		fatalError("Use --args to provide inferior arguments", 3);
	}
	params.addArg(std::wstring(value));
}

CmdLineParamDef paramDefs[] = {
	CmdLineParamDef("Selection of debuggee and its files"),
	CmdLineParamDef(NULL, "--args", STRING_PARAM, "Arguments after executable-file are passed to inferior", NULL, &handleArgs),
	CmdLineParamDef(NULL, "--exec=EXECFILE", STRING_PARAM, "Use EXECFILE as the executable.", NULL, &handleExec),
	CmdLineParamDef("Output and user interface control"),
	CmdLineParamDef(NULL, "--interpreter=mi2", STRING_PARAM, "Turn on GDB MI interface mode", NULL, &handleInterpreter),
	CmdLineParamDef("Operating modes"),
	CmdLineParamDef(NULL, "--help", NO_PARAMS, "Print this message and then exit", NULL, &showHelp),
	CmdLineParamDef(NULL, "--version", NO_PARAMS, "Print version information and then exit", NULL, &showVersion),
	CmdLineParamDef("-v", NULL, NO_PARAMS, "Verbose output", NULL, &handleVerbose),
	CmdLineParamDef("-q", "--silent", NO_PARAMS, "Don't print version info on startup", NULL, &handleSilent),
	CmdLineParamDef("Other options"),
	CmdLineParamDef(NULL, "--cd=DIR", STRING_PARAM, "Change current directory to DIR.", NULL, &handleDir),
	CmdLineParamDef("-n", "--nx", NO_PARAMS, "Do not execute commands found in any initializaton file", NULL, &handleNx),
	CmdLineParamDef("-t", "--tty", STRING_PARAM, "Run using named pipe for standard input/output", NULL, &handleNx),
	CmdLineParamDef(NULL, "--log-file=FILE", STRING_PARAM, "Set log file for debugger internal logging.", NULL, &handleLogFile),
	CmdLineParamDef(NULL, "--log-level=FATAL|ERROR|WARN|INFO|DEBUG|TRACE", STRING_PARAM, "Set log level for debugger internal logging.", NULL, &handleLogLevel),
	CmdLineParamDef()
};

void dumpParameterHelp() {
	for (int i = 0; !paramDefs[i].isLast(); i++) {
		if (paramDefs[i].isHelpSection()) {
			printf("\n%s:\n\n", paramDefs[i].description);
		} else {
			if (paramDefs[i].longName)
				printf("  %-16s %s\n", paramDefs[i].longName, paramDefs[i].description);
			if (paramDefs[i].shortName)
				printf("  %-16s %s\n", paramDefs[i].shortName, paramDefs[i].description);
		}
	}
}

CmdLineParamDef * findParam(const wchar_t * &name) {
	if (name[0] == '-' && name[1] != '-') {
		for (int i = 0; !paramDefs[i].isLast(); i++) {
			if (paramDefs[i].isHelpSection())
				continue;
			if (!paramDefs[i].shortName)
				continue;
			if (paramDefs[i].shortName[1] == name[1]) {
				name += 2;
				return &paramDefs[i];
			}
		}
	} else if (name[0] == '-' && name[1] == '-') {
		for (int i = 0; !paramDefs[i].isLast(); i++) {
			if (paramDefs[i].isHelpSection())
				continue;
			if (!paramDefs[i].longName)
				continue;
			int j = 0;
			for (; name[j] && paramDefs[i].longName[j] && (paramDefs[i].longName[j] != '=') && name[j] == paramDefs[i].longName[j]; j++)
				;
			if ((!paramDefs[i].longName[j] || paramDefs[i].longName[j] == '=') && (!name[j] || name[j] == '=')) {
				name += j;
				if (name[0] == '=')
					name++;
				return &paramDefs[i];
			}
		}
	}
	return NULL;
}

void parseCommandLine(int argc, wchar_t *argv[]) {
	for (int i = 1; i < argc; i++) {
		wchar_t * v = argv[i];
		if (v[0] == '-') {
			const wchar_t * value = v;
			CmdLineParamDef * param = findParam(value);
			if (!param) {
				fatalError(std::wstring(L"Unknown command line parameter ") + v, 1);
			}
			if (param->paramType != NO_PARAMS) {
				if (!value[0]) {
					if (i == argc - 1) {
						fatalError(std::wstring(L"Value not specified for parameter ") + v, 1);
					}
					i++;
					value = argv[i];
				}
			}
			else {
				if (value[0]) {
					fatalError(std::wstring(L"Value not allowed for parameter ") + v, 1);
				}
			}
			if (param->handler) {
				param->handler(param, value);
			} else {
				defParamHandler(param, value);
			}
		}
		else {
			nonParamHandler(v);
		}
	}
	// handle logging
	if (!params.logFile.empty()) {
		CRLog::log_level level = CRLog::LL_INFO;
		if (params.logLevel == L"FATAL")
			level = CRLog::LL_FATAL;
		else if (params.logLevel == L"ERROR")
			level = CRLog::LL_ERROR;
		else if (params.logLevel == L"DEBUG")
			level = CRLog::LL_DEBUG;
		else if (params.logLevel == L"TRACE")
			level = CRLog::LL_TRACE;
		CRLog::setFileLogger(toUtf8(params.logFile).c_str(), true);
		CRLog::setLogLevel(level);
	}
	params.dumpParams();
}
