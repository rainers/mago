#include <windows.h>
#include "miutils.h"
#include "MIEngine.h"
#include "../../DebugEngine/MagoNatDE/PendingBreakpoint.h"
//#include "micommand.h"

//const char * toUtf8z(const std::wstring s) { 
//	return toUtf8(s).c_str(); 
//}
//const wchar_t * toUtf16z(const std::string s) { 
//	return toUtf16(s).c_str(); 
//}

std::string toUtf8(const std::wstring s) {
	StringBuffer buf;
	for (unsigned i = 0; i < s.length(); i++) {
		unsigned ch = s[i];
		if (!(ch & ~0x7F)) {
			buf.append((unsigned char)ch);
		}
		else if (!(ch & ~0x7FF)) {
			buf.append((unsigned char)(((ch >> 6) & 0x1F) | 0xC0));
			buf.append((unsigned char)(((ch)& 0x3F) | 0x80));
		}
		else if (!(ch & ~0xFFFF)) {
			buf.append((unsigned char)(((ch >> 12) & 0x0F) | 0xE0));
			buf.append((unsigned char)(((ch >> 6) & 0x3F) | 0x80));
			buf.append((unsigned char)(((ch)& 0x3F) | 0x80));
		}
		else if (!(ch & ~0x1FFFFF)) {
			buf.append((unsigned char)(((ch >> 18) & 0x07) | 0xF0));
			buf.append((unsigned char)(((ch >> 12) & 0x3F) | 0x80));
			buf.append((unsigned char)(((ch >> 6) & 0x3F) | 0x80));
			buf.append((unsigned char)(((ch)& 0x3F) | 0x80));
		}
		else {
			buf.append((unsigned char)(((ch >> 24) & 0x03) | 0xF8));
			buf.append((unsigned char)(((ch >> 18) & 0x3F) | 0x80));
			buf.append((unsigned char)(((ch >> 12) & 0x3F) | 0x80));
			buf.append((unsigned char)(((ch >> 6) & 0x3F) | 0x80));
			buf.append((unsigned char)(((ch)& 0x3F) | 0x80));
		}
	}
	return buf.str();
}

std::wstring toUtf16(const std::string s) {
	WstringBuffer buf;
	for (unsigned i = 0; i < s.length(); i++) {
		wchar_t ch = 0;
		unsigned int ch0 = (unsigned int)s[i];
		unsigned int ch1 = (unsigned int)((i + 1 < s.length()) ? s[i + 1] : 0);
		unsigned int ch2 = (unsigned int)((i + 2 < s.length()) ? s[i + 2] : 0);
		unsigned int ch3 = (unsigned int)((i + 3 < s.length()) ? s[i + 3] : 0);
		unsigned int ch4 = (unsigned int)((i + 4 < s.length()) ? s[i + 4] : 0);
		unsigned int ch5 = (unsigned int)((i + 5 < s.length()) ? s[i + 5] : 0);

		if (!(ch0 & 0x80)) {
			// 0x00..0x7F single byte
			// 0x80 == 10000000
			// !(ch0 & 0x80) => ch0 < 10000000
			ch = (wchar_t)ch0;
		}
		else if ((ch0 & 0xE0) == 0xC0) {
			// two bytes 110xxxxx 10xxxxxx
			ch = (wchar_t)(((ch0 & 0x1F) << 6) | (ch1 & 0x3F));
		}
		else if ((ch0 & 0xF0) == 0xE0) {
			// three bytes 1110xxxx 10xxxxxx 10xxxxxx
			ch = (wchar_t)(((ch0 & 0x0F) << 12) | ((ch1 & 0x3F) << 6) | (ch2 & 0x3F));
		}
		else if ((ch0 & 0xF8) == 0xF0) {
			// four bytes 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
			ch = (wchar_t)(((ch0 & 0x07) << 18) | ((ch1 & 0x3F) << 12) | ((ch2 & 0x3F) << 6) | (ch3 & 0x3F));
		}
		else if ((ch0 & 0xFC) == 0xF8) {
			// five bytes 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
			ch = (wchar_t)(((ch0 & 0x03) << 24) | ((ch1 & 0x3F) << 18) | ((ch2 & 0x3F) << 12) | ((ch3 & 0x3F) << 6) | (ch4 & 0x3F));
		}
		else if ((ch0 & 0xFE) == 0xFC) {
			// six bytes 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
			ch = (wchar_t)(((ch0 & 0x01) << 30) | ((ch1 & 0x3F) << 24) | ((ch2 & 0x3F) << 18) | ((ch3 & 0x3F) << 12) | ((ch4 & 0x3F) << 6) | (ch5 & 0x3F));
		}
		buf.append(ch);
	}
	return buf.wstr();
}

