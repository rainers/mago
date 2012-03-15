/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#ifndef __METRIC_H__
#define __METRIC_H__


typedef const unsigned short* LPCWSTR_ushort;


// ------------------------------------------------------------------
// Predefined metric names

// "CLSID"
extern LPCWSTR_ushort metricCLSID;
// "Name"
extern LPCWSTR_ushort metricName;
// "Language"
extern LPCWSTR_ushort metricLanguage;



#ifndef NO_DBGMETRIC // if NO_DBGMETIC is defined, don't include functions

// ------------------------------------------------------------------
// General purpose metric routines

//HRESULT GetMetric(LPCWSTR_ushort pszMachine, LPCWSTR_ushort pszType, REFGUID guidSection, LPCWSTR_ushort pszMetric, VARIANT* pvarValue, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall GetMetric(LPCWSTR_ushort pszMachine, LPCWSTR_ushort pszType, REFGUID guidSection, LPCWSTR_ushort pszMetric, _Out_ DWORD* pdwValue, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall GetMetric(LPCWSTR_ushort pszMachine, LPCWSTR_ushort pszType, REFGUID guidSection, LPCWSTR_ushort pszMetric, BSTR* pbstrValue, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall GetMetric(LPCWSTR_ushort pszMachine, LPCWSTR_ushort pszType, REFGUID guidSection, LPCWSTR_ushort pszMetric, _Out_ GUID* pguidValue, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall GetMetric(LPCWSTR_ushort pszMachine, LPCWSTR_ushort pszType, REFGUID guidSection, LPCWSTR_ushort pszMetric, _Out_opt_cap_post_count_(*pdwSize, *pdwSize) GUID* rgguidValues, _Out_ DWORD* pdwSize, LPCWSTR_ushort pszAltRoot);

