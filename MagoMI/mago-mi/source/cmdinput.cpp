
#include "cmdinput.h"
#include <Guard.h>
#include "readline.h"

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
		DWORD dwWaitResult = WaitForSingleObject(
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
		return *this;
	}
};



static CmdInput * _instance = NULL;

static Mutex _consoleGuard;

CmdInput::CmdInput() : _callback(NULL), _closed(false)
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

/// write line to stdout
bool writeStdout(std::wstring s) {
	TimeCheckedGuardedArea area(_consoleGuard, "writeStdout");
	HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
	WstringBuffer buf;
	std::string lineNoEol = toUtf8(s);
	CRLog::debug("STDOUT: %s", lineNoEol.c_str());
	buf = s;
	buf += L"\n";
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
	buf += L"\n";
	std::string line = toUtf8(buf.wstr());
	if (_cmdinput.inConsole()) {
		// erase current edit line
		readline_interrupt();
	}
	DWORD bytesWritten = 0;
	CRLog::debug("STDERR: %s", line.c_str());
	return WriteFile(h_out, line.c_str(), line.length(), &bytesWritten, NULL) != 0;
}

// global stdinput object
CmdInput _cmdinput;

#define OUT_BUFFER_SIZE 16384
/// formatted output to debugger stdout
bool writeStdout(const char * fmt, ...) {
	va_list args;
	va_start(args, fmt);
	static char buffer[OUT_BUFFER_SIZE];
	vsnprintf(buffer, OUT_BUFFER_SIZE - 1, fmt, args);
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
	vsnprintf(buffer, OUT_BUFFER_SIZE - 1, fmt, args);
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
					TimeCheckedGuardedArea area(_consoleGuard, "stdin poll");
					if (!_readlinePromptShown) {
						HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
						DWORD bytesWritten = 0;
						WriteFile(h_out, "(gdb)\n", 6, &bytesWritten, NULL);
						_readlinePromptShown = true;
					}
				}
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
