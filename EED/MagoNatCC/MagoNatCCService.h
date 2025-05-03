// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

// This file defines the CMagoNatCCService class, which is the one and only
// COM object exported from the sample dll.

#include "MagoNatCC.Contract.h"

struct IEnumDebugPropertyInfo2;
struct IEnumDebugPropertyInfoAsync;

class ATL_NO_VTABLE CMagoNatCCService :
    // Inherit from CMagoNatCCServiceContract to provide the list of interfaces that
    // this class implements (interface list comes from MagoNatCC.vsdconfigxml)
    public CMagoNatCCServiceContract,

    // Inherit from CComObjectRootEx to provide ATL support for reference counting and
    // object creation.
    public CComObjectRootEx<CComMultiThreadModel>,

    // Inherit from CComCoClass to provide ATL support for exporting this class from
    // DllGetClassObject
    public CComCoClass<CMagoNatCCService, &CMagoNatCCServiceContract::ClassId>
{
protected:
    CMagoNatCCService()
    {
    }
    ~CMagoNatCCService()
    {
    }

    HRESULT STDMETHODCALLTYPE _GetItems(
        _In_ Evaluation::DkmEvaluationResultEnumContext* pEnumContext,
        _In_ IEnumDebugPropertyInfo2* pEnum,
        _In_ UINT32 StartIndex,
        _In_ UINT32 Count,
        _Out_ DkmArray<Evaluation::DkmEvaluationResult*>& Items);
    HRESULT STDMETHODCALLTYPE _GetItemsAsync(
        _In_ Evaluation::DkmEvaluationResultEnumContext* pEnumContext,
        _In_ IEnumDebugPropertyInfoAsync* pEnum,
        _In_ UINT32 Count,
        _In_ IDkmCompletionRoutine<Evaluation::DkmGetChildrenAsyncResult>* pCompletionGetChildren,
        _In_ IDkmCompletionRoutine<Evaluation::DkmEvaluationEnumAsyncResult>* pCompletionRoutine);

public:
    DECLARE_NO_REGISTRY();
    DECLARE_NOT_AGGREGATABLE(CMagoNatCCService);

public:
    // IDkmLanguageExpressionEvaluator
    HRESULT STDMETHODCALLTYPE EvaluateExpression(
        _In_ Evaluation::DkmInspectionContext* pInspectionContext,
        _In_ DkmWorkList* pWorkList,
        _In_ Evaluation::DkmLanguageExpression* pExpression,
        _In_ CallStack::DkmStackWalkFrame* pStackFrame,
        _In_ IDkmCompletionRoutine<Evaluation::DkmEvaluateExpressionAsyncResult>* pCompletionRoutine
    );

    HRESULT STDMETHODCALLTYPE GetChildren(
        _In_ Evaluation::DkmEvaluationResult* pResult,
        _In_ DkmWorkList* pWorkList,
        _In_ UINT32 InitialRequestSize,
        _In_ Evaluation::DkmInspectionContext* pInspectionContext,
        _In_ IDkmCompletionRoutine<Evaluation::DkmGetChildrenAsyncResult>* pCompletionRoutine
    );

    HRESULT STDMETHODCALLTYPE GetFrameLocals(
        _In_ Evaluation::DkmInspectionContext* pInspectionContext,
        _In_ DkmWorkList* pWorkList,
        _In_ CallStack::DkmStackWalkFrame* pStackFrame,
        _In_ IDkmCompletionRoutine<Evaluation::DkmGetFrameLocalsAsyncResult>* pCompletionRoutine
    );

    HRESULT STDMETHODCALLTYPE GetFrameArguments(
        _In_ Evaluation::DkmInspectionContext* pInspectionContext,
        _In_ DkmWorkList* pWorkList,
        _In_ CallStack::DkmStackWalkFrame* pFrame,
        _In_ IDkmCompletionRoutine<Evaluation::DkmGetFrameArgumentsAsyncResult>* pCompletionRoutine
    );

    HRESULT STDMETHODCALLTYPE GetItems(
        _In_ Evaluation::DkmEvaluationResultEnumContext* pEnumContext,
        _In_ DkmWorkList* pWorkList,
        _In_ UINT32 StartIndex,
        _In_ UINT32 Count,
        _In_ IDkmCompletionRoutine<Evaluation::DkmEvaluationEnumAsyncResult>* pCompletionRoutine
    );

    HRESULT STDMETHODCALLTYPE SetValueAsString(
        _In_ Evaluation::DkmEvaluationResult* pResult,
        _In_ DkmString* pValue,
        _In_ UINT32 Timeout,
        _Deref_out_opt_ DkmString** ppErrorText
    );

    HRESULT STDMETHODCALLTYPE GetUnderlyingString(
        _In_ Evaluation::DkmEvaluationResult* pResult,
        _Deref_out_opt_ DkmString** ppStringValue
    );

    // IDkmLanguageReturnValueEvaluator
    virtual HRESULT STDMETHODCALLTYPE EvaluateReturnValue(
        _In_ Evaluation::DkmInspectionContext* pInspectionContext,
        _In_ DkmWorkList* pWorkList,
        _In_ CallStack::DkmStackWalkFrame* pStackFrame,
        _In_ Evaluation::DkmRawReturnValue* pRawReturnValue,
        _In_ IDkmCompletionRoutine<Evaluation::DkmEvaluateReturnValueAsyncResult>* pCompletionRoutine
    );

    // IDkmExceptionTriggerHitNotification
    virtual HRESULT STDMETHODCALLTYPE OnExceptionTriggerHit(
        _In_ Exceptions::DkmExceptionTriggerHit* pHit,
        _In_ DkmEventDescriptorS* pEventDescriptor
    );

    // IDkmNativeSteppingCallSiteProvider
    virtual HRESULT STDMETHODCALLTYPE GetSteppingCallSites(
        _In_ Native::DkmNativeInstructionAddress* pNativeAddress,
        _In_ const DkmArray<Symbols::DkmSteppingRange>& SteppingRanges,
        _Out_ DkmArray<Stepping::DkmNativeSteppingCallSite*>* pCallSites
    );

    // IDkmNativeJustMyCodeProvider158
    virtual HRESULT STDMETHODCALLTYPE IsUserCodeExtended(
        _In_ Native::DkmNativeInstructionAddress* pNativeAddress,
        _In_ DkmWorkList* pWorkList,
        _In_ IDkmCompletionRoutine<Native::DkmIsUserCodeExtendedAsyncResult>* pCompletionRoutine
    );

    // IDkmLanguageFrameDecoder
    virtual HRESULT STDMETHODCALLTYPE GetFrameName(
        _In_ Evaluation::DkmInspectionContext* pInspectionContext,
        _In_ DkmWorkList* pWorkList,
        _In_ CallStack::DkmStackWalkFrame* pFrame,
        _In_ Evaluation::DkmVariableInfoFlags_t ArgumentFlags,
        _In_ IDkmCompletionRoutine<Evaluation::DkmGetFrameNameAsyncResult>* pCompletionRoutine
    );

    virtual HRESULT STDMETHODCALLTYPE GetFrameReturnType(
        _In_ Evaluation::DkmInspectionContext* pInspectionContext,
        _In_ DkmWorkList* pWorkList,
        _In_ CallStack::DkmStackWalkFrame* pFrame,
        _In_ IDkmCompletionRoutine<Evaluation::DkmGetFrameReturnTypeAsyncResult>* pCompletionRoutine
    );
};

OBJECT_ENTRY_AUTO(CMagoNatCCService::ClassId, CMagoNatCCService)
