#pragma once

#include <string>
#include <array>
#include <vector>
#include <stdint.h>
#include "logger.h"
#include "../../Include/SmartPtr.h"

typedef unsigned __int64 ulong;
// magic constant for non-specified MI request id
const uint64_t UNSPECIFIED_REQUEST_ID = 0xFFFFFFFFFFFFFFFEull;

/// MI interface command IDs
enum MiCommandId {
	CMD_UNKNOWN,
	CMD_GDB_EXIT, // -gdb-exit quit
	CMD_HELP, // help
	CMD_EXEC_RUN, // run -exec-run
	CMD_EXEC_CONTINUE, // continue -exec-continue
	CMD_EXEC_INTERRUPT, // interrupt -exec-interrupt
	CMD_EXEC_FINISH, // finish -exec-finish
	CMD_EXEC_NEXT, // next -exec-next
	CMD_EXEC_NEXT_INSTRUCTION, // nexti -exec-next-instruction
	CMD_EXEC_STEP, // step -exec-step
	CMD_EXEC_STEP_INSTRUCTION, // stepi -exec-step-instruction
	CMD_BREAK_INSERT, // break -break-insert
	CMD_BREAK_DELETE, // delete -break-delete
	CMD_BREAK_ENABLE, // enable -break-enable
	CMD_BREAK_DISABLE, // disable -break-disable
	CMD_BREAK_LIST, // -break-list
	CMD_LIST_THREAD_GROUPS, // info break -list-thread-groups
	CMD_THREAD_INFO, // info thread -thread-info
	CMD_THREAD_LIST_IDS, // info threads -thread-list-ids
	CMD_STACK_LIST_FRAMES, // -stack-list-frames backtrace
	CMD_STACK_INFO_DEPTH, // -stack-info-depth
	CMD_STACK_LIST_ARGUMENTS, // -stack-list-variables
	CMD_STACK_LIST_VARIABLES, // -stack-list-variables
	CMD_STACK_LIST_LOCALS, // -stack-list-locals
	CMD_VAR_CREATE, // -var-create
	CMD_VAR_UPDATE, // -var-update
	CMD_VAR_DELETE, // -var-delete
	CMD_VAR_SET_FORMAT, // -var-set-format
	CMD_LIST_FEATURES, // -list-features
	CMD_GDB_VERSION, // -gdb-version
	CMD_ENVIRONMENT_CD, // -environment-cd
	CMD_GDB_SHOW, // -gdb-show
	CMD_INTERPRETER_EXEC, // -interpreter-exec
	CMD_DATA_EVALUATE_EXPRESSION, // -data-evaluate-expression
	CMD_GDB_SET, // -gdb-set
	CMD_MAINTENANCE, // maintenance
	CMD_ENABLE_PRETTY_PRINTING, // -enable-pretty-printing
	CMD_SOURCE, // source
	CMD_FILE_EXEC_AND_SYMBOLS, // -file-exec-and-symbols
	CMD_HANDLE, // handle
};



#define DEFAULT_GUARD_TIMEOUT 50
#ifdef _DEBUG
#define DEBUG_LOCK_WAIT 1
#endif

class Mutex {
	HANDLE _mutex;
public:
	Mutex()
	{
		_mutex = CreateMutex(NULL, FALSE, NULL);
	}

	~Mutex()
	{
		CloseHandle(_mutex);
	}

	void Lock()
	{
		WaitForSingleObject(
			_mutex,    // handle to mutex
			INFINITE);  // no time-out interval
	}

	void Unlock()
	{
		ReleaseMutex(_mutex);
	}
};

