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

//#include <atlbase.h>
#include "../../DebugEngine/MagoNatDE/Common.h"
#include "../../DebugEngine/MagoNatDE/PendingBreakpoint.h"
#include "../../DebugEngine/MagoNatDE/Program.h"
#include "../../DebugEngine/MagoNatDE/Engine.h"

#define IUNKNOWN_STUB() \
	virtual HRESULT STDMETHODCALLTYPE QueryInterface( \
		/* [in] */ REFIID riid, \
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) { \
		*ppvObject = NULL; \
		return E_NOINTERFACE; \
	} \
	virtual ULONG STDMETHODCALLTYPE AddRef(void) { \
		return 1000; \
	} \
	virtual ULONG STDMETHODCALLTYPE Release(void) { \
		return 1000; \
	}

#define ADDREF_RELEASE_STUB() \
	virtual ULONG STDMETHODCALLTYPE AddRef(void) { \
		return 1000; \
	} \
	virtual ULONG STDMETHODCALLTYPE Release(void) { \
		return 1000; \
	}


class MIDebugPort;
class MIDebugProcess : public IDebugProcess2 {
public:
	MIDebugProcess(IDebugPort2* port, AD_PROCESS_ID id) : _id(id), _port(port) {}
	~MIDebugProcess() {}
private:
	AD_PROCESS_ID _id;
	IDebugPort2 * _port;
public:
	// IUnknown
	IUNKNOWN_STUB()

