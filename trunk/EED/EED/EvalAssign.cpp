/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

// Assign: cast and assign

#include "Common.h"
#include "Expression.h"
#include "Declaration.h"
#include "Type.h"
#include "TypeCommon.h"


// There are 3 things that let an expression be an l-value.
// Keep these in mind when deciding that an expression needs to be an l-value:
//
// 1. Responding well to EvalMode_Address.
// 2. Returning a DataObject with an address.
// 3. Being a NamingExpression and having a Declaration.
//
// Another consideration is that we can add a Declaration member to DataObject.


namespace MagoEE
{
    //----------------------------------------------------------------------------
    //  CastExpr
    //----------------------------------------------------------------------------

    HRESULT CastExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
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

        Type*       childType = Child->_Type;

        if ( _TypeTo != NULL )
        {
            RefPtr<Type>    resolvedTypeTo = _TypeTo->Resolve( evalData, typeEnv, binder );

            if ( resolvedTypeTo == NULL )
                return E_MAGOEE_TYPE_RESOLVE_FAILED;

            if ( Child->TrySetType( resolvedTypeTo ) )
            {
                _Type = resolvedTypeTo;
            }
            else if ( CanCast( childType, resolvedTypeTo ) )
            {
                _Type = resolvedTypeTo;
            }
            else
                return E_MAGOEE_BAD_CAST;
        }
        else if ( childType->Mod == FlagsTo )
        {
            // no change
            _Type = childType;
        }
        else
        {
            switch ( FlagsTo & (MODshared | MODconst) )
            {
            case MODshared | MODconst:  _Type = childType->MakeSharedConst();   break;
            case MODconst:              _Type = childType->MakeConst();         break;
            case MODshared:             _Type = childType->MakeShared();        break;
            default:
                if ( (FlagsTo & MODinvariant) == MODinvariant )
                    _Type = childType->MakeInvariant();
                else
                    _Type = childType->MakeMutable();
                break;
            }
        }

        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT CastExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT hr = S_OK;
        DataObject  childObj = { 0 };

        hr = Child->Evaluate( EvalMode_Value, evalData, binder, childObj );
        if ( FAILED( hr ) )
            return hr;

        obj._Type = _Type;

        //      it's not an l-value unless *casting to original type* and child was an l-value
        //      the point is that we're saying it's an r-value only, even though it could be an l-value

        // ***  But for simplicity, we'll leave this as an r-value.
        //      Note that we're not a NamingExpression so we don't have a Declaration field, 
        //      for those l-values that don't have addresses.
        _ASSERT( obj.Addr == 0 );

        AssignValue( childObj, obj );