WstringBuffer & WstringBuffer::appendUtf8(const char * s) {
	while (s && s[0]) {
		append(s[0]);
		s++;
	}
	return *this;
}

WstringBuffer & WstringBuffer::pad(wchar_t ch, int len) {
	while (length() < len)
		append(ch);
	return *this;
}

// appends number
WstringBuffer & WstringBuffer::appendUlongLiteral(uint64_t n) {
	wchar_t buf[32];
	wsprintf(buf, L"%I64d", n);
	append(buf);
	return *this;
}

/// append command line parameter, quote if if needed
WstringBuffer & WstringBuffer::appendCommandLineParameter(std::wstring s) {
	if (s.empty())
		return *this;
	if (last() != 0 && last() != ' ')
		append(L" ");
	bool needQuotes = false;
	for (unsigned i = 0; i < s.length(); i++)
		if (s[i] == ' ')
			needQuotes = true;
	if (needQuotes)
		append(L"\"");
	for (unsigned i = 0; i < s.length(); i++) {
		wchar_t ch = s[i];
		if (ch == '\"')
			append(L"\"");
		else if (ch == '\n')
			append(L"\\n");
		else
			append(ch);
	}
	if (needQuotes)
		append(L"\"");
	return *this;
}

WstringBuffer & WstringBuffer::appendStringLiteral(std::wstring s) {
	append('\"');
	for (size_t i = 0; i < s.length(); i++) {
		wchar_t ch = s[i];
		switch (ch) {
		case '\\':
			append(L"\\\\");
			break;
		case '\"':
			append(L"\\\"");
			break;
		case '\n':
			append(L"\\n");
			break;
		case '\t':
			append(L"\\t");
			break;
		case '\r':
			append(L"\\r");
			break;
		case '\0':
			append(L"\\0");
			break;
		default:
			append(ch);
			break;
		}
	}
	append('\"');
	return *this;
}

/// helper function converts BSTR string to std::wstring and frees original string
std::wstring fromBSTR(BSTR & bstr) {
	if (!bstr)
		return std::wstring();
	std::wstring res = bstr;
	SysFreeString(bstr);
	bstr = NULL;
	return res;
}

void StackFrameInfo::dumpMIFrame(WstringBuffer & buf, bool showLevel) {
	buf.append('{');
	if (showLevel) {
		buf.appendUlongParamAsString(L"level", frameIndex);
	}
	buf.appendStringParamIfNonEmpty(L"addr", address, '{');
	if (!functionName.empty()) {
		buf.appendStringParamIfNonEmpty(L"func", functionName, '{');
		buf.append(L",args=[]"); //{name=\"a\",value=\"5\"}
		//buf.appendStringParamIfNonEmpty(L"args", std::wstring(L"[]"), '{'); // TODO
	}
	buf.appendStringParamIfNonEmpty(L"file", sourceBaseName, '{');
	buf.appendStringParamIfNonEmpty(L"fullname", sourceFileName, '{');
	if (sourceLine != 0)
		buf.appendUlongParamAsString(L"line", sourceLine, '{');
	buf.appendStringParamIfNonEmpty(L"from", moduleName, '{');
	buf.append('}');
}

