#pragma once

#include <string>
#include <array>
#include <vector>
#include <stdint.h>
#include "logger.h"

typedef unsigned __int64 ulong;
// magic constant for non-specified MI request id
const uint64_t UNSPECIFIED_REQUEST_ID = 0xFFFFFFFFFFFFFFFEull;

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

struct StringBuffer : public Buffer<char> {
	StringBuffer & operator = (const std::string & s) { assign(s.c_str(), s.length()); return *this; }
	StringBuffer & operator += (char ch) { append(ch); return *this; }
	std::string str() { return std::string(c_str(), length()); }
	std::wstring wstr() { return toUtf16(str()); }
};


struct WstringBuffer : public Buffer<wchar_t> {
	WstringBuffer & operator = (const std::wstring & s) { assign(s.c_str(), s.length()); return *this; }
	WstringBuffer & operator += (const std::wstring & s) { append(s.c_str(), s.length()); return *this; }
	WstringBuffer & operator += (wchar_t ch) { append(ch); return *this; }
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

/// returns true if value ends with ending
inline bool endsWith(std::wstring const & value, std::wstring const & ending)
{
	if (ending.size() > value.size()) 
		return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

struct MICommand {
	uint64_t requestId;
	/// true if command is prefixed with single -
	bool miCommand;
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

	MICommand();
	~MICommand();
	// parse MI command, returns true if successful
	bool parse(std::wstring line);
};

struct BreakpointInfo {
	uint64_t id;
	uint64_t requestId;
	std::wstring address;
	std::wstring functionName;
	std::wstring fileName;
	std::wstring labelName;
	int line;
	bool enabled;
	bool pending;
	bool temporary;
	BreakpointInfo();
	~BreakpointInfo() {}

	BreakpointInfo & operator = (const BreakpointInfo & v);
	bool fromCommand(MICommand & cmd);
	bool parametersAreSet();
	// debug dump
	std::wstring dumpParams();
};

// file related helper functions

bool fileExists(std::wstring fname);
std::wstring unquoteString(std::wstring s);
std::wstring relativeToAbsolutePath(std::wstring s);
bool isAbsolutePath(std::wstring s);
std::wstring getCurrentDirectory();

void testEngine();


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
	StackFrameInfo()
		: threadId(0)
		, frameIndex(0)
		, sourceLine(0)
		, sourceColumn(0)
	{

	}
	void dumpMIFrame(WstringBuffer & buf);
};

typedef std::vector<StackFrameInfo> StackFrameInfoVector;

