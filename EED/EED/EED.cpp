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
    bool gExpandableStrings = true;
    bool gHideReferencePointers = true;
    bool gRemoveLeadingHexZeroes = false;
    bool gRecombineTuples = true;
    bool gShowDArrayLengthInType = true;
    bool gCallDebuggerFunctions = true;

    uint32_t gMaxArrayLength = 1000;

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
        const EvalResult& parentVal,
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
        EvalResult pointeeObj = { 0 };
        std::wstring pointeeExpr;
        const EvalResult* pparentVal = &parentVal;

    L_retry:
        if ( pparentVal->ObjVal._Type->IsReference() )
        {
            en = new EEDEnumStruct( true );
        }
        else if ( pparentVal->ObjVal._Type->IsPointer() )
        {
            // no children for void pointers
            auto ntype = pparentVal->ObjVal._Type->AsTypeNext()->GetNext();
            if ( ntype == NULL || ntype->GetBackingTy() == Tvoid )
                 return E_FAIL;

            if ( ntype->IsReference() || ntype->IsSArray() || ntype->IsDArray() || ntype->IsAArray() )
            {
                pointeeObj.ObjVal._Type = ntype;
                pointeeObj.ObjVal.Addr = pparentVal->ObjVal.Value.Addr;

                hr = binder->GetValue( pointeeObj.ObjVal.Addr, pointeeObj.ObjVal._Type, pointeeObj.ObjVal.Value );
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
        else if ( pparentVal->ObjVal._Type->IsSArray() )
        {
            en = new EEDEnumSArray();
        }
        else if ( pparentVal->ObjVal._Type->IsDArray() )
        {
            if ( fmtopts.specifier == FormatSpecRaw )
                en = new EEDEnumRawDArray();
            else
                en = new EEDEnumDArray();
        }
        else if ( pparentVal->ObjVal._Type->IsAArray() )
        {
            en = new EEDEnumAArray( binder->GetAAVersion() );
        }
        else if ( auto ts = pparentVal->ObjVal._Type->AsTypeStruct() )
        {
            if ( fmtopts.specifier != FormatSpecRaw && pparentVal->ObjVal.Addr != 0 )
            {
                Address fnaddr;
                if ( RefPtr<Type> fntype = GetDebuggerCall( ts, L"debuggerExpanded", fnaddr ) )
                {
                    auto func = fntype->AsTypeFunction();
                    pointeeObj.ObjVal._Type = func->GetReturnType();
                    hr = binder->CallFunction( fnaddr, func, pparentVal->ObjVal.Addr, pointeeObj.ObjVal );
                    if ( hr == S_OK )
                    {
                        pparentVal = &pointeeObj;
                        goto L_retry;
                    }
                }
            }
            en = new EEDEnumStruct();
        }
        else if ( pparentVal->ObjVal._Type->AsTypeTuple() )
        {
            en = new EEDEnumTuple();
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
            RefPtr<Type> type;
        L_retry:
            type = pparentVal->_Type;

            // ReadOnly
            if ( (type->AsTypeStruct() != NULL)
                || type->IsSArray() )
            {
                // some types just don't allow assignment
            }
            else if ( pparentVal->Addr != 0 )
            {
                result.ReadOnly = false;
            }
            else if ( expr && expr->AsNamingExpression() != NULL ) 
            {
                Declaration* decl = expr->AsNamingExpression()->Decl;
                result.ReadOnly = (decl == NULL) || decl->IsConstant();
            }

            // HasString
            auto typeHasString = [&](Type* t)
            {
                if ( auto tn = t->AsTypeNext() )
                    if ( tn->GetNext()->IsChar() )
                    {
                        if ( t->IsPointer() )
                            return pparentVal->Value.Addr != 0;
                        if ( t->IsSArray() || t->IsDArray() )
                            return true;
                    }
                return false;
            };
            if( typeHasString( type ) )
                result.HasString = true;
            else if ( auto ts = type->AsTypeStruct() )
            {
                Address fnaddr;
                if( RefPtr<Type> fntype = GetDebuggerCall( ts, L"debuggerStringView", fnaddr ) )
                    if( auto tret = fntype->AsTypeFunction()->GetReturnType() )
                        result.HasString = typeHasString( tret );
            }

            // HasChildren/HasRawChildren
            if ( type->IsPointer() )
            {
                auto ntype = type->AsTypeNext()->GetNext();
                if ( ntype == NULL || ntype->GetBackingTy() == Tvoid )
                    result.HasChildren = result.HasRawChildren = false;

                else if ( type->IsReference() || ntype->IsSArray() || ntype->IsDArray() || ntype->IsAArray() )
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
                result.HasChildren = result.HasRawChildren = pparentVal->Value.Addr != 0;
            }
            else if ( ITypeSArray* sa = type->AsTypeSArray() )
            {
                result.HasChildren = result.HasRawChildren = sa->GetLength() > 0;
            }
            else if ( type->IsDArray() )
            {
                if( !result.HasString || gExpandableStrings )
                    result.HasChildren = pparentVal->Value.Array.Length > 0;
                result.HasRawChildren = true;
            }
            else if( type->IsAArray() )
            {
                int aaVersion = -1;
                union
                {
                    BB64            mBB;
                    BB64_V1         mBB_V1;
                };
                if( EEDEnumAArray::ReadBB( binder, type, pparentVal->Value.Addr, aaVersion, mBB ) == S_OK )
                    result.HasChildren = result.HasRawChildren = (aaVersion == 1 ? mBB_V1.used - mBB_V1.deleted : mBB.nodes) > 0;
            }
            else if ( ITypeStruct* ts = type->AsTypeStruct() )
            {
                if( pparentVal->Addr != 0 )
                {
                    Address fnaddr;
                    if( RefPtr<Type> fntype = GetDebuggerCall( ts, L"debuggerExpanded", fnaddr ) )
                    {
                        auto func = fntype->AsTypeFunction();
                        pointeeObj._Type = func->GetReturnType();
                        HRESULT hr = E_FAIL;
                        if ( pointeeObj._Type->AsTypeSArray() )
                            hr = S_OK; // no need to read value to fill traits
                        else if ( auto rts = pointeeObj._Type->AsTypeStruct() )
                            if ( ts->GetUdtKind() != MagoEE::Udt_Class )
                                hr = S_OK; // no need to read value to fill traits
                        if ( hr != S_OK )
                            hr = binder->CallFunction( fnaddr, func, pparentVal->Addr, pointeeObj );
                        if (hr == S_OK)
                        {
                            pparentVal = &pointeeObj;
                            goto L_retry;
                        }
                    }
                }
                RefPtr<Declaration> decl = type->GetDeclaration();
                if ( ts->GetUdtKind() != MagoEE::Udt_Class )
                {
                    RefPtr<IEnumDeclarationMembers> members;
                    if ( decl->EnumMembers( members.Ref() ) )
                        result.HasChildren = result.HasRawChildren = members->GetCount() > 0;
                }
                else if ( pparentVal->Addr != 0 )
                {
                    bool hasVTable = false;
                    bool hasChildren = false;
                    bool hasDynamicClass = false;
                    if ( gShowVTable && expr && expr->GetObjectKind() != ObjectKind_CastExpression )
                    {
                        RefPtr<Declaration> vshape;
                        hasVTable = decl->GetVTableShape( vshape.Ref() ) && vshape;
                    }
                    if ( !hasVTable )
                    {
                        RefPtr<IEnumDeclarationMembers> members;
                        if ( decl->EnumMembers( members.Ref() ) )
                            hasChildren = members->GetCount() > 0;
                    }
                    if ( !hasVTable && !hasChildren && ( !expr || expr->GetObjectKind() != ObjectKind_CastExpression ) )
                    {
                        MagoEE::UdtKind kind;
                        std::wstring className;
                        if ( decl->GetUdtKind( kind ) && kind == MagoEE::Udt_Class )
                            binder->GetClassName( pparentVal->Addr, className, false );
                        hasDynamicClass = !className.empty();
                    }
                    result.HasChildren = result.HasRawChildren = hasVTable || hasChildren || hasDynamicClass;
                }
            }
            else if ( ITypeTuple* tt = type->AsTypeTuple() )
            {
                result.HasChildren = result.HasRawChildren = tt->GetLength() > 0;
            }
        }
    }

    RefPtr<Type> GetDebuggerCall( ITypeStruct* ts, const wchar_t* call, Address& fnaddr )
    {
        RefPtr<Declaration> decl = ts->FindObject( call );
        RefPtr<Type> dgtype;
        if ( decl && decl->GetAddress( fnaddr ) && decl->GetType( dgtype.Ref() ) )
            if ( dgtype->IsDelegate() ) // delegate has pointer to function as "next"
                if ( auto ptrtype = dgtype->AsTypeNext()->GetNext()->AsTypeNext() )
                    if ( RefPtr<Type> fntype = ptrtype->GetNext() )
                        if ( fntype->AsTypeFunction() )
                            return fntype;
        return nullptr;
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
        L"Cannot allocate trampoline function",
        L"Function call execution failed",
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

    ///////////////////////////////////////////////////////////
    std::wstring to_wstring( const wchar_t* str, size_t len )
    {
        return std::wstring( str, len );
    }
    
    std::wstring to_wstring( const char* str, size_t slen )
    {
        int len = MultiByteToWideChar( CP_UTF8, 0, str, slen, NULL, 0 );
        std::wstring wname;
        wname.resize( len );
        MultiByteToWideChar( CP_UTF8, 0, str, slen, (wchar_t*)wname.data(), len );
        return wname;
    }
    
    template<typename CH>
    static int t_isfield( const CH* str )
    {
        for (int i = 0; i < 7; i++)
            if (str[i] != "_field_"[i])
                return false;
        return true;
    }
    
    // return field index
    template<typename CH>
    int t_GetTupleName( const CH* str, size_t len, std::wstring* tupleName )
    {
        // detect tuple field "__%s_field_%llu"
        if( len < 10 )
            return -1;
    
        if( str[0] != '_' || str[1] != '_' || !isdigit( str[len - 1] ) )
            return -1;
    
        int index = 0;
        for( ; len > 0 && isdigit( str[len-1] ); --len )
            index = index * 10 + str[len - 1] - '0';
        
        if ( len < 7 || !t_isfield( str + len - 7 ) )
            return -1;
    
        if( tupleName )
            *tupleName = to_wstring( str + 2, len - 9 );
    
        return index;
    }
    
    int GetTupleName( const char* sym, size_t len, std::wstring* tupleName )
    {
        return t_GetTupleName( sym, len, tupleName );
    }
    
    int GetTupleName( const wchar_t* sym, std::wstring* tupleName )
    {
        return t_GetTupleName( sym, wcslen( sym ), tupleName );
    }
}