/// convert number to string
std::wstring toWstring(uint64_t n) {
	WstringBuffer buf;
	buf.appendUlongLiteral(n);
	return buf.wstr();
}

/// parse whole string as ulong, return false if failed
bool toUlong(std::wstring s, uint64_t &value) {
	if (!parseUlong(s, value))
		return false;
	return s.empty();
}

/// trying to parse beginning of string as unsigned long; if found sequence of digits, trims beginning digits from s, puts parsed number into n, and returns true.
bool parseUlong(std::wstring & s, uint64_t &value) {
	if (s.empty())
		return false;
	uint64_t n = 0;
	size_t i = 0;
	for (; i < s.length() && s[i] >= '0' && s[i] <= '9'; i++)
		n = n * 10 + (unsigned)(s[i] - '0');
	if (i == 0)
		return false;
	value = n;
	s = s.substr(i, s.length() - i);
	return true;
}

bool isValidIdentChar(wchar_t ch) {
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_') || (ch == '-');
}

/// parse beginning of string as identifier, allowed chars: a..z, A..Z, _, - (if successful, removes ident from s and puts it to value, and returns true)
bool parseIdentifier(std::wstring & s, std::wstring & value) {
	if (s.empty())
		return false;
	size_t i = 0;
	bool foundLetters = false;
	for (; i < s.length() && isValidIdentChar(s[i]); i++) {
		if (s[i] != '-' && s[i] != '_')
			foundLetters = true;
	}
	if (!i || !foundLetters)
		return false;
	value = s.substr(0, i);
	s = s.substr(i, s.length() - i);
	return true;
}

// trim spaces and tabs from beginning of string
void skipWhiteSpace(std::wstring &s) {
	size_t i = 0;
	for (; i < s.length() && (s[i] == ' ' || s[i] == '\t'); i++) {
	}
	if (i > 0) {
		if (i < s.length())
			s = s.substr(i, s.length() - i);
		else
			s = std::wstring();
	}
}

std::wstring processQuotedChars(std::wstring s) {
	WstringBuffer buf;
	for (unsigned i = 0; i < s.length(); i++) {
		wchar_t ch = s[i];
		wchar_t nextch = i + 1 < s.length() ? s[i + 1] : 0;
		if (ch == '\\' && nextch == '\\') {
			buf.append('\\');
			i++;
		}
		else if (ch == '\\' && nextch == 't') {
			buf.append('\\');
			i++;
		}
		else {
			buf.append(ch);
		}
	}
	return buf.wstr();
}

// split space separated parameters (properly handling spaces inside "double quotes")
void splitSpaceSeparatedParams(std::wstring s, wstring_vector & items) {
	size_t start = 0;
	size_t i = 0;
	bool insideStringLiteral = false;
	for (; i < s.length(); i++) {
		wchar_t ch = s[i];
		wchar_t nextch = i + 1 < s.length() ? s[i + 1] : 0;
		if ((ch == ' ' || ch == '\t') && !insideStringLiteral) {
			if (i > start)
				items.push_back(processQuotedChars(s.substr(start, i - start)));
			start = i + 1;
		}
		else if (ch == '\"') {
			insideStringLiteral = !insideStringLiteral;
		}
		else if (insideStringLiteral && ch == '\\' && nextch == '\"') {
			i++;
		}
		else if (ch == '\\' && nextch == '\\') {
			// convert double backslashes to single backslashes (CDT support)
			i++;
		}
	}
	if (i > start)
		items.push_back(processQuotedChars(s.substr(start, i - start)));
}

// returns true if string is like -v -pvalue
bool isShortParamName(std::wstring & s) {
	return s.length() > 1 && s[0] == '-' && s[1] != '-' && isValidIdentChar(s[1]);
}
// returns true if string is like --param --param=value
bool isLongParamName(std::wstring & s) {
	return s.length() > 2 && s[0] == '-' && s[1] == '-' && s[2] != '-' && isValidIdentChar(s[2]);
}
// returns true if string is like -v -pvalue --param --param=value
bool isParamName(std::wstring & s) {
	return isShortParamName(s) || isLongParamName(s);
}

