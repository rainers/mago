
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

bool fileExists(const char * fn) {
	FILE * f = fopen(fn, "rb");
	if (!f)
		return false;
	fclose(f);
	return true;
}

void getDirName(const char * fn, char * buf) {
	strcpy(buf, fn);
	int i = strlen(buf) - 1;
	for (; i > 0; i--)
		if (buf[i] == '/' || buf[i] == '\\')
			break;
	buf[i + 1] = 0;
}

class PipeThread {
	HANDLE _hin;
	HANDLE _hout;
	const char * _logdir;
	FILE * _logfile;
	DWORD _idThread;
	HANDLE _hThread;
public:
	static DWORD WINAPI threadProc( void * param) //_In_ LPVOID lpParameter
	{
		PipeThread * _this = (PipeThread*)param;
		return _this->run();
	}
	DWORD run() {
		if (_logfile) {
			fprintf(_logfile, "Polling thread is started\r\n");
			fflush(_logfile);
		}
		for (;;) {
			if (!poll()) {
				break;
			}
		}
		if (_logfile) {
			fprintf(_logfile, "\r\nPolling thread is finished\r\n");
			fflush(_logfile);
		}
		return 0;
	}
	PipeThread(HANDLE hin, HANDLE hout, const char * logdir, const char * logFile) {
		_hin = hin;
		_hout = hout;
		char fn[4096];
		strcpy(fn, logdir);
		strcat(fn, logFile);
		_logfile = fopen(fn, "wb");
		_idThread = NULL;
		_hThread = CreateThread(NULL, 16384, &threadProc, this, 0, &_idThread);
	}
	bool poll() {
		char buf[4096];
		DWORD bytesRead = 0;
		DWORD bytesAvail = 0;
		DWORD bytesLeftThisMessage = 0;
		DWORD res = WaitForSingleObject(_hin, INFINITE);
		if (res != WAIT_OBJECT_0)
			return false;
		if (!ReadFile(_hin, buf, 4095, &bytesRead, NULL)) {
			return false;
		}
		if (bytesRead) {
			DWORD bytesWritten = 0;
			if (!WriteFile(_hout, buf, bytesRead, &bytesWritten, NULL) || bytesWritten != bytesRead)
				return false;
			if (!bytesWritten)
				return false;
			if (_logfile) {
				fwrite(buf, bytesRead, 1, _logfile);
				fflush(_logfile);
			}
		}
		return true;
	}
	void wait() {
		WaitForSingleObject(_hThread, INFINITE);
	}
	~PipeThread() {
		if (_logfile)
			fclose(_logfile);
	}
};


