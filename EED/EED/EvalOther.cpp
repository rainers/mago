/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

// Other: ID, member, index, ...

#include "Common.h"
#include "EED.h"
#include "Expression.h"
#include "Declaration.h"
#include "Type.h"
#include "TypeCommon.h"
#include "NameTable.h"
#include "ITypeEnv.h"
#include "Property.h"
#include "PropTables.h"
#include "SharedString.h"
#include "TypeUnresolved.h"


const HRESULT E_NOT_FOUND = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

    
namespace MagoEE
{
    HRESULT Eval( IValueBinder* binder, Declaration* decl, DataObject& obj )
    {
        if ( obj.Addr == 0 )
        {
            // no address, rely on declaration for address and type
            return binder->GetValue( decl, obj.Value );
        }
        else
        {
            // has address
            _ASSERT( obj._Type != NULL );

            return binder->GetValue( obj.Addr, obj._Type, obj.Value );
        }
    }

    HRESULT TypeCheckAAKey( Type* actualKeyType, Type* declaredKeyType )
    {
        _ASSERT( actualKeyType != NULL );
        _ASSERT( declaredKeyType != NULL );

        if ( !declaredKeyType->IsBasic()
            && !declaredKeyType->IsPointer()
            && (declaredKeyType->AsTypeEnum() == NULL)
            && !declaredKeyType->IsDelegate()
            && (declaredKeyType->AsTypeStruct() == NULL) 
            && !declaredKeyType->IsSArray() 
            && !declaredKeyType->IsDArray() )
            return E_MAGOEE_BAD_INDEX;
        if ( declaredKeyType->IsReference() )
            return E_MAGOEE_BAD_INDEX;
        if ( declaredKeyType->IsSArray() || declaredKeyType->IsDArray() )
        {
            RefPtr<Type>    elemType = declaredKeyType->AsTypeNext()->GetNext();

            if ( !elemType->IsBasic()
                && !elemType->IsPointer()
                && (elemType->AsTypeEnum() == NULL)
                && !elemType->IsDelegate()
                && (elemType->AsTypeStruct() == NULL) )
                return E_MAGOEE_BAD_INDEX;
            if ( elemType->IsReference() )
                return E_MAGOEE_BAD_INDEX;
        }
        if ( (declaredKeyType->AsTypeStruct() != NULL) 
            || declaredKeyType->IsSArray() )
        {
            if ( !actualKeyType->Equals( declaredKeyType ) )
                return E_MAGOEE_BAD_INDEX;
        }
        else
        {
            if ( !CastExpr::CanCast( actualKeyType, declaredKeyType ) )
                return E_MAGOEE_BAD_INDEX;
        }

        return S_OK;
    }

    HRESULT FindAAElement( 
        const DataObject& array, 
        const DataObject& key, 
        Type* keyType, 
        IValueBinder* binder, 
        Address& addr )
    {
        _ASSERT( keyType != NULL );
        _ASSERT( binder != NULL );

        DataObject  finalKey = { 0 };

        finalKey._Type = keyType;
        finalKey.Addr = key.Addr;

        if ( (keyType->AsTypeStruct() != NULL)
            || keyType->IsSArray() )
        {
            if ( key.Addr == 0 )
                return E_FAIL;
        }
        else
        {
            CastExpr::AssignValue( key, finalKey );
        }

        return binder->GetValue( array.Value.Addr, finalKey, addr );
    }


    //----------------------------------------------------------------------------
    //  ConditionalExpr
    //----------------------------------------------------------------------------

    HRESULT ConditionalExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = PredicateExpr->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;
        if ( PredicateExpr->Kind != DataKind_Value )
            return E_MAGOEE_VALUE_EXPECTED;
        if ( PredicateExpr->_Type == NULL )
            return E_MAGOEE_NO_TYPE;

        hr = TrueExpr->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;
        if ( TrueExpr->Kind != DataKind_Value )
            return E_MAGOEE_VALUE_EXPECTED;
        if ( TrueExpr->_Type == NULL )
            return E_MAGOEE_NO_TYPE;

        hr = FalseExpr->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;
        if ( FalseExpr->Kind != DataKind_Value )
            return E_MAGOEE_VALUE_EXPECTED;
        if ( FalseExpr->_Type == NULL )
            return E_MAGOEE_NO_TYPE;

        if ( !PredicateExpr->_Type->CanImplicitCastToBool() )
            return E_MAGOEE_BAD_BOOL_CAST;
        // TODO: actually, they only have to be compatible with the thing this whole expression fits in
        if ( !TrueExpr->_Type->Equals( FalseExpr->_Type ) )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        _Type = TrueExpr->_Type;
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT ConditionalExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        HRESULT hr = S_OK;
        DataObject  pred = { 0 };

        hr = PredicateExpr->Evaluate( EvalMode_Value, evalData, binder, pred );
        if ( FAILED( hr ) )
            return hr;

        if ( ConvertToBool( pred ) )
        {
            hr = TrueExpr->Evaluate( mode, evalData, binder, obj );
        }
        else
        {
            hr = FalseExpr->Evaluate( mode, evalData, binder, obj );
        }

        if ( FAILED( hr ) )
            return hr;

        // our value can have a return address, the sub-expression will already have put it in obj
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  AddressOfExpr
    //----------------------------------------------------------------------------

