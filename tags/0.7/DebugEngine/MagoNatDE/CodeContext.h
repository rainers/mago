/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class Module;


    [uuid("D3EC4955-6944-411D-92DD-FEEC3C41AA7C")]
    interface IMagoMemoryContext : IUnknown
    {
        STDMETHOD( GetAddress )( Address& address ) = 0;
    };


    class ATL_NO_VTABLE CodeContext : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugCodeContext2,
        public IMagoMemoryContext
    {
        Address         mAddr;
        RefPtr<Module>  mModule;

        MagoST::SymHandle   mFuncSH;
        MagoST::SymHandle   mBlockSH;

        CComPtr<IDebugDocumentContext2> mDocContext;

    public:
        CodeContext();
        ~CodeContext();

    DECLARE_NOT_AGGREGATABLE(CodeContext)

    BEGIN_COM_MAP(CodeContext)
        COM_INTERFACE_ENTRY(IDebugCodeContext2)
        COM_INTERFACE_ENTRY(IDebugMemoryContext2)
        COM_INTERFACE_ENTRY(IMagoMemoryContext)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugMemoryContext2 

        STDMETHOD( GetName )( 
            BSTR* pbstrName );
        STDMETHOD( GetInfo )( 
            CONTEXT_INFO_FIELDS dwFields,
            CONTEXT_INFO*       pInfo );
        STDMETHOD( Add )( 
            UINT64                 dwCount,
            IDebugMemoryContext2** ppMemCxt );
        STDMETHOD( Subtract )( 
            UINT64                 dwCount,
            IDebugMemoryContext2** ppMemCxt );
        STDMETHOD( Compare )( 
            CONTEXT_COMPARE        compare,
            IDebugMemoryContext2** rgpMemoryContextSet,
            DWORD                  dwMemoryContextSetLen,
            DWORD*                 pdwMemoryContext );

        //////////////////////////////////////////////////////////// 
        // IDebugCodeContext2 

        STDMETHOD( GetDocumentContext )( 
            IDebugDocumentContext2** ppSrcCxt );
        STDMETHOD( GetLanguageInfo )( 
            BSTR* pbstrLanguage,
            GUID* pguidLanguage );

        //////////////////////////////////////////////////////////// 
        // IMagoCodeContext 

        STDMETHOD( GetAddress )( Address& addr );

    public:
        static HRESULT MakeCodeContext( 
            Address newAddr, 
            Module* mod,
            IDebugDocumentContext2* docContext, 
            IDebugMemoryContext2** ppMemCxt );

        HRESULT Init( Address addr, Module* mod, IDebugDocumentContext2* docContext );

    private:
        HRESULT FindFunction();
        BSTR GetFunctionName();
        TEXT_POSITION GetFunctionOffset();
    };
}