class Process {
	const char * _exename;
	int _argc;
	const char ** _argv;
	const char * _logdir;
#ifdef _WIN32
	HANDLE _stdin_read;
	HANDLE _stdin_write;
	HANDLE _stderr_read;
	HANDLE _stderr_write;
	HANDLE _stdout_read;
	HANDLE _stdout_write;
	PROCESS_INFORMATION _pi;
#endif
public:
	Process(const char * exename, int argc, const char ** argv, const char * logdir) : _exename(exename), _argc(argc), _argv(argv), _logdir(logdir) {
#ifdef _WIN32
		_stdin_read = NULL;
		_stdin_write = NULL;
		_stderr_read = NULL;
		_stderr_write = NULL;
		_stdout_read = NULL;
		_stdout_write = NULL;
		memset(&_pi, 0, sizeof(_pi));
#endif
	}
	void close() {
		if (_stdin_read)
			CloseHandle(_stdin_read);
		if (_stdin_write)
			CloseHandle(_stdin_write);
		if (_stderr_write)
			CloseHandle(_stderr_write);
		if (_stdout_write)
			CloseHandle(_stdout_write);
		_stdin_read = NULL;
		_stdin_write = NULL;
		_stderr_write = NULL;
		_stdout_write = NULL;
	}
	~Process() {
#ifdef _WIN32
		if (_pi.hThread)
			CloseHandle(_pi.hThread);
		if (_pi.hProcess)
			CloseHandle(_pi.hProcess);
#endif
	}
	bool start() {
#ifdef _WIN32
		char cmdline[16384];
		strcpy(cmdline, _exename);
		for (int i = 1; i < _argc; i++) {
			strcat(cmdline, " ");
			strcat(cmdline, _argv[i]);
		}
		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;
		STARTUPINFOA si;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES;
		if (!CreatePipe(&_stdin_read, &_stdin_write, &saAttr, 1)) {
			// cannot create pipe
			return false;
		}
		if (!SetHandleInformation(_stdin_write, HANDLE_FLAG_INHERIT, 0)) {
			// cannot set pipe params
			return false;
		}
		if (!CreatePipe(&_stderr_read, &_stderr_write, &saAttr, 1)) {
			// cannot create pipe
			return false;
		}
		if (!SetHandleInformation(_stderr_read, HANDLE_FLAG_INHERIT, 0)) {
			// cannot set pipe params
			return false;
		}
		if (!CreatePipe(&_stdout_read, &_stdout_write, &saAttr, 1)) {
			// cannot create pipe
			return false;
		}
		if (!SetHandleInformation(_stdout_read, HANDLE_FLAG_INHERIT, 0)) {
			// cannot set pipe params
			return false;
		}
		si.hStdError = _stderr_write;
		si.hStdOutput = _stdout_write;
		si.hStdInput = _stdin_read;

		BOOL res = CreateProcessA(
			_exename, //_In_opt_    LPCTSTR               lpApplicationName,
			cmdline, //_Inout_opt_ LPTSTR                lpCommandLine,
			NULL, //_In_opt_    LPSECURITY_ATTRIBUTES lpProcessAttributes,
			NULL, //_In_opt_    LPSECURITY_ATTRIBUTES lpThreadAttributes,
			TRUE, //_In_        BOOL                  bInheritHandles,
			0, //_In_        DWORD                 dwCreationFlags,
			NULL, //_In_opt_    LPVOID                lpEnvironment,
			NULL, //_In_opt_    LPCTSTR               lpCurrentDirectory,
			&si, //_In_        LPSTARTUPINFO         lpStartupInfo,
			&_pi //_Out_       LPPROCESS_INFORMATION lpProcessInformation
			);
		if (!res) {
			return false;
		}

#endif
		return true;
	}
	int getExitCode() {
		DWORD exitCode = 0;
		GetExitCodeProcess(_pi.hProcess, &exitCode);
		return exitCode;
	}
	void poll() {
		
		PipeThread stdErrThread(_stderr_read, GetStdHandle(STD_ERROR_HANDLE), _logdir, "gdb_stderr.log");
		PipeThread stdOutThread(_stdout_read, GetStdHandle(STD_OUTPUT_HANDLE), _logdir, "gdb_stdout.log");
		PipeThread stdInThread(GetStdHandle(STD_INPUT_HANDLE), _stdin_write, _logdir, "gdb_stdin.log");

		//bool stdErrFinished = false;
		//bool stdOutFinished = false;
		//bool stdInFinished = false;

		//while (!stdErrFinished || !stdOutFinished || !stdInFinished) {
		//	stdErrFinished = stdErrFinished || !stdErrThread.poll();
		//	stdOutFinished = stdOutFinished || !stdOutThread.poll();
		//	stdInFinished = stdInFinished || !stdInThread.poll();
		//}
		WaitForSingleObject(_pi.hProcess, INFINITE);
		close();
		char ch = 'A';
		DWORD bytesWritten = 0;
		WriteFile(_stdin_read, &ch, 1, &bytesWritten, NULL);
		stdErrThread.wait();
		stdOutThread.wait();
		//stdInThread.wait();
	}
};

int main(int argc, const char ** argv) {
	char * gdbexe = getenv("GDB_EXECUTABLE");
	char logdir[4096];
	getDirName(argv[0], logdir);

#ifdef _DEBUG
	if (!gdbexe) {
		char settings[4096];
		strcpy(settings, logdir);
		strcat(settings, "gdblogger.ini");
		FILE * settingsFile = fopen(settings, "rt");
		if (settingsFile) {
			static char tmp[4096];
			if (fscanf(settingsFile, "%s\n", &tmp) == 1) {
				if (fileExists(tmp))
					gdbexe = tmp;
			}
			fclose(settingsFile);
		}

		if (!gdbexe) {
			static char buf[1024];
			getDirName(argv[0], buf);
			strcat(buf, "mago-mi.exe");
			gdbexe = buf; // "mago-mi.exe";
		}
	}
#endif

	if (!gdbexe) {
		fprintf(stderr, "Please specify GDB_EXECUTABLE environment variable pointing to GDB debugger executable or put its name to file gdblogger.ini");
		return 1;
	}
	if (!fileExists(gdbexe)) {
		fprintf(stderr, "File %s cannot be found", gdbexe);
		return 2;
	}
	Process process(gdbexe, argc, argv, logdir);
	if (!process.start()) {
		fprintf(stderr, "Failed to start GDB executable %s", gdbexe);
		return 3;
	}
	process.poll();
	int res = process.getExitCode();
	return res;
}

