
#include "cmdinput.h"
#include <Guard.h>
#include "readline.h"


static CmdInput * _instance = NULL;
Guard _consoleGuard;

CmdInput::CmdInput() : _callback(NULL), _closed(false)
{
	_instance = this;
	_inConsole = isInConsole() != 0;
}

CmdInput::~CmdInput() {
	_instance = NULL;
}

/// write line to stdout
bool CmdInput::writeLine(std::wstring s) {
	GuardedArea area(_consoleGuard);
	HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
	WstringBuffer line;
	line = s;
	line += L"\r\n";
	DWORD bytesWritten = 0;
	return WriteFile(h_out, line.c_str(), line.length()*sizeof(wchar_t), &bytesWritten, NULL) != 0;
}

/// write line to stderr
bool CmdInput::writeStderrLine(std::wstring s) {
	GuardedArea area(_consoleGuard);
	HANDLE h_out = GetStdHandle(STD_ERROR_HANDLE);
	WstringBuffer line;
	line = s;
	line += L"\r\n";
	DWORD bytesWritten = 0;
	return WriteFile(h_out, line.c_str(), line.length()*sizeof(wchar_t), &bytesWritten, NULL) != 0;
}

/// returns true if stdin/stdout is closed
bool CmdInput::isClosed() {
	if (_closed)
		return true;
	// check status
	GuardedArea area(_consoleGuard);
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
		char *line;
		line = readline("(gdb) ");
		if (!line) {
			_closed = true;
			return false;
		}
		add_history(line);
		std::string s = line;
		free(line);
		_buf = s;
		lineCompleted();
	} else {
		{
			GuardedArea area(_consoleGuard);
			HANDLE h_in = GetStdHandle(STD_INPUT_HANDLE);
			if (h_in && h_in != INVALID_HANDLE_VALUE) {
				for (;;) {
					//printf("Waiting for STDIN input\n");
					//DWORD res = WaitForSingleObject(h_in, 100);
					//printf("Wait result: %x\n", res);
					//if (res == WAIT_OBJECT_0) {
					char ch = 0;
					DWORD bytesRead = 0;
					//printf("Reading character from stdin\n");
					if (ReadFile(h_in, &ch, 1, &bytesRead, NULL)) {
						// byte is read from stdin
						if (bytesRead == 1) {
							//printf("Character read: %c (%d)\n", ch, ch);
							if (ch != '\r') // ignore \r in \r\n
								_buf.append(ch);
							if (ch == '\n')
								break; // full line is entered
						}
					}
					else {
						//ERROR_OPERATION_ABORTED;
						DWORD err = GetLastError();
						if (err == ERROR_BROKEN_PIPE) {
							_closed = true;
						}
						//printf("Reading failed, ch = %d, bytesRead = %d, lastError=%d\n", ch, bytesRead, GetLastError());
						break;
					}
					//} else if (res == WAIT_TIMEOUT) {
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
	return !isClosed();
}
