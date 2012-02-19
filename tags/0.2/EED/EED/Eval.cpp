/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

// Base: Base class methods, stubs, and conversions

#include "Common.h"
#include "Expression.h"
#include "Declaration.h"
#include "Type.h"
#include "TypeCommon.h"
#include "ITypeEnv.h"


namespace MagoEE
{
    HRESULT Expression::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( binder );
        _ASSERT( false );
        return E_NOTIMPL;
    }

    HRESULT Expression::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( mode );
        UNREFERENCED_PARAMETER( binder );
        UNREFERENCED_PARAMETER( obj );
        _ASSERT( false );
        return E_NOTIMPL;
    }

    //----------------------------------------------------------------------------
    //  Conversions
    //----------------------------------------------------------------------------

    bool Expression::ConvertToBool( DataObject& obj )
    {
        if ( obj._Type->IsPointer() )
        {
            return obj.Value.Addr != 0;
        }
        else if ( obj._Type->IsComplex() )
        {
        // TODO: this needs to be cleared up
        //       In DMD, explicitly casting to bool treats all imaginaries as a 0 zero, known as false.
        //       While implicit casting (for example in and && expr) treats non-zero as true, and zero as false.
#if 0
            return !obj.Value.Complex80Value.RealPart.IsZero();
#else
            return !obj.Value.Complex80Value.RealPart.IsZero()
                || !obj.Value.Complex80Value.ImaginaryPart.IsZero();
#endif
        }
        else if ( obj._Type->IsImaginary() )
        {
        // TODO: this needs to be cleared up
        //       In DMD, explicitly casting to bool treats all imaginaries as a 0 zero, known as false.
        //       While implicit casting (for example in and && expr) treats non-zero as true, and zero as false.
#if 0
            return false;
#else
            return !obj.Value.Float80Value.IsZero();
#endif
        }
        else if ( obj._Type->IsFloatingPoint() )
        {
            return !obj.Value.Float80Value.IsZero();
        }
        else if ( obj._Type->IsIntegral() )
        {
            return obj.Value.UInt64Value != 0;
        }
        else if ( obj._Type->IsDArray() )
        {
            return obj.Value.Array.Addr != 0;
        }
        else if ( obj._Type->IsSArray() )
        {
            return obj.Addr != 0;
        }
        else if ( obj._Type->IsAArray() )
        {
            return obj.Value.Addr != 0;
        }
        else if ( obj._Type->IsDelegate() )
        {
            return (obj.Value.Delegate.ContextAddr != 0) || (obj.Value.Delegate.FuncAddr != 0);
        }

        _ASSERT( false );
        return false;
    }

    Complex10 Expression::ConvertToComplex( DataObject& x )
    {
        Complex10  result;

        Type*   type = x._Type.Get();

        if ( type->IsImaginary() )
        {
            result.RealPart.Zero();
            result.ImaginaryPart = x.Value.Float80Value;
        }
        else if ( type->IsReal() )
        {
            result.RealPart = x.Value.Float80Value;
            result.ImaginaryPart.Zero();
        }
        else if ( type->IsIntegral() )
        {
            if ( type->IsSigned() )
                result.RealPart.FromInt64( x.Value.Int64Value );
            else
                result.RealPart.FromUInt64( x.Value.UInt64Value );
            result.ImaginaryPart.Zero();
        }
        else if ( type->IsPointer() )
        {
            result.RealPart.FromUInt64( x.Value.Addr );
            result.ImaginaryPart.Zero();
        }
        else
        {
            _ASSERT( type->IsComplex() );
            result = x.Value.Complex80Value;
        }

        return result;
    }

    // TODO: what's the right name for this? Is it really ReinterpretFloat? ...Real?
    Real10 Expression::ConvertToFloat( DataObject& x )
    {
        Real10  result;

        Type*   type = x._Type.Get();

        if ( type->IsIntegral() )
        {
            if ( type->IsSigned() )
                result.FromInt64( x.Value.Int64Value );
            else
                result.FromUInt64( x.Value.UInt64Value );
        }
        else
        {
            _ASSERT( type->IsReal() || type->IsImaginary() );
            result = x.Value.Float80Value;
        }

        return result;
    }

    void Expression::ConvertToDArray( const DataObject& source, DataObject& dest )
    {
        Type*               destType = dest._Type;
        Type*               srcType = source._Type;
        Address             addr = 0;
        dlength_t           srcLen = 0;

        _ASSERT( srcType->IsSArray() || srcType->IsDArray() );

        if ( srcType->IsSArray() )
        {
            addr = source.Addr;
            srcLen = srcType->AsTypeSArray()->GetLength();
        }
        else if ( srcType->IsDArray() )
        {
            addr = source.Value.Array.Addr;
            srcLen = source.Value.Array.Length;
        }

        Type*               srcElemType = srcType->AsTypeNext()->GetNext();
        Type*               destElemType = destType->AsTypeNext()->GetNext();
        dlength_t           totalSize = srcLen * srcElemType->GetSize();
        dlength_t           destLen = 0;

        // get the length if the sizes line up
        if ( (destElemType->GetSize() > 0) && ((totalSize % destElemType->GetSize()) == 0) )
            destLen = totalSize / destElemType->GetSize();
        // TODO: maybe we should fail if the sizes don't line up
        //      ...and we can tell the user with an explicit error code

        if ( destLen == 0 )
            addr = 0;

        dest.Value.Array.Addr = addr;
        dest.Value.Array.Length = destLen;
    }

    void Expression::PromoteInPlace( DataObject& x )
    {
        _ASSERT( x._Type != NULL );

        switch ( x._Type->GetBackingTy() )
        {
        case Tint32:    x.Value.UInt64Value = (int32_t) x.Value.UInt64Value;    break;
        case Tuns32:    x.Value.UInt64Value = (uint32_t) x.Value.UInt64Value;   break;
        case Tint16:    x.Value.UInt64Value = (int16_t) x.Value.UInt64Value;    break;
        case Tuns16:    x.Value.UInt64Value = (uint16_t) x.Value.UInt64Value;   break;
        case Tint8:     x.Value.UInt64Value = (int8_t) x.Value.UInt64Value;     break;
        case Tuns8:     x.Value.UInt64Value = (uint8_t) x.Value.UInt64Value;    break;
        default:        _ASSERT( x._Type->GetSize() == 8 );
        }
    }

    void Expression::PromoteInPlace( DataObject& x, Type* targetType )
    {
        _ASSERT( x._Type != NULL );
        _ASSERT( targetType != NULL );

        if ( (targetType->GetSize() == 8) || targetType->IsSigned() )
        {
            PromoteInPlace( x );
        }
        else
        {
            _ASSERT( targetType->GetSize() == 4 );
            _ASSERT( x._Type->GetSize() <= 4 );

            uint32_t    y = 0;

            // certain operations have intermediate results where the operands were promoted to uint
            // the standard PromoteInPlace assumes an end result

            switch ( x._Type->GetBackingTy() )
            {
            case Tint32:    y = (int32_t) x.Value.UInt64Value;    break;
            case Tuns32:    y = (uint32_t) x.Value.UInt64Value;   break;
            case Tint16:    y = (int16_t) x.Value.UInt64Value;    break;
            case Tuns16:    y = (uint16_t) x.Value.UInt64Value;   break;
            case Tint8:     y = (int8_t) x.Value.UInt64Value;     break;
            case Tuns8:     y = (uint8_t) x.Value.UInt64Value;    break;
            default:        _ASSERT( x._Type->GetSize() == 8 );
            }

            x.Value.UInt64Value = y;
        }
    }

    RefPtr<Type> Expression::PromoteComplexType( ITypeEnv* typeEnv, Type* t )
    {
        _ASSERT( t->IsIntegral() || t->IsFloatingPoint() );

        RefPtr<Type>    type;

        if ( t->IsComplex() )
        {
            type = typeEnv->GetType( t->GetBackingTy() );
        }
        else if ( t->IsFloatingPoint() )
        {
            switch ( t->GetBackingTy() )
            {
            case Tfloat32:  case Timaginary32:  type = typeEnv->GetType( Tcomplex32 );  break;
            case Tfloat64:  case Timaginary64:  type = typeEnv->GetType( Tcomplex64 );  break;
            case Tfloat80:  case Timaginary80:  type = typeEnv->GetType( Tcomplex80 );  break;
            default:
                _ASSERT_EXPR( false, L"Unknown float size." );
            }
        }
        else if ( t->IsIntegral() )
            type = typeEnv->GetType( Tcomplex32 );
        else
            _ASSERT_EXPR( false, L"Can't cast to float." );

        return type;
    }

    RefPtr<Type> Expression::PromoteImaginaryType( ITypeEnv* typeEnv, Type* t )
    {
        _ASSERT( t->IsIntegral() || t->IsReal() || t->IsImaginary() );

        RefPtr<Type>    type;

        if ( t->IsFloatingPoint() )
        {
            switch ( t->GetBackingTy() )
            {
            case Tfloat32:  case Timaginary32:  type = typeEnv->GetType( Timaginary32 );    break;
            case Tfloat64:  case Timaginary64:  type = typeEnv->GetType( Timaginary64 );    break;
            case Tfloat80:  case Timaginary80:  type = typeEnv->GetType( Timaginary80 );    break;
            default:
                _ASSERT_EXPR( false, L"Unknown float size." );
            }
        }
        else if ( t->IsIntegral() )
            type = typeEnv->GetType( Timaginary32 );
        else
            _ASSERT_EXPR( false, L"Can't cast to float." );

        return type;
    }

    RefPtr<Type> Expression::PromoteFloatType( ITypeEnv* typeEnv, Type* t )
    {
        _ASSERT( t->IsIntegral() || t->IsReal() );

        RefPtr<Type>    type;

        if ( t->IsFloatingPoint() )
        {
            type = typeEnv->GetType( t->GetBackingTy() );
        }
        else if ( t->IsIntegral() )
            type = typeEnv->GetType( Tfloat32 );
        else
            _ASSERT_EXPR( false, L"Can't cast to float." );

        return type;
    }

    RefPtr<MagoEE::Type> Expression::PromoteIntType( ITypeEnv* typeEnv, MagoEE::Type* t )
    {
        _ASSERT( t->IsIntegral() );

        RefPtr<Type>    intType = typeEnv->GetType( Tint32 );
        RefPtr<Type>    type;

        if ( t->GetSize() >= intType->GetSize() )
        {
            type = typeEnv->GetType( t->GetBackingTy() );
        }
        else
        {
            type = intType;
        }

        return type;
    }

    // TODO: there's a lot in common between all the Get*CommonType methods

    RefPtr<Type> Expression::GetCommonType( ITypeEnv* typeEnv, Type* left, Type* right )
    {
        RefPtr<Type>    ltype;
        RefPtr<Type>    rtype;
        RefPtr<Type>    type;

        // TODO: test if they're the same type and (floating-point or integral with size >= sizeof( int ))
        //      if so, the common type is the same as left and right's

        if ( left->IsComplex() || right->IsComplex() 
            || (left->IsImaginary() != right->IsImaginary()) )
        {
            ltype = PromoteComplexType( typeEnv, left );
            rtype = PromoteComplexType( typeEnv, right );

            if ( ltype == NULL || rtype == NULL )
                return NULL;

            if ( ltype->GetSize() > rtype->GetSize() )
                type = ltype;
            else
                type = rtype;
        }
        else if ( left->IsImaginary() && right->IsImaginary() )
        {
            ltype = PromoteImaginaryType( typeEnv, left );
            rtype = PromoteImaginaryType( typeEnv, right );

            if ( ltype == NULL || rtype == NULL )
                return NULL;

            if ( ltype->GetSize() > rtype->GetSize() )
                type = ltype;
            else
                type = rtype;
        }
        else if ( left->IsReal() || right->IsReal() )
        {
            ltype = PromoteFloatType( typeEnv, left );
            rtype = PromoteFloatType( typeEnv, right );

            if ( ltype == NULL || rtype == NULL )
                return NULL;

            if ( ltype->GetSize() > rtype->GetSize() )
                type = ltype;
            else
                type = rtype;
        }
        else if ( left->IsIntegral() && right->IsIntegral() )
        {
            ltype = PromoteIntType( typeEnv, left );
            rtype = PromoteIntType( typeEnv, right );

            if ( ltype == NULL || rtype == NULL )
                return NULL;

            if ( ltype->GetSize() == rtype->GetSize() )
            {
                if ( !ltype->IsSigned() )
                    type = ltype;
                else
                    type = rtype;
            }
            else if ( ltype->GetSize() > rtype->GetSize() )
                type = ltype;
            else // is less
                type = rtype;
        }
        // else we're dealing with non-scalars, which can't be combined, so return NULL

        return type;
    }

    RefPtr<Type> Expression::GetMulCommonType( ITypeEnv* typeEnv, Type* left, Type* right )
    {
        RefPtr<Type>    ltype;
        RefPtr<Type>    rtype;
        RefPtr<Type>    type;

        // TODO: test if they're the same type and (floating-point or integral with size >= sizeof( int ))
        //      if so, the common type is the same as left and right's

        if ( left->IsComplex() || right->IsComplex() )
        {
            ltype = PromoteComplexType( typeEnv, left );
            rtype = PromoteComplexType( typeEnv, right );

            if ( ltype == NULL || rtype == NULL )
                return NULL;

            if ( ltype->GetSize() > rtype->GetSize() )
                type = ltype;
            else
                type = rtype;
        }
        else if ( left->IsImaginary() && right->IsImaginary() )
        {
            ltype = PromoteFloatType( typeEnv, left );
            rtype = PromoteFloatType( typeEnv, right );

            if ( ltype == NULL || rtype == NULL )
                return NULL;

            if ( ltype->GetSize() > rtype->GetSize() )
                type = ltype;
            else
                type = rtype;
        }
        else if ( left->IsImaginary() || right->IsImaginary() )
        {
            ltype = PromoteImaginaryType( typeEnv, left );
            rtype = PromoteImaginaryType( typeEnv, right );

            if ( ltype == NULL || rtype == NULL )
                return NULL;

            if ( ltype->GetSize() > rtype->GetSize() )
                type = ltype;
            else
                type = rtype;
        }
        else if ( left->IsReal() || right->IsReal() )
        {
            ltype = PromoteFloatType( typeEnv, left );
            rtype = PromoteFloatType( typeEnv, right );

            if ( ltype == NULL || rtype == NULL )
                return NULL;

            if ( ltype->GetSize() > rtype->GetSize() )
                type = ltype;
            else
                type = rtype;
        }
        else if ( left->IsIntegral() && right->IsIntegral() )
        {
            ltype = PromoteIntType( typeEnv, left );
            rtype = PromoteIntType( typeEnv, right );

            if ( ltype == NULL || rtype == NULL )
                return NULL;

            if ( ltype->GetSize() == rtype->GetSize() )
            {
                if ( !ltype->IsSigned() )
                    type = ltype;
                else
                    type = rtype;
            }
            else if ( ltype->GetSize() > rtype->GetSize() )
                type = ltype;
            else // is less
                type = rtype;
        }
        // else we're dealing with non-scalars, which can't be combined, so return NULL

        return type;
    }

    RefPtr<Type> Expression::GetModCommonType( ITypeEnv* typeEnv, Type* left, Type* right )
    {
        RefPtr<Type>    ltype;
        RefPtr<Type>    rtype;
        RefPtr<Type>    type;

        // TODO: test if they're the same type and (floating-point or integral with size >= sizeof( int ))
        //      if so, the common type is the same as left and right's

        if ( left->IsComplex() || right->IsComplex() )
        {
            ltype = PromoteComplexType( typeEnv, left );
            rtype = PromoteComplexType( typeEnv, right );

            if ( ltype == NULL || rtype == NULL )
                return NULL;

            if ( ltype->GetSize() > rtype->GetSize() )
                type = ltype;
            else
                type = rtype;
        }
        else if ( left->IsImaginary() )
        {
            ltype = PromoteImaginaryType( typeEnv, left );
            rtype = PromoteImaginaryType( typeEnv, right );

            if ( ltype == NULL || rtype == NULL )
                return NULL;

            if ( ltype->GetSize() > rtype->GetSize() )
                type = ltype;
            else
                type = rtype;
        }
        else if ( left->IsReal() || right->IsReal() || right->IsImaginary() )
        {
            ltype = PromoteFloatType( typeEnv, left );
            rtype = PromoteFloatType( typeEnv, right );

            if ( ltype == NULL || rtype == NULL )
                return NULL;

            if ( ltype->GetSize() > rtype->GetSize() )
                type = ltype;
            else
                type = rtype;
        }
        else if ( left->IsIntegral() && right->IsIntegral() )
        {
            ltype = PromoteIntType( typeEnv, left );
            rtype = PromoteIntType( typeEnv, right );

            if ( ltype == NULL || rtype == NULL )
                return NULL;

            if ( ltype->GetSize() == rtype->GetSize() )
            {
                if ( !ltype->IsSigned() )
                    type = ltype;
                else
                    type = rtype;
            }
            else if ( ltype->GetSize() > rtype->GetSize() )
                type = ltype;
            else // is less
                type = rtype;
        }
        // else we're dealing with non-scalars, which can't be combined, so return NULL

        return type;
    }


    //----------------------------------------------------------------------------
    //  BinExpr
    //----------------------------------------------------------------------------

    HRESULT BinExpr::SemanticVerifyChildren( const EvalData& evalData,ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;

        hr = Left->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;
        if ( Left->Kind != DataKind_Value )
            return E_MAGOEE_VALUE_EXPECTED;
        if ( Left->_Type == NULL )
            return E_MAGOEE_NO_TYPE;

        hr = Right->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;
        if ( Right->Kind != DataKind_Value )
            return E_MAGOEE_VALUE_EXPECTED;
        if ( Right->_Type == NULL )
            return E_MAGOEE_NO_TYPE;

        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  ArithmeticBinExpr
    //----------------------------------------------------------------------------

    bool ArithmeticBinExpr::AllowOnlyIntegral()
    {
        return false;
    }

    HRESULT ArithmeticBinExpr::UInt64Op( uint64_t left, uint64_t right, uint64_t& result )
    {
        UNREFERENCED_PARAMETER( left );
        UNREFERENCED_PARAMETER( right );
        UNREFERENCED_PARAMETER( result );
        _ASSERT( false );
        return E_NOTIMPL;
    }

    HRESULT ArithmeticBinExpr::Int64Op( int64_t left, int64_t right, int64_t& result )
    {
        UNREFERENCED_PARAMETER( left );
        UNREFERENCED_PARAMETER( right );
        UNREFERENCED_PARAMETER( result );
        _ASSERT( false );
        return E_NOTIMPL;
    }

    HRESULT ArithmeticBinExpr::Float80Op( const Real10& left, const Real10& right, Real10& result )
    {
        UNREFERENCED_PARAMETER( left );
        UNREFERENCED_PARAMETER( right );
        UNREFERENCED_PARAMETER( result );
        _ASSERT( false );
        return E_NOTIMPL;
    }

    HRESULT ArithmeticBinExpr::Complex80Op( const Complex10& left, const Complex10& right, Complex10& result )
    {
        UNREFERENCED_PARAMETER( left );
        UNREFERENCED_PARAMETER( right );
        UNREFERENCED_PARAMETER( result );
        _ASSERT( false );
        return E_NOTIMPL;
    }
}
