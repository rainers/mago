/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "SymUtil.h"
#include "DataElement.h"

//#include <dia2.h>


HRESULT FindBasicType( const wchar_t* name, MagoEE::ITypeEnv* typeEnv, MagoEE::Declaration*& decl )
{
    MagoEE::ENUMTY  ty = MagoEE::Tnone;

    if ( wcscmp( name, L"bool" ) == 0 )
        ty = MagoEE::Tbool;
    else if ( wcscmp( name, L"bit" ) == 0 )
        ty = MagoEE::Tbit;
    else if ( wcscmp( name, L"byte" ) == 0 )
        ty = MagoEE::Tint8;
    else if ( wcscmp( name, L"ubyte" ) == 0 )
        ty = MagoEE::Tuns8;
    else if ( wcscmp( name, L"short" ) == 0 )
        ty = MagoEE::Tint16;
    else if ( wcscmp( name, L"ushort" ) == 0 )
        ty = MagoEE::Tuns16;
    else if ( wcscmp( name, L"int" ) == 0 )
        ty = MagoEE::Tint32;
    else if ( wcscmp( name, L"uint" ) == 0 )
        ty = MagoEE::Tuns32;
    else if ( wcscmp( name, L"long" ) == 0 )
        ty = MagoEE::Tint64;
    else if ( wcscmp( name, L"ulong" ) == 0 )
        ty = MagoEE::Tuns64;
    else if ( wcscmp( name, L"float" ) == 0 )
        ty = MagoEE::Tfloat32;
    else if ( wcscmp( name, L"double" ) == 0 )
        ty = MagoEE::Tfloat64;
    else if ( wcscmp( name, L"real" ) == 0 )
        ty = MagoEE::Tfloat80;
    else if ( wcscmp( name, L"ifloat" ) == 0 )
        ty = MagoEE::Timaginary32;
    else if ( wcscmp( name, L"idouble" ) == 0 )
        ty = MagoEE::Timaginary64;
    else if ( wcscmp( name, L"ireal" ) == 0 )
        ty = MagoEE::Timaginary80;
    else if ( wcscmp( name, L"cfloat" ) == 0 )
        ty = MagoEE::Tcomplex32;
    else if ( wcscmp( name, L"cdouble" ) == 0 )
        ty = MagoEE::Tcomplex64;
    else if ( wcscmp( name, L"creal" ) == 0 )
        ty = MagoEE::Tcomplex80;
    else if ( wcscmp( name, L"char" ) == 0 )
        ty = MagoEE::Tchar;
    else if ( wcscmp( name, L"wchar" ) == 0 )
        ty = MagoEE::Twchar;
    else if ( wcscmp( name, L"dchar" ) == 0 )
        ty = MagoEE::Tdchar;
    else if ( wcscmp( name, L"void" ) == 0 )
        ty = MagoEE::Tvoid;

    MagoEE::Type*   type = typeEnv->GetType( ty );
    if ( type != NULL )
    {
        decl = new BasicTypeDefDataElement( type );
        decl->AddRef();
        return S_OK;
    }

    return E_FAIL;
}


#if 0
void PrintSymProps( IDiaSymbol* sym )
{
    HRESULT hr = S_OK;
    CComPtr<IDiaPropertyStorage>    props;

    hr = sym->QueryInterface( IID_IDiaPropertyStorage, (void**) &props );
    if ( FAILED( hr ) )
        return;

    CComPtr<IEnumSTATPROPSTG>   enumProps;
    STATPROPSTG                 prop = { 0 };
    ULONG                       fetchCount = 0;

    hr = props->Enum( &enumProps );
    if ( FAILED( hr ) )
        return;

    while ( S_OK == enumProps->Next( 1, &prop, &fetchCount ) )
    {
        PROPSPEC pspec = { PRSPEC_PROPID, prop.propid };
        PROPVARIANT vt = { VT_EMPTY };

        if ( props->ReadMultiple( 1, &pspec, &vt ) == S_OK )
        {
            switch( vt.vt ){
                case VT_BOOL:
                    wprintf( L"%32s:\t %s\n", prop.lpwstrName, vt.bVal ? L"true" : L"false" );
                    break;
                case VT_I2:
                    wprintf( L"%32s:\t %d\n", prop.lpwstrName, vt.iVal );
                    break;
                case VT_UI2:
                    wprintf( L"%32s:\t %d\n", prop.lpwstrName, vt.uiVal );
                    break;
                case VT_I4:
                    wprintf( L"%32s:\t %d\n", prop.lpwstrName, vt.intVal );
                    break;
                case VT_UI4:
                    wprintf( L"%32s:\t 0x%0x\n", prop.lpwstrName, vt.uintVal );
                    break;
                case VT_UI8:
                    wprintf( L"%32s:\t 0x%x\n", prop.lpwstrName, vt.uhVal.QuadPart );
                    break;
                case VT_BSTR:
                    wprintf( L"%32s:\t %s\n", prop.lpwstrName, vt.bstrVal );
                    break;
                case VT_UNKNOWN:
                    wprintf( L"%32s:\t %p\n", prop.lpwstrName, vt.punkVal );
                    break;
                case VT_SAFEARRAY:
                    break;
                default:
                   break;
            }
            VariantClear((VARIANTARG*) &vt);
        }
    }
}
#endif
