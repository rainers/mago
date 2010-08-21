/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "SingleDocumentContext.h"


namespace Mago
{
    HRESULT SingleDocumentContext::EnumCodeContexts( 
       IEnumDebugCodeContexts2** ppEnumCodeCxts
    )
    {
        _ASSERT( false );
        UNREFERENCED_PARAMETER( ppEnumCodeCxts );
        return E_FAIL;
    }

    HRESULT SingleDocumentContext::Init(
        const wchar_t* filename,
        TEXT_POSITION& statementBegin,
        TEXT_POSITION& statementEnd,
        const wchar_t* langName,
        const GUID& langGuid )
    {
        return DocumentContext::Init( filename, statementBegin, statementEnd, langName, langGuid );
    }
}
