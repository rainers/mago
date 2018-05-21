/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "EED.h"
#include "TypeEnv.h"
#include "SimpleNameTable.h"
#include "Scanner.h"
#include "Parser.h"
#include "Expression.h"
#include "PropTables.h"
#include "EnumValues.h"


namespace MagoEE
{
    class EEDParsedExpr : public IEEDParsedExpr
    {
        long                mRefCount;
        RefPtr<Expression>  mExpr;
        RefPtr<NameTable>   mStrTable;      // expr holds refs to strings in here
        RefPtr<ITypeEnv>    mTypeEnv;       // eval will need this

    public:
        EEDParsedExpr( Expression* e, NameTable* strTable, ITypeEnv* typeEnv )
            :   mRefCount( 0 ),
                mExpr( e ),
                mStrTable( strTable ),
                mTypeEnv( typeEnv )
        {
            _ASSERT( e != NULL );
            _ASSERT( strTable != NULL );
            _ASSERT( typeEnv != NULL );
        }

        virtual void AddRef()
        {
            InterlockedIncrement( &mRefCount );
        }

        virtual void Release()
        {
            long    newRef = InterlockedDecrement( &mRefCount );
            _ASSERT( newRef >= 0 );
            if ( newRef == 0 )
            {
                delete this;
            }
        }

        virtual HRESULT Bind( const EvalOptions& options, IValueBinder* binder )
        {
            HRESULT     hr = S_OK;
            EvalData    evalData = { 0 };

            evalData.Options = options;
            evalData.TypeEnv = mTypeEnv;

            hr = mExpr->Semantic( evalData, mTypeEnv, binder );
            if ( FAILED( hr ) )
                return hr;

            return S_OK;
        }

        virtual HRESULT Evaluate( const EvalOptions& options, IValueBinder* binder, EvalResult& result )
        {
            HRESULT     hr = S_OK;
            EvalData    evalData = { 0 };

            evalData.Options = options;
            evalData.TypeEnv = mTypeEnv;

            hr = mExpr->Evaluate( EvalMode_Value, evalData, binder, result.ObjVal );
            if ( FAILED( hr ) )
                return hr;

            FillValueTraits( result, mExpr );

            return S_OK;
        }

    };

    bool gShowVTable = false;

    HRESULT Init()
    {
        InitPropTables();
        return S_OK;
    }

    void Uninit()
    {
        FreePropTables();
    }

    HRESULT MakeTypeEnv( int ptrSize, ITypeEnv*& typeEnv )
    {
        RefPtr<TypeEnv> env = new TypeEnv( ptrSize );

        if ( env == NULL )
            return E_OUTOFMEMORY;

        if ( !env->Init() )
            return E_FAIL;

        typeEnv = env.Detach();
        return S_OK;
    }

    HRESULT MakeNameTable( NameTable*& nameTable )
    {
        nameTable = new SimpleNameTable();

        if ( nameTable == NULL )
            return E_OUTOFMEMORY;

        nameTable->AddRef();
        return S_OK;
    }

    HRESULT StripFormatSpecifier( std::wstring& text, FormatOptions& fmtopt )
    {
        size_t textlen = text.size();
        if( textlen > 2 && text[textlen - 2] == ',')
        {
            fmtopt.specifier = text[textlen - 1];
            text.resize(textlen - 2);
        }
        return S_OK;
    }

    HRESULT AppendFormatSpecifier( std::wstring& text, const FormatOptions& fmtopt )
    {
        if ( fmtopt.specifier )
        {
            text.push_back( ',' );
            text.push_back( (wchar_t) fmtopt.specifier );
        }
        return S_OK;
    }

    HRESULT ParseText( const wchar_t* text, ITypeEnv* typeEnv, NameTable* strTable, IEEDParsedExpr*& expr )
    {
        if ( (text == NULL) || (typeEnv == NULL) || (strTable == NULL) )
            return E_INVALIDARG;

        Scanner scanner( text, wcslen( text ), strTable );
        Parser  parser( &scanner, typeEnv );
        RefPtr<Expression>  e;

        try
        {
            scanner.NextToken();
            e = parser.ParseExpression();

            if ( scanner.GetToken().Code != TOKeof )
                return E_MAGOEE_SYNTAX_ERROR;
        }
        catch ( int errCode )
        {
            UNREFERENCED_PARAMETER( errCode );
            _RPT2( _CRT_WARN, "Failed to parse, error %d. Text=\"%ls\".\n", errCode, text );
            return E_MAGOEE_SYNTAX_ERROR;
        }

        expr = new EEDParsedExpr( e, strTable, typeEnv );
        if ( expr == NULL )
            return E_OUTOFMEMORY;

        expr->AddRef();

        return S_OK;
    }


