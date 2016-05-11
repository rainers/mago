#pragma once

#include <windows.h>
#include "miutils.h"

/// input callback interface
class CmdInputCallback {
public:
	virtual ~CmdInputCallback() {}
	/// called on new input line
	virtual void onInputLine(std::wstring &s) = 0;
	/// called when ctrl+c or ctrl+break is called
	virtual void onCtrlBreak() = 0;
};

/// console or redirected stdin input
/// Supports readline editor for windows console
struct CmdInput {
private:
	CmdInputCallback * _callback;
	bool _inConsole;
	bool _closed;
	StringBuffer _buf;
	void lineCompleted();
public:
	CmdInput();
	~CmdInput();
	bool inConsole() { return _inConsole; }
	/// sets input callback
	void setCallback(CmdInputCallback * callback) {
		_callback = callback;
	}

	/// returns true if stdin/stdout is closed
	bool isClosed();
	/// poll input, return false if stdin is closed or eof
	bool poll();
};

/// global cmd input object
extern CmdInput _cmdinput;

/// write line to stdout, returns false if writing is failed
bool writeStdout(std::wstring s);
/// write line to stderr, returns false if writing is failed
bool writeStderr(std::wstring s);

/// formatted output to debugger stdout
bool writeStdout(const char * fmt, ...);
/// formatted output to debugger stdout
bool writeStdout(const wchar_t * fmt, ...);
/// formatted output to debugger stderr
bool writeStderr(const char * fmt, ...);
/// formatted output to debugger stderr
bool writeStderr(const wchar_t * fmt, ...);
