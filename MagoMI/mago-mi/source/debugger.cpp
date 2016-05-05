#include "debugger.h"

void InitDebug()
{
	int f = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	f |= _CRTDBG_LEAK_CHECK_DF;     // should always use in debug build
	f |= _CRTDBG_CHECK_ALWAYS_DF;   // check on free AND alloc
	_CrtSetDbgFlag(f);

	//_CrtSetAllocHook( LocalMemAllocHook );
	//SetLocalMemWorkingSetLimit( 550 );
}

Debugger::Debugger() : _mod(NULL) {
	InitDebug();
}

Debugger::~Debugger() {
	if (_mod != NULL)
		_mod->Release();
}

void Debugger::AddRef()
{
}

void Debugger::Release()
{
}

void Debugger::OnProcessStart(IProcess* process)
{
	fprintf(stderr, "OnProcessStart\n");
}

void Debugger::OnProcessExit(IProcess* process, DWORD exitCode)
{
	fprintf(stderr, "OnProcessExit\n");
	//_processExited = true;
}

void Debugger::OnThreadStart(IProcess* process, Thread* thread)
{
	fprintf(stderr, "OnThreadStart\n");
}

void Debugger::OnThreadExit(IProcess* process, DWORD threadId, DWORD exitCode)
{
	fprintf(stderr, "OnThreadExit\n");
}

void Debugger::OnModuleLoad(IProcess* process, IModule* module)
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

	if (_mod == NULL)
	{
		_mod = module;
		_mod->AddRef();
	}
}

void Debugger::OnModuleUnload(IProcess* process, Address baseAddr)
{
	if (sizeof(Address) == sizeof(uintptr_t))
		printf("  %p\n", baseAddr);
	else
		printf("  %08I64x\n", baseAddr);
}

void Debugger::OnOutputString(IProcess* process, const wchar_t* outputString)
{
	printf("  '%ls'\n", outputString);
}


//bool Debugger::GetProcessExited() {
//	return _processExited;
//}
//bool GetLoadCompleted() {
//	return _loadCompleted;
//}
void Debugger::OnLoadComplete(IProcess* process, DWORD threadId)
{
	fprintf(stderr, "OnLoadComplete\n");
	UINT_PTR    baseAddr = (UINT_PTR)_mod->GetImageBase();
	//_loadCompleted = true;

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

RunMode Debugger::OnException(IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec)
{
	if (sizeof(Address) == sizeof(uintptr_t))
		printf("  %p %08x\n", exceptRec->ExceptionAddress, exceptRec->ExceptionCode);
	else
		printf("  %08I64x %08x\n", exceptRec->ExceptionAddress, exceptRec->ExceptionCode);
	//return RunMode_Break;
	return RunMode_Run;
}

RunMode Debugger::OnBreakpoint(IProcess* process, uint32_t threadId, Address address, bool embedded)
{
	if (sizeof(Address) == sizeof(uintptr_t))
		printf("  breakpoint at %p\n", address);
	else
		printf("  breakpoint at %08I64x\n", address);

	//mHitBp = true;

	UINT_PTR    baseAddr = (UINT_PTR)_mod->GetImageBase();

	//mExec->RemoveBreakpoint( process, baseAddr + 0x0001137A, (void*) 257 );
	//mExec->SetBreakpoint( process, baseAddr + 0x0001137A, (void*) 257 );
	//mExec->RemoveBreakpoint( process, baseAddr + 0x00011395, (void*) 129 );
	//mExec->SetBreakpoint( process, baseAddr + 0x00011395, (void*) 129 );

	return RunMode_Break;
}

void Debugger::OnStepComplete(IProcess* process, uint32_t threadId)
{
	printf("  Step complete\n");
}

void Debugger::OnAsyncBreakComplete(IProcess* process, uint32_t threadId)
{
}

void Debugger::OnError(IProcess* process, HRESULT hrErr, EventCode event)
{
	printf("  *** ERROR: %08x while %d\n", hrErr, event);
}

ProbeRunMode Debugger::OnCallProbe(
	IProcess* process, uint32_t threadId, Address address, AddressRange& thunkRange)
{
	return ProbeRunMode_Run;
}
