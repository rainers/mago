/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class Thread;
    class ExprContext;
    class IRegisterSet;


    class StackFrame : 
        public CComObjectRootEx< CComMultiThreadModel >,
        public IDebugStackFrame2
    {
        struct LineInfo
        {
            CComBSTR        Filename;
            Address         Address;
            TEXT_POSITION   LineBegin;
            TEXT_POSITION   LineEnd;
            CComBSTR        LangName;
            GUID            LangGuid;
        };

        RefPtr<IRegisterSet>            mRegSet;
        RefPtr<Thread>                  mThread;
        RefPtr<Module>                  mModule;
        Address                         mPC;

        MagoST::SymHandle               mFuncSH;
        MagoST::SymHandle               mBlockSH;

        RefPtr<ExprContext>             mExprContext;
        Guard                           mExprContextGuard;

    public:
        StackFrame();
        ~StackFrame();

    DECLARE_NOT_AGGREGATABLE(StackFrame)

    BEGIN_COM_MAP(StackFrame)
        COM_INTERFACE_ENTRY(IDebugStackFrame2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugStackFrame2 

        STDMETHOD( GetCodeContext )( 
           IDebugCodeContext2** ppCodeCxt );

        STDMETHOD( GetDocumentContext )( 
           IDebugDocumentContext2** ppCxt );

        STDMETHOD( GetName )( 
           BSTR* pbstrName );

        STDMETHOD( GetInfo )( 
           FRAMEINFO_FLAGS dwFieldSpec,
           UINT            nRadix,
           FRAMEINFO*      pFrameInfo );

        STDMETHOD( GetPhysicalStackRange )( 
           UINT64* paddrMin,
           UINT64* paddrMax );

        STDMETHOD( GetExpressionContext )( 
           IDebugExpressionContext2** ppExprCxt );

        STDMETHOD( GetLanguageInfo )( 
           BSTR* pbstrLanguage,
           GUID* pguidLanguage );

        STDMETHOD( GetDebugProperty )( 
           IDebugProperty2** ppDebugProp );

        STDMETHOD( EnumProperties )( 
           DEBUGPROP_INFO_FLAGS      dwFieldSpec,
           UINT                      nRadix,
           REFIID                    refiid,
           DWORD                     dwTimeout,
           ULONG*                    pcelt,
           IEnumDebugPropertyInfo2** ppEnum );

        STDMETHOD( GetThread )( 
           IDebugThread2** ppThread );

    public:
        void Init(
            Address pc,
            IRegisterSet* regSet,
            Thread* thread,
            Module* module );

    private:
        HRESULT GetLineInfo( LineInfo& info );

        HRESULT GetFunctionName( 
            FRAMEINFO_FLAGS flags, 
            UINT radix, 
            BSTR* funcName );

        HRESULT AppendFunctionNameWithSymbols( 
            FRAMEINFO_FLAGS flags, 
            UINT radix, 
            CString& fullName );

        HRESULT AppendFunctionNameWithAddress( 
            FRAMEINFO_FLAGS flags, 
            UINT radix, 
            CString& fullName );

        HRESULT AppendFunctionNameWithSymbols( 
            FRAMEINFO_FLAGS flags, 
            UINT radix, 
            MagoST::ISession* session,
            MagoST::ISymbolInfo* symInfo, 
            CString& fullName );

        HRESULT FindFunction();

        HRESULT MakeExprContext();
    };
}