	//IDebugProcess2
	virtual HRESULT STDMETHODCALLTYPE GetInfo(
		/* [in] */ PROCESS_INFO_FIELDS Fields,
		/* [out] */ __RPC__out PROCESS_INFO *pProcessInfo) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE EnumPrograms(
		/* [out] */ __RPC__deref_out_opt IEnumDebugPrograms2 **ppEnum) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetName(
		/* [in] */ GETNAME_TYPE gnType,
		/* [out] */ __RPC__deref_out_opt BSTR *pbstrName) {
		*pbstrName = (wchar_t*)L"DebugProcess";
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetServer(
		/* [out] */ __RPC__deref_out_opt IDebugCoreServer2 **ppServer) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Terminate(void) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Attach(
		/* [in] */ __RPC__in_opt IDebugEventCallback2 *pCallback,
		/* [size_is][in] */ __RPC__in_ecount_full(celtSpecificEngines) GUID *rgguidSpecificEngines,
		/* [in] */ DWORD celtSpecificEngines,
		/* [length_is][size_is][out] */ __RPC__out_ecount_part(celtSpecificEngines, celtSpecificEngines) HRESULT *rghrEngineAttach) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE CanDetach(void) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Detach(void) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPhysicalProcessId(
		/* [out] */ __RPC__out AD_PROCESS_ID *pProcessId) {
		*pProcessId = _id;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetProcessId(
		/* [out] */ __RPC__out GUID *pguidProcessId) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetAttachedSessionName(
		/* [out] */ __RPC__deref_out_opt BSTR *pbstrSessionName) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE EnumThreads(
		/* [out] */ __RPC__deref_out_opt IEnumDebugThreads2 **ppEnum) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE CauseBreak(void) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPort(
		/* [out] */ __RPC__deref_out_opt IDebugPort2 **ppPort) {
		*ppPort = _port;
		return S_OK;
	}
};

typedef std::map<DWORD, RefPtr<MIDebugProcess>> MIProcessMap;

class MIDebugPort : public IDebugPort2, public IDebugDefaultPort2, public IDebugPortNotify2, public IDebugPortSupplier2 {
public:
	MIDebugPort() {}
	~MIDebugPort() {}
protected:
	MIProcessMap _processMap;
public:
	// IUnknown

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) {
		if (!memcmp(&riid, &IID_IDebugDefaultPort2, sizeof(IID))) {
			*ppvObject = (IDebugDefaultPort2*)this;
			return S_OK;
		}
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}

	ADDREF_RELEASE_STUB()


	virtual HRESULT STDMETHODCALLTYPE GetPortName(
			/* [out] */ __RPC__deref_out_opt BSTR *pbstrName) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPortId(
		/* [out] */ __RPC__out GUID *pguidPort) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPortRequest(
		/* [out] */ __RPC__deref_out_opt IDebugPortRequest2 **ppRequest) {
		*ppRequest = NULL;
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPortSupplier(
		/* [out] */ __RPC__deref_out_opt IDebugPortSupplier2 **ppSupplier) {
		*ppSupplier = (IDebugPortSupplier2*)this;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetProcess(
		/* [in] */ AD_PROCESS_ID ProcessId,
		/* [out] */ __RPC__deref_out_opt IDebugProcess2 **ppProcess) {

		MIProcessMap::iterator it = _processMap.find(ProcessId.ProcessId.dwProcessId);
		if (it != _processMap.end()) {
			*ppProcess = it->second.Get();
			// found existing process
			return S_OK;
		}
		// create new process
		MIDebugProcess * pProc = new MIDebugProcess(this, ProcessId);
		RefPtr<MIDebugProcess> proc = pProc;
		_processMap.insert(MIProcessMap::value_type(ProcessId.ProcessId.dwProcessId, proc));
		*ppProcess = pProc;
		return S_OK;
		//return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE EnumProcesses(
		/* [out] */ __RPC__deref_out_opt IEnumDebugProcesses2 **ppEnum) {
		return E_NOTIMPL;
	}


	// IDebugDefaultPort2
	virtual HRESULT STDMETHODCALLTYPE GetPortNotify(
		/* [out] */ __RPC__deref_out_opt IDebugPortNotify2 **ppPortNotify) {
		*ppPortNotify = (IDebugPortNotify2 *)this;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetServer(
		/* [out] */ __RPC__deref_out_opt IDebugCoreServer3 **ppServer) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE QueryIsLocal(void) {
		return E_NOTIMPL;
	}


	//IDebugPortNotify2
	virtual HRESULT STDMETHODCALLTYPE AddProgramNode(
		/* [in] */ __RPC__in_opt IDebugProgramNode2 *pProgramNode) {
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE RemoveProgramNode(
		/* [in] */ __RPC__in_opt IDebugProgramNode2 *pProgramNode) {
		return S_OK;
	}

	//IDebugPortSupplier2
	virtual HRESULT STDMETHODCALLTYPE GetPortSupplierName(
		/* [out] */ __RPC__deref_out_opt BSTR *pbstrName) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPortSupplierId(
		/* [out] */ __RPC__out GUID *pguidPortSupplier) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPort(
		/* [in] */ __RPC__in REFGUID guidPort,
		/* [out] */ __RPC__deref_out_opt IDebugPort2 **ppPort) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE EnumPorts(
		/* [out] */ __RPC__deref_out_opt IEnumDebugPorts2 **ppEnum) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE CanAddPort(void) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE AddPort(
		/* [in] */ __RPC__in_opt IDebugPortRequest2 *pRequest,
		/* [out] */ __RPC__deref_out_opt IDebugPort2 **ppPort) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE RemovePort(
		/* [in] */ __RPC__in_opt IDebugPort2 *pPort) {
		return E_NOTIMPL;
	}
};

class CThisExeModule : public CAtlExeModuleT <CThisExeModule>
{};
CThisExeModule _AtlModule;

class TestCallback : public CComObjectRootEx<CComMultiThreadModel>, public IDebugEventCallback2 {
public:
	TestCallback() {}
	~TestCallback() {}
	DECLARE_NOT_AGGREGATABLE(TestCallback)
	BEGIN_COM_MAP(TestCallback)
		COM_INTERFACE_ENTRY(IDebugEventCallback2)
	END_COM_MAP()
	//virtual ULONG STDMETHODCALLTYPE AddRef() {}
	//virtual ULONG STDMETHODCALLTYPE Release() {}
	//virtual HRESULT STDMETHODCALLTYPE QueryInterface(
	//	/* [in] */ REFIID riid,
	//	/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) {
	//	return E_NOTIMPL;
	//}
	virtual HRESULT STDMETHODCALLTYPE Event(
		/* [in] */ __RPC__in_opt IDebugEngine2 *pEngine,
		/* [in] */ __RPC__in_opt IDebugProcess2 *pProcess,
		/* [in] */ __RPC__in_opt IDebugProgram2 *pProgram,
		/* [in] */ __RPC__in_opt IDebugThread2 *pThread,
		/* [in] */ __RPC__in_opt IDebugEvent2 *pEvent,
		/* [in] */ __RPC__in REFIID riidEvent,
		/* [in] */ DWORD dwAttrib) {
		// Event
		printf("TestCallback.Event() is called\n");
		return S_OK;
	}
};

void testEngine() {
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	//_atl.RegisterTypeLib();
	CComObject<Mago::Engine> * engine = NULL;
	CComObject<Mago::Engine>::CreateInstance(&engine);
	//engine = new Mago::Engine();
	IDebugProcess2* debugProcess = NULL;
	CComObject<TestCallback> * callback = NULL;
	CComObject<TestCallback>::CreateInstance(&callback);

	MIDebugPort miDebugPort;
	//CComObject<MIDebugPort>::CreateInstance(&pPort);

	//info.Dir = executableInfo.dir.c_str();
	//info.Exe = executableInfo.exename.c_str();
	//info.CommandLine = executableInfo.exename.c_str();
	//CComPtr<IDebugDefaultPort2> spDefaultPort;
	//HRESULT hr = pPort->QueryInterface(&spDefaultPort);

	HRESULT hr = engine->LaunchSuspended(
		NULL, //pszMachine,
		&miDebugPort, //IDebugPort2*          pPort,
		executableInfo.exename.c_str(), //LPCOLESTR             pszExe,
		NULL, //LPCOLESTR             pszArgs,
		executableInfo.dir.c_str(), //LPCOLESTR             pszDir,
		NULL, //BSTR                  bstrEnv,
		NULL, //LPCOLESTR             pszOptions,
		0, //LAUNCH_FLAGS          dwLaunchFlags,
		NULL, //DWORD                 hStdInput,
		NULL, //DWORD                 hStdOutput,
		NULL, //DWORD                 hStdError,
		callback, //IDebugEventCallback2* pCallback,
		&debugProcess //IDebugProcess2**      ppDebugProcess
		);
	printf("Launched result=%d\n", hr);
	if (FAILED(hr)) {
		printf("Launch failed: result=%d\n", hr);
		return;
	}
	class ProgramGetter : public Mago::ProgramCallback {
	public:
		Mago::Program* program;
		ProgramGetter() : program(NULL) {}
		virtual bool    AcceptProgram(Mago::Program* program) {
			this->program = program;
			return true;
		}
	};
	ProgramGetter programCallback;
	engine->ForeachProgram(&programCallback);
	IDebugProgram2* rgpProgram = programCallback.program;
	IDebugProgramNode2* rgpProgramNode = NULL;
	hr = engine->Attach(
		&rgpProgram, //IDebugProgram2** rgpPrograms,
		&rgpProgramNode, //IDebugProgramNode2** rgpProgramNodes,
		1, //DWORD celtPrograms,
		callback, //IDebugEventCallback2* pCallback,
		ATTACH_REASON_LAUNCH //ATTACH_REASON dwReason
		);
	if (FAILED(hr)) {
		printf("Attach failed: result=%d\n", hr);
		return;
	}

	hr = engine->ResumeProcess(debugProcess);
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