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

            FillValueTraits( result );

            return S_OK;
        }

        void FillValueTraits( EvalResult& result )
        {
            result.ReadOnly = true;
            result.HasString = false;
            result.HasChildren = false;

            if ( mExpr->Kind == DataKind_Value )
            {
                RefPtr<Type>    type = result.ObjVal._Type;

                if ( (type->AsTypeStruct() != NULL)
                    || type->IsSArray() )
                {
                    // some types just don't allow assignment
                }
                else if ( result.ObjVal.Addr != 0 )
                {
                    result.ReadOnly = false;
                }
                else if ( mExpr->AsNamingExpression() != NULL ) 
                {
                    Declaration* decl = mExpr->AsNamingExpression()->Decl;
                    result.ReadOnly = (decl == NULL) || decl->IsConstant();
                }

                if ( (type->AsTypeNext() != NULL) && type->AsTypeNext()->GetNext()->IsChar() )
                {
                    if ( type->IsPointer() || type->IsSArray() || type->IsDArray() )
                        result.HasString = true;
                }

                if ( type->IsPointer() || type->IsSArray() || type->IsDArray()  || type->IsAArray()
                    || (type->AsTypeStruct() != NULL) )
                {
                    result.HasChildren = true;
                }
            }
        }
    };


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
            en = new EEDEnumPointer();
        }
        else if ( parentVal._Type->IsSArray() )
        {
            en = new EEDEnumSArray();
        }
        else if ( parentVal._Type->IsDArray() )
        {
            en = new EEDEnumDArray();
        }
        else if ( parentVal._Type->IsAArray() )
        {
            en = new EEDEnumAArray();
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
}