// split line into two parts (before,after) by specified character, returns true if character is found, otherwise s will be placed into before
bool splitByChar(std::wstring & s, wchar_t ch, std::wstring & before, std::wstring & after) {
	for (size_t i = 0; i < s.length(); i++) {
		if (s[i] == ch) {
			if (i > 0)
				before = s.substr(0, i);
			else
				before.clear();
			if (i + 1 < s.length())
				after = s.substr(i + 1, s.length() - i - 1);
			else
				after.clear();
			return true;
		}
	}
	before = s;
	after.clear();
	return false;
}

// split line into two parts (before,after) by specified character (search from end of s), returns true if character is found, otherwise s will be placed into before
bool splitByCharRev(std::wstring & s, wchar_t ch, std::wstring & before, std::wstring & after) {
	for (int i = (int)s.length() - 1; i >= 0; i--) {
		if (s[i] == ch) {
			if (i > 0)
				before = s.substr(0, i);
			else
				before.clear();
			if (i + 1 < (int)s.length())
				after = s.substr(i + 1, s.length() - i - 1);
			else
				after.clear();
			return true;
		}
	}
	before = s;
	after.clear();
	return false;
}

// debug dump
std::wstring BreakpointInfo::dumpParams() {
	WstringBuffer buf;
	buf.append(L"BreakpointInfo {");
	buf.appendStringParamIfNonEmpty(L"fileName", fileName);
	buf.appendStringParamIfNonEmpty(L"functionName", functionName);
	buf.appendStringParamIfNonEmpty(L"labelName", labelName);
	buf.appendStringParamIfNonEmpty(L"address", address);
	if (line)
		buf.appendUlongParam(L"line", line);
	buf.append(L"}");
	return buf.wstr();
}

std::wstring getExeName()
{
    for (int alloclen = 256; ; alloclen *= 2)
    {
        std::auto_ptr<wchar_t> name(new wchar_t[alloclen]);
        DWORD len = GetModuleFileNameW(NULL, name.get(), alloclen);
        if (len < alloclen)
            return name.get();
    }
}

/// get directory name for file, e.g. for "/dir/subdir/file.ext" return "/dir/subdir"
std::wstring getDirName(std::wstring fname) {
	if (fname.empty())
		return fname;
	int i = ((int)fname.length()) - 1;
	for (; i >= 0; i--) {
		if (fname[i] == '/' || fname[i] == '\\')
			return i > 0 ? fname.substr(0, i) : std::wstring();
	}
	return fname;
}

/// get base name for file name, e.g. for "/dir/subdir/file.ext" return "file.ext"
std::wstring getBaseName(std::wstring fname) {
	if (fname.empty())
		return fname;
	int i = ((int)fname.length()) - 1;
	for (; i >= 0; i--) {
		if (fname[i] == '/' || fname[i] == '\\')
			return fname.substr(i + 1, fname.length() - i - 1);
	}
	return fname;
}

void LocalVariableInfo::dumpMiVariable(WstringBuffer & buf, bool includeTypes, bool includeValues, bool fullSyntax) {
	if (!includeTypes && !includeValues && !fullSyntax) {
		//if (buf.last() != '[')
		buf.appendStringLiteral(varName);
	}
	else {
		buf.append(L"{");
		buf.appendStringParam(L"name", varName);
		if (includeTypes)
			buf.appendStringParam(L"type", varType);
		if (includeValues)
			buf.appendStringParam(L"value", varValue);
		buf.append(L"}");
	}
}

void VariableObject::dumpVariableInfo(WstringBuffer & buf, bool forUpdate) {
	buf.appendStringParam(L"name", name);
	buf.appendStringParam(L"type", type);
	buf.appendStringParam(L"value", value);
	buf.appendStringParam(L"numchild", L"0");
	if (forUpdate) {
		buf.appendStringParam(L"in_scope", inScope ? L"true" : L"false");
		buf.appendStringParam(L"type_changed", L"false");
	}
}