class TimeCheckedGuardedArea
{
	Mutex&  mGuard;
#ifdef DEBUG_LOCK_WAIT
	const char * lockName;
	uint64_t warnTimeout;
	uint64_t startWaitLock;
	uint64_t endWaitLock;
	uint64_t endLock;
#endif
public:
	explicit TimeCheckedGuardedArea(Mutex& guard, const char * name, uint64_t timeout = DEFAULT_GUARD_TIMEOUT)
		: mGuard(guard)
#ifdef DEBUG_LOCK_WAIT
		, lockName(name), warnTimeout(timeout), startWaitLock(0), endWaitLock(0), endLock(0)
#endif
	{
		UNREFERENCED_PARAMETER(timeout);
		UNREFERENCED_PARAMETER(name);
#ifdef DEBUG_LOCK_WAIT
		startWaitLock = GetCurrentTimeMillis();
#endif
		mGuard.Lock();
#ifdef DEBUG_LOCK_WAIT
		endWaitLock = GetCurrentTimeMillis();
		if (endWaitLock - startWaitLock > warnTimeout)
			CRLog::warn("Lock wait timeout exceeded for guard %s: %llu ms", lockName, endWaitLock - startWaitLock);
#endif
	}

	~TimeCheckedGuardedArea()
	{
		mGuard.Unlock();
#ifdef DEBUG_LOCK_WAIT
		endLock = GetCurrentTimeMillis();
		if (endLock - endWaitLock > warnTimeout)
			CRLog::warn("Lock hold timeout exceeded for guard %s: %llu ms", lockName, endLock - endWaitLock);
#endif
	}

private:
	TimeCheckedGuardedArea& operator =(TimeCheckedGuardedArea& other) {
		UNREFERENCED_PARAMETER(other);
		return *this;
	}
};




template <typename T>
struct Buffer {
private:
	T * _buf;
	int _size;
	int _len;
public:
	Buffer() : _buf(NULL), _size(0), _len(0) {}
	~Buffer() {
		clear();
	}
	int length() const { return _len; }
	int size() const { return _size; }
	void reset() {
		_len = 0;
		if (_buf)
			_buf[_len] = 0;
	}
	void clear() {
		_len = 0;
		_size = 0;
		if (_buf) {
			free(_buf);
			_buf = NULL;
		}
	}
	T * ptr() { return _buf; }
	void reserve(int sz) {
		// ensure there is sz free items in buffer
		int newSize = _len + sz + 1;
		if (newSize > _size) {
			if (newSize < 64) {
				newSize = 64;
			} else {
				if (newSize < _size * 2)
					newSize = _size * 2;
			}
			_buf = (T*)realloc(_buf, newSize * sizeof(T));
			for (int i = _size; i < newSize; i++)
				_buf[i] = 0; // fill with zeroes
			_size = newSize;
		}
	}
	/// if length is greater than requested, remove extra items from end of buffer
	void truncate(int newLength) {
		if (newLength >= 0 && _len > newLength) {
			_len = newLength;
			if (_buf)
				_buf[_len] = 0;
		}
	}
	void append(T item) {
		reserve(1);
		_buf[_len++] = item;
		_buf[_len] = 0; // trailing zero, just in case
	}
	Buffer & operator += (T item) {
		append(item);
		return *this;
	}
	Buffer & operator += (const Buffer & items) {
		append(items.c_str(), items.length());
		return *this;
	}
	/// append z-string
	Buffer & operator += (const T * items) {
		append(items);
		return *this;
	}
	/// append several items
	void append(const T * items, size_t count) {
		if (count <= 0 || !items)
			return;
		reserve(count);
		for (unsigned i = 0; i < count; i++) {
			_buf[_len + i] = items[i];
		}
		_len += count;
		_buf[_len] = 0; // trailing zero, just in case
	}
	void append(const T * s) {
		int len = 0;
		for (; s[len]; len++)
			;
		append(s, len);
	}
	/// replace item
	void replace(T replaceWhat, T replaceWith) {
		for (int i = 0; i < _len; i++)
			if (_buf[i] == replaceWhat)
				_buf[i] = replaceWith;
	}
	void assign(const T * s, size_t count) {
		reset();
		append(s, count);
	}
	void assign(const T * s) {
		reset();
		append(s);
	}
	/// return item by index, or 0 if index out of bounds
	T operator[] (int index) { return (index >= 0 && index < _len) ? _buf[index] : 0; }
	const T * c_str() const {
		return _buf;
	}
	bool empty() const { return _length == 0; }
	bool isNull() const { return !_buf; }
	/// returns last item
	T last() const { return (_len > 0) ? _buf[_len - 1] : 0; }
	/// return true if last item equals to specified one
	bool endsWith(T item) const {
		return (last() == item);
	}
	void trimEol() {
		while (last() == '\n' || last() == '\r')
			truncate(_len - 1);
	}
	void appendIfLastItemNotEqual(T itemToAppend, T lastItemNotEqual) {
		if (last() != lastItemNotEqual)
			append(itemToAppend);
	}
	void appendIfLastItemNotEqual(const T * zitemsToAppend, T lastItemNotEqual) {
		if (last() != lastItemNotEqual)
			append(zitemsToAppend);
	}
};

