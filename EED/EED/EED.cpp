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

            FillValueTraits( binder, result, mExpr );

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
        DataObject pointeeObj = { 0 };
        std::wstring pointeeExpr;
        const DataObject* pparentVal = &parentVal;

    L_retry:
        if ( pparentVal->_Type->IsReference() )
        {
            en = new EEDEnumStruct( true );
        }
        else if ( pparentVal->_Type->IsPointer() )
        {
            // no children for void pointers
            auto ntype = pparentVal->_Type->AsTypeNext()->GetNext();
            if ( ntype == NULL || ntype->GetBackingTy() == Tvoid )
                 return E_FAIL;

            if ( ntype->IsReference() || ntype->IsSArray() || ntype->IsDArray() || ntype->IsAArray() || ntype->AsTypeStruct() )
            {
                pointeeObj._Type = ntype;
                pointeeObj.Addr = pparentVal->Value.Addr;

                hr = binder->GetValue( pointeeObj.Addr, pointeeObj._Type, pointeeObj.Value );
                if( hr == S_OK )
                {
                    pointeeExpr.append( L"*(" ).append( parentExprText ).append( 1, L')' );
                    parentExprText = pointeeExpr.c_str();
                    pparentVal = &pointeeObj;
                    goto L_retry;
                }
            }
            en = new EEDEnumPointer();
        }
        else if ( pparentVal->_Type->IsSArray() )
        {
            en = new EEDEnumSArray();
        }
        else if ( pparentVal->_Type->IsDArray() )
        {
            if ( fmtopts.specifier == FormatSpecRaw )
                en = new EEDEnumRawDArray();
            else
                en = new EEDEnumDArray();
        }
        else if ( pparentVal->_Type->IsAArray() )
        {
            en = new EEDEnumAArray( binder->GetAAVersion() );
        }
        else if ( pparentVal->_Type->AsTypeStruct() != NULL )
        {
            en = new EEDEnumStruct();
        }
        else
            return E_FAIL;

        if ( en == NULL )
            return E_OUTOFMEMORY;

        hr = en->Init( binder, parentExprText, *pparentVal, typeEnv, strTable );
        if ( FAILED( hr ) )
            return hr;

        enumerator = en.Detach();

        return S_OK;
    }

    void FillValueTraits( IValueBinder* binder, EvalResult& result, Expression* expr )
    {
        result.ReadOnly = true;
        result.HasString = false;
        result.HasChildren = false;
        result.HasRawChildren = false;

        DataObject pointeeObj = { 0 };
        const DataObject* pparentVal = &result.ObjVal;

        if ( !expr || expr->Kind == DataKind_Value )
        {
        L_retry:
            RefPtr<Type>    type = pparentVal->_Type;

            // ReadOnly
            if ( (type->AsTypeStruct() != NULL)
                || type->IsSArray() )
            {
                // some types just don't allow assignment
            }
            else if (pparentVal->Addr != 0 )
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
                auto ntype = type->AsTypeNext()->GetNext();
                if ( ntype == NULL || ntype->GetBackingTy() == Tvoid )
                    result.HasChildren = result.HasRawChildren = false;

                else if ( ntype->IsReference() || ntype->IsSArray() || ntype->IsDArray() || ntype->IsAArray() || ntype->AsTypeStruct() )
                {
                    // auto-follow through pointer
                    pointeeObj._Type = ntype;
                    pointeeObj.Addr = pparentVal->Value.Addr;

                    HRESULT hr = binder->GetValue( pointeeObj.Addr, pointeeObj._Type, pointeeObj.Value );
                    if ( hr == S_OK )
                    {
                        pparentVal = &pointeeObj;
                        goto L_retry;
                    }

                }
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
            else if (ITypeStruct* ts = type->AsTypeStruct() )
            {
                RefPtr<Declaration> decl = type->GetDeclaration();
                RefPtr<IEnumDeclarationMembers> members;
                if ( decl->EnumMembers( members.Ref() ) )
                    result.HasChildren = result.HasRawChildren = members->GetCount() > 0;
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
        L"Cannot call functions with arguments",
        L"Failed to read register",
        L"Unsupported calling convention",
        L"Function calls not allowed",
        L"Function call may have side effects",
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
