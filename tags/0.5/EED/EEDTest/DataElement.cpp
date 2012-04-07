/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DataElement.h"

using namespace std;


Element* DataFactory::NewElement( const wchar_t* name )
{
    auto_ptr<Element>   elem;

    if ( _wcsicmp( name, L"variable" ) == 0 )
    {
        elem.reset( new VarDataElement() );
    }
    else if ( _wcsicmp( name, L"constant" ) == 0 )
    {
        elem.reset( new ConstantDataElement() );
    }
    else if ( _wcsicmp( name, L"field" ) == 0 )
    {
        elem.reset( new FieldDataElement() );
    }
    else if ( _wcsicmp( name, L"typedef" ) == 0 )
    {
        elem.reset( new TypeDefDataElement() );
    }
    else if ( _wcsicmp( name, L"struct" ) == 0 )
    {
        elem.reset( new StructDataElement() );
    }
    else if ( _wcsicmp( name, L"namespace" ) == 0 )
    {
        elem.reset( new NamespaceDataElement() );
    }
    else if ( _wcsicmp( name, L"typeref" ) == 0 )
    {
        elem.reset( new TypeRefDataElement() );
    }
    else if ( _wcsicmp( name, L"typepointer" ) == 0 )
    {
        elem.reset( new TypePointerDataElement() );
    }
    else if ( _wcsicmp( name, L"typesarray" ) == 0 )
    {
        elem.reset( new TypeSArrayDataElement() );
    }
    else if ( _wcsicmp( name, L"typedarray" ) == 0 )
    {
        elem.reset( new TypeDArrayDataElement() );
    }
    else if ( _wcsicmp( name, L"typeaarray" ) == 0 )
    {
        elem.reset( new TypeAArrayDataElement() );
    }
    else if ( _wcsicmp( name, L"null" ) == 0 )
    {
        elem.reset( new NullValueElement() );
    }
    else if ( _wcsicmp( name, L"intvalue" ) == 0 )
    {
        elem.reset( new IntValueElement() );
    }
    else if ( _wcsicmp( name, L"realvalue" ) == 0 )
    {
        elem.reset( new RealValueElement() );
    }
    else if ( _wcsicmp( name, L"complexvalue" ) == 0 )
    {
        elem.reset( new ComplexValueElement() );
    }
    else if ( _wcsicmp( name, L"structValue" ) == 0 )
    {
        elem.reset( new StructValueDataElement() );
    }
    else if ( _wcsicmp( name, L"fieldValue" ) == 0 )
    {
        elem.reset( new FieldValueDataElement() );
    }
    else if ( _wcsicmp( name, L"addressof" ) == 0 )
    {
        elem.reset( new AddressOfValueDataElement() );
    }
    else if ( _wcsicmp( name, L"arrayValue" ) == 0 )
    {
        elem.reset( new ArrayValueDataElement() );
    }
    else if ( _wcsicmp( name, L"sliceValue" ) == 0 )
    {
        elem.reset( new SliceValueDataElement() );
    }
    else if ( _wcsicmp( name, L"declRef" ) == 0 )
    {
        elem.reset( new DeclRefDataElement() );
    }

    return elem.release();
}


bool FindDeclaration( const wchar_t* namePath, MagoEE::ITypeEnv* typeEnv, IScope* topScope, MagoEE::Declaration*& outDecl )
{
    HRESULT hr = S_OK;
    const wchar_t*  wordStart = namePath;
    const wchar_t*  p = namePath;
    wstring         name;
    RefPtr<MagoEE::Declaration> decl;

    for ( ; ; p++ )
    {
        if ( (*p == L'.') || (*p == L'\0') )
        {
            size_t  len = p - wordStart;

            if ( len == 0 )
                throw L"Empty identifiers not allowed.";

            name.clear();
            name.append( wordStart, len );
            wordStart = p + 1;

            // TODO: Declaration should dervice from IScope, and that would make this simpler
            if ( decl.Get() == NULL )
                hr = topScope->FindObject( name.c_str(), decl.Ref() );
            else
            {
                RefPtr<MagoEE::Declaration> oldDecl;

                oldDecl.Attach( decl.Detach() );
                hr = oldDecl->FindObject( name.c_str(), decl.Ref() );
            }
            _ASSERT( hr == S_OK );

            if ( *p == L'\0' )
                break;
        }
    }

    outDecl = decl.Detach();
    return outDecl != NULL;
}
