#include <windows.h>
#include "miutils.h"
#include "MIEngine.h"
#include "../../DebugEngine/MagoNatDE/PendingBreakpoint.h"

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

// appends number
WstringBuffer & WstringBuffer::appendUlongLiteral(uint64_t n) {
	wchar_t buf[32];
	wsprintf(buf, L"%I64d", n);
	append(buf);
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

MICommand::MICommand() 
	: requestId(UNSPECIFIED_REQUEST_ID) 
	, commandId(CMD_UNKNOWN)
	, miCommand(false)
{

}
MICommand::~MICommand() {
	
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

/// compare name (till \0 or space) with s (till \0)
bool cmpCommand(const char * name, const wchar_t * s) {
	if (!name || !name[0])
		return false;
	if (!s || !s[0])
		return false;
	int i = 0;
	for (; name[i] && name[i] != ' ' && s[i]; i++)
		if (name[i] != s[i])
			break;
	if (s[i])
		return false;
	return !name[i] || name[i] == ' ';
}

/// skip till beginning of next word
bool nextWord(const char * &s) {
	while (*s && s[0] != ' ')
		s++;
	while (s[0] == ' ')
		s++;
	return s[0] != 0;
}

struct MiCommandInfo {
	MiCommandId id;
	const char * name;
	const char * nonMiName;
	const char * description;
	MiCommandInfo() : id(CMD_UNKNOWN), name(NULL), nonMiName(NULL), description(NULL) {}
	MiCommandInfo(MiCommandId _id, const char * _name, const char * _nonMiName, const char * _description)
		: id(_id), name(_name), nonMiName(_nonMiName), description(_description) 
	{
	}
	bool compare(MICommand & v) {
		if (cmpCommand(name, v.commandName.c_str())) {
			v.commandId = id;
			return true;
		}
		if (cmpCommand(nonMiName, v.commandName.c_str())) {
			const char * cmdName = nonMiName;
			int i = 0;
			while (nextWord(cmdName)) {
				if (i + 1 >= v.params.size())
					return false;
				std::wstring param = v.params[i];
				if (!cmpCommand(cmdName, param.c_str()))
					return false;
				i++;
			}
			if (i > 0) {
				// remove matched additional words for command
				v.params.erase(v.params.begin(), v.params.begin() + i);
			}
			v.commandId = id;
			return true;
		}
		return false;
	}
};

static MiCommandInfo MI_COMMANDS[] = {
	MiCommandInfo(CMD_GDB_EXIT, "-gdb-exit", "quit", "quit debugger"),
	MiCommandInfo(CMD_HELP, NULL, "help", ""),
	MiCommandInfo(CMD_EXEC_RUN, "-exec-run", "run", ""),
	MiCommandInfo(CMD_EXEC_CONTINUE, "-exec-continue", "continue", ""),
	MiCommandInfo(CMD_EXEC_INTERRUPT, "-exec-interrupt", "interrupt", ""),
	MiCommandInfo(CMD_EXEC_FINISH, "-exec-finish", "finish", ""),
	MiCommandInfo(CMD_EXEC_NEXT, "-exec-next", "next", ""),
	MiCommandInfo(CMD_EXEC_NEXT_INSTRUCTION, "-exec-next-instruction", "nexti", ""),
	MiCommandInfo(CMD_EXEC_STEP, "-exec-step", "step", ""),
	MiCommandInfo(CMD_EXEC_STEP_INSTRUCTION, "-exec-step-instruction", "stepi", ""),
	MiCommandInfo(CMD_BREAK_INSERT, "-break-insert", "break", ""),
	MiCommandInfo(CMD_BREAK_DELETE, "-break-delete", "delete", ""),
	MiCommandInfo(CMD_BREAK_ENABLE, "-break-enable", "enable", ""),
	MiCommandInfo(CMD_BREAK_DISABLE, "-break-disable", "disable", ""),
	MiCommandInfo(CMD_BREAK_LIST, "-break-list", NULL, ""),
	MiCommandInfo(CMD_LIST_THREAD_GROUPS, "-list-thread-groups", "info break", ""),
	MiCommandInfo(CMD_THREAD_INFO, "-thread-info", "info thread", ""),
	MiCommandInfo(CMD_THREAD_LIST_IDS, "-thread-list-ids", "info threads", ""),
	MiCommandInfo(CMD_STACK_LIST_FRAMES, "-stack-list-frames backtrace", NULL, ""),
	MiCommandInfo(CMD_STACK_INFO_DEPTH, "-stack-info-depth", NULL, ""),
	MiCommandInfo(CMD_STACK_LIST_VARIABLES, "-stack-list-variables", NULL, ""),
	MiCommandInfo(CMD_STACK_LIST_LOCALS, "-stack-list-locals", NULL, ""),
	MiCommandInfo(CMD_VAR_CREATE, "-var-create", NULL, ""),
	MiCommandInfo(CMD_VAR_UPDATE, "-var-update", NULL, ""),
	MiCommandInfo(CMD_VAR_DELETE, "-var-delete", NULL, ""),
	MiCommandInfo(CMD_VAR_SET_FORMAT, "-var-set-format", NULL, ""),
	MiCommandInfo(CMD_LIST_FEATURES, "-list-features", NULL, ""),
	MiCommandInfo(CMD_GDB_VERSION, "-gdb-version", NULL, ""),
	MiCommandInfo(CMD_ENVIRONMENT_CD, "-environment-cd", NULL, ""),
	MiCommandInfo(CMD_GDB_SHOW, "-gdb-show", NULL, ""),
	MiCommandInfo(CMD_INTERPRETER_EXEC, "-interpreter-exec", NULL, ""),
	MiCommandInfo(CMD_DATA_EVALUATE_EXPRESSION, "-data-evaluate-expression", NULL, ""),
	MiCommandInfo(CMD_GDB_SET, "-gdb-set", NULL, ""),
	MiCommandInfo(CMD_MAINTENANCE, NULL, "maintenance", ""),
	MiCommandInfo(CMD_SOURCE, NULL, "source", ""),
	MiCommandInfo(CMD_FILE_EXEC_AND_SYMBOLS, "-file-exec-and-symbols", NULL, ""),
	MiCommandInfo(CMD_HANDLE, NULL, "handle", ""),
	MiCommandInfo()
};

/// find command by name and set its id, returns false for unknown command
bool findCommand(MICommand & v) {
	for (int i = 0; MI_COMMANDS[i].id != CMD_UNKNOWN; i++) {
		if (MI_COMMANDS[i].compare(v))
			return true;
	}
	return false;
}

// return true if value is required
bool doesParameterRequireValue(std::wstring p) {
	static const wchar_t * KNOWN_PARAMS_WITH_VALUES_LIST[] = {
		L"--thread-group",
		L"--thread",
		L"--frame",
		NULL
	};
	for (int i = 0; KNOWN_PARAMS_WITH_VALUES_LIST[i]; i++)
		if (p == KNOWN_PARAMS_WITH_VALUES_LIST[i])
			return true;
	return false;
}

// returns true if embedded value is found
bool splitParamAndValue(std::wstring & s, std::wstring & name, std::wstring & value) {
	if (isShortParamName(s)) {
		if (s.length() == 2) {
			// -c
			name = s;
			value.clear();
			return false;
		}
		else {
			// -cvalue
			name = s.substr(0, 2);
			value = s.substr(2, s.length() - 2);
			return true;
		}
	}
	else if (isLongParamName(s)) {
		// --paramname or --paramname=value
		return splitByChar(s, '=', name, value);
	}
	else {
		// not a parameter - put into value
		name.clear();
		value = s;
		return false;
	}
}



void collapseParams(wstring_vector & items, param_vector & namedParams) {
	for (size_t i = 0; i < items.size(); i++) {
		std::wstring item = items[i];
		std::wstring next = i + 1 < items.size() ? items[i + 1] : std::wstring();
		std::wstring name;
		std::wstring value;
		if (isParamName(item)) {
			// --param or -p
			if (splitParamAndValue(item, name, value)) {
				// has both name and value: --param=value or -pvalue
				wstring_pair pair;
				pair.first = name;
				pair.second = value;
				namedParams.push_back(pair);
			}
			else {
				// separate values for params not supported
				if (!doesParameterRequireValue(item)) {
					// no value
					wstring_pair pair;
					pair.first = name;
					namedParams.push_back(pair);
				}
				else {
					if (isParamName(next) || next.empty()) { // short params cannot have separate values
						// no value
						wstring_pair pair;
						pair.first = name;
						namedParams.push_back(pair);
					}
					else {
						// next item is value for this param
						wstring_pair pair;
						pair.first = name;
						pair.second = next;
						namedParams.push_back(pair);
						// skip one item - it's already used as value
						i++;
					}
				}
			}

		}
		else {
			wstring_pair pair;
			pair.second = item;
			namedParams.push_back(pair);
		}

	}
}

/// returns true if there is specified named parameter in cmd
bool MICommand::hasParam(std::wstring name) {
	for (unsigned i = 0; i < namedParams.size(); i++)
		if (namedParams[i].first == name)
			return true;
	return false;
}

// find parameter by name
std::wstring MICommand::findParam(std::wstring name) {
	for (unsigned i = 0; i < namedParams.size(); i++)
		if (namedParams[i].first == name)
			return namedParams[i].second;
	return std::wstring();
}

// get parameter --thread-id
uint64_t MICommand::getUlongParam(std::wstring name, uint64_t defValue) {
	std::wstring v = findParam(name);
	if (v.empty())
		return defValue;
	uint64_t tid = 0;
	if (!toUlong(v, tid))
		return defValue;
	return tid;
}

bool MICommand::parse(std::wstring s) {
	requestId = UNSPECIFIED_REQUEST_ID;
	commandName.clear();
	tail.clear();
	params.clear();
	namedParams.clear();
	unnamedValues.clear();
	commandText = s;
	parseUlong(s, requestId);
	if (!parseIdentifier(s, commandName))
		return false;
	if (commandName[0] == '-' && commandName[1] != '-')
		miCommand = true;
	skipWhiteSpace(s);
	tail = s;
	splitSpaceSeparatedParams(s, params);
	// find id for command
	findCommand(*this);
	collapseParams(params, namedParams);
	for (size_t i = 0; i < namedParams.size(); i++) {
		if (namedParams[i].first.empty() && !namedParams[i].second.empty())
			unnamedValues.push_back(namedParams[i].second);
	}
	return true;
}

// debug dump
std::wstring MICommand::dumpCommand() {
	WstringBuffer buf;
	buf.append(L"MICommand {");
	buf.appendStringParam(L"commandName", commandName);
	buf.append(L" params=[ ");
	for (size_t i = 0; i < params.size(); i++) {
		buf.append(L"`");
		buf += params[i];
		buf.append(L"` ");
	}
	buf.append(L"] ");
	buf.append(L" namedParams={");
	for (size_t i = 0; i < namedParams.size(); i++) {
		buf.append(L"`");
		buf += namedParams[i].first;
		buf.append(L"`=`");
		buf += namedParams[i].second;
		buf.append(L"` ");
	}
	buf.append(L"} ");
	buf.append(L" unnamedValues=[ ");
	for (size_t i = 0; i < unnamedValues.size(); i++) {
		buf.append(L"`");
		buf += unnamedValues[i];
		buf.append(L"` ");
	}
	buf.append(L"] ");
	buf.append(L"}");
	return buf.wstr();
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

void LocalVariableInfo::dumpMiVariable(WstringBuffer & buf, bool includeTypes, bool includeValues) {
	if (!includeTypes && !includeValues) {
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
		std::wstring value = cmd.namedParams[i].second;
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
		std::wstring value = cmd.unnamedValues[i];
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