VariableObjectRef VariableObjectList::find(std::wstring name, int * pvarIndex) {
	for (unsigned i = 0; i < size(); i++) {
		if (at(i)->name == name) {
			if (pvarIndex)
				*pvarIndex = (int)i;
			return at(i);
		}
	}
	if (pvarIndex)
		*pvarIndex = -1;
	return VariableObjectRef();
}

VariableObjectRef VariableObjectList::find(std::wstring frameAddress, std::wstring expr, int * pvarIndex) {
	for (unsigned i = 0; i < size(); i++) {
		if (at(i)->frame == frameAddress && at(i)->expr == expr) {
			if (pvarIndex)
				*pvarIndex = (int)i;
			return at(i);
		}
	}
	if (pvarIndex)
		*pvarIndex = -1;
	return VariableObjectRef();
}


/// print mi2 breakpoint info
void BreakpointInfo::printBreakpointInfo(WstringBuffer & buf) {
	buf.append(L"{");
	buf.appendUlongParamAsString(L"number", id);
	buf.appendStringParam(L"type", std::wstring(L"breakpoint"));
	buf.appendStringParam(L"disp", temporary ? std::wstring(L"del") : std::wstring(L"keep"));
	buf.appendStringParam(L"enabled", enabled ? std::wstring(L"y") : std::wstring(L"n"));
	buf.appendStringParamIfNonEmpty(L"addr", (pending && address.empty()) ? std::wstring(L"<PENDING>") : address);
	buf.appendStringParamIfNonEmpty(L"func", functionName);
	buf.appendStringParamIfNonEmpty(L"file", getBaseName(fileName));
	buf.appendStringParamIfNonEmpty(L"fullname", fileName);
	if (boundLine || line)
		buf.appendUlongParamAsString(L"line", boundLine ? boundLine : line);
	if (pending)
		buf.appendStringParam(L"pending", insertCommandText);
	buf.append(L",thread-groups=[\"i1\"]");
	//if (times)
		buf.appendUlongParamAsString(L"times", times);
	buf.appendStringParamIfNonEmpty(L"original-location", originalLocation);
	//buf.appendStringParam(L"thread-groups", std::wstring(L"breakpoint"));
	buf.append(L"}");
}

BreakpointInfoRef BreakpointInfoList::findByPendingBreakpoint(IDebugPendingBreakpoint2 * bp) {
	for (size_t i = 0; i < size(); i++)
		if (at(i)->getPendingBreakpoint() == bp)
			return at(i);
	return BreakpointInfoRef();
}

BreakpointInfoRef BreakpointInfoList::findByBoundBreakpoint(IDebugBoundBreakpoint2 * bp) {
	for (size_t i = 0; i < size(); i++)
		if (at(i)->getBoundBreakpoint() == bp)
			return at(i);
	return BreakpointInfoRef();
}

BreakpointInfoRef BreakpointInfoList::findById(uint64_t id) {
	for (size_t i = 0; i < size(); i++)
		if (at(i)->id == id)
			return at(i);
	return BreakpointInfoRef();
}

bool BreakpointInfoList::removeItem(BreakpointInfoRef & bp) {
	for (size_t i = 0; i < size(); i++)
		if (at(i).Get() == bp.Get()) {
			BreakpointInfoRef found = at(i);
			erase(begin() + i);
			return true;
		}
	return false;
}

BreakpointInfo::BreakpointInfo() 
	: _pendingBreakpoint(NULL)
	, _boundBreakpoint(NULL)
	, id(0)
	, requestId(0)
	, line(0)
	, boundLine(0)
	, times(0)
	, enabled(true)
	, pending(false)
	, temporary(false)
	, bound(false)
	, error(false)
{
}

