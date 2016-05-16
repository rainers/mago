#include <string>

// Command line arguments parser
extern "C" void parseCommandLine(int argc, wchar_t *argv[]);

#define MAX_PARAM_COUNT 10000
struct ExecutableInfo {
	std::wstring exename;
	int argCount;
	std::wstring  args[MAX_PARAM_COUNT];
	std::wstring dir;
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
	void dumpParams();

	bool hasExecutableSpecified();
};

extern ExecutableInfo params;

