#pragma once

#include "../../DebugEngine/MagoNatDE/Common.h"

namespace Mago {
	class Engine;
};

//class IDebugProcess2;

class MIDebugPort;

class MIEngine {
protected:
	CComObject<Mago::Engine> * engine;
	IDebugProcess2* debugProcess;
	IDebugEventCallback2 * callback;
	MIDebugPort * debugPort;
public:
	MIEngine();
	~MIEngine();

	HRESULT Init(IDebugEventCallback2 * callback);

	HRESULT Launch(const wchar_t * pszExe,
		const wchar_t * pszArgs,
		const wchar_t * pszDir
		);
	HRESULT ResumeProcess();
};