std::string toUtf8(const std::wstring s);
std::wstring toUtf16(const std::string s);
#define toUtf8z(x) toUtf8(x).c_str()
#define toUtf16z(x) toUtf16(x).c_str()

struct StringBuffer : public Buffer<char> {
	StringBuffer & operator = (const std::string & s) { assign(s.c_str(), s.length()); return *this; }
	StringBuffer & operator += (char ch) { append(ch); return *this; }
	std::string str() { return std::string(c_str(), length()); }
	std::wstring wstr() { return toUtf16(str()); }
};


struct WstringBuffer : public Buffer<wchar_t> {
	WstringBuffer & pad(wchar_t ch, int len);
	WstringBuffer & operator = (const std::wstring & s) { assign(s.c_str(), s.length()); return *this; }
	WstringBuffer & operator += (const std::wstring & s) { append(s.c_str(), s.length()); return *this; }
	WstringBuffer & operator += (wchar_t ch) { append(ch); return *this; }
	WstringBuffer & appendUtf8(const char * s);
	std::string str() { return toUtf8(wstr()); }
	std::wstring wstr() { return std::wstring(c_str(), length()); }
	// appends double quoted string, e.g. "Some message.\n"
	WstringBuffer & appendStringLiteral(std::wstring s);
	// appends number
	WstringBuffer & appendUlongLiteral(uint64_t n);
	// appends number if non zero
	WstringBuffer & appendUlongIfNonEmpty(uint64_t n) { if (n != UNSPECIFIED_REQUEST_ID) appendUlongLiteral(n); return *this; }
	WstringBuffer & appendUlongParam(const wchar_t * paramName, uint64_t value, wchar_t appendCommaIfNotThisChar = '{') {
		if (last() != appendCommaIfNotThisChar)
			append(',');
		append(paramName);
		append('=');
		appendUlongLiteral(value);
		return *this;
	}
	WstringBuffer & appendUlongParamAsString(const wchar_t * paramName, uint64_t value, wchar_t appendCommaIfNotThisChar = '{') {
		if (last() != appendCommaIfNotThisChar)
			append(',');
		append(paramName);
		append('=');
		appendStringLiteral(std::to_wstring(value));
		return *this;
	}
	WstringBuffer & appendStringParam(const wchar_t * paramName, std::wstring value, wchar_t appendCommaIfNotThisChar = '{') {
		if (last() != appendCommaIfNotThisChar)
			append(',');
		append(paramName);
		append('=');
		appendStringLiteral(value);
		return *this;
	}
	WstringBuffer & appendStringParamIfNonEmpty(const wchar_t * paramName, std::wstring value, wchar_t appendCommaIfNotThisChar = '{') {
		if (!value.empty())
			appendStringParam(paramName, value, appendCommaIfNotThisChar);
		return *this;
	}
};

typedef std::vector<std::wstring> wstring_vector;
typedef std::pair<std::wstring, std::wstring> wstring_pair;
typedef std::vector<wstring_pair> param_vector;

// various parsing utility functions

