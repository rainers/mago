#include <string>

// Command line arguments parser
extern "C" void parseCommandLine(int argc, wchar_t *argv[]);

#define MAX_PARAM_COUNT 10000
struct ExecutableInfo {
	std::wstring exename;
	int paramCount;
	std::wstring  params[MAX_PARAM_COUNT];
	std::wstring dir;

	ExecutableInfo();
	~ExecutableInfo();
	void clear();
	void setExecutable(std::wstring exe);
	void setDir(std::wstring directory);
	void addParam(std::wstring param);
	void dumpParams();
};

extern ExecutableInfo executableInfo;

