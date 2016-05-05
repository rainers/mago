
// Command line arguments parser
extern "C" void parseCommandLine(int argc, wchar_t *argv[]);

#define MAX_PARAM_COUNT 10000
struct ExecutableInfo {
	const wchar_t * exename;
	int paramCount;
	const wchar_t * params[MAX_PARAM_COUNT];
	const wchar_t * dir;

	ExecutableInfo();
	~ExecutableInfo();
	void setExecutable(const wchar_t * exe);
	void setDir(const wchar_t * directory);
	void addParam(const wchar_t * param);
	void dumpParams();
};

extern ExecutableInfo executableInfo;

bool fileExists(const wchar_t * fname);
const wchar_t * dupAndUnquoteString(const wchar_t * s);
const wchar_t * relativeToAbsolutePath(const wchar_t * s);
bool isAbsolutePath(const wchar_t * s);
void fixPathDelimiters(wchar_t * s);
const wchar_t * getCurrentDirectory();
