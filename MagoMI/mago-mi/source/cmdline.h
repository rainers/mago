
// Command line arguments parser
extern "C" void parseCommandLine(int argc, char *argv[]);

#define MAX_PARAM_COUNT 10000
struct ExecutableInfo {
	const char * exename;
	int paramCount;
	const char * params[MAX_PARAM_COUNT];

	ExecutableInfo();
	~ExecutableInfo();
	void setExecutable(const char * exe);
	void addParam(const char * param);
	void dumpParams();
};

extern ExecutableInfo executableInfo;

bool fileExists(const char * fname);
