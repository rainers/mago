#include <windows.h>
#include "miutils.h"

std::string toUtf8(const std::wstring s) {
	StringBuffer buf;
	for (unsigned i = 0; i < s.length(); i++) {
		wchar_t ch = s[i];
		if (ch < 128) {
			buf.append((char)ch);
		}
		else {
			// TODO: add UTF conversion
			buf.append((char)ch);
		}
	}
	return buf.str();
}

std::wstring toUtf16(const std::string s) {
	WstringBuffer buf;
	for (unsigned i = 0; i < s.length(); i++) {
		char ch = s[i];
		char ch2 = (i + 1 < s.length()) ? s[i + 1] : 0;
		char ch3 = (i + 2 < s.length()) ? s[i + 2] : 0;
		// TODO: add UTF conversion
		buf.append(ch);
	}
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
		int len = buf.length();
		if (buf.last() != '\\')
			buf += '\\';
		buf += s;
	}
	buf.replace('/', '\\');
	return buf.wstr();
}





//#include <atlbase.h>
#include "../../DebugEngine/MagoNatDE/Common.h"
#include "../../DebugEngine/MagoNatDE/PendingBreakpoint.h"
#include "../../DebugEngine/MagoNatDE/Program.h"
#include "../../DebugEngine/MagoNatDE/Engine.h"

class CThisExeModule : public CAtlExeModuleT <CThisExeModule>
{};
CThisExeModule _AtlModule;

void testEngine() {
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	//_atl.RegisterTypeLib();
	CComObject<Mago::Engine> * engine = NULL;
	CComObject<Mago::Engine>::CreateInstance(&engine);
	//engine = new Mago::Engine();
}
