
#include "MIEngine.h"
#include "../../DebugEngine/MagoNatDE/PendingBreakpoint.h"
#include "../../DebugEngine/MagoNatDE/Program.h"
#include "../../DebugEngine/MagoNatDE/Engine.h"


// fake debug port

#define IUNKNOWN_STUB() \
	virtual HRESULT STDMETHODCALLTYPE QueryInterface( \
		/* [in] */ REFIID riid, \
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) { \
		UNREFERENCED_PARAMETER(riid); \
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
		UNREFERENCED_PARAMETER(Fields);
		UNREFERENCED_PARAMETER(pProcessInfo);
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE EnumPrograms(
		/* [out] */ __RPC__deref_out_opt IEnumDebugPrograms2 **ppEnum) {
		UNREFERENCED_PARAMETER(ppEnum);
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetName(
		/* [in] */ GETNAME_TYPE gnType,
		/* [out] */ __RPC__deref_out_opt BSTR *pbstrName) {
		UNREFERENCED_PARAMETER(gnType);
		*pbstrName = SysAllocString(L"DebugProcess");
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetServer(
		/* [out] */ __RPC__deref_out_opt IDebugCoreServer2 **ppServer) {
		UNREFERENCED_PARAMETER(ppServer);
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
		UNREFERENCED_PARAMETER(pCallback);
		UNREFERENCED_PARAMETER(rgguidSpecificEngines);
		UNREFERENCED_PARAMETER(celtSpecificEngines);
		UNREFERENCED_PARAMETER(rghrEngineAttach);
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
		UNREFERENCED_PARAMETER(pguidProcessId);
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetAttachedSessionName(
		/* [out] */ __RPC__deref_out_opt BSTR *pbstrSessionName) {
		UNREFERENCED_PARAMETER(pbstrSessionName);
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE EnumThreads(
		/* [out] */ __RPC__deref_out_opt IEnumDebugThreads2 **ppEnum) {
		UNREFERENCED_PARAMETER(ppEnum);
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

class MIDebugPort : /*public IDebugPort2, */ public IDebugDefaultPort2, public IDebugPortNotify2, public IDebugPortSupplier2 {
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
		UNREFERENCED_PARAMETER(pbstrName);
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPortId(
		/* [out] */ __RPC__out GUID *pguidPort) {
		UNREFERENCED_PARAMETER(pguidPort);
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
		UNREFERENCED_PARAMETER(ppEnum);
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
		UNREFERENCED_PARAMETER(ppServer);
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE QueryIsLocal(void) {
		return E_NOTIMPL;
	}


	//IDebugPortNotify2
	virtual HRESULT STDMETHODCALLTYPE AddProgramNode(
		/* [in] */ __RPC__in_opt IDebugProgramNode2 *pProgramNode) {
		UNREFERENCED_PARAMETER(pProgramNode);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE RemoveProgramNode(
		/* [in] */ __RPC__in_opt IDebugProgramNode2 *pProgramNode) {
		UNREFERENCED_PARAMETER(pProgramNode);
		return S_OK;
	}

	//IDebugPortSupplier2
	virtual HRESULT STDMETHODCALLTYPE GetPortSupplierName(
		/* [out] */ __RPC__deref_out_opt BSTR *pbstrName) {
		UNREFERENCED_PARAMETER(pbstrName);
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPortSupplierId(
		/* [out] */ __RPC__out GUID *pguidPortSupplier) {
		UNREFERENCED_PARAMETER(pguidPortSupplier);
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPort(
		/* [in] */ __RPC__in REFGUID guidPort,
		/* [out] */ __RPC__deref_out_opt IDebugPort2 **ppPort) {
		UNREFERENCED_PARAMETER(guidPort);
		UNREFERENCED_PARAMETER(ppPort);
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE EnumPorts(
		/* [out] */ __RPC__deref_out_opt IEnumDebugPorts2 **ppEnum) {
		UNREFERENCED_PARAMETER(ppEnum);
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE CanAddPort(void) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE AddPort(
		/* [in] */ __RPC__in_opt IDebugPortRequest2 *pRequest,
		/* [out] */ __RPC__deref_out_opt IDebugPort2 **ppPort) {
		UNREFERENCED_PARAMETER(pRequest);
		UNREFERENCED_PARAMETER(ppPort);
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE RemovePort(
		/* [in] */ __RPC__in_opt IDebugPort2 *pPort) {
		UNREFERENCED_PARAMETER(pPort);
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
	UNREFERENCED_PARAMETER(riid);
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
	/* [in] */ DWORD dwAttrib) 
{
	// Event
	UNREFERENCED_PARAMETER(dwAttrib);
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

HRESULT MIEngine::Init(MIEventCallback * eventCallback) {
	HRESULT hr = S_OK;
	CComObject<Mago::Engine> * pengine = NULL;
	hr = CComObject<Mago::Engine>::CreateInstance(&pengine);
	if (FAILED(hr))
		return hr;
	this->engine = pengine;
	this->callback = eventCallback;
	this->debugPort = new MIDebugPort();
	return S_OK;
}

HRESULT MIEngine::Launch(
		const wchar_t * pszExe,
		const wchar_t * pszArgs,
		const wchar_t * pszDir,
		const wchar_t * pszTerminalNamedPipe
	) {
	HANDLE hPipe = NULL;
	if (pszTerminalNamedPipe && pszTerminalNamedPipe[0]) {
		CRLog::error("Terminal named pipe: %s", toUtf8z(pszTerminalNamedPipe));
		SECURITY_ATTRIBUTES sa;
		memset(&sa, 0, sizeof(sa));
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;
		hPipe = CreateFile(
			pszTerminalNamedPipe,   // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE,
			0,              // no sharing 
			&sa,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe
			FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, // flags and attributes  
			NULL);
		if (hPipe == INVALID_HANDLE_VALUE) {
			CRLog::error("Cannot open terminal named pipe %s result code is %d", toUtf8z(pszTerminalNamedPipe), GetLastError());
			return E_FAIL;
		}
		//DWORD mode = PIPE_READMODE_BYTE | PIPE_WAIT;
		//DWORD mode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
		//SetNamedPipeHandleState(hPipe, PIPE_READMODE_BYTE | PIPE_WAIT, NULL, NULL);
		//SetNamedPipeHandleState(hPipe, &mode, NULL, NULL);
		//DWORD bytesWritten = 0;
		//WriteFile(hPipe, "test\n", 5, &bytesWritten, NULL);
		//WriteFile(hPipe, "test2\n", 5, &bytesWritten, NULL);
	}
	HRESULT hr = engine->LaunchSuspended(
		NULL, //pszMachine,
		debugPort, //IDebugPort2*          pPort,
		pszExe, //LPCOLESTR             pszExe,
		pszArgs, //LPCOLESTR             pszArgs,
		pszDir, //LPCOLESTR             pszDir,
		NULL, //BSTR                  bstrEnv,
		NULL, //LPCOLESTR             pszOptions,
		0, //LAUNCH_FLAGS          dwLaunchFlags,
		(DWORD)hPipe, //DWORD                 hStdInput,
		(DWORD)hPipe, //DWORD                 hStdOutput,
		(DWORD)hPipe, //DWORD                 hStdError,
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
		virtual bool AcceptProgram(Mago::Program* programItem) {
			this->program = programItem;
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

class MIBreakpointRequest
	: public CComObjectRootEx<CComMultiThreadModel>, public IDebugBreakpointRequest2, public IDebugDocumentPosition2, public IDebugFunctionPosition2
{
	BreakpointInfo bpInfo;
	BP_LOCATION_TYPE locationType;
public:
	MIBreakpointRequest() {
		locationType = BPLT_NONE;
	}
	~MIBreakpointRequest() {

	}

	DECLARE_NOT_AGGREGATABLE(MIBreakpointRequest)

	BEGIN_COM_MAP(MIBreakpointRequest)
		COM_INTERFACE_ENTRY(IDebugBreakpointRequest2)
		COM_INTERFACE_ENTRY(IDebugDocumentPosition2)
		COM_INTERFACE_ENTRY(IDebugFunctionPosition2)
	END_COM_MAP()

	//IDebugBreakpointRequest2
	virtual HRESULT STDMETHODCALLTYPE GetLocationType(
		/* [out] */ __RPC__out BP_LOCATION_TYPE *pBPLocationType) {
		*pBPLocationType = locationType;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetRequestInfo(
		/* [in] */ BPREQI_FIELDS dwFields,
		/* [out] */ __RPC__out BP_REQUEST_INFO *pBPRequestInfo) {
		memset(pBPRequestInfo, 0, sizeof(BP_REQUEST_INFO));
		if (dwFields & BPREQI_BPLOCATION) {
			pBPRequestInfo->dwFields |= BPREQI_BPLOCATION;
			pBPRequestInfo->bpLocation.bpLocationType = locationType;
			if (locationType == BPLT_CODE_FILE_LINE) {
				pBPRequestInfo->bpLocation.bpLocation.bplocCodeFileLine.pDocPos = this;
			}
			else if (locationType == BPLT_CODE_FUNC_OFFSET) {
				pBPRequestInfo->bpLocation.bpLocation.bplocCodeFuncOffset.pFuncPos = this;
			}

		}
		if (dwFields & BPREQI_FLAGS) {
			pBPRequestInfo->dwFields |= BPREQI_FLAGS;
			//pBPRequestInfo->dwFlags |= 
		}
		return S_OK;
	}

	// IDebugDocumentPosition2
	virtual HRESULT STDMETHODCALLTYPE GetFileName(
		/* [out] */ __RPC__deref_out_opt BSTR *pbstrFileName) 
	{
		*pbstrFileName = bpInfo.fileName.empty() ? NULL : SysAllocString(bpInfo.fileName.c_str());
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDocument(
		/* [out] */ __RPC__deref_out_opt IDebugDocument2 **ppDoc) {
		ppDoc = NULL;
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE IsPositionInDocument(
		/* [in] */ __RPC__in_opt IDebugDocument2 *pDoc) {
		UNREFERENCED_PARAMETER(pDoc);
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetRange(
		/* [full][out][in] */ __RPC__inout_opt TEXT_POSITION *pBegPosition,
		/* [full][out][in] */ __RPC__inout_opt TEXT_POSITION *pEndPosition) {
		pBegPosition->dwColumn = 1;
		pBegPosition->dwLine = bpInfo.line - 1;
		pEndPosition->dwColumn = 1;
		pEndPosition->dwLine = bpInfo.line - 1;
		return S_OK;
	}

	//IDebugFunctionPosition2

	virtual HRESULT STDMETHODCALLTYPE GetFunctionName(
		/* [out] */ __RPC__deref_out_opt BSTR *pbstrFunctionName) {
		*pbstrFunctionName = bpInfo.fileName.empty() ? NULL : SysAllocString(bpInfo.functionName.c_str());
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetOffset(
		/* [full][out][in] */ __RPC__inout_opt TEXT_POSITION *pPosition) {
		pPosition->dwLine = 0;
		pPosition->dwColumn = 0;
		return S_OK;
	}

	void init(BreakpointInfo * bp) {
		bpInfo = *bp;
		if (!bpInfo.fileName.empty() && bpInfo.line) {
			locationType = BPLT_CODE_FILE_LINE;
		} else if (!bpInfo.functionName.empty()) {
			locationType = BPLT_CODE_FUNC_OFFSET;
		}
		else if (!bpInfo.address.empty()) {
			locationType = BPLT_CODE_ADDRESS;
		}
	}
};

HRESULT MIEngine::CreatePendingBreakpoint(BreakpointInfoRef & bp) {
	RefPtr<MIBreakpointRequest> request;
	IDebugPendingBreakpoint2* pPendingBP;
	HRESULT hr = MakeCComObject(request);
	if (FAILED(hr)) {
		CRLog::error("Pending breakpoint request creation failed");
		return hr;
	}
	request->AddRef();
	request->init(bp);
	hr = engine->CreatePendingBreakpoint(request.Get(), &pPendingBP);
	if (FAILED(hr)) {
		CRLog::error("Pending breakpoint creation failed");
		return hr;
	}
	if (bp->enabled)
		pPendingBP->Enable(TRUE);
	bp->setPending(pPendingBP);
	return S_OK;
}
