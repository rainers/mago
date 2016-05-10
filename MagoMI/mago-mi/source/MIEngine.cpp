
#include "MIEngine.h"
#include "../../DebugEngine/MagoNatDE/PendingBreakpoint.h"
#include "../../DebugEngine/MagoNatDE/Program.h"
#include "../../DebugEngine/MagoNatDE/Engine.h"


// fake debug port

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



// Init ATL
class CThisExeModule : public CAtlExeModuleT <CThisExeModule>
{};
CThisExeModule _AtlModule;




// MIEngine

MIEngine::MIEngine() : engine(NULL), debugProcess(NULL), callback(NULL), debugPort(NULL) {
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
}

MIEngine::~MIEngine() {
	if (engine)
		engine->Release();
	if (debugPort)
		debugPort->Release();
}

HRESULT MIEngine::Init(IDebugEventCallback2 * callback) {
	HRESULT hr = S_OK;
	CComObject<Mago::Engine> * pengine = NULL;
	hr = CComObject<Mago::Engine>::CreateInstance(&pengine);
	if (FAILED(hr))
		return hr;
	this->engine = pengine;
	this->callback = callback;
	this->debugPort = new MIDebugPort();
}

HRESULT MIEngine::Launch(const wchar_t * pszExe,
	const wchar_t * pszArgs,
	const wchar_t * pszDir
	) {
	HRESULT hr = engine->LaunchSuspended(
		NULL, //pszMachine,
		debugPort, //IDebugPort2*          pPort,
		pszExe, //LPCOLESTR             pszExe,
		pszArgs, //LPCOLESTR             pszArgs,
		pszDir, //LPCOLESTR             pszDir,
		NULL, //BSTR                  bstrEnv,
		NULL, //LPCOLESTR             pszOptions,
		0, //LAUNCH_FLAGS          dwLaunchFlags,
		NULL, //DWORD                 hStdInput,
		NULL, //DWORD                 hStdOutput,
		NULL, //DWORD                 hStdError,
		callback, //IDebugEventCallback2* pCallback,
		&debugProcess //IDebugProcess2**      ppDebugProcess
		);
	if (FAILED(hr)) {
		printf("Launch failed. Result=%d\n", hr);
		return hr;
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
		return hr;
	}

	return hr;
}

HRESULT MIEngine::ResumeProcess() {
	HRESULT hr = engine->ResumeProcess(debugProcess);
	if (FAILED(hr)) {
		printf("Resume process failed: result=%d\n", hr);
		return hr;
	}
	return hr;
}
