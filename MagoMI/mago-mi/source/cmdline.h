#include <string>

#define EMULATED_GDB_VERSION L"7.1.90"
#define MAGO_MI_VERSION L"0.3.1"
#define VERSION_STRING L"GNU gdb (mago-mi " MAGO_MI_VERSION L") " EMULATED_GDB_VERSION
#define VERSION_EXPLANATION_STRING L"(Actually it's mago-mi debugger. Version shows GDB for Eclipse CDT compatibility)"

// Command line arguments parser
extern "C" void parseCommandLine(int argc, wchar_t *argv[]);

#define MAX_PARAM_COUNT 10000
struct ExecutableInfo {
	std::wstring exename;
	wstring_vector args;
	std::wstring dir;
	std::wstring logFile;
	std::wstring logLevel;
	std::wstring tty;
	int argCount() { return (int)args.size(); }
	bool verbose;
	bool miMode;
	bool silent;
	bool stopOnEntry;

	ExecutableInfo();
	~ExecutableInfo();
	void clear();
	void setExecutable(std::wstring exe);
	void setDir(std::wstring directory);
	void addArg(std::wstring param);
	void setTty(std::wstring tty);
	void dumpParams();

	bool hasExecutableSpecified();
};

extern ExecutableInfo params;

