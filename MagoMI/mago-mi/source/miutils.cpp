#include <windows.h>
#include "miutils.h"

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
	wsprintf(buf, L"%lld", n);
	append(buf);
	return *this;
}

WstringBuffer & WstringBuffer::appendStringLiteral(std::wstring s) {
	append('\"');
	for (size_t i = 0; i < s.length(); i++) {
		wchar_t ch = s[i];
		switch (ch) {
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
	, miCommand(false)
{

}
MICommand::~MICommand() {
	
}

void StackFrameInfo::dumpMIFrame(WstringBuffer & buf) {
	buf.append('{');
	if (!functionName.empty()) {
		buf.appendStringParamIfNonEmpty(L"func", functionName, '{');
		buf.appendStringParamIfNonEmpty(L"args", std::wstring(L"[]"), '{'); // TODO
	}
	else {
		buf.appendStringParamIfNonEmpty(L"addr", address, '{');
	}
	buf.appendStringParamIfNonEmpty(L"file", sourceBaseName, '{');
	buf.appendStringParamIfNonEmpty(L"fullname", sourceFileName, '{');
	if (sourceLine != 0)
		buf.appendUlongParam(L"line", sourceLine, '{');
	buf.appendStringParamIfNonEmpty(L"from", moduleName, '{');
	buf.append('}');
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

bool MICommand::parse(std::wstring s) {
	requestId = UNSPECIFIED_REQUEST_ID;
	parseUlong(s, requestId);
	if (!parseIdentifier(s, commandName))
		return false;
	if (commandName[0] == '-' && commandName[1] != '-')
		miCommand = true;
	return true;
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


