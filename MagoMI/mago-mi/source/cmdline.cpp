#include "cmdline.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

ExecutableInfo executableInfo;
ExecutableInfo::ExecutableInfo() : exename(NULL), paramCount(0) {

}
ExecutableInfo::~ExecutableInfo() {
	if (exename)
		free((void*)exename);
	exename = NULL;
	for (int i = 0; i < paramCount; i++) {
		free((void*)params[i]);
	}
	paramCount = 0;
}

const char * dupAndUnquoteString(const char * s) {
	if (!s || !s[0])
		return NULL;
	if (s[0] == '\"') {
		s++;
		char * res = _strdup(s);
		if (res[strlen(res) - 1] == '\"')
			res[strlen(res) - 1] = '\0';
		return res;
	}
	return _strdup(s);
}

void ExecutableInfo::setExecutable(const char * exe) {
	if (!exe) {
		exename = (const char*)NULL;
		return;
	}
	exename = dupAndUnquoteString(exe);

}

void ExecutableInfo::addParam(const char * param) {
	if (paramCount >= MAX_PARAM_COUNT) {
		fprintf(stderr, "Too many executable file parameters");
		exit(3);
	}
	params[paramCount++] = _strdup(param);
}

void ExecutableInfo::dumpParams() {
	if (exename) {
		printf("Executable file: %s\n", exename);
		if (paramCount) {
			printf("Inferior arguments: ");
			for (int i = 0; i < paramCount; i++) {
				printf("%s ", params[i]);
			}
			printf("\n");
		}
	}
}

bool fileExists(const char * fname) {
	FILE * f = fopen(fname, "rb");
	if (!f)
		return false;
	fclose(f);
	return true;
}

enum CmdLineParamType {
	NO_PARAMS,
	STRING_PARAM,
};

struct CmdLineParamDef;
typedef void(*parameterHandler)(CmdLineParamDef * param, const char * value);

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

void showHelp(CmdLineParamDef * param, const char * value) {
	printf("This is Mago debugger. Usage:\n");
	printf("    mago-mi [options] [executable-file]\n");
	printf("    mago-mi [options] --args executable-file [inferior-arguments ...]\n");
	dumpParameterHelp();
	exit(0);
}

void showVersion(CmdLineParamDef * param, const char * value) {
	printf("mago-mi debugger v0.1\n");
	exit(0);
}

void defParamHandler(CmdLineParamDef * param, const char * value) {
	// TODO
}

void handleInterpreter(CmdLineParamDef * param, const char * value) {
	if (strcmp(value, "mi2")) {
		fprintf(stderr, "Only mi2 interpreter is supported");
		exit(2);
	}
}

static bool argsFound = false;

void handleArgs(CmdLineParamDef * param, const char * value) {
	if (executableInfo.exename) {
		fprintf(stderr, "Executable file already specified");
		exit(3);
	}
	executableInfo.setExecutable(value);
	argsFound = true;
}

void handleExec(CmdLineParamDef * param, const char * value) {
	if (executableInfo.exename) {
		fprintf(stderr, "Executable file already specified");
		exit(3);
	}
	executableInfo.setExecutable(value);
}

void nonParamHandler(const char * value) {
	if (!executableInfo.exename) {
		executableInfo.setExecutable(value);
		return;
	}
	if (!argsFound) {
		fprintf(stderr, "Use --args to provide inferior arguments");
		exit(3);
	}
	executableInfo.addParam(value);
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

CmdLineParamDef * findParam(const char * &name) {
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
				name += j + 1;
				return &params[i];
			}
		}
	}
	return NULL;
}

void parseCommandLine(int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		char * v = argv[i];
		if (v[0] == '-') {
			const char * value = v;
			CmdLineParamDef * param = findParam(value);
			if (!param) {
				fprintf(stderr, "Unknown command line parameter %s", v);
				exit(1);
			}
			if (param->paramType != NO_PARAMS) {
				if (!value[0]) {
					if (i == argc - 1) {
						fprintf(stderr, "Value not specified for parameter %s", v);
						exit(1);
					}
					i++;
					value = argv[i];
				}
			}
			else {
				if (value[0]) {
					fprintf(stderr, "Value not allowed for parameter %s", v);
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