//HRESULT SetMetric(LPCWSTR_ushort pszType, REFGUID guidSection, LPCWSTR_ushort pszMetric, const VARIANT varValue);
HRESULT __stdcall SetMetric(LPCWSTR_ushort pszType, REFGUID guidSection, LPCWSTR_ushort pszMetric, const DWORD dwValue, bool fUserSpecific, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall SetMetric(LPCWSTR_ushort pszType, REFGUID guidSection, LPCWSTR_ushort pszMetric, LPCWSTR_ushort pszValue, bool fUserSpecific, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall SetMetric(LPCWSTR_ushort pszType, REFGUID guidSection, LPCWSTR_ushort pszMetric, REFGUID guidValue, bool fUserSpecific, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall SetMetric(LPCWSTR_ushort pszType, REFGUID guidSection, LPCWSTR_ushort pszMetric, _In_count_(dwSize) const GUID* rgguidValues, DWORD dwSize, bool fUserSpecific, LPCWSTR_ushort pszAltRoot);

HRESULT __stdcall EnumMetricSections(LPCWSTR_ushort pszMachine, LPCWSTR_ushort pszType, _Out_opt_cap_post_count_(*pdwSize, *pdwSize) GUID* rgguidSections, _Out_ DWORD* pdwSize, LPCWSTR_ushort pszAltRoot);

HRESULT __stdcall RemoveMetric(LPCWSTR_ushort pszType, REFGUID guidSection, LPCWSTR_ushort pszMetric, LPCWSTR_ushort pszAltRoot);

HRESULT __stdcall SetMetricLocale(WORD wLangId);
WORD __stdcall GetMetricLocale();

HRESULT ReadTextFileAsBstr(LPCWSTR_ushort szFileName, BSTR *pbstrFileContent);

#endif // end ifndef NO_DBGMETRIC



// Predefined metric types
// "Engine"
extern LPCWSTR_ushort metrictypeEngine;
// "PortSupplier"
extern LPCWSTR_ushort metrictypePortSupplier;
// "Exception"
extern LPCWSTR_ushort metrictypeException;
// "EEExtensions"
extern LPCWSTR_ushort metricttypeEEExtension;

// Predefined engine metric names
// AddressBP
extern LPCWSTR_ushort metricAddressBP;
// AlwaysLoadLocal
extern LPCWSTR_ushort metricAlwaysLoadLocal;
// LoadInDebuggeeSession
extern LPCWSTR_ushort metricLoadInDebuggeeSession;
// LoadedByDebuggee
extern LPCWSTR_ushort metricLoadedByDebuggee;
// Attach
extern LPCWSTR_ushort metricAttach;
// CallStackBP
extern LPCWSTR_ushort metricCallStackBP;
// ConditionalBP
extern LPCWSTR_ushort metricConditionalBP;
// DataBP
extern LPCWSTR_ushort metricDataBP;
// Disassembly
extern LPCWSTR_ushort metricDisassembly;
// Dump writing
extern LPCWSTR_ushort metricDumpWriting;
// ENC
extern LPCWSTR_ushort metricENC;
// Exceptions
extern LPCWSTR_ushort metricExceptions;
// FunctionBP
extern LPCWSTR_ushort metricFunctionBP;
// HitCountBP
extern LPCWSTR_ushort metricHitCountBP;
// JITDebug
extern LPCWSTR_ushort metricJITDebug;
// Memory
extern LPCWSTR_ushort metricMemory;
// Port supplier
extern LPCWSTR_ushort metricPortSupplier;
// Registers
extern LPCWSTR_ushort metricRegisters;
// SetNextStatement
extern LPCWSTR_ushort metricSetNextStatement;
// SuspendThread
extern LPCWSTR_ushort metricSuspendThread;
// WarnIfNoSymbols
extern LPCWSTR_ushort metricWarnIfNoSymbols;
// Filtering non-user frames
extern LPCWSTR_ushort metricShowNonUserCode;
// What CLSID provides program nodes?
extern LPCWSTR_ushort metricProgramProvider;
// Always load the program provider locally?
extern LPCWSTR_ushort metricAlwaysLoadProgramProviderLocal;
// Use engine to watch for process events instead of program provider?
extern LPCWSTR_ushort metricEngineCanWatchProcess;
// Can we do remote debugging?
extern LPCWSTR_ushort metricRemoteDebugging;
// Should the encmgr use native's encbuild.dll to build for enc?
extern LPCWSTR_ushort metricEncUseNativeBuilder;
// When debugging a 64-bit process under WOW, should we load the engine 'remotely'
// or in the devenv process (which is running under WOW64)
extern LPCWSTR_ushort metricLoadUnderWOW64;
// When debugging a 64-bit process under WOW, should we load the program provider
// 'remotely' or in the devenv process (which is running under WOW64)
extern LPCWSTR_ushort metricLoadProgramProviderUnderWOW64;
// Stop on unhandled exceptions thrown across app domain boundaries
extern LPCWSTR_ushort metricStopOnExceptionCrossingManagedBoundary;
// Warn user if there is no "user" code on launch
extern LPCWSTR_ushort metricWarnIfNoUserCodeOnLaunch;
// Priority for engine automatic selection (preference given to higher)
extern LPCWSTR_ushort metricAutoSelectPriority;
// engines not compatible with this engine (only for automatic engine selection)
extern LPCWSTR_ushort metricAutoSelectIncompatibleList;
// engines not compatible with this engine
extern LPCWSTR_ushort metricIncompatibleList;
// Disable JIT optimizations while debugging
extern LPCWSTR_ushort metricDisableJITOptimization;
// Default memory organization 0=little endian (most typical), 1=big endian
extern LPCWSTR_ushort metricBigEndian;

// Filtering non-user frames
extern LPCWSTR_ushort metricShowNonUserCode;

// Stepping in "user" code only
extern LPCWSTR_ushort metricJustMyCodeStepping;
// CLR Version that debuggee is going to use e.g. "v2.0.41104"
extern LPCWSTR_ushort metricCLRVersionForDebugging;
// Allow all threads to run when doing a funceval
extern LPCWSTR_ushort metricAllThreadsRunOnFuncEval;
// Use Shim API to get ICorDebug
extern LPCWSTR_ushort metricUseShimAPI;
// Attempt to map breakpoints in client-side script
extern LPCWSTR_ushort metricMapClientBreakpoints;
// Enable funceval quick abort
extern LPCWSTR_ushort metricEnableFuncEvalQuickAbort;
// Specify detour dll names for funceval quick abort
extern LPCWSTR_ushort metricFuncEvalQuickAbortDlls;
// Specify EXEs for which we shouldn't do FEQA
extern LPCWSTR_ushort metricFuncEvalQuickAbortExcludeList;


// Predefined EE metric names
// Engine
extern LPCWSTR_ushort metricEngine;
// Preload Modules
extern LPCWSTR_ushort metricPreloadModules;

// ThisObjectName
extern LPCWSTR_ushort metricThisObjectName;

// Predefined EE Extension metric names
// ExtensionDll
extern LPCWSTR_ushort metricExtensionDll;
// RegistersSupported
extern LPCWSTR_ushort metricExtensionRegistersSupported;
// RegistersEntryPoint
extern LPCWSTR_ushort metricExtensionRegistersEntryPoint;
// TypesSupported
extern LPCWSTR_ushort metricExtensionTypesSupported;
// TypesEntryPoint
extern LPCWSTR_ushort metricExtensionTypesEntryPoint;

// Predefined PortSupplier metric names
// PortPickerCLSID
extern LPCWSTR_ushort metricPortPickerCLSID;
// DisallowUserEnteredPorts
extern LPCWSTR_ushort metricDisallowUserEnteredPorts;
// PidBase
extern LPCWSTR_ushort metricPidBase;


#ifndef NO_DBGMETRIC // if NO_DBGMETIC is defined, don't include functions

// ------------------------------------------------------------------
// Engine-specific metric routines

HRESULT __stdcall EnumDebugEngines(LPCWSTR_ushort pszMachine, REFGUID guidPortSupplier, BOOL fRequireRemoteDebugging, _Out_opt_cap_post_count_(*pdwSize, *pdwSize) GUID* rgguidEngines, _Out_ DWORD* pdwSize, LPCWSTR_ushort pszAltRoot);

#endif // end ifndef NO_DBGMETRIC



#ifndef NO_DBGMETRIC // if NO_DBGMETIC is defined, don't include functions

// ------------------------------------------------------------------
// EE-specific metric routines

HRESULT __stdcall GetEEMetric(REFGUID guidLang, REFGUID guidVendor, LPCWSTR_ushort pszMetric, _Out_ DWORD* pdwValue, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall GetEEMetric(REFGUID guidLang, REFGUID guidVendor, LPCWSTR_ushort pszMetric, BSTR* pbstrValue, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall GetEEMetric(REFGUID guidLang, REFGUID guidVendor, LPCWSTR_ushort pszMetric, _Out_ GUID* pguidValue, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall GetEEMetric(REFGUID guidLang, REFGUID guidVendor, LPCWSTR_ushort pszMetric, _Out_opt_cap_post_count_(*pdwSize, *pdwSize) GUID* rgguidValues, _Out_ DWORD* pdwSize, LPCWSTR_ushort pszAltRoot);

HRESULT __stdcall SetEEMetric(REFGUID guidLang, REFGUID guidVendor, LPCWSTR_ushort pszMetric, DWORD dwValue, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall SetEEMetric(REFGUID guidLang, REFGUID guidVendor, LPCWSTR_ushort pszMetric, LPCWSTR_ushort pszValue, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall SetEEMetric(REFGUID guidLang, REFGUID guidVendor, LPCWSTR_ushort pszMetric, REFGUID guidValue, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall SetEEMetric(REFGUID guidLang, REFGUID guidVendor, LPCWSTR_ushort pszMetric, _In_count_(dwSize) const GUID* rgguidValues, DWORD dwSize, LPCWSTR_ushort pszAltRoot);

HRESULT __stdcall EnumEEs(_Out_opt_cap_post_count_(*pdwSize, *pdwSize) GUID* rgguidLang, _Out_opt_cap_post_count_(*pdwSize, *pdwSize) GUID* rgguidVendor, _Out_ DWORD* pdwSize, LPCWSTR_ushort pszAltRoot);

HRESULT __stdcall RemoveEEMetric(REFGUID guidLang, REFGUID guidVendor, LPCWSTR_ushort pszMetric, LPCWSTR_ushort pszAltRoot);

HRESULT __stdcall GetEEMetricFile(REFGUID guidLang, REFGUID guidVendor, LPCWSTR_ushort pszMetric, BSTR* pbstrValue, LPCWSTR_ushort pszAltRoot);

#endif // end ifndef NO_DBGMETRIC



#ifndef NO_DBGMETRIC // if NO_DBGMETIC is defined, don't include functions

// ------------------------------------------------------------------
// SP-specific metric routines

HRESULT __stdcall GetSPMetric(REFGUID guidSymbolType, LPCWSTR_ushort pszStoreType, LPCWSTR_ushort pszMetric, BSTR* pbstrValue, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall GetSPMetric(REFGUID guidSymbolType, LPCWSTR_ushort pszStoreType, LPCWSTR_ushort pszMetric, _Out_ GUID* pguidValue, LPCWSTR_ushort pszAltRoot);

HRESULT __stdcall SetSPMetric(REFGUID guidSymbolType, LPCWSTR_ushort pszStoreType, LPCWSTR_ushort pszMetric, LPCWSTR_ushort pszValue, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall SetSPMetric(REFGUID guidSymbolType, LPCWSTR_ushort pszStoreType, LPCWSTR_ushort pszMetric, REFGUID guidValue, LPCWSTR_ushort pszAltRoot);

HRESULT __stdcall RemoveSPMetric(REFGUID guidSymbolType, LPCWSTR_ushort pszStoreType, LPCWSTR_ushort pszMetric, LPCWSTR_ushort pszAltRoot);

#endif // end ifndef NO_DBGMETRIC



// Predefined SP store types
// "file"
extern LPCWSTR_ushort storetypeFile;
// "metadata"
extern LPCWSTR_ushort storetypeMetadata;


#ifndef NO_DBGMETRIC // if NO_DBGMETIC is defined, don't include functions

// ------------------------------------------------------------------
// Exception metric routines

HRESULT __stdcall GetExceptionMetric(REFGUID guidType, LPCWSTR_ushort pszException, _Out_opt_ DWORD* pdwState, _Out_opt_ DWORD* pdwCode, LPCWSTR_ushort pszAltRoot);

HRESULT __stdcall SetExceptionMetric(REFGUID guidType, LPCWSTR_ushort pszParent, LPCWSTR_ushort pszException, DWORD dwState, DWORD dwCode, LPCWSTR_ushort pszAltRoot);

HRESULT __stdcall EnumExceptionMetrics(REFGUID guidType, LPCWSTR_ushort pszParent, _Out_opt_cap_post_count_(*pdwSize, *pdwSize) BSTR* rgbstrExceptions, _Out_opt_cap_post_count_(*pdwSize, *pdwSize) DWORD* rgdwState, _Out_opt_cap_post_count_(*pdwSize, *pdwSize) DWORD* rgdwCode, _Out_ DWORD* pdwSize, LPCWSTR_ushort pszAltRoot);

HRESULT __stdcall RemoveExceptionMetric(REFGUID guidType, LPCWSTR_ushort pszParent, LPCWSTR_ushort pszException, LPCWSTR_ushort pszAltRoot);
HRESULT __stdcall RemoveAllExceptionMetrics(REFGUID guidType, LPCWSTR_ushort pszAltRoot);

#endif // end ifndef NO_DBGMETRIC


#endif // __METRIC_H__