    HRESULT AddressOfExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = Child->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;
        if ( Child->Kind != DataKind_Value )
            return E_MAGOEE_VALUE_EXPECTED;
        if ( Child->_Type == NULL )
            return E_MAGOEE_NO_TYPE;

        hr = typeEnv->NewPointer( Child->_Type, _Type.Ref() );
        if ( FAILED( hr ) )
            return hr;

        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT AddressOfExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT hr = S_OK;
        DataObject  pointed = { 0 };

        hr = Child->Evaluate( EvalMode_Address, evalData, binder, pointed );
        if ( FAILED( hr ) )
            return hr;

        if ( pointed.Addr == 0 )
            return E_MAGOEE_NO_ADDRESS;

        obj._Type = _Type;
        obj.Value.Addr = pointed.Addr;
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  PointerExpr
    //----------------------------------------------------------------------------

    HRESULT PointerExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = Child->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;
        if ( Child->Kind != DataKind_Value )
            return E_MAGOEE_VALUE_EXPECTED;
        if ( Child->_Type == NULL )
            return E_MAGOEE_NO_TYPE;

        if ( !Child->_Type->IsPointer() && !Child->_Type->IsSArray() && !Child->_Type->IsDArray() )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        if ( Child->_Type->IsReference() )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        _ASSERT( Child->_Type->AsTypeNext() != NULL );
        RefPtr<Type>    voidType = typeEnv->GetType( Tvoid );
        if ( Child->_Type->AsTypeNext()->GetNext()->Equals( voidType ) )
            return E_MAGOEE_BAD_TYPES_FOR_OP;
        if ( Child->_Type->AsTypeNext()->GetNext()->IsFunction() )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        _Type = Child->_Type->AsTypeNext()->GetNext();
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT PointerExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        HRESULT hr = S_OK;
        DataObject  pointer = { 0 };

        hr = Child->Evaluate( EvalMode_Value, evalData, binder, pointer );
        if ( FAILED( hr ) )
            return hr;

        obj._Type = _Type;

        if ( Child->_Type->IsPointer() )
            obj.Addr = pointer.Value.Addr;
        else if ( Child->_Type->IsSArray() )
            obj.Addr = pointer.Addr;
        else if ( Child->_Type->IsDArray() )
            obj.Addr = pointer.Value.Array.Addr;
        else
            _ASSERT( false );

        if ( mode == EvalMode_Value )
        {
            hr = binder->GetValue( obj.Addr, _Type, obj.Value );
            if ( FAILED( hr ) )
                return hr;
        }

        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  IndexExpr
    //----------------------------------------------------------------------------

    HRESULT IndexExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = Child->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;
        if ( Child->Kind != DataKind_Value )
            return E_MAGOEE_VALUE_EXPECTED;
        if ( Child->_Type == NULL )
            return E_MAGOEE_NO_TYPE;

        Type*   childType = Child->_Type;
        if ( !childType->IsPointer() && !childType->IsSArray() && !childType->IsDArray()
            && !childType->IsAArray() && !childType->AsTypeTuple() )
            return E_MAGOEE_BAD_INDEX;

        if ( childType->IsReference() )
            return E_MAGOEE_BAD_INDEX;

        if ( Args->List.size() != 1 )
            return E_MAGOEE_BAD_INDEX;

        Expression* index = Args->List.front();
        EvalData    indexData = evalData;

        if ( childType->IsSArray() || childType->IsDArray() || childType->AsTypeTuple() )
            indexData.HasArrayLength = true;

        hr = index->Semantic( indexData, typeEnv, binder );

        if ( FAILED( hr ) )
            return hr;
        if ( index->Kind != DataKind_Value )
            return E_MAGOEE_VALUE_EXPECTED;
        if ( index->_Type == NULL )
            return E_MAGOEE_NO_TYPE;
        if ( childType->IsAArray() )
        {
            hr = TypeCheckAAKey( index->_Type, childType->AsTypeAArray()->GetIndex() );
            if ( FAILED( hr ) )
                return hr;

            index->TrySetType( childType->AsTypeAArray()->GetIndex() );
        }
        else if ( !index->_Type->IsIntegral() && !childType->AsTypeTuple() )
            return E_MAGOEE_BAD_INDEX;