static uint64_t nextBreakpointId = 1;

uint64_t BreakpointInfo::assignId() {
	id = nextBreakpointId++;
	return id;
}

void BreakpointInfo::setPending(IDebugPendingBreakpoint2 * pPendingBp) {
	_pendingBreakpoint = pPendingBp;
}

void BreakpointInfo::setBound(IDebugBoundBreakpoint2 * pBoundBp) {
	_boundBreakpoint = pBoundBp;
	pending = false;
	bound = true;
}

void BreakpointInfo::setBindError() {
	error = true;
}

bool BreakpointInfo::bind() {
	if (!_pendingBreakpoint)
		return false;
	HRESULT hr = _pendingBreakpoint->Bind();
	if (FAILED(hr)) {
		CRLog::error("pendingBreakpoint->Bind is failed: hr=%d", hr);
		return false;
	}
	return true;
}

BreakpointInfo::~BreakpointInfo() {
	if (_pendingBreakpoint)
		_pendingBreakpoint->Release();
	if (_boundBreakpoint)
		_boundBreakpoint->Release();
}

static const wchar_t * sourceFileExtensions[] = {
	L".d",
	L".di",
	L".h",
	L".c",
	L".cpp",
	L".hpp",
	NULL
};

/// returns true if s is most likely file name
bool looksLikeFileName(std::wstring s) {
	for (unsigned i = 0; sourceFileExtensions[i]; i++)
		if (endsWith(s, std::wstring(sourceFileExtensions[i])))
			return true;
	for (unsigned i = 0; i < s.length(); i++) {
		if (s[i] == '/' || s[i] == '\\')
			return true;
	}
	return false;
}

bool BreakpointInfo::validateParameters() {
	if (!fileName.empty() && line) // file:line
		return true;
	// only file:line breakpoint is supported by Mago
	//if (!functionName.empty()) // function or file:function
	//	return true;
	return false;
}

BreakpointInfo & BreakpointInfo::operator = (const BreakpointInfo & v) {
	id = v.id;
	requestId = v.requestId;
	address = std::wstring(v.address);
	functionName = std::wstring(v.functionName);
	fileName = std::wstring(v.fileName);
	labelName = std::wstring(v.labelName);
	line = v.line;
	enabled = v.enabled;
	pending = v.pending;
	temporary = v.temporary;
	return *this;
}

bool BreakpointInfo::fromCommand(MICommand & cmd) {
	insertCommandText = cmd.tail; // commandText;
	// try named params
	for (size_t i = 0; i < cmd.namedParams.size(); i++) {
		std::wstring name = cmd.namedParams[i].first;
		std::wstring value = unquoteString(cmd.namedParams[i].second);
		if (name == L"--source")
			fileName = value;
		else if (name == L"--function")
			functionName = value;
		else if (name == L"--label")
			labelName = value;
		else if (name == L"--line") {
			uint64_t n = 0;
			if (parseUlong(value, n) && value.empty())
				line = (int)n;
		} else if (name == L"-t") // temporary
			temporary = true;
		else if (name == L"-f")
			pending = true; // create pending if location is not found
		else if (name == L"-d")
			enabled = false; // create disabled
	}
	// try unnamed params
	for (size_t i = 0; i < cmd.unnamedValues.size(); i++) {
		std::wstring value = unquoteString(cmd.unnamedValues[i]);
		uint64_t n = 0;
		std::wstring tmp = value;
		if (parseUlong(tmp, n) && tmp.empty()) {
			// it was just a line number
			line = (int)n;
		}
		else {
			std::wstring part1, part2;
			if (splitByCharRev(value, ':', part1, part2)) {
				// there was : char
				tmp = part2;
				if (parseUlong(tmp, n) && tmp.empty()) {
					// there was a number after :
					// filename:line
					fileName = part1;
					line = (int)n;
				}
				else {
					// filename:function or filename:label
					if (looksLikeFileName(part1)) {
						fileName = part1;
						functionName = part2;
					}
					else {
						functionName = part1;
						labelName = part2;
					}
				}
			}
			else {
				functionName = value;
			}

		}

	}

	if (!fileName.empty() && line)
		originalLocation = fileName + L":" + toWstring(line);
	return validateParameters();
}


