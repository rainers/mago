
#include "cmdinput.h"
#include <Guard.h>
#include "readline.h"



static CmdInput * _instance = NULL;

static Mutex _consoleGuard;

CmdInput::CmdInput() : _callback(NULL), _closed(false), _enabled(true)
{
	_instance = this;
	_inConsole = isInConsole() != 0;
}

CmdInput::~CmdInput() {
	_instance = NULL;
	free_history();
}


static bool _readlineEditActive = false;
static bool _readlinePromptShown = false;

void CmdInput::showPrompt() {
	if (_inConsole)
		return;
	TimeCheckedGuardedArea area(_consoleGuard, "stdin poll");
	if (_enabled && !_readlinePromptShown) {
		HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD bytesWritten = 0;
		CRLog::debug("STDOUT: (gdb) ");
		if (!WriteFile(h_out, "(gdb) \r\n", 8, &bytesWritten, NULL)) {
			DWORD err = GetLastError();
			CRLog::error("ReadFile error %d", err);
			_closed = true;
		}
		_readlinePromptShown = true;
	}
}

void CmdInput::enable(bool enabled) {
	_enabled = enabled;
	showPrompt();
}

/// write line to stdout
bool writeStdout(std::wstring s) {
	TimeCheckedGuardedArea area(_consoleGuard, "writeStdout");
	HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
	WstringBuffer buf;
	std::string lineNoEol = toUtf8(s);
	CRLog::debug("STDOUT: %s", lineNoEol.c_str());
	buf = s;
	buf += L"\r\n";
	if (_cmdinput.inConsole()) {
		if (_readlineEditActive) {
			readline_interrupt();
			_readlineEditActive = false;
		}
	}
	if (_cmdinput.inConsole()) {
		// erase current edit line
		DWORD charsWritten = 0;
		SetConsoleMode(h_out, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
		//WriteConsoleW(h_out, buf.ptr(), buf.length(), &charsWritten, NULL);
		return (WriteConsoleW(h_out, buf.ptr(), buf.length(), &charsWritten, NULL) != 0);
	}
	else {
		std::string line = toUtf8(buf.wstr());
		DWORD bytesWritten = 0;
		//printf("line to write: %s", line.c_str());
		bool res = WriteFile(h_out, line.c_str(), line.length(), &bytesWritten, NULL) != 0;
		if (res) {
			FlushFileBuffers(h_out);
			WriteFile(h_out, line.c_str(), 0, &bytesWritten, NULL);
		}
		//res = WriteFile(h_out, "\n", 1, &bytesWritten, NULL) != 0;
		return res;
	}
}

/// write line to stderr
bool writeStderr(std::wstring s) {
	TimeCheckedGuardedArea area(_consoleGuard, "writeStderr");
	HANDLE h_out = GetStdHandle(STD_ERROR_HANDLE);
	WstringBuffer buf;
	buf = s;
	buf += L"\r\n";
	std::string line = toUtf8(buf.wstr());
	if (_cmdinput.inConsole()) {
		// erase current edit line
		readline_interrupt();
	}
	DWORD bytesWritten = 0;
	CRLog::debug("STDERR: %s", line.c_str());
	bool res = WriteFile(h_out, line.c_str(), line.length(), &bytesWritten, NULL) != 0;
	if (res) {
		FlushFileBuffers(h_out);
		WriteFile(h_out, line.c_str(), 0, &bytesWritten, NULL);
	}
	return res;
}

// global stdinput object
CmdInput _cmdinput;

#define OUT_BUFFER_SIZE 16384
/// formatted output to debugger stdout
bool writeStdout(const char * fmt, ...) {
	va_list args;
	va_start(args, fmt);
	static char buffer[OUT_BUFFER_SIZE];
	vsnprintf_s(buffer, OUT_BUFFER_SIZE - 1, fmt, args);
	va_end(args);
	std::string s = std::string(buffer);
	std::wstring ws = toUtf16(s);
	return writeStdout(ws);
}

/// formatted output to debugger stderr
bool writeStderr(const char * fmt, ...) {
	va_list args;
	va_start(args, fmt);
	static char buffer[OUT_BUFFER_SIZE];
	vsnprintf_s(buffer, OUT_BUFFER_SIZE - 1, fmt, args);
	va_end(args);
	std::string s = std::string(buffer);
	std::wstring ws = toUtf16(s);
	return writeStderr(ws);
}

/// formatted output to debugger stdout
bool writeStdout(const wchar_t * fmt, ...) {
	va_list args;
	va_start(args, fmt);
	static wchar_t buffer[OUT_BUFFER_SIZE];
	_vsnwprintf_s(buffer, OUT_BUFFER_SIZE - 1, fmt, args);
	va_end(args);
	std::wstring ws = std::wstring(buffer);
	return writeStdout(ws);
}

/// formatted output to debugger stderr
bool writeStderr(const wchar_t * fmt, ...) {
	va_list args;
	va_start(args, fmt);
	static wchar_t buffer[OUT_BUFFER_SIZE];
	_vsnwprintf_s(buffer, OUT_BUFFER_SIZE - 1, fmt, args);
	va_end(args);
	std::wstring ws = std::wstring(buffer);
	return writeStderr(ws);
}

/// returns true if stdin/stdout is closed
bool CmdInput::isClosed() {
	if (_closed)
		return true;
	if (_inConsole)
		return false;
	// check status
	TimeCheckedGuardedArea area(_consoleGuard, "isClosed");
	HANDLE h_in = GetStdHandle(STD_INPUT_HANDLE);
	if (h_in && h_in != INVALID_HANDLE_VALUE) {
		DWORD res = WaitForSingleObject(h_in, 10);
		if (res != WAIT_FAILED) {
			return false;
		}
	}
	_closed = true;
	return _closed;
}

void CmdInput::lineCompleted() {
	_buf.trimEol();
	std::wstring res = _buf.wstr();
	_buf.reset();
	std::string s = toUtf8(res);
	CRLog::debug("STDIN: %s", s.c_str());
	_readlinePromptShown = false;
	if (_callback) {
		_callback->onInputLine(res);
	}
}

/// poll input, return false if stdin is closed or eof
bool CmdInput::poll() {
	if (_closed)
		return false;
	if (_inConsole) {
		//
		wchar_t * line = NULL;
		int res = READLINE_ERROR;
		{
			TimeCheckedGuardedArea area(_consoleGuard, "readline_poll");
			res = readline_poll("(gdb) ", &line);
			if (res == READLINE_IN_PROGRESS)
				_readlineEditActive = true;
		}
		if (res == READLINE_READY) {
			//wprintf(L"Line: %s\n", line);
			if (line) {
				std::wstring s = line;
				std::string histLine = toUtf8(s);
				add_history((char*)histLine.c_str());
				free(line);
				_buf = histLine;
				lineCompleted();
			}
		}
		else if (res == READLINE_CTRL_C) {
			//wprintf(L"Ctrl+C is pressed\n");
			if (_callback) {
				_callback->onCtrlBreak();
			}
		}
		else if (res == READLINE_ERROR) {
			_closed = true;
			return false;
		}
		return true;
	} else {
		{
			HANDLE h_in = GetStdHandle(STD_INPUT_HANDLE);


			if (h_in && h_in != INVALID_HANDLE_VALUE) {
				{
					showPrompt();
				}
				for (;;) {
					//CRLog::debug("Waiting for STDIN input");
					//DWORD res = WaitForSingleObject(h_in, 100);
					//CRLog::debug("Wait result: %x", res);

					DWORD bytesAvailable = 0;
					if (!PeekNamedPipe(h_in, NULL, 0, NULL, &bytesAvailable, NULL)) {
						DWORD err = GetLastError();
						CRLog::error("PeekNamedPipe error %d", err);
						_closed = true;
						break;
					}
					//CRLog::debug("PeekNamedPipe result: %x", bytesAvailable);

					if (bytesAvailable) {//res == WAIT_OBJECT_0) {
						char ch = 0;
						DWORD bytesRead = 0;
						//CRLog::trace("Reading character from stdin");
						if (ReadFile(h_in, &ch, 1, &bytesRead, NULL)) {
							// byte is read from stdin
							if (bytesRead == 1) {
								//printf("Character read: %c (%d)", ch, ch);
								if (ch != '\r') // ignore \r in \r\n
									_buf.append(ch);
								if (ch == '\n')
									break; // full line is entered
							}
						}
						else {
							//ERROR_OPERATION_ABORTED;
							DWORD err = GetLastError();
							CRLog::error("ReadFile error %d", err);
							if (err == ERROR_BROKEN_PIPE) {
								_closed = true;
							}
							//printf("Reading failed, ch = %d, bytesRead = %d, lastError=%d\n", ch, bytesRead, GetLastError());
							break;
						}
					}
					else {
						//CRLog::trace("no data in stdin, sleeping");
						Sleep(100);
						break;
					}//if (res == WAIT_TIMEOUT) {
					//	CRLog::trace("stdin polling timeout");
					//	break;
					//}
					//} else if (res == WAIT_FAILED) {
					//	_closed = true;
					//	break;
					//}
				}
			}
		}
		if (_buf.endsWith('\n')) {
			lineCompleted();
		}
	}
	if (isClosed()) {
		CRLog::trace("input is closed");
	}
	return !isClosed();
}