/// trying to parse beginning of string as unsigned long; if found sequence of digits, trims beginning digits from s, puts parsed number into n, and returns true.
bool parseUlong(std::wstring & s, uint64_t &value);
/// parse whole string as ulong, return false if failed
bool toUlong(std::wstring s, uint64_t &value);
/// convert number to string
std::wstring toWstring(uint64_t n);
/// parse beginning of string as identifier, allowed chars: a..z, A..Z, _, - (if successful, removes ident from s and puts it to value, and returns true)
bool parseIdentifier(std::wstring & s, std::wstring & value);
// trim spaces and tabs from beginning of string
void skipWhiteSpace(std::wstring &s);
// split space separated parameters (properly handling spaces inside "double quotes")
void splitSpaceSeparatedParams(std::wstring s, wstring_vector & items);
// returns true if string is like -v -pvalue
bool isShortParamName(std::wstring & s);
// returns true if string is like --param --param=value
bool isLongParamName(std::wstring & s);
// returns true if string is like -v -pvalue --param --param=value
bool isParamName(std::wstring & s);
// split line into two parts (before,after) by specified character, returns true if character is found, otherwise s will be placed into before
bool splitByChar(std::wstring & s, wchar_t ch, std::wstring & before, std::wstring & after);
// split line into two parts (before,after) by specified character (search from end of string), returns true if character is found, otherwise s will be placed into before
bool splitByCharRev(std::wstring & s, wchar_t ch, std::wstring & before, std::wstring & after);

