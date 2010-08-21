/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class ATL_NO_VTABLE DocumentContext : 
        public IDebugDocumentContext2
    {
        TEXT_POSITION               mStatementBegin;
        TEXT_POSITION               mStatementEnd;
        CComBSTR                    mFilename;
        CComBSTR                    mLangName;
        GUID                        mLangGuid;
        //CComPtr<IEnumDebugCodeContexts2>    mEnumContext;

    public:
        DocumentContext();
        ~DocumentContext();

        //////////////////////////////////////////////////////////// 
        // IDebugDocumentContext2 

        STDMETHOD( GetDocument )( IDebugDocument2** ppDocument );
        STDMETHOD( GetName )( 
            GETNAME_TYPE gnType,
            BSTR*        pbstrFileName );
        //STDMETHOD( EnumCodeContexts )( IEnumDebugCodeContexts2** ppEnumCodeCxts );
        STDMETHOD( GetLanguageInfo )( 
            BSTR* pbstrLanguage,
            GUID* pguidLanguage );
        STDMETHOD( GetStatementRange )(    
            TEXT_POSITION* pBegPosition,
            TEXT_POSITION* pEndPosition );
        STDMETHOD( GetSourceRange )( 
            TEXT_POSITION* pBegPosition,
            TEXT_POSITION* pEndPosition );
        STDMETHOD( Compare )(    
            DOCCONTEXT_COMPARE       compare,
            IDebugDocumentContext2** rgpDocContextSet,
            DWORD                    dwDocContextSetLen,
            DWORD*                   pdwDocContext );
        STDMETHOD( Seek )( 
            int                      nCount,
            IDebugDocumentContext2** ppDocContext );

    public:
        HRESULT Init(
            const wchar_t* filename,
            TEXT_POSITION& statementBegin,
            TEXT_POSITION& statementEnd,
            const wchar_t* langName,
            const GUID& langGuid );
    };
}
