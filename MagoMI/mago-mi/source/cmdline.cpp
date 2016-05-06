#include "cmdline.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "miutils.h"

ExecutableInfo executableInfo;
ExecutableInfo::ExecutableInfo() : paramCount(0) {
	setDir(getCurrentDirectory());
}

void ExecutableInfo::clear() {
	exename.clear();
	dir.clear();
	for (int i = 0; i < paramCount; i++) {
		params[i].clear();
	}
	paramCount = 0;
}

ExecutableInfo::~ExecutableInfo() {
	clear();
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

void ExecutableInfo::addParam(std::wstring param) {
	if (paramCount >= MAX_PARAM_COUNT) {
		fprintf(stderr, "Too many executable file parameters");
		exit(3);
	}
	params[paramCount++] = param;
}

void ExecutableInfo::dumpParams() {
	if (!exename.empty()) {
		wprintf(L"Executable file: %s\n", exename.c_str());
		if (paramCount) {
			wprintf(L"Inferior arguments: ");
			for (int i = 0; i < paramCount; i++) {
				wprintf(L"%s ", params[i].c_str());
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

void showHelp(CmdLineParamDef * param, const wchar_t * value) {
	printf("This is Mago debugger. Usage:\n");
	printf("    mago-mi [options] [executable-file]\n");
	printf("    mago-mi [options] --args executable-file [inferior-arguments ...]\n");
	dumpParameterHelp();
	exit(0);
}

void showVersion(CmdLineParamDef * param, const wchar_t * value) {
	printf("mago-mi debugger v0.1\n");
	exit(0);
}

void defParamHandler(CmdLineParamDef * param, const wchar_t * value) {
	// TODO
}

void handleInterpreter(CmdLineParamDef * param, const wchar_t * value) {
	if (wcscmp(value, L"mi2")) {
		fprintf(stderr, "Only mi2 interpreter is supported");
		exit(2);
	}
}

static bool argsFound = false;

void handleArgs(CmdLineParamDef * param, const wchar_t * value) {
	if (!executableInfo.exename.empty()) {
		fprintf(stderr, "Executable file already specified");
		exit(3);
	}
	executableInfo.setExecutable(value);
	argsFound = true;
}

void handleExec(CmdLineParamDef * param, const wchar_t * value) {
	if (!executableInfo.exename.empty()) {
		fprintf(stderr, "Executable file already specified");
		exit(3);
	}
	executableInfo.setExecutable(std::wstring(value));
}

void handleDir(CmdLineParamDef * param, const wchar_t * value) {
	executableInfo.setDir(value);
}

void nonParamHandler(const wchar_t * value) {
	if (executableInfo.exename.empty()) {
		executableInfo.setExecutable(std::wstring(value));
		return;
	}
	if (!argsFound) {
		fprintf(stderr, "Use --args to provide inferior arguments");
		exit(3);
	}
	executableInfo.addParam(std::wstring(value));
}

CmdLineParamDef params[] = {
	CmdLineParamDef("Selection of debuggee and its files"),
	CmdLineParamDef(NULL, "--args", STRING_PARAM, "Arguments after executable-file are passed to inferior", NULL, &handleArgs),
	CmdLineParamDef(NULL, "--exec=EXECFILE", STRING_PARAM, "Use EXECFILE as the executable.", NULL, &handleExec),
	CmdLineParamDef("Output and user interface control"),
	CmdLineParamDef(NULL, "--interpreter=INTERP", STRING_PARAM, "Print this message and then exit", NULL, &handleInterpreter),
	CmdLineParamDef("Operating modes"),
	CmdLineParamDef(NULL, "--help", NO_PARAMS, "Print this message and then exit", NULL, &showHelp),
	CmdLineParamDef(NULL, "--version", NO_PARAMS, "Print version information and then exit", NULL, &showVersion),
	CmdLineParamDef("Other options"),
	CmdLineParamDef(NULL, "--cd=DIR", STRING_PARAM, "Change current directory to DIR.", NULL, &handleDir),
	CmdLineParamDef()
};

void dumpParameterHelp() {
	for (int i = 0; !params[i].isLast(); i++) {
		if (params[i].isHelpSection()) {
			printf("\n%s:\n\n", params[i].description);
		} else {
			if (params[i].longName)
				printf("  %-16s %s\n", params[i].longName, params[i].description);
			if (params[i].shortName)
				printf("  %-16s %s\n", params[i].shortName, params[i].description);
		}
	}
}

CmdLineParamDef * findParam(const wchar_t * &name) {
	if (name[0] == '-' && name[1] != '-') {
		for (int i = 0; !params[i].isLast(); i++) {
			if (params[i].isHelpSection())
				continue;
			if (params[i].shortName[1] == name[1]) {
				name += 2;
				return &params[i];
			}
		}
	} else if (name[0] == '-' && name[1] == '-') {
		for (int i = 0; !params[i].isLast(); i++) {
			if (params[i].isHelpSection())
				continue;
			int j = 0;
			for (; name[j] && params[i].longName[j] && (params[i].longName[j] != '=') && name[j] == params[i].longName[j]; j++)
				;
			if ((!params[i].longName[j] || params[i].longName[j] == '=') && (!name[j] || name[j] == '=')) {
				name += j;
				if (name[0] == '=')
					name++;
				return &params[i];
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
