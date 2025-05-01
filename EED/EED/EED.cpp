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

        virtual HRESULT Evaluate( const EvalOptions& options, IValueBinder* binder, EvalResult& result,
            std::function<HRESULT(HRESULT, EvalResult)> complete )
        {
            HRESULT     hr = S_OK;
            EvalData    evalData = { 0 };

            evalData.Options = options;
            evalData.TypeEnv = mTypeEnv;

            hr = mExpr->Evaluate( EvalMode_Value, evalData, binder, result.ObjVal );
            if ( FAILED( hr ) )
                return hr;
            return FillValueTraits( binder, result, mExpr, complete );
        }

    };

	bool gShowVTable = false;
    bool gExpandableStrings = true;
    bool gHideReferencePointers = true;
    bool gRemoveLeadingHexZeroes = false;
    bool gRecombineTuples = true;
    bool gShowDArrayLengthInType = true;
    bool gCallDebuggerFunctions = true;
    bool gCallDebuggerRanges = true;
    bool gCallDebuggerUseMagoGC = true;

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
            if ( gCallDebuggerFunctions && fmtopts.specifier != FormatSpecRaw && pparentVal->ObjVal.Addr != 0 )
            {
                Address fnaddr;
                if ( RefPtr<Type> fntype = GetDebuggerProp( binder, ts, L"__debugExpanded", fnaddr ) )
                {
                    hr = EvalDebuggerProp( binder, fntype, fnaddr, pparentVal->ObjVal.Addr, pointeeObj.ObjVal, {} );
                    if ( hr == S_OK )
                    {
                        pparentVal = &pointeeObj;
                        goto L_retry;
                    }
                }
                if( gCallDebuggerRanges && IsForwardRange( binder, pparentVal->ObjVal._Type ) )
                {
                    en = new EEDEnumRange();
                }
            }
            if( !en )
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

    HRESULT FillValueTraits( IValueBinder* binder, EvalResult& result, Expression* expr,
        std::function<HRESULT(HRESULT, EvalResult)> complete )
    {
        result.ReadOnly = true;
        result.HasString = false;
        result.HasChildren = false;
        result.HasRawChildren = false;

        bool readAccess = true; // set to false if only type should be evaluated
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
            else if ( readAccess && pparentVal->Addr != 0 )
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
                            return !readAccess || pparentVal->Value.Addr != 0;
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
                if ( gCallDebuggerFunctions )
                    if( RefPtr<Type> fntype = GetDebuggerProp( binder, ts, L"__debugStringView", fnaddr ) )
                        if( auto tret = GetDebuggerPropType( fntype ) )
                            result.HasString = typeHasString( tret );
            }

            // HasChildren/HasRawChildren
            if ( type->IsPointer() )
            {
                auto ntype = type->AsTypeNext()->GetNext();
                if ( ntype == NULL || ntype->GetBackingTy() == Tvoid )
                    result.HasChildren = result.HasRawChildren = false;

                else if ( readAccess && ( type->IsReference() || ntype->IsSArray() || ntype->IsDArray() || ntype->IsAArray() ) )
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
                result.HasChildren = result.HasRawChildren = !readAccess || pparentVal->Value.Addr != 0;
            }
            else if ( ITypeSArray* sa = type->AsTypeSArray() )
            {
                result.HasChildren = result.HasRawChildren = sa->GetLength() > 0;
            }
            else if ( type->IsDArray() )
            {
                if( !result.HasString || gExpandableStrings )
                    result.HasChildren = !readAccess || pparentVal->Value.Array.Length > 0;
                result.HasRawChildren = true;
            }
            else if( type->IsAArray() )
            {
                int aaVersion = -1;
                union
                {
                    BB64    mBB;
                    BB64_V1 mBB_V1;
                };
                if( !readAccess )
                    result.HasChildren = result.HasRawChildren = true;
                else if( EEDEnumAArray::ReadBB( binder, type, pparentVal->Value.Addr, aaVersion, mBB ) == S_OK )
                    result.HasChildren = result.HasRawChildren = (aaVersion == 1 ? mBB_V1.used - mBB_V1.deleted : mBB.nodes) > 0;
            }
            else if ( ITypeStruct* ts = type->AsTypeStruct() )
            {
                if ( gCallDebuggerFunctions )
                {
                    Address fnaddr;
                    if ( RefPtr<Type> fntype = GetDebuggerProp( binder, ts, L"__debugExpanded", fnaddr ) )
                    {
                        pointeeObj._Type = GetDebuggerPropType( fntype );
                        HRESULT hr = E_FAIL;
                        if ( pointeeObj._Type->AsTypeSArray() )
                            hr = S_OK; // no need to read value to fill traits
                        else if ( auto rts = pointeeObj._Type->AsTypeStruct() )
                            if ( ts->GetUdtKind() != MagoEE::Udt_Class )
                                hr = S_OK; // no need to read value to fill traits
                        if ( hr != S_OK )
                        {
                            readAccess = false;
                            hr = S_OK; // EvalDebuggerProp(binder, fntype, fnaddr, pparentVal->Addr, pointeeObj, {});
                        }
                        if ( hr == S_OK )
                        {
                            pparentVal = &pointeeObj;
                            goto L_retry;
                        }
                    }
                    else if( gCallDebuggerRanges && IsForwardRange( binder, type ) )
                    {
                        result.HasChildren = result.HasRawChildren = true;
                        goto L_done;
                    }
                }
                RefPtr<Declaration> decl = type->GetDeclaration();
                if ( ts->GetUdtKind() != MagoEE::Udt_Class )
                {
                    RefPtr<IEnumDeclarationMembers> members;
                    if ( decl->EnumMembers( members.Ref() ) )
                        result.HasChildren = result.HasRawChildren = members->GetCount() > 0;
                }
                else if ( !readAccess || pparentVal->Addr != 0 )
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
                        if ( readAccess && decl->GetUdtKind( kind ) && kind == MagoEE::Udt_Class )
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
    L_done:
        HRESULT hr = S_OK;
        if( complete )
            hr = complete( hr, result );
        return hr;
    }

    RefPtr<Type> GetDebuggerProp( IValueBinder* binder, ITypeStruct* ts, const wchar_t* call, Address& fnaddr )
    {
        RefPtr<Declaration> decl = ts->FindObject( call );
        RefPtr<Type> dgtype;
        if ( decl )
        {
            if( !decl->GetType( dgtype.Ref() ) )
                return nullptr;
            if ( !decl->GetAddress( fnaddr ) )
                return nullptr;
        }
        else
        {
            HRESULT hr = binder->FindDebugFunc( call, ts, dgtype.Ref(), fnaddr );
            if( hr != S_OK )
                return nullptr;
        }
        if ( dgtype )
        {
            int off;
            if ( dgtype->IsDelegate() ) // delegate has pointer to function as "next"
            {
                if ( auto ptrtype = dgtype->AsTypeNext()->GetNext()->AsTypeNext() )
                    if ( RefPtr<Type> fntype = ptrtype->GetNext() )
                        if ( fntype->AsTypeFunction() )
                            return fntype;
            }
            else if ( dgtype->IsFunction() )
            {
                if ( auto fntype = dgtype->AsTypeFunction() )
                    if ( auto paramList = fntype->GetParams() )
                        if ( paramList->List.size() == 1 )
                            if ( auto ptype = paramList->List.front()->_Type )
                                if ( ptype->IsPointer() || ptype->IsReference() )
                                    if( ts->Equals( ptype->AsTypeNext()->GetNext() ) )
                                        return dgtype;
            }
            else if ( decl->GetOffset( off ) )
            {
                fnaddr = off;
                return dgtype; // type to field
            }
        }
        return nullptr;
    }

    RefPtr<Type> GetDebuggerPropType( Type* fntype )
    {
        if (auto func = fntype->AsTypeFunction())
            return func->GetReturnType();
        return fntype;
    }

    HRESULT EvalDebuggerProp( IValueBinder* binder, RefPtr<Type> fntype, Address fnaddr,
                              Address objAddr, DataObject& propValue,
                              std::function<HRESULT(HRESULT, DataObject)> complete )
    {
        HRESULT hr;
        if( auto func = fntype->AsTypeFunction() )
        {
            propValue._Type = func->GetReturnType();
            hr = binder->CallFunction( fnaddr, func, objAddr, propValue, true, complete );
        }
        else
        {
            propValue._Type = fntype;
            propValue.Addr = objAddr + fnaddr;
            hr = binder->GetValue( propValue.Addr, fntype, propValue.Value );
        }
        return hr;
    }

    bool IsForwardRange( IValueBinder* binder, Type* type )
    {
        auto ts = type->AsTypeStruct();
        if (!ts)
            return false;
        Address addrSave = 0, addrEmpty = 0, addrFront = 0, addrPop = 0;
        RefPtr<Type> typeSave  = GetDebuggerProp( binder, ts, L"save", addrSave );
        RefPtr<Type> typeEmpty = typeSave  ? GetDebuggerProp( binder, ts, L"empty", addrEmpty ) : nullptr;
        RefPtr<Type> typeFront = typeEmpty ? GetDebuggerProp( binder, ts, L"front", addrFront ) : nullptr;
        RefPtr<Type> typePop   = typeFront ? GetDebuggerProp( binder, ts, L"popFront", addrPop ) : nullptr;
        return typePop != nullptr;
    }

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
        L"Expression has invalid address",
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
        L"Invalid or missing magogc*.dll",
        L"Function call execution failed",
        L"division by zero", // DkmILFailureReason::DivideByZero
        L"memory read error",
        L"memory write error",
        L"register read error",
        L"register write error",
        L"aborted",
        L"string too long",
        L"evaluation timeout",
        L"multiple function evaluations",
        L"fail to abort",
        L"not supported for minidump",
        L"unhandled exception",
        L"not supported on a user-mode scheduled thread",
        L"bounds error",
        L"unsupported operation",
        L"function evaluation error",
        L"failed to evaluate across enclave boundaries", // DkmILFailureReason::AttemptedToCrossEnclaveBoundaries
    };

    static_assert(sizeof(gErrStrs) / sizeof(gErrStrs[0]) == E_MAGOEE_NUMERRORS - E_MAGOEE_BASE, "error mismatch");

    // returns: S_FALSE on error not found

    HRESULT GetErrorString( HRESULT hresult, std::wstring& outStr )
    {
        DWORD   fac = HRESULT_FACILITY( hresult );
        DWORD   code = HRESULT_CODE( hresult );

        wchar_t codeStr[256] = L"";
        if ( fac != MagoEE::HR_FACILITY )
        {
            swprintf_s(codeStr, 256, L"E%08x: ", hresult);
            DWORD nRet = FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, hresult, 0, codeStr + 11, _countof(codeStr) - 11, NULL);
            if ( nRet == 0 )
                swprintf_s( codeStr + 11, _countof(codeStr) - 11, L"unknown system error" );
        }
        else
        {
            if ( code >= _countof( gErrStrs ) )
                code = 0;

            swprintf_s( codeStr, _countof(codeStr), L"D%04d: Error: %s", code + 1, gErrStrs[code] );
        }

        outStr = codeStr;
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

    // return field index
    template<typename CH>
    int t_GetParamIndex( const CH* str, size_t len )
    {
        // detect tuple field "__param_%llu"
        if( len < 9 )
            return -1;

        for( int i = 0; i < 8; i++ )
            if( str[i] != "__param_"[i] )
                return -1;

        int index = 0;
        for( ; len > 0 && isdigit( str[len - 1] ); --len )
            index = index * 10 + str[len - 1] - '0';

        if ( len > 8 )
            return -1;

        return index;
    }

    int GetParamIndex( const char* sym, size_t len )
    {
        return t_GetParamIndex( sym, len );
    }

    int GetParamIndex( const wchar_t* sym )
    {
        return t_GetParamIndex( sym, wcslen(sym) );
    }
}
