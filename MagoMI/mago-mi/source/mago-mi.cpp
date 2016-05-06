// mago-mi.cpp: определяет точку входа для консольного приложения.
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "cmdline.h"
#include "cmdinput.h"

//#include "Common.h"
//#include "Process.h"
//#include "Module.h"
//#include "EventCallback.h"
//#include "DebuggerProxy.h"
#include "debugger.h"
//#include "Module.h"
//#include "Engine.h"


class _EventCallback : public IEventCallback
{
	Exec*       mExec;
	IModule*    mMod;
	bool        mHitBp;
	bool _loadCompleted;
	bool _processExited;

public:
	_EventCallback()
		: mExec(NULL),
		mMod(NULL),
		mHitBp(false),
		_loadCompleted(false),
		_processExited(false)
	{
	}

	~_EventCallback()
	{
		if (mMod != NULL)
			mMod->Release();
	}

	void SetExec(Exec* exec)
	{
		mExec = exec;
	}

	bool GetModule(IModule*& mod)
	{
		mod = mMod;
		mod->AddRef();
		return mod != NULL;
	}

	bool GetHitBp()
	{
		return mHitBp;
	}

	virtual void AddRef()
	{
	}

	virtual void Release()
	{
	}

	virtual void OnProcessStart(IProcess* process)
	{
		fprintf(stderr, "OnProcessStart\n");
	}

	virtual void OnProcessExit(IProcess* process, DWORD exitCode)
	{
		fprintf(stderr, "OnProcessExit\n");
		_processExited = true;
	}

	virtual void OnThreadStart(IProcess* process, Thread* thread)
	{
		fprintf(stderr, "OnThreadStart\n");
	}

	virtual void OnThreadExit(IProcess* process, DWORD threadId, DWORD exitCode)
	{
		fprintf(stderr, "OnThreadExit\n");
	}

	virtual void OnModuleLoad(IProcess* process, IModule* module)
	{
		char*   macName = "";

		switch (module->GetMachine())
		{
		case IMAGE_FILE_MACHINE_I386: macName = "x86"; break;
		case IMAGE_FILE_MACHINE_IA64: macName = "ia64"; break;
		case IMAGE_FILE_MACHINE_AMD64: macName = "x64"; break;
		}

		if (sizeof(Address) == sizeof(uintptr_t))
			printf("  %p %d %s '%ls'\n", module->GetImageBase(), module->GetSize(), macName, module->GetPath());
		else
			printf("  %08I64x %d %s '%ls'\n", module->GetImageBase(), module->GetSize(), macName, module->GetPath());

		if (mMod == NULL)
		{
			mMod = module;
			mMod->AddRef();
		}
	}

	virtual void OnModuleUnload(IProcess* process, Address baseAddr)
	{
		if (sizeof(Address) == sizeof(uintptr_t))
			printf("  %p\n", baseAddr);
		else
			printf("  %08I64x\n", baseAddr);
	}

	virtual void OnOutputString(IProcess* process, const wchar_t* outputString)
	{
		printf("  '%ls'\n", outputString);
	}


	bool GetProcessExited() {
		return _processExited;
	}
	bool GetLoadCompleted() {
		return _loadCompleted;
	}
	virtual void OnLoadComplete(IProcess* process, DWORD threadId)
	{
		fprintf(stderr, "OnLoadComplete\n");
		UINT_PTR    baseAddr = (UINT_PTR)mMod->GetImageBase();
		_loadCompleted = true;

		// 0x003C137A, 0x003C1395
		// 1137A, 11395

#if 0
		mExec->SetBreakpoint(process, baseAddr + 0x0001137A, (void*)33);
		mExec->SetBreakpoint(process, baseAddr + 0x00011395, (void*)17);

		//mExec->SetBreakpoint( process, 0x003C137A, (void*) 257 );

		//mExec->RemoveBreakpoint( process, 0x003C137A, (void*) 33 );
		//mExec->RemoveBreakpoint( process, 0x003C137A, (void*) 257 );

		//mExec->RemoveBreakpoint( process, 0x003C1395, (void*) 33 );

		//mExec->RemoveBreakpoint( process, 0x003C1395, (void*) 17 );
#endif
	}

	virtual RunMode OnException(IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec)
	{
		if (sizeof(Address) == sizeof(uintptr_t))
			printf("  %p %08x\n", exceptRec->ExceptionAddress, exceptRec->ExceptionCode);
		else
			printf("  %08I64x %08x\n", exceptRec->ExceptionAddress, exceptRec->ExceptionCode);
		//return RunMode_Break;
		return RunMode_Run;
	}

