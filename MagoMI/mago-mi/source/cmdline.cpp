#include "cmdline.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "miutils.h"

// global object
ExecutableInfo params;

ExecutableInfo::ExecutableInfo() 
	: argCount(0)
	, verbose(false) 
	, miMode(false)
{
	setDir(getCurrentDirectory());
}

void ExecutableInfo::clear() {
	exename.clear();
	dir.clear();
	for (int i = 0; i < argCount; i++) {
		args[i].clear();
	}
	argCount = 0;
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
	std::wstring tmp = unquoteString(exe);
	exename = relativeToAbsolutePath(tmp);
}

void ExecutableInfo::setDir(std::wstring directory) {
	if (directory.empty()) {
		dir.clear();
		return;
	}
	std::wstring tmp = unquoteString(directory);
	dir = relativeToAbsolutePath(tmp);
}

void ExecutableInfo::addArg(std::wstring param) {
	if (argCount >= MAX_PARAM_COUNT) {
		fprintf(stderr, "Too many executable file parameters");
		exit(3);
	}
	args[argCount++] = param;
}

void ExecutableInfo::dumpParams() {
	if (!exename.empty()) {
		wprintf(L"Executable file: %s\n", exename.c_str());
		if (argCount) {
			wprintf(L"Inferior arguments: ");
			for (int i = 0; i < argCount; i++) {
				wprintf(L"%s ", args[i].c_str());
			}
			wprintf(L"\n");
		}
	}
	if (!dir.empty())
		wprintf(L"Directory: %s\n", dir.c_str());
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
	printf("mago-mi debugger v0.1\n");
	exit(0);
}

void defParamHandler(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	UNREFERENCED_PARAMETER(value);
	// TODO
}

static void handleInterpreter(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	if (wcscmp(value, L"mi2")) {
		fprintf(stderr, "Only mi2 interpreter is supported");
		exit(2);
	}
	params.miMode = true;
}

static bool argsFound = false;

static void handleArgs(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	if (params.hasExecutableSpecified()) {
		fprintf(stderr, "Executable file already specified");
		exit(3);
	}
	params.setExecutable(value);
	argsFound = true;
}

static void handleExec(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	if (params.hasExecutableSpecified()) {
		fprintf(stderr, "Executable file already specified");
		exit(3);
	}
	params.setExecutable(std::wstring(value));
}

static void handleDir(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	params.setDir(value);
}

static void handleVerbose(CmdLineParamDef * param, const wchar_t * value) {
	UNREFERENCED_PARAMETER(param);
	UNREFERENCED_PARAMETER(value);
	params.verbose = true;
}

void nonParamHandler(const wchar_t * value) {
	if (params.exename.empty()) {
		params.setExecutable(std::wstring(value));
		return;
	}
	if (!argsFound) {
		fprintf(stderr, "Use --args to provide inferior arguments");
		exit(3);
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
	CmdLineParamDef("Other options"),
	CmdLineParamDef(NULL, "--cd=DIR", STRING_PARAM, "Change current directory to DIR.", NULL, &handleDir),
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
				fwprintf(stderr, L"Unknown command line parameter %s", v);
				exit(1);
			}
			if (param->paramType != NO_PARAMS) {
				if (!value[0]) {
					if (i == argc - 1) {
						fwprintf(stderr, L"Value not specified for parameter %s", v);
						exit(1);
					}
					i++;
					value = argv[i];
				}
			}
			else {
				if (value[0]) {
					fwprintf(stderr, L"Value not allowed for parameter %s", v);
					exit(1);
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
}