        return S_OK;
    }

    // A := B
    //      B-> integral    floating    pointer
    // integral X           X           X
    // floating X           X           X
    // pointer  X           X           X

    bool CastExpr::CanImplicitCast( Type* source, Type* dest )
    {
        // even though the D langauge doesn't allow the assignment of a float to a pointer,
        // we'll allow it here to for our own simplicity and convenience, and to 
        // match VC++'s debugger.
        // For reference, DMD gives this error when assigning a float to a pointer:
        // Real D: Error: cannot implicitly convert expression (99.9) of type double to int*

        return CanCast( source, dest );
    }

    // TODO: is this all?
    // down to across
    //          integral    floating    pointer
    // integral X           X           X
    // floating X           X           
    // pointer  X           X           X

    bool CastExpr::CanCast( Type* source, Type* dest )
    {
        if (    (dest->IsIntegral() && source->IsIntegral())
            ||  (dest->IsIntegral() && source->IsFloatingPoint())
            ||  (dest->IsIntegral() && source->IsPointer())

            ||  (dest->IsFloatingPoint() && source->IsIntegral())
            ||  (dest->IsFloatingPoint() && source->IsFloatingPoint())
            ||  (dest->IsFloatingPoint() && source->IsPointer())

            ||  (dest->IsPointer() && source->IsIntegral())
            ||  (dest->IsPointer() && source->IsPointer())
            ||  (dest->IsPointer() && source->IsFloatingPoint())
            ||  (dest->IsPointer() && source->IsSArray())
            ||  (dest->IsPointer() && source->IsDArray())
            ||  (dest->IsPointer() && source->IsAArray())
            ||  (dest->IsPointer() && source->IsDelegate())

            ||  (dest->IsDArray() && source->IsSArray())
            ||  (dest->IsDArray() && source->IsDArray())

            ||  (dest->IsBool() && source->IsSArray())
            ||  (dest->IsBool() && source->IsDArray())
            ||  (dest->IsBool() && source->IsAArray())
            ||  (dest->IsBool() && source->IsDelegate())
            )
            return true;

        return false;
    }

    // TODO: is this OK not returning anything?
    void CastExpr::AssignValue( DataObject& source, DataObject& dest )
    {
        Type*   destType = dest._Type;
        Type*   srcType = source._Type;

        if ( destType->IsBool() )
        {
            dest.Value.UInt64Value = ConvertToBool( source ) ? 1 : 0;
        }
        else if ( destType->IsComplex() )
        {
            dest.Value.Complex80Value = ConvertToComplex( source );
        }
        else if ( destType->IsImaginary() )
        {
            if ( srcType->IsComplex() )
            {
                dest.Value.Float80Value = source.Value.Complex80Value.ImaginaryPart;
            }
            else if ( srcType->IsImaginary() )
            {
                dest.Value.Float80Value = source.Value.Float80Value;
            }
            else
            {
                _ASSERT( srcType->IsPointer()
                    || srcType->IsIntegral()
                    || srcType->IsReal() );

                dest.Value.Float80Value.Zero();
            }
        }
        else if ( destType->IsReal() )
        {
            if ( srcType->IsComplex() )
            {
                dest.Value.Float80Value = source.Value.Complex80Value.RealPart;
            }
            else if ( srcType->IsImaginary() )
            {
                dest.Value.Float80Value.Zero();
            }
            else if ( srcType->IsReal() )
            {
                dest.Value.Float80Value = source.Value.Float80Value;
            }
            else if ( srcType->IsIntegral() )
            {
                if ( srcType->IsSigned() )
                    dest.Value.Float80Value.FromInt64( source.Value.Int64Value );
                else
                    dest.Value.Float80Value.FromUInt64( source.Value.UInt64Value );
            }
            else
            {
                _ASSERT( srcType->IsPointer() );

                dest.Value.Float80Value.FromUInt64( source.Value.Addr );
            }
        }
        else if ( destType->IsIntegral() )
        {
            if ( srcType->IsComplex() )
            {
                if ( (destType->GetSize() == 8) && !destType->IsSigned() )
                    dest.Value.UInt64Value = source.Value.Complex80Value.RealPart.ToUInt64();
                else if ( (destType->GetSize() == 8) || (!destType->IsSigned() && destType->GetSize() == 4) )
                    dest.Value.Int64Value = source.Value.Complex80Value.RealPart.ToInt64();
                else if ( (destType->GetSize() == 4) || (!destType->IsSigned() && destType->GetSize() == 2) )
                    dest.Value.Int64Value = source.Value.Complex80Value.RealPart.ToInt32();
                else
                    dest.Value.Int64Value = source.Value.Complex80Value.RealPart.ToInt16();
            }
            else if ( srcType->IsImaginary() )
            {
                dest.Value.Int64Value = 0;
            }
            else if ( srcType->IsReal() )
            {
                if ( (destType->GetSize() == 8) && !destType->IsSigned() )
                    dest.Value.UInt64Value = source.Value.Float80Value.ToUInt64();
                else if ( (destType->GetSize() == 8) || (!destType->IsSigned() && destType->GetSize() == 4) )
                    dest.Value.Int64Value = source.Value.Float80Value.ToInt64();
                else if ( (destType->GetSize() == 4) || (!destType->IsSigned() && destType->GetSize() == 2) )
                    dest.Value.Int64Value = source.Value.Float80Value.ToInt32();
                else
                    dest.Value.Int64Value = source.Value.Float80Value.ToInt16();
            }
            else if ( srcType->IsIntegral() )
            {
                dest.Value.UInt64Value = source.Value.UInt64Value;
            }
            else
            {
                _ASSERT( srcType->IsPointer() );
                dest.Value.UInt64Value = source.Value.Addr;
            }

            PromoteInPlace( dest );
        }
        else if ( destType->IsPointer() )
        {
            if ( srcType->IsComplex() )
            {
                dest.Value.Addr = source.Value.Complex80Value.RealPart.ToUInt64();
            }
            else if ( srcType->IsImaginary() )
            {
                dest.Value.Addr = 0;
            }
            else if ( srcType->IsReal() )
            {
                dest.Value.Addr = source.Value.Float80Value.ToUInt64();
            }
            else if ( srcType->IsIntegral() )
            {
                dest.Value.Addr = source.Value.UInt64Value;
            }
            else if ( srcType->IsPointer() )
            {
                RefPtr<Type>    nextSrc = srcType->AsTypeNext()->GetNext();
                RefPtr<Type>    nextDest = destType->AsTypeNext()->GetNext();

                if ( (nextSrc != NULL) && (nextSrc->AsTypeStruct() != NULL)
                    && (nextDest != NULL) && (nextDest->AsTypeStruct() != NULL) )
                {
                    int offset = 0;

                    if ( nextSrc->AsTypeStruct()->GetBaseClassOffset( nextDest, offset ) )
                    {
                        source.Value.Addr += offset;
                    }
                }

                dest.Value.Addr = source.Value.Addr;
            }
            else if ( srcType->IsDArray() )
            {
                dest.Value.Addr = source.Value.Array.Addr;
            }
            else if ( srcType->IsSArray() )
            {
                dest.Value.Addr = source.Addr;
            }
            else if ( srcType->IsAArray() )
            {
                dest.Value.Addr = source.Value.Addr;
            }
            else if ( srcType->IsDelegate() )
            {
                dest.Value.Addr = source.Value.Delegate.ContextAddr;
            }
            else
                _ASSERT( false );

            if ( destType->GetSize() == 4 )
                dest.Value.Addr &= 0xFFFFFFFF;
        }
        else if ( destType->IsDArray() )
        {
            ConvertToDArray( source, dest );
        }
        else
            _ASSERT( false );
    }


    //----------------------------------------------------------------------------
    //  AssignExpr
    //----------------------------------------------------------------------------

    HRESULT AssignExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = SemanticVerifyChildren( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;

        if ( Right->TrySetType( Left->_Type ) )
        {
            // OK
        }
        else if ( !CastExpr::CanImplicitCast( Right->_Type, Left->_Type ) )
            return E_MAGOEE_BAD_CAST;

        _Type = Left->_Type;
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT AssignExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        // strictly an r-value
        // it goes along with returning no address
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT hr = S_OK;
        DataObject  right = { 0 };
        RefPtr<Declaration> decl;

        if ( Left->AsNamingExpression() != NULL )
            decl = Left->AsNamingExpression()->Decl;

        if ( (decl != NULL) && decl->IsVar() )
        {
            // all set, this declaration will be used to set the value
            // it can be a memory object or a register, it doesn't matter
            obj._Type = _Type;
        }
        else
        {
            // otherwise, it has to be a declaration or expression with an address
            hr = Left->Evaluate( EvalMode_Address, evalData, binder, obj );
            if ( FAILED( hr ) )
                return hr;
        }

        hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
        if ( FAILED( hr ) )
            return hr;

        CastExpr::AssignValue( right, obj );

        // TODO: if we can't set the value, then what value should we return?
        //       - right side?
        //       - old left side?
        //       - new left side?
        //       Right now we're returning new left side.
        //       All this applies to CombinedAssignExpr, too.

        if ( evalData.Options.AllowAssignment )
        {
            if ( obj.Addr != 0 )
                hr = binder->SetValue( obj.Addr, _Type, obj.Value );
            else if ( decl != NULL )
                hr = binder->SetValue( decl, obj.Value );
            else
                hr = E_MAGOEE_LVALUE_EXPECTED;
        }

        // strictly an r-value
        // it goes along with disallowing mode == EvalMode_Address
        // note that we're also not a NamingExpression, 
        // so we don't have a declaration field to pass to our parent
        obj.Addr = 0;
        return hr;
    }


    //----------------------------------------------------------------------------
    //  CombinedAssignExpr
    //----------------------------------------------------------------------------

    // A := B
    //      B-> integral    floating    pointer
    // integral X           X           X
    // floating X           X           X
    // pointer  X                       X

    HRESULT CombinedAssignExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = Child->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;

        if ( !CastExpr::CanImplicitCast( Child->_Type, Child->Left->_Type ) )
            return E_MAGOEE_BAD_CAST;

        _Type = Child->Left->_Type;
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT CombinedAssignExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        // strictly an r-value
        // it goes along with returning no address
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT hr = S_OK;
        DataObject  childObj = { 0 };

        hr = Child->Evaluate( EvalMode_Value, evalData, binder, childObj );
        if ( FAILED( hr ) )
            return hr;

        obj._Type = _Type;
        CastExpr::AssignValue( childObj, obj );

        RefPtr<Declaration> decl;
        if ( Child->Left->AsNamingExpression() != NULL )
            decl = Child->Left->AsNamingExpression()->Decl;

        if ( evalData.Options.AllowAssignment )
        {
            obj.Addr = Child->LeftAddr;
            if ( obj.Addr != 0 )
                hr = binder->SetValue( obj.Addr, _Type, obj.Value );
            else if ( decl != NULL )
                hr = binder->SetValue( decl, obj.Value );
            else
                hr = E_MAGOEE_LVALUE_EXPECTED;
        }

        // strictly an r-value
        // it goes along with disallowing mode == EvalMode_Address
        // note that we're also not a NamingExpression, 
        // so we don't have a declaration field to pass to our parent
        obj.Addr = 0;

        if ( IsPostOp )
        {
            // take the original value of the left child (the l-value of this assignment)
            // and return that as our end result
            obj.Value = Child->LeftValue;
        }

        return hr;
    }
}