/// returns true if value ends with ending
inline bool endsWith(std::wstring const & value, std::wstring const & ending)
{
	if (ending.size() > value.size()) 
		return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

enum PrintLevel {
	PRINT_NO_VALUES = 0,
	PRINT_ALL_VALUES = 1,
	PRINT_SIMPLE_VALUES = 2,
};

void getCommandsHelp(wstring_vector & res, bool forMi);

struct MICommand {
	uint64_t requestId;
	MiCommandId commandId;
	std::wstring threadGroupId;
	unsigned threadId;
	unsigned frameId;
	PrintLevel printLevel;// 0=--no-values 1=--all-values 2=--simple-values
	bool skipUnavailable;//--skip-unavailable
	bool noFrameFilters; //--no-frame-filters
	/// true if command is prefixed with single -
	bool miCommand;
	/// original command text
	std::wstring commandText;
	/// command name string
	std::wstring commandName;
	/// tail after command till end of line
	std::wstring tail;
	/// individual parameters from tail
	wstring_vector params;
	/// named parameters - pairs (key, value)
	param_vector namedParams;
	/// parameters with values w/o names
	wstring_vector unnamedValues;

	// debug dump
	std::wstring dumpCommand();

	/// returns true if there is specified named parameter in cmd
	bool hasParam(std::wstring name);
	// find parameter by name
	std::wstring findParam(std::wstring name);
	std::wstring unnamedValue(unsigned index = 0) { return index < unnamedValues.size() ? unnamedValues[index] : std::wstring(); }
	// get parameter --thread-id
	uint64_t getUlongParam(std::wstring name, uint64_t defValue = 0);

	MICommand();
	~MICommand();
	// parse MI command, returns true if successful
	bool parse(std::wstring line);
private:
	void handleKnownParams();
};


class RefCountedBase {
private:
	int refCount;
public:
	RefCountedBase() : refCount(0) {}
	virtual ~RefCountedBase() {}
	void AddRef() {
		refCount++;
	}
	void Release() {
		if (--refCount <= 0)
			delete this;
	}
};

struct IDebugBoundBreakpoint2;
struct IDebugPendingBreakpoint2;
class BreakpointInfo : public RefCountedBase {
private:
	IDebugPendingBreakpoint2 * _pendingBreakpoint;
	IDebugBoundBreakpoint2 * _boundBreakpoint;
public:
	uint64_t id;
	uint64_t requestId;
	std::wstring insertCommandText;
	std::wstring address;
	std::wstring functionName;
	std::wstring fileName;
	std::wstring labelName;
	std::wstring moduleName;
	std::wstring originalLocation;
	int line;
	int boundLine;
	int times;
	bool enabled;
	bool pending;
	bool temporary;
	bool bound;
	bool error;
	std::wstring errorMessage;
	BreakpointInfo();
	virtual ~BreakpointInfo();

	uint64_t assignId();

	BreakpointInfo & operator = (const BreakpointInfo & v);
	bool fromCommand(MICommand & cmd);
	bool validateParameters();
	// debug dump
	std::wstring dumpParams();
	/// print mi2 breakpoint info
	void printBreakpointInfo(WstringBuffer & buf);
	/// request binding, return true if request is sent ok
	bool bind();

	void setPending(IDebugPendingBreakpoint2 * pPendingBp);
	void setBound(IDebugBoundBreakpoint2 * pBoundBp);
	void setBindError();
	IDebugPendingBreakpoint2 * getPendingBreakpoint() { return _pendingBreakpoint; }
	IDebugBoundBreakpoint2 * getBoundBreakpoint() { return _boundBreakpoint; }

};

typedef RefPtr<BreakpointInfo> BreakpointInfoRef;
class BreakpointInfoList : public std::vector<BreakpointInfoRef> {
public:
	BreakpointInfoList() {}
	~BreakpointInfoList() {}
	BreakpointInfoRef findById(uint64_t id);
	BreakpointInfoRef findByPendingBreakpoint(IDebugPendingBreakpoint2 * bp);
	BreakpointInfoRef findByBoundBreakpoint(IDebugBoundBreakpoint2 * bp);
	bool addItem(BreakpointInfoRef & bp) { push_back(bp); return true;  }
	bool removeItem(BreakpointInfoRef & bp);
};

// file related helper functions

bool fileExists(std::wstring fname);
std::wstring unquoteString(std::wstring s);
std::wstring quoteString(std::wstring s);
std::wstring fixPathDelimiters(std::wstring s);
std::wstring relativeToAbsolutePath(std::wstring s);
bool isAbsolutePath(std::wstring s);
std::wstring getCurrentDirectory();
/// get base name for file name, e.g. for "/dir/subdir/file.ext" return "file.ext"
std::wstring getBaseName(std::wstring fname);
/// get directory name for file, e.g. for "/dir/subdir/file.ext" return "/dir/subdir"
std::wstring getDirName(std::wstring fname);

void testEngine();


enum LocalVariableKind {
	VAR_KIND_UNKNOWN, // unknown type
	VAR_KIND_THIS, // this pointer
	VAR_KIND_ARG, // function argument
	VAR_KIND_LOCAL, // stack variable
};

/// helper function converts BSTR string to std::wstring and frees original string
std::wstring fromBSTR(BSTR & bstr);

class LocalVariableInfo : public RefCountedBase {
private:
public:
	LocalVariableKind varKind;
	std::wstring varFullName;
	std::wstring varName;
	std::wstring varType;
	std::wstring varValue;
	bool expandable;
	bool readonly;
	void dumpMiVariable(WstringBuffer & buf, bool includeTypes, bool includeValues);
	LocalVariableInfo() : varKind(VAR_KIND_UNKNOWN), expandable(false), readonly(false) {}
	virtual ~LocalVariableInfo() {}
};

typedef RefPtr<LocalVariableInfo> LocalVariableInfoRef;
class LocalVariableList : public std::vector<LocalVariableInfoRef> {
public:
	LocalVariableList() {}
	~LocalVariableList() {}
};

struct StackFrameInfo {
	DWORD threadId;
	int frameIndex;
	std::wstring address;
	std::wstring moduleName;
	std::wstring functionName;
	std::wstring sourceFileName;
	std::wstring sourceBaseName;
	int sourceLine;
	int sourceColumn;
	LocalVariableList args;
	StackFrameInfo()
		: threadId(0)
		, frameIndex(0)
		, sourceLine(0)
		, sourceColumn(0)
	{

	}
	void dumpMIFrame(WstringBuffer & buf, bool showLevel = false);
};

typedef std::vector<StackFrameInfo> StackFrameInfoVector;


class VariableObject : public RefCountedBase {
private:
public:
	std::wstring name;
	std::wstring frame;
	std::wstring expr;
	std::wstring type;
	std::wstring value;
	bool inScope;
	void dumpVariableInfo(WstringBuffer & buf, bool forUpdate);
	VariableObject() : inScope(true) {}
	virtual ~VariableObject() {}
};

typedef RefPtr<VariableObject> VariableObjectRef;
class VariableObjectList : public std::vector<VariableObjectRef> {
public:
	VariableObjectList() {}
	~VariableObjectList() {}
	VariableObjectRef find(std::wstring frameAddress, std::wstring expr, int * pvarIndex = NULL);
	VariableObjectRef find(std::wstring name, int * pvarIndex = NULL);
};