	virtual RunMode OnBreakpoint(IProcess* process, uint32_t threadId, Address address, bool embedded)
	{
		if (sizeof(Address) == sizeof(uintptr_t))
			printf("  breakpoint at %p\n", address);
		else
			printf("  breakpoint at %08I64x\n", address);

		mHitBp = true;

		UINT_PTR    baseAddr = (UINT_PTR)mMod->GetImageBase();

		//mExec->RemoveBreakpoint( process, baseAddr + 0x0001137A, (void*) 257 );
		//mExec->SetBreakpoint( process, baseAddr + 0x0001137A, (void*) 257 );
		//mExec->RemoveBreakpoint( process, baseAddr + 0x00011395, (void*) 129 );
		//mExec->SetBreakpoint( process, baseAddr + 0x00011395, (void*) 129 );

		return RunMode_Break;
	}

	virtual void OnStepComplete(IProcess* process, uint32_t threadId)
	{
		printf("  Step complete\n");
	}

	virtual void OnAsyncBreakComplete(IProcess* process, uint32_t threadId)
	{
	}

	virtual void OnError(IProcess* process, HRESULT hrErr, EventCode event)
	{
		printf("  *** ERROR: %08x while %d\n", hrErr, event);
	}

	virtual ProbeRunMode OnCallProbe(
		IProcess* process, uint32_t threadId, Address address, AddressRange& thunkRange)
	{
		return ProbeRunMode_Run;
	}
};

//int main(int argc, char *argv[])
int wmain(int argc, wchar_t* argv[])
{
	parseCommandLine(argc, argv);
	executableInfo.dumpParams();
	if (executableInfo.exename && !fileExists(executableInfo.exename)) {
		fprintf(stderr, "%s: no such file or directory", executableInfo.exename);
		//exit(4);
	}

	Debugger debugger;
	if (executableInfo.exename) {

		//Mago::Engine * engine;

		//engine = new Mago::Engine();
		//MagoNativeEngine * engine;
		//engine = new MagoNativeEngine();

		_EventCallback   callback;
		Exec        exec;
		HRESULT     hr = S_OK;
		LaunchInfo  info = { 0 };
		MagoCore::DebuggerProxy debuggerProxy;

		//Mago::Module * module = new Mago::Module();
		//delete module;
		callback.SetExec(&exec);
		hr = exec.Init(&callback, &debuggerProxy);
		if (FAILED(hr)) {
			fprintf(stderr, "Cannot start debugging");
			//goto Error;
			exit(10);
		}
		info.Dir = executableInfo.dir;
		info.Exe = executableInfo.exename;
		info.CommandLine = executableInfo.exename;
		RefPtr<IProcess> proc;
		hr = exec.Launch(&info, proc.Ref());
		if (FAILED(hr)) {
			fprintf(stderr, "Cannot start debugging");
			//goto Error;
			exit(10);
		}
		uint32_t    pid = 0;

		pid = proc->GetId();

		bool sawLoadCompleted = false;

		for (; !callback.GetProcessExited(); )
		{
			HRESULT hr = exec.WaitForEvent(1000);//DefaultTimeoutMillis
			if (hr == E_TIMEOUT)
				continue;

			if (FAILED(hr)) {
				fprintf(stderr, "Wait for event failed\n");
				exit(10);
			}
			if (FAILED(exec.DispatchEvent())) {
				fprintf(stderr, "Dispatch event failed\n");
				exit(10);
			}

			if (proc->IsStopped())
			{
				if (FAILED(exec.Continue(proc, true))) {
					fprintf(stderr, "Continue failed");
					exit(10);
				}

				//fprintf(stderr, "process is stopped\n");
				//if (!sawLoadCompleted && callback.GetLoadCompleted())
				//{
				//	fprintf(stderr, "saw load completed\n");
				//	sawLoadCompleted = true;
				//	//break;
				//}
				//else
				//{
				//	if (FAILED(exec.Continue(proc, true))) {
				//		fprintf(stderr, "Continue failed");
				//		exit(10);
				//	}
				//}
			}
		}

		if (!callback.GetProcessExited()) {

		}
	}

	class TestCmdInputCallback : public CmdInputCallback {
	public:
		virtual ~TestCmdInputCallback() {}
		/// called on new input line
		virtual void onInputLine(std::wstring &s) {
			fwprintf(stdout, L"Input line: %s\n", s.c_str());
		}
		/// called when ctrl+c or ctrl+break is called
		virtual void onCtrlBreak() {
			fwprintf(stdout, L"Ctrl+Break is pressed\n");
		}
	};

	TestCmdInputCallback inputCallback;
	CmdInput cmdInput;
	cmdInput.setCallback(&inputCallback);
	while (!cmdInput.isClosed())
		cmdInput.poll();
	printf("End of stdin");

	return 0;
}