        if( auto tt = childType->AsTypeTuple() )
        {
            // index must be compile time constant
            DataObject indexObj = { 0 };
            indexData.ArrayLength = tt->GetLength();
            hr = index->Evaluate( EvalMode_Value, indexData, binder, indexObj );
            if ( FAILED( hr ) )
                return hr;

            int64_t idx = indexObj.Value.Int64Value;
            if( idx < 0 || idx >= tt->GetLength() )
                return E_MAGOEE_BAD_INDEX;
            _Type = tt->GetElementType( (uint32_t)idx );
        }
        else
        {
            ITypeNext* typeNext = Child->_Type->AsTypeNext();
            _ASSERT( typeNext != NULL );
            if ( typeNext->GetNext() == NULL )
                return E_MAGOEE_BAD_INDEX;

            RefPtr<Type> voidType = typeEnv->GetType( Tvoid );
            if ( typeNext->GetNext()->Equals( voidType ) )
                return E_MAGOEE_BAD_TYPES_FOR_OP;

            _Type = typeNext->GetNext();
        }
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT IndexExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& elem )
    {
        // TODO: if index value is a signed integral, then should we allow negative?
        //  pointers allow it and work as expected
        //  static arrays don't allow it at compile time
        //  dynamic arrays throw a RangeError exception at runtime

        HRESULT     hr = S_OK;
        DataObject  array = { 0 };
        DataObject  index = { 0 };
        EvalData    indexData = evalData;
        Address     addr = 0;

        hr = Child->Evaluate( EvalMode_Value, evalData, binder, array );
        if ( FAILED( hr ) )
            return hr;

        if ( Child->_Type->IsSArray() )
        {
            if ( array.Addr == 0 )
                return E_FAIL;

            indexData.HasArrayLength = true;
            indexData.ArrayLength = Child->_Type->AsTypeSArray()->GetLength();
            array.Value.Addr = array.Addr;
        }
        else if ( Child->_Type->IsDArray() )
        {
            indexData.HasArrayLength = true;
            indexData.ArrayLength = array.Value.Array.Length;
            array.Value.Addr = array.Value.Array.Addr;
        }
        else if ( auto tt = Child->_Type->AsTypeTuple() )
        {
            indexData.HasArrayLength = true;
            indexData.ArrayLength = tt->GetLength();
        }
        else if ( Child->_Type->IsPointer() )
        // else if it's a pointer, then value already has address
            ;
        else if ( Child->_Type->IsAArray() )
        // assoc. arrays are handled differently
            ;
        else
        {
            _ASSERT( false );
            return E_NOTIMPL;
        }

        hr = Args->List.front()->Evaluate( EvalMode_Value, indexData, binder, index );
        if ( FAILED( hr ) )
            return hr;

        if ( auto taa = Child->_Type->AsTypeAArray() )
        {
            hr = FindAAElement( array, index, taa->GetIndex(), binder, addr );
            if ( hr == E_NOT_FOUND )
                return E_MAGOEE_ELEMENT_NOT_FOUND;
            if ( FAILED( hr ) )
                return hr;
        }
        else if ( auto tt = Child->_Type->AsTypeTuple() )
        {
            int64_t idx = index.Value.Int64Value;
            if ( idx < 0 || idx >= tt->GetLength() )
                return E_MAGOEE_BAD_INDEX;
            auto elemDecl = tt->GetElementDecl( (uint32_t)idx );
            if( elemDecl->IsField() )
            {
                int offset = 0;
                if( !elemDecl->GetOffset( offset ) )
                    return E_MAGOEE_NO_ADDRESS;
                addr = array.Addr + offset;
            }
            else if ( !elemDecl->GetAddress( addr ) )
                return E_MAGOEE_NO_ADDRESS;
        }
        else
        {
            uint32_t    size = _Type->GetSize();
            doffset_t   offset = size * index.Value.Int64Value;
            addr = array.Value.Addr + offset;
        }

        elem.Addr = addr;
        elem._Type = _Type;

        if ( mode == EvalMode_Value )
        {
            hr = binder->GetValue( elem.Addr, _Type, elem.Value );
            if ( FAILED( hr ) )
                return hr;
        }

        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  SliceExpr
    //----------------------------------------------------------------------------

    HRESULT SliceExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT     hr = S_OK;
        EvalData    indexData = evalData;
        ClearEvalData();

        hr = Child->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;
        if ( Child->Kind != DataKind_Value )
            return E_MAGOEE_VALUE_EXPECTED;
        if ( Child->_Type == NULL )
            return E_MAGOEE_NO_TYPE;

        Type*   childType = Child->_Type;
        if ( !childType->IsPointer() && !childType->IsSArray() && !childType->IsDArray() )
            return E_MAGOEE_BAD_INDEX;

        ITypeNext*  typeNext = Child->_Type->AsTypeNext();
        _ASSERT( typeNext != NULL );
        if ( typeNext->GetNext() == NULL )
            return E_MAGOEE_BAD_INDEX;

        RefPtr<Type>    voidType = typeEnv->GetType( Tvoid );
        if ( typeNext->GetNext()->Equals( voidType ) )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        if ( childType->IsPointer() && ((From == NULL) || (To == NULL)) )
            return E_MAGOEE_BAD_INDEX;
        // only allow [] or [e1..e2]
        if ( (childType->IsSArray() || childType->IsDArray()) && ((From == NULL) != (To == NULL)) )
            return E_MAGOEE_BAD_INDEX;

        if ( childType->IsSArray() || childType->IsDArray() )
            indexData.HasArrayLength = true;

        if ( From != NULL )
        {
            hr = From->Semantic( indexData, typeEnv, binder );
            if ( FAILED( hr ) )
                return hr;
            if ( From->Kind != DataKind_Value )
                return E_MAGOEE_VALUE_EXPECTED;
            if ( From->_Type == NULL )
                return E_MAGOEE_NO_TYPE;
            if ( !From->_Type->IsIntegral() )
                return E_MAGOEE_BAD_INDEX;
        }

        if ( To != NULL )
        {
            hr = To->Semantic( indexData, typeEnv, binder );
            if ( FAILED( hr ) )
                return hr;
            if ( To->Kind != DataKind_Value )
                return E_MAGOEE_VALUE_EXPECTED;
            if ( To->_Type == NULL )
                return E_MAGOEE_NO_TYPE;
            if ( !To->_Type->IsIntegral() )
                return E_MAGOEE_BAD_INDEX;
        }

        if ( childType->IsDArray() )
            _Type = childType;
        else
        {
            hr = typeEnv->NewDArray( typeNext->GetNext(), _Type.Ref() );
            if ( FAILED( hr ) )
                return hr;
        }

        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT SliceExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        // TODO: if index value is a signed integral, then should we allow negative?
        //  pointers allow it and work as expected
        //  static arrays don't allow it at compile time
        //  dynamic arrays throw a RangeError exception at runtime

        HRESULT     hr = S_OK;
        DataObject  array = { 0 };
        DataObject  index = { 0 };
        DataObject  limit = { 0 };
        EvalData    indexData = evalData;
        Address     addr = 0;

        hr = Child->Evaluate( EvalMode_Value, evalData, binder, array );
        if ( FAILED( hr ) )
            return hr;

        if ( Child->_Type->IsSArray() )
        {
            if ( array.Addr == 0 )
                return E_FAIL;

            indexData.HasArrayLength = true;
            indexData.ArrayLength = Child->_Type->AsTypeSArray()->GetLength();
            addr = array.Addr;
            limit.Value.Int64Value = Child->_Type->AsTypeSArray()->GetLength();
        }
        else if ( Child->_Type->IsDArray() )
        {
            indexData.HasArrayLength = true;
            indexData.ArrayLength = array.Value.Array.Length;
            addr = array.Value.Array.Addr;
            limit.Value.Int64Value = array.Value.Array.Length;
        }
        else if ( Child->_Type->IsPointer() )
        // else if it's a pointer, then value already has address
        {
            addr = array.Value.Addr;
        }
        else
        {
            _ASSERT( false );
            return E_NOTIMPL;
        }

        if ( From != NULL )
        {
            hr = From->Evaluate( EvalMode_Value, indexData, binder, index );
            if ( FAILED( hr ) )
                return hr;
        }
        else
            _ASSERT( index.Value.Int64Value == 0 );

        if ( To != NULL )
        {
            hr = To->Evaluate( EvalMode_Value, indexData, binder, limit );
            if ( FAILED( hr ) )
                return hr;
        }

        uint32_t    size = _Type->AsTypeNext()->GetNext()->GetSize();
        doffset_t   offset = size * index.Value.Int64Value;

        obj.Value.Array.Addr = addr + offset;
        obj.Value.Array.Length = limit.Value.Int64Value - index.Value.Int64Value;
        obj._Type = _Type;

        // can't have a negative length (Length is unsigned, but the idea still holds)
        if ( limit.Value.Int64Value < index.Value.Int64Value )
            return E_MAGOEE_BAD_INDEX;

        return S_OK;
    }

    //----------------------------------------------------------------------------
    template<typename FINDFN>
    static HRESULT findTuple( const wchar_t* Id, IValueBinder* binder, Declaration*& decl, FINDFN find )
    {
        if( !gRecombineTuples )
            return E_NOT_FOUND;

        wchar_t field[300];
        _snwprintf( field, 300, L"__%s_field_0", Id );
        RefPtr<Declaration> first;
        HRESULT hr = find( field, first.Ref() );
        if ( FAILED( hr ) )
            return hr;

        std::vector<RefPtr<Declaration>> fields;
        fields.push_back( first );

        for (int i = 1; ; i++)
        {
            RefPtr<Declaration> fdecl;
            _snwprintf( field, 300, L"__%s_field_%d", Id, i );
            hr = find( field, fdecl.Ref() );
            if ( FAILED( hr ) )
                break;
            fields.push_back( fdecl );
        }
        return binder->NewTuple( Id, fields, decl );
    }

    //----------------------------------------------------------------------------
    //  IdExpr
    //----------------------------------------------------------------------------

    HRESULT IdExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( typeEnv );

        HRESULT hr = S_OK;
        ClearEvalData();

        hr = FindObject( Id->Str, binder, Decl.Ref() );
        if( FAILED( hr ) )
        {
            hr = findTuple( Id->Str, binder, Decl.Ref(),
                [&]( const wchar_t* id, Declaration*& decl )
                {
                    return FindObject( id, binder, decl );
                });
            if ( FAILED( hr ) )
                return hr;
        }

        Decl->GetType( _Type.Ref() );

        if ( Decl->IsField()        // in this case, it's a field of "this"
            || Decl->IsVar()
            || Decl->IsConstant() )
        {
            Kind = DataKind_Value;
        }
        else
        {
            Kind = DataKind_Declaration;
        }

        if ( (Kind == DataKind_Value) && (_Type == NULL) )
            return E_MAGOEE_NO_TYPE;

        return S_OK;
    }

    HRESULT IdExpr::FindObject( const wchar_t* name, IValueBinder* binder, Declaration*& decl )
    {
        HRESULT hr = binder->FindObject( name, decl, IValueBinder::FindObjectLocal );
        if ( hr == S_OK )
            return S_OK;

        hr = binder->FindObject( name, decl, IValueBinder::FindObjectClosure );
        if ( hr == S_OK )
            return S_OK;

        while ( true )
        {
            // now look in the class

            RefPtr<Declaration> thisDecl;
            RefPtr<Type>        thisType;
            RefPtr<Declaration> childDecl;

            hr = binder->GetThis( thisDecl.Ref() );
            if ( FAILED( hr ) )
                break;

            if ( !thisDecl->GetType( thisType.Ref() ) )
                break;

            if ( thisType->IsPointer() )
                thisType = thisType->AsTypeNext()->GetNext();

            if ( thisType->AsTypeStruct() == NULL )
                break;

            childDecl = thisType->AsTypeStruct()->FindObject( name );
            if ( childDecl == NULL )
                break;

            decl = childDecl.Detach();
            return S_OK;
        }
        hr = binder->FindObject( name, decl, IValueBinder::FindObjectGlobal | IValueBinder::FindObjectRegister );
        if ( hr == S_OK )
            return S_OK;

        return E_MAGOEE_SYMBOL_NOT_FOUND;
    }

    HRESULT IdExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( evalData );

        if ( Kind != DataKind_Value )
            return E_MAGOEE_VALUE_EXPECTED;

        obj._Type = _Type;

        if ( Decl->IsField() )
        {
            int     offset = 0;
            Address thisAddr = 0;

            HRESULT hr = GetThisAddress( binder, thisAddr );
            if ( FAILED( hr ) )
                return hr;

            if ( !Decl->GetOffset( offset ) )
                return E_FAIL;

            obj.Addr = thisAddr + offset;
        }
        else if ( !Decl->IsRegister() )
        {
            if ( !Decl->GetAddress( obj.Addr ) )
                obj.Addr = 0;
        }

        if ( mode == EvalMode_Address )
        {
            if ( obj.Addr != 0 )
                return S_OK;

            return E_MAGOEE_NO_ADDRESS;
        }

        // evaluate a scalar we might have
        return Eval( binder, Decl, obj );
    }

    HRESULT IdExpr::GetThisAddress( IValueBinder* binder, Address& addr )
    {
        HRESULT             hr = S_OK;
        RefPtr<Declaration> thisDecl;
        RefPtr<Type>        thisType;

        hr = binder->GetThis( thisDecl.Ref() );
        if ( FAILED( hr ) )
            return hr;

        if ( !thisDecl->GetType( thisType.Ref() ) )
            return E_FAIL;

        if ( !thisType->IsPointer() )
        {
            if ( !thisDecl->GetAddress( addr ) )
                return E_FAIL;
        }
        else
        {
            DataObject  obj = { 0 };

            hr = Eval( binder, thisDecl, obj );
            if ( FAILED( hr ) )
                return hr;

            addr = obj.Value.Addr;
        }

        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  DotExpr
    //----------------------------------------------------------------------------

    HRESULT DotExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();
        Property = NULL;

        RefPtr<SharedString>    namePath;
        hr = MakeName( 0, namePath );
        if ( FAILED( hr ) && (hr != E_MAGOEE_SYMBOL_NOT_FOUND) )
            return hr;

        if ( SUCCEEDED( hr ) )
        {
            const wchar_t*  name = mNamePath->GetCut( mNamePathLen );

            binder->FindObject( name, Decl.Ref(), IValueBinder::FindObjectAny );

            mNamePath->ReleaseCut();
        }

        if ( Decl == NULL )
        {
            hr = Child->Semantic( evalData, typeEnv, binder );
            if ( FAILED( hr ) )
                return hr;

            // if child is value or type
            if ( Child->Kind != DataKind_Declaration )
            {
                ITypeStruct* t = NULL;

                if ( Child->_Type == NULL )
                    return E_MAGOEE_NO_TYPE;

                if ( Child->_Type->AsTypeStruct() != NULL )
                    t = Child->_Type->AsTypeStruct();
                else if ( Child->_Type->IsPointer() 
                    && (Child->_Type->AsTypeNext()->GetNext()->AsTypeStruct() != NULL) )
                    t = Child->_Type->AsTypeNext()->GetNext()->AsTypeStruct();

                if ( t != NULL )
                {
                    Decl = t->FindObject( Id->Str );
                    if( !Decl )
                    { 
                        hr = findTuple(Id->Str, binder, Decl.Ref(),
                            [&](const wchar_t* id, Declaration*& decl)
                            {
                                RefPtr<Declaration> iddecl = t->FindObject( id );
                                if( !iddecl )
                                    return E_NOT_FOUND;
                                decl = iddecl.Detach();
                                return S_OK;
                            });
                        if( FAILED( hr ) )
                            return hr;
                    }

                }
            }
            else
            {
                NamingExpression*   namer = Child->AsNamingExpression();
                if ( namer != NULL )
                {
                    _ASSERT( namer->Decl != NULL );
                    namer->Decl->FindObject( Id->Str, Decl.Ref() );
                    if( Decl == NULL )
                    {
                        hr = findTuple( Id->Str, binder, Decl.Ref(),
                            [&](const wchar_t* id, Declaration*& decl)
                            {
                                return namer->Decl->FindObject( id, decl);
                            });
                        if (FAILED(hr))
                            return hr;
                    }
                }
            }
        }

        if ( Decl != NULL )
        {
            Decl->GetType( _Type.Ref() );

            if ( Decl->IsVar() || Decl->IsConstant() || Decl->IsFunction() )
            {
                Kind = DataKind_Value;
            }
            else if ( Decl->IsField() )
            {
                if ( Child->Kind == DataKind_Value )
                    Kind = DataKind_Value;
                else
                    Kind = DataKind_Declaration;
            }
            else
            {
                Kind = DataKind_Declaration;
            }
        }
        else
        {
            hr = SemanticStdProperty( typeEnv );
            if ( FAILED( hr ) )
                return hr;
        }

        if ( (Kind == DataKind_Value) && (_Type == NULL) )
            return E_MAGOEE_NO_TYPE;

        return S_OK;
    }

    HRESULT DotExpr::SemanticStdProperty( ITypeEnv* typeEnv )
    {
        StdProperty*        prop = NULL;
        RefPtr<Declaration> childDecl;
        RefPtr<Type>        childType;

        if ( Child->AsNamingExpression() != NULL )
            childDecl = Child->AsNamingExpression()->Decl;

        childType = Child->_Type;

        if ( (childDecl != NULL) && childDecl->IsField() )
        {
            prop = FindFieldProperty( Id->Str );
        }

        if ( (prop == NULL) && (Child->_Type != NULL) )
        {
            prop = Child->_Type->FindProperty( Id->Str );
        }

        if ( prop == NULL )
            return E_MAGOEE_SYMBOL_NOT_FOUND;

        if ( !prop->GetType( typeEnv, childType, childDecl, _Type.Ref() ) )
            return E_FAIL;

        Property = prop;

        _ASSERT( _Type != NULL );
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT DotExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        HRESULT hr = S_OK;

        if ( Kind != DataKind_Value )
            return E_MAGOEE_VALUE_EXPECTED;

        obj._Type = _Type;

        if ( Property != NULL )
        {
            if ( mode == EvalMode_Address )
                return E_MAGOEE_NO_ADDRESS;

            return EvaluateStdProperty( evalData, binder, obj );
        }

        if ( Decl->IsField() || _Type->IsDelegate() )
        {
            // apply the parent's address
            int         offset = 0;
            DataObject  parent = { 0 };

            hr = Child->Evaluate( EvalMode_Value, evalData, binder, parent );
            if ( FAILED( hr ) )
                return hr;

            // as opposed to a pointer or reference to the class
            if ( parent._Type->AsTypeStruct() != NULL )
                parent.Value.Addr = parent.Addr;

            if ( parent.Value.Addr == 0 )
                return E_FAIL;

            if( _Type->IsDelegate() )
            {
                int vtblOffset;
                if( Decl->GetVtblOffset( vtblOffset ) )
                {
                    DataValue vtblptr = { 0 };
                    auto vtblptrType = _Type->AsTypeNext()->GetNext(); // any type of pointer
                    hr = binder->GetValue( parent.Value.Addr, vtblptrType, vtblptr );
                    if ( FAILED( hr ) )
                        return hr;
                    if( vtblptr.Addr == 0 )
                        return E_MAGOEE_NO_ADDRESS;

                    hr = binder->GetValue( vtblptr.Addr + vtblOffset, vtblptrType, vtblptr );
                    if ( FAILED( hr ) )
                        return hr;

                    obj.Value.Delegate.FuncAddr = vtblptr.Addr;
                }
                else if( !Decl->GetAddress( obj.Value.Delegate.FuncAddr ) )
                    return E_MAGOEE_NO_ADDRESS;

                obj.Value.Delegate.ContextAddr = parent.Value.Addr;
                return S_OK;
            }
            else
            {
                if ( !Decl->GetOffset( offset ) )
                    return E_FAIL;

                obj.Addr = parent.Value.Addr + offset;
            }
        }
        // else is some other value: constant, var
        else
            Decl->GetAddress( obj.Addr );

        if ( mode == EvalMode_Address )
        {
            if ( obj.Addr != 0 )
                return S_OK;

            return E_MAGOEE_NO_ADDRESS;
        }

        // evaluate a scalar we might have
        return Eval( binder, Decl, obj );
    }

    HRESULT DotExpr::EvaluateStdProperty( const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        _ASSERT( Property != NULL );

        HRESULT             hr = S_OK;
        RefPtr<Declaration> childDecl;
        RefPtr<Type>        childType;

        if ( Child->AsNamingExpression() != NULL )
            childDecl = Child->AsNamingExpression()->Decl;

        childType = Child->_Type;

        if ( Property->UsesParentValue() )
        {
            DataObject  parent = { 0 };

            hr = Child->Evaluate( EvalMode_Value, evalData, binder, parent );
            if ( FAILED( hr ) )
                return hr;

            Property->GetValue( Child->_Type, childDecl, parent.Value, obj.Value );
        }
        else
        {
            Property->GetValue( Child->_Type, childDecl, obj.Value );
        }

        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  ThisExpr
    //----------------------------------------------------------------------------

    HRESULT ThisExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( typeEnv );

        HRESULT hr = S_OK;
        ClearEvalData();

        hr = binder->GetThis( Decl.Ref() );
        if ( FAILED( hr ) )
            return hr;

        // has to have a type
        if ( !Decl->GetType( _Type.Ref() ) )
            return E_MAGOEE_NO_TYPE;

        // and the type has to be struct compatible: struct or ptr to struct
        if ( !_Type->CanRefMember() )
            return E_FAIL;

        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT ThisExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( evalData );

        obj._Type = _Type;
        Decl->GetAddress( obj.Addr );

        if ( mode == EvalMode_Address )
        {
            if ( obj.Addr != 0 )
                return S_OK;

            return E_MAGOEE_NO_ADDRESS;
        }

        // evaluate a scalar value (pointer) we might have
        return Eval( binder, Decl, obj );
    }


    //----------------------------------------------------------------------------
    //  SuperExpr
    //----------------------------------------------------------------------------

    HRESULT SuperExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( typeEnv );

        HRESULT hr = S_OK;
        ClearEvalData();

        hr = binder->GetSuper( Decl.Ref() );
        if ( FAILED( hr ) )
            return hr;

        // has to have a type
        if ( !Decl->GetType( _Type.Ref() ) )
            return E_FAIL;

        // and the type has to be struct compatible: struct or ptr to struct
        if ( !_Type->CanRefMember() )
            return E_FAIL;

        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT SuperExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( evalData );

        obj._Type = _Type;
        Decl->GetAddress( obj.Addr );

        if ( mode == EvalMode_Address )
        {
            if ( obj.Addr != 0 )
                return S_OK;

            return E_MAGOEE_NO_ADDRESS;
        }

        // evaluate a scalar value (pointer) we might have
        return Eval( binder, Decl, obj );
    }


    //----------------------------------------------------------------------------
    //  TypeExpr
    //----------------------------------------------------------------------------

    HRESULT TypeExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        ClearEvalData();

        _Type = UnresolvedType->Resolve( evalData, typeEnv, binder );
        if ( _Type == NULL )
            return E_MAGOEE_TYPE_RESOLVE_FAILED;

        Kind = DataKind_Type;
        return S_OK;
    }

    HRESULT TypeExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( mode );
        UNREFERENCED_PARAMETER( binder );
        UNREFERENCED_PARAMETER( obj );

        // can't evaluate a type
        return E_MAGOEE_VALUE_EXPECTED;
    }


    //----------------------------------------------------------------------------
    //  DollarExpr
    //----------------------------------------------------------------------------

    HRESULT DollarExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( binder );

        ClearEvalData();

        if ( !evalData.HasArrayLength )
            return E_MAGOEE_BAD_INDEX;

        _Type = typeEnv->GetType( Tuns32 );
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT DollarExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( binder );

        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        obj.Value.UInt64Value = evalData.ArrayLength;
        obj._Type = _Type;
        _ASSERT( obj.Addr == 0 );
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  DotTemplateInstanceExpr
    //----------------------------------------------------------------------------

    HRESULT DotTemplateInstanceExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        std::wstring    fullId;
        ClearEvalData();

        RefPtr<SharedString>    namePath;
        hr = MakeName( 0, namePath );
        if ( FAILED( hr ) && (hr != E_MAGOEE_SYMBOL_NOT_FOUND) )
            return hr;

        if ( SUCCEEDED( hr ) )
        {
            const wchar_t*  name = mNamePath->GetCut( mNamePathLen );

            binder->FindObject( name, Decl.Ref(), IValueBinder::FindObjectAny );

            mNamePath->ReleaseCut();
        }

        if ( Decl == NULL )
        {
            hr = Child->Semantic( evalData, typeEnv, binder );
            if ( FAILED( hr ) )
                return hr;

            fullId.append( Instance->Id->Str );
            fullId.append( Instance->ArgumentString->Str );

            // if child is value or type
            if ( Child->Kind != DataKind_Declaration )
            {
                ITypeStruct* t = NULL;

                if ( Child->_Type == NULL )
                    return E_MAGOEE_NO_TYPE;

                if ( Child->_Type->AsTypeStruct() != NULL )
                    t = Child->_Type->AsTypeStruct();
                else if ( Child->_Type->IsPointer() 
                    && (Child->_Type->AsTypeNext()->GetNext()->AsTypeStruct() != NULL) )
                    t = Child->_Type->AsTypeNext()->GetNext()->AsTypeStruct();

                if ( t != NULL )
                {
                    Decl = t->FindObject( fullId.c_str() );
                }
            }
            else
            {
                NamingExpression*   namer = Child->AsNamingExpression();
                if ( namer != NULL )
                {
                    _ASSERT( namer->Decl != NULL );
                    namer->Decl->FindObject( fullId.c_str(), Decl.Ref() );
                }
            }
        }

        if ( (Decl == NULL) || !Decl->IsType() )
            return E_MAGOEE_SYMBOL_NOT_FOUND;

        Decl->GetType( _Type.Ref() );

        Kind = DataKind_Declaration;

        if ( _Type == NULL )
            return E_MAGOEE_NO_TYPE;

        return S_OK;
    }

    HRESULT DotTemplateInstanceExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( mode );
        UNREFERENCED_PARAMETER( binder );
        UNREFERENCED_PARAMETER( obj );

        // can't evaluate a type
        return E_MAGOEE_VALUE_EXPECTED;
    }


    //----------------------------------------------------------------------------
    //  ScopeExpr
    //----------------------------------------------------------------------------

    HRESULT ScopeExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( typeEnv );

        HRESULT hr = S_OK;
        std::wstring    fullId;
        ClearEvalData();

        fullId.append( Instance->Id->Str );
        fullId.append( Instance->ArgumentString->Str );

        hr = binder->FindObject( fullId.c_str(), Decl.Ref(), IValueBinder::FindObjectAny );
        if ( FAILED( hr ) )
            return hr;

        Decl->GetType( _Type.Ref() );

        Kind = DataKind_Declaration;

        if ( !Decl->IsType() || (_Type == NULL) )
            return E_MAGOEE_NO_TYPE;

        return S_OK;
    }

    HRESULT ScopeExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( mode );
        UNREFERENCED_PARAMETER( binder );
        UNREFERENCED_PARAMETER( obj );

        // can't evaluate a type
        return E_MAGOEE_VALUE_EXPECTED;
    }


    //----------------------------------------------------------------------------
    //  InExpr
    //----------------------------------------------------------------------------

    HRESULT InExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = Right->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;
        if ( Right->Kind != DataKind_Value )
            return E_MAGOEE_VALUE_EXPECTED;
        if ( Right->_Type == NULL )
            return E_MAGOEE_NO_TYPE;

        Type*   childType = Right->_Type;
        if ( !childType->IsAArray() )
            return E_MAGOEE_BAD_INDEX;

        ITypeNext*  typeNext = Right->_Type->AsTypeNext();
        _ASSERT( typeNext != NULL );
        if ( typeNext->GetNext() == NULL )
            return E_MAGOEE_BAD_INDEX;

        RefPtr<Type>    voidType = typeEnv->GetType( Tvoid );
        if ( typeNext->GetNext()->Equals( voidType ) )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        hr = Left->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;
        if ( Left->Kind != DataKind_Value )
            return E_MAGOEE_VALUE_EXPECTED;
        if ( Left->_Type == NULL )
            return E_MAGOEE_NO_TYPE;

        hr = TypeCheckAAKey( Left->_Type, childType->AsTypeAArray()->GetIndex() );
        if ( FAILED( hr ) )
            return hr;

        Left->TrySetType( childType->AsTypeAArray()->GetIndex() );

        hr = typeEnv->NewPointer( typeNext->GetNext(), _Type.Ref() );
        if ( FAILED( hr ) )
            return hr;

        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT InExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;
        DataObject  array = { 0 };
        DataObject  index = { 0 };
        Address     addr = 0;

        hr = Right->Evaluate( EvalMode_Value, evalData, binder, array );
        if ( FAILED( hr ) )
            return hr;

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, index );
        if ( FAILED( hr ) )
            return hr;

        hr = FindAAElement( array, index, Right->_Type->AsTypeAArray()->GetIndex(), binder, addr );
        if ( hr == E_NOT_FOUND )
            addr = 0;
        else if ( FAILED( hr ) )
            return hr;

        obj.Addr = 0;
        obj._Type = _Type;
        obj.Value.Addr = addr;

        return S_OK;
    }

    //----------------------------------------------------------------------------
    //  CallExpr
    //----------------------------------------------------------------------------

    HRESULT CallExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr;

        hr = Child->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;
        if ( Child->_Type == NULL )
            return E_MAGOEE_NO_TYPE;

        auto type = Child->_Type;
        if ( type->IsDelegate() ) // delegate has pointer to function as "next"
        {
            if( auto ptrtype = type->AsTypeNext()->GetNext()->AsTypeNext() )
                type = ptrtype->GetNext();
        }
        else if( type->IsPointer() && type->AsTypeNext()->GetNext()->IsFunction() )
            type = type->AsTypeNext()->GetNext();

        ITypeFunction* func = type->AsTypeFunction();
        if ( !func )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        ParameterList* paramList = func->GetParams();
        if( !paramList )
            return E_MAGOEE_TYPE_RESOLVE_FAILED;

        auto it = paramList->List.begin();

        for( Expression* expr : Args->List )
        {
            if( it == paramList->List.end() )
                return E_MAGOEE_TOO_MANY_ARGUMENTS;

            hr = expr->Semantic( evalData, typeEnv, binder );
            if ( FAILED( hr ) )
                return hr;
            if ( expr->Kind != DataKind_Value )
                return E_MAGOEE_VALUE_EXPECTED;
            if ( expr->_Type == NULL )
                return E_MAGOEE_NO_TYPE;

            if( !expr->_Type->Equals( it->Get()->_Type ) )
                return E_MAGOEE_BAD_TYPES_FOR_OP;
            ++it;
        }
        if( it != paramList->List.end() )
            return E_MAGOEE_TOO_FEW_ARGUMENTS;

        _Type = func->GetReturnType();
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT CallExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        HRESULT hr;

        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        if ( !evalData.Options.AllowFuncExec )
            return E_MAGOEE_NOFUNCCALL;

        if ( Args->List.size() != 0 )
            return E_MAGOEE_CALLARGS_NOT_IMPLEMENTED;

        auto ne = Child->AsNamingExpression();
        if (ne == NULL)
            return E_MAGOEE_TYPE_RESOLVE_FAILED;

        DataObject callee = { 0 };
        if ( Child->Kind == DataKind_Value )
        {
            hr = Child->Evaluate( EvalMode_Value, evalData, binder, callee );
            if ( FAILED( hr) )
                return hr;
        }
        else
        {
            if( !ne->Decl || !ne->Decl->GetAddress( callee.Addr ) )
                return E_MAGOEE_NO_ADDRESS;
        }

        Address addr = callee.Addr;
        Address ctxt = 0;

        auto type = Child->_Type;
        if ( type->IsDelegate() ) // delegate has pointer to function as "next"
        {
            if( auto ptrtype = type->AsTypeNext()->GetNext()->AsTypeNext() )
            {
                type = ptrtype->GetNext();
                addr = callee.Value.Delegate.FuncAddr;
                ctxt = callee.Value.Delegate.ContextAddr;
            }
        }
        else if (type->IsPointer() && type->AsTypeNext()->GetNext()->IsFunction())
        {
            type = type->AsTypeNext()->GetNext();
            addr = callee.Value.Addr;
        }
        ITypeFunction* func = type->AsTypeFunction();

        if ( !evalData.Options.AllowAssignment && !func->IsPure() )
            return E_MAGOEE_HASSIDEEFFECT;

        obj._Type = _Type;
        hr = binder->CallFunction( addr, func, ctxt, obj );
        return hr;
    }

}