std::wstring fixPathDelimiters(std::wstring s) {
	WstringBuffer buf;
	for (unsigned i = 0; i < s.length(); i++) {
		wchar_t ch = s[i];
		if (ch == '/')
			ch = '\\';
		buf.append(ch);
	}
	return buf.wstr();
}

std::wstring quoteString(std::wstring s) {
	if (s.empty())
		return L"\"\"";
	WstringBuffer buf;
	buf.appendStringLiteral(s);
	return buf.wstr();
}

std::wstring unquoteString(std::wstring s) {
	if (s.empty())
		return s;
	if (s[0] == '\"') {
		if (s.length() > 1 && s[s.length() - 1] == '\"')
			return s.substr(1, s.length() - 2);
		return s.substr(1, s.length() - 1);
	}
	return s;
}

bool fileExists(std::wstring fname) {
	HANDLE h = CreateFileW(fname.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE)
		return false;
	CloseHandle(h);
	return true;
}

bool isAbsolutePath(std::wstring s) {
	if (s.empty())
		return false;
	if (s[0] && s[1] == ':' && s[2] == '\\')
		return true;
	if (s[0] == '\\' && s[1] == '\\')
		return true;
	if (s[0] == '\\' || s[0] == '/')
		return true;
	return false;
}

std::wstring getCurrentDirectory() {
	wchar_t buf[4096];
	GetCurrentDirectoryW(4095, buf);
	return std::wstring(buf);
}

std::wstring relativeToAbsolutePath(std::wstring s) {
	WstringBuffer buf;
	if (isAbsolutePath(s)) {
		buf += s;
	} else {
		buf = getCurrentDirectory();
		if (buf.last() != '\\')
			buf += '\\';
		buf += s;
	}
	buf.replace('/', '\\');
	return buf.wstr();
}


#if 0

#include "cmdline.h"
#include "readline.h"

#include "MIEngine.h"


#if 1
void testReadLine() {
	wprintf(L"Testing new line input\n");
	for (int i = 0; i < 10000000; i++) {
		wchar_t * line = NULL;
		int res = readline_poll("(gdb) ", &line);
		if (res == READLINE_READY) {
			DWORD count;
			wprintf(L"Line:", line);
			WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), line, wcslen(line), &count, NULL);
			wprintf(L"\n", line);
			if (line) {
				std::wstring s = line;
				std::string histLine = toUtf8(s);
				add_history((char*)histLine.c_str());
				free(line);
			}
		}
		else if (res == READLINE_CTRL_C) {
			wprintf(L"Ctrl+C is pressed\n");
		}
		else if (res == READLINE_ERROR)
			break;
		if (i > 0 && (i % 1000) == 0) {
			readline_interrupt();
			printf("Some additional lines - input interrupted %d\n", i);
			printf("And one more line\n");
			//printf("Third line\n");
		}
	}
}
#endif

void testEngine() {

	testReadLine();

	MIEngine engine;
	MIEventCallback callback;
	engine.Init(&callback);

	HRESULT hr = engine.Launch(
		executableInfo.exename.c_str(), //LPCOLESTR             pszExe,
		NULL, //LPCOLESTR             pszArgs,
		executableInfo.dir.c_str() //LPCOLESTR             pszDir,
		);
	printf("Launched result=%d\n", hr);
	if (FAILED(hr)) {
		printf("Launch failed: result=%d\n", hr);
		return;
	}

	hr = engine.ResumeProcess();
	if (FAILED(hr)) {
		printf("Resume process failed: result=%d\n", hr);
		return;
	}

	//
	printf("Process is resumed\n");
	char *line;
	line = readline("(gdb) ");
}

#endif

