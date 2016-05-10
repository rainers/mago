
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

HRESULT STDMETHODCALLTYPE MIEngine::QueryInterface(
	/* [in] */ REFIID riid,
	/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) {
	*ppvObject = NULL;
	return E_NOINTERFACE;
}
ULONG STDMETHODCALLTYPE MIEngine::AddRef(void) {
	return 1000;
}
ULONG STDMETHODCALLTYPE MIEngine::Release(void) {
	return 1000;
}

HRESULT STDMETHODCALLTYPE MIEngine::Event(
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
			return callback->OnDebugEngineCreated(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugProgramCreateEvent2, sizeof(IID))) {
		IDebugProgramCreateEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugProgramCreateEvent2, (void**)&pevent))) {
			return callback->OnDebugProgramCreated(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugProgramDestroyEvent2, sizeof(IID))) {
		IDebugProgramDestroyEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugProgramDestroyEvent2, (void**)&pevent))) {
			return callback->OnDebugProgramDestroy(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugLoadCompleteEvent2, sizeof(IID))) {
		IDebugLoadCompleteEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugLoadCompleteEvent2, (void**)&pevent))) {
			return callback->OnDebugLoadComplete(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugEntryPointEvent2, sizeof(IID))) {
		IDebugEntryPointEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugEntryPointEvent2, (void**)&pevent))) {
			return callback->OnDebugEntryPoint(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugThreadCreateEvent2, sizeof(IID))) {
		IDebugThreadCreateEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugThreadCreateEvent2, (void**)&pevent))) {
			return callback->OnDebugThreadCreate(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugThreadDestroyEvent2, sizeof(IID))) {
		IDebugThreadDestroyEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugThreadDestroyEvent2, (void**)&pevent))) {
			return callback->OnDebugThreadDestroy(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugStepCompleteEvent2, sizeof(IID))) {
		IDebugStepCompleteEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugStepCompleteEvent2, (void**)&pevent))) {
			return callback->OnDebugStepComplete(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugBreakEvent2, sizeof(IID))) {
		IDebugBreakEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugBreakEvent2, (void**)&pevent))) {
			return callback->OnDebugBreak(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugOutputStringEvent2, sizeof(IID))) {
		IDebugOutputStringEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugOutputStringEvent2, (void**)&pevent))) {
			return callback->OnDebugOutputString(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugModuleLoadEvent2, sizeof(IID))) {
		IDebugModuleLoadEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugModuleLoadEvent2, (void**)&pevent))) {
			return callback->OnDebugModuleLoad(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugSymbolSearchEvent2, sizeof(IID))) {
		IDebugSymbolSearchEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugSymbolSearchEvent2, (void**)&pevent))) {
			return callback->OnDebugSymbolSearch(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugBreakpointEvent2, sizeof(IID))) {
		IDebugBreakpointEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugBreakpointEvent2, (void**)&pevent))) {
			return callback->OnDebugBreakpoint(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugBreakpointBoundEvent2, sizeof(IID))) {
		IDebugBreakpointBoundEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugBreakpointBoundEvent2, (void**)&pevent))) {
			return callback->OnDebugBreakpointBound(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugBreakpointErrorEvent2, sizeof(IID))) {
		IDebugBreakpointErrorEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugBreakpointErrorEvent2, (void**)&pevent))) {
			return callback->OnDebugBreakpointError(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugBreakpointUnboundEvent2, sizeof(IID))) {
		IDebugBreakpointUnboundEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugBreakpointUnboundEvent2, (void**)&pevent))) {
			return callback->OnDebugBreakpointUnbound(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugExceptionEvent2, sizeof(IID))) {
		IDebugExceptionEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugExceptionEvent2, (void**)&pevent))) {
			return callback->OnDebugException(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	else if (!memcmp(&riidEvent, &IID_IDebugMessageEvent2, sizeof(IID))) {
		IDebugMessageEvent2 * pevent = NULL;
		if (SUCCEEDED(pEvent->QueryInterface(IID_IDebugMessageEvent2, (void**)&pevent))) {
			return callback->OnDebugMessage(pEngine, pProcess, pProgram, pThread, pevent);
		}
		return E_NOINTERFACE;
	}
	printf("TestCallback.Event() is called\n");
	return S_OK;
}

HRESULT MIEngine::Init(MIEventCallback * callback) {
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
		this, //IDebugEventCallback2* pCallback,
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
		this, //IDebugEventCallback2* pCallback,
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
