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


#if 1

#include "cmdline.h"
#include "readline.h"

#include "MIEngine.h"

#if 0
class MICallback : public IDebugEventCallback2 {
public:
	MICallback() {}
	~MICallback() {}
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
			/* [in] */ REFIID riid, 
			/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) {
		*ppvObject = NULL; 
		return E_NOINTERFACE; 
	} 
	virtual ULONG STDMETHODCALLTYPE AddRef(void) {
			
			return 1000; 
	} 
	virtual ULONG STDMETHODCALLTYPE Release(void) {
				
			return 1000; 
	}
	virtual HRESULT STDMETHODCALLTYPE Event(
		/* [in] */ __RPC__in_opt IDebugEngine2 *pEngine,
		/* [in] */ __RPC__in_opt IDebugProcess2 *pProcess,
		/* [in] */ __RPC__in_opt IDebugProgram2 *pProgram,
		/* [in] */ __RPC__in_opt IDebugThread2 *pThread,
		/* [in] */ __RPC__in_opt IDebugEvent2 *pEvent,
		/* [in] */ __RPC__in REFIID riidEvent,
		/* [in] */ DWORD dwAttrib) {
		// Event
		if (!memcmp(&riidEvent, &IID_IDebugEngineCreateEvent2, sizeof(IID))) {
			IDebugEngineCreateEvent2 * pevent = NULL;
			if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugEngineCreateEvent2, (void**)&pevent))) {
			}
		} else if (!memcmp(&riidEvent, &IID_IDebugProgramCreateEvent2, sizeof(IID))) {

		}
		printf("TestCallback.Event() is called\n");
		return S_OK;
	}
};
#endif

void testEngine() {

	wprintf(L"Testing new line input\n");
	for (int i = 0; i < 10000000; i++) {
		wchar_t * line = NULL;
		int res = readline_poll("(gdb) ", &line);
		if (res == READLINE_READY) {
			wprintf(L"Line: %s\n", line);
			free(line);
		}
		else if (res == READLINE_CTRL_C) {
			wprintf(L"Ctrl+C is pressed\n");
		}
		else if (res == READLINE_ERROR)
			break;
		if (i > 0 && (i % 10) == 0) {
			readline_interrupt();
			printf("Some additional lines - input interrupted\n");
			printf("And one more line\n");
			printf("Third line\n");
		}
	}

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