    HRESULT EnumValueChildren( 
        IValueBinder* binder, 
        const wchar_t* parentExprText, 
        const DataObject& parentVal, 
        ITypeEnv* typeEnv,
        NameTable* strTable,
        const FormatOptions& fmtopts,
        IEEDEnumValues*& enumerator )
    {
        if ( (binder == NULL) || (parentExprText == NULL) 
            || (typeEnv == NULL) || (strTable == NULL) )
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        RefPtr<EEDEnumValues>   en;

        if ( parentVal._Type->IsReference() )
        {
            en = new EEDEnumStruct( true );
        }
        else if ( parentVal._Type->IsPointer() )
        {
            // no children for void pointers
            if ( auto ptype = parentVal._Type->AsTypeNext() )
                if ( ptype->GetNext()->GetBackingTy() == Tvoid )
                    return E_FAIL;

            en = new EEDEnumPointer();
        }
        else if ( parentVal._Type->IsSArray() )
        {
            en = new EEDEnumSArray();
        }
        else if ( parentVal._Type->IsDArray() )
        {
            if ( fmtopts.specifier == FormatSpecRaw )
                en = new EEDEnumRawDArray();
            else
                en = new EEDEnumDArray();
        }
        else if ( parentVal._Type->IsAArray() )
        {
            en = new EEDEnumAArray( binder->GetAAVersion() );
        }
        else if ( parentVal._Type->AsTypeStruct() != NULL )
        {
            en = new EEDEnumStruct();
        }
        else
            return E_FAIL;

        if ( en == NULL )
            return E_OUTOFMEMORY;

        hr = en->Init( binder, parentExprText, parentVal, typeEnv, strTable );
        if ( FAILED( hr ) )
            return hr;

        enumerator = en.Detach();

        return S_OK;
    }

    void FillValueTraits( EvalResult& result, Expression* expr )
    {
        result.ReadOnly = true;
        result.HasString = false;
        result.HasChildren = false;
        result.HasRawChildren = false;

        if ( !expr || expr->Kind == DataKind_Value )
        {
            RefPtr<Type>    type = result.ObjVal._Type;

            // ReadOnly
            if ( (type->AsTypeStruct() != NULL)
                || type->IsSArray() )
            {
                // some types just don't allow assignment
            }
            else if ( result.ObjVal.Addr != 0 )
            {
                result.ReadOnly = false;
            }
            else if ( expr && expr->AsNamingExpression() != NULL ) 
            {
                Declaration* decl = expr->AsNamingExpression()->Decl;
                result.ReadOnly = (decl == NULL) || decl->IsConstant();
            }

            // HasString
            if ( (type->AsTypeNext() != NULL) && type->AsTypeNext()->GetNext()->IsChar() )
            {
                if ( type->IsPointer() )
                    result.HasString = result.ObjVal.Value.Addr != 0;
                else if ( type->IsSArray() || type->IsDArray() )
                    result.HasString = true;
            }

            // HasChildren/HasRawChildren
            if ( type->IsPointer() )
            {
                if( type->AsTypeNext()->GetNext()->GetBackingTy() == Tvoid )
                    result.HasChildren = result.HasRawChildren = false;
                else
                    result.HasChildren = result.HasRawChildren = result.ObjVal.Value.Addr != 0;
            }
            else if ( ITypeSArray* sa = type->AsTypeSArray() )
            {
                result.HasChildren = result.HasRawChildren = sa->GetLength() > 0;
            }
            else if ( type->IsDArray() )
            {
                result.HasChildren = result.ObjVal.Value.Array.Length > 0;
                result.HasRawChildren = true;
            }
            else if( type->IsAArray() )
            {
                result.HasChildren = result.HasRawChildren = result.ObjVal.Value.Addr != 0;
            }
            else if ( type->AsTypeStruct() )
            {
                result.HasChildren = result.HasRawChildren = true;
            }
        }
    }

    static const wchar_t    gCommonErrStr[] = L": Error: ";

    static const wchar_t*   gErrStrs[] = 
    {
        L"Expression couldn't be evaluated",
        L"Syntax error",
        L"Incompatible types for operator",
        L"Value expected",
        L"Expression has no type",
        L"Type resolve failed",
        L"Bad cast",
        L"Expression has no address",
        L"L-value expected",
        L"Can't cast to bool",
        L"Divide by zero",
        L"Bad indexing operation",
        L"Symbol not found",
        L"Element not found",
        L"Too many function arguments",
        L"Too few function arguments",
        L"Calling functions not implemented",
        L"failed to read register",
    };

    // returns: S_FALSE on error not found

    HRESULT GetErrorString( HRESULT hresult, std::wstring& outStr )
    {
        DWORD   fac = HRESULT_FACILITY( hresult );
        DWORD   code = HRESULT_CODE( hresult );

        if ( fac != MagoEE::HR_FACILITY )
            return S_FALSE;

        if ( code >= _countof( gErrStrs ) )
            code = 0;

        wchar_t codeStr[10];
        swprintf_s( codeStr, 10, L"D%04d", code + 1 );
        outStr = codeStr;
        outStr.append( gCommonErrStr );
        outStr.append( gErrStrs[code] );

        return S_OK;
    }

}
