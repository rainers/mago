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

