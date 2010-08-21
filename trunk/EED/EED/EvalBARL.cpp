/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

// BARL: Bitwise, Arithmetic, Relational, and Logical

#include "Common.h"
#include "Expression.h"
#include "Declaration.h"
#include "Type.h"
#include "TypeCommon.h"
#include "ITypeEnv.h"


namespace MagoEE
{
    //----------------------------------------------------------------------------
    //  OrOrExpr
    //----------------------------------------------------------------------------

    HRESULT OrOrExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = SemanticVerifyChildren( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;

        if ( !Left->_Type->CanImplicitCastToBool() || !Right->_Type->CanImplicitCastToBool() )
            return E_MAGOEE_BAD_BOOL_CAST;

        _Type = typeEnv->GetType( Tbool );
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT OrOrExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;
        DataObject  left = { 0 };
        DataObject  right = { 0 };

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, left );
        if ( FAILED( hr ) )
            return hr;

        if ( !ConvertToBool( left ) )
        {
            hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
            if ( FAILED( hr ) )
                return hr;

            obj.Value.UInt64Value = ConvertToBool( right ) ? 1 : 0;
        }
        else
            obj.Value.UInt64Value = 1;

        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  AndAndExpr
    //----------------------------------------------------------------------------

    HRESULT AndAndExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = SemanticVerifyChildren( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;

        if ( !Left->_Type->CanImplicitCastToBool() || !Right->_Type->CanImplicitCastToBool() )
            return E_MAGOEE_BAD_BOOL_CAST;

        _Type = typeEnv->GetType( Tbool );
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT AndAndExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;
        DataObject  left = { 0 };
        DataObject  right = { 0 };

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, left );
        if ( FAILED( hr ) )
            return hr;

        if ( ConvertToBool( left ) )
        {
            hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
            if ( FAILED( hr ) )
                return hr;

            obj.Value.UInt64Value = ConvertToBool( right ) ? 1 : 0;
        }
        else
            obj.Value.UInt64Value = 0;

        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  NotExpr
    //----------------------------------------------------------------------------

    HRESULT NotExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
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

        if ( !Child->_Type->CanImplicitCastToBool() )
            return E_MAGOEE_BAD_BOOL_CAST;

        _Type = typeEnv->GetType( Tbool );
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT NotExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;
        DataObject  childObj = { 0 };

        hr = Child->Evaluate( EvalMode_Value, evalData, binder, childObj );
        if ( FAILED( hr ) )
            return hr;

        obj.Value.UInt64Value = ConvertToBool( childObj ) ? 0 : 1;

        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  ArithmeticBinExpr
    //----------------------------------------------------------------------------

    HRESULT ArithmeticBinExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = SemanticVerifyChildren( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;

        if ( AllowOnlyIntegral() )
        {
            if ( !Left->_Type->IsIntegral() || !Right->_Type->IsIntegral() )
                return E_MAGOEE_BAD_TYPES_FOR_OP;
        }

        _Type = GetCommonType( typeEnv, Left->_Type.Get(), Right->_Type.Get() );
        if ( _Type.Get() == NULL )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT ArithmeticBinExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        HRESULT     hr = S_OK;
        DataObject  left = { 0 };
        DataObject  right = { 0 };

        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, left );
        if ( FAILED( hr ) )
            return hr;

        hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
        if ( FAILED( hr ) )
            return hr;

        obj._Type = _Type;
        obj.Addr = 0;

        if ( _Type->IsComplex() )
        {
            Complex10   leftVal = ConvertToComplex( left );
            Complex10   rightVal = ConvertToComplex( right );

            hr = Complex80Op( leftVal, rightVal, obj.Value.Complex80Value );
        }
        else if ( _Type->IsFloatingPoint() )
        {
            // same operation, no matter if it's real or imaginary
            Real10  leftVal = ConvertToFloat( left );
            Real10  rightVal = ConvertToFloat( right );

            hr = Float80Op( leftVal, rightVal, obj.Value.Float80Value );
        }
        else
        {
            _ASSERT( _Type->IsIntegral() );
            PromoteInPlace( left, _Type.Get() );
            PromoteInPlace( right, _Type.Get() );

            if ( _Type->IsSigned() )
            {
                int64_t    leftVal = left.Value.Int64Value;
                int64_t    rightVal = right.Value.Int64Value;

                hr = Int64Op( leftVal, rightVal, obj.Value.Int64Value );
            }
            else
            {
                uint64_t    leftVal = left.Value.UInt64Value;
                uint64_t    rightVal = right.Value.UInt64Value;

                hr = UInt64Op( leftVal, rightVal, obj.Value.UInt64Value );
            }

            PromoteInPlace( obj );
        }

        if ( FAILED( hr ) )     // failed the operation
            return hr;

        LeftAddr = left.Addr;
        LeftValue = left.Value;

        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  AddExpr
    //----------------------------------------------------------------------------

    HRESULT AddExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = SemanticVerifyChildren( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;

        Type*   ltype = Left->_Type.Get();
        Type*   rtype = Right->_Type.Get();

        if ( ltype->IsPointer() )
        {
            if ( !rtype->IsIntegral() || (ltype->AsTypeNext()->GetNext() == NULL) )
                return E_MAGOEE_BAD_TYPES_FOR_OP;

            _Type = ltype;
        }
        else if ( rtype->IsPointer() )
        {
            if ( !ltype->IsIntegral() || (rtype->AsTypeNext()->GetNext() == NULL) )
                return E_MAGOEE_BAD_TYPES_FOR_OP;

            _Type = rtype;
        }
        else
        {
            _Type = GetCommonType( typeEnv, Left->_Type.Get(), Right->_Type.Get() );
            if ( _Type.Get() == NULL )
                return E_MAGOEE_BAD_TYPES_FOR_OP;
        }

        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT AddExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        Type*   ltype = Left->_Type.Get();
        Type*   rtype = Right->_Type.Get();

        if ( (ltype->IsImaginary() && (rtype->IsReal() || rtype->IsIntegral()))
            || ((ltype->IsReal() || ltype->IsIntegral()) && rtype->IsImaginary()) )
        {
            return EvaluateMakeComplex( mode, evalData, binder, obj );
        }
        else if ( _Type->IsPointer() )
        {
            return EvaluatePtrAdd( mode, evalData, binder, obj );
        }
        else
        {
            return ArithmeticBinExpr::Evaluate( mode, evalData, binder, obj );
        }
    }

    HRESULT AddExpr::EvaluateMakeComplex( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;
        DataObject  left = { 0 };
        DataObject  right = { 0 };
        Type*       ltype = Left->_Type.Get();

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, left );
        if ( FAILED( hr ) )
            return hr;

        hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
        if ( FAILED( hr ) )
            return hr;

        if ( ltype->IsReal() || ltype->IsIntegral() )
        {
            Real10  y = ConvertToFloat( left );
            obj.Value.Complex80Value.RealPart = y;
            obj.Value.Complex80Value.ImaginaryPart = right.Value.Float80Value;
        }
        else
        {
            Real10  y = ConvertToFloat( right );
            obj.Value.Complex80Value.RealPart = y;
            obj.Value.Complex80Value.ImaginaryPart = left.Value.Float80Value;
        }

        // TODO: check all of this
        // if we're dealing with a += b, where ifloat a, float b; or float a, ifloat b
        // then that's an illegal operation, we can't return an address for the left side here,
        // we shouldn't have gotten this far, and Semantic should have caught that
        _ASSERT( LeftAddr == 0 );

        _ASSERT( _Type->IsComplex() );
        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }

    HRESULT AddExpr::EvaluatePtrAdd( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;
        DataObject  left = { 0 };
        DataObject  right = { 0 };
        Type*       ltype = Left->_Type.Get();

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, left );
        if ( FAILED( hr ) )
            return hr;

        hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
        if ( FAILED( hr ) )
            return hr;

        RefPtr<Type>    pointed = _Type->AsTypeNext()->GetNext();
        uint32_t        size = pointed->GetSize();
        doffset_t       offset = 0;
        Address         addr = 0;

        if ( ltype->IsPointer() )
        {
            addr = left.Value.Addr;
            offset = right.Value.Int64Value;

            LeftAddr = left.Addr;
            LeftValue = left.Value;
        }
        else
        {
            addr = right.Value.Addr;
            offset = left.Value.Int64Value;

            // TODO: you can't do (integral += pointer), so check for that somewhere else
            _ASSERT( LeftAddr == 0 );
        }

        obj.Value.Addr = addr + (size * offset);

        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }

    HRESULT AddExpr::Int64Op( int64_t left, int64_t right, int64_t& result )
    {
        result = left + right;
        return S_OK;
    }

    HRESULT AddExpr::UInt64Op( uint64_t left, uint64_t right, uint64_t& result )
    {
        result = left + right;
        return S_OK;
    }

    HRESULT AddExpr::Float80Op( const Real10& left, const Real10& right, Real10& result )
    {
        result.Add( left, right );
        return S_OK;
    }

    HRESULT AddExpr::Complex80Op( const Complex10& left, const Complex10& right, Complex10& result )
    {
        result.Add( left, right );
        return S_OK;
    }


    // TODO: AddExpr and MinExpr have a lot in common
    //----------------------------------------------------------------------------
    //  MinExpr
    //----------------------------------------------------------------------------

    HRESULT MinExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = SemanticVerifyChildren( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;

        Type*   ltype = Left->_Type.Get();
        Type*   rtype = Right->_Type.Get();

        if ( ltype->IsPointer() && rtype->IsPointer() )
        {
            if ( !ltype->Equals( rtype ) )
                return E_MAGOEE_BAD_TYPES_FOR_OP;
            if ( (ltype->AsTypeNext()->GetNext() == NULL) || (rtype->AsTypeNext()->GetNext() == NULL) )
                return E_MAGOEE_BAD_TYPES_FOR_OP;

            _Type = typeEnv->GetAliasType( Tptrdiff_t );
        }
        else if ( ltype->IsPointer() )
        {
            if ( !rtype->IsIntegral() || (ltype->AsTypeNext()->GetNext() == NULL) )
                return E_MAGOEE_BAD_TYPES_FOR_OP;

            _Type = ltype;
        }
        else
        {
            // (int - ptr) will make this fail, as it should
            _Type = GetCommonType( typeEnv, Left->_Type.Get(), Right->_Type.Get() );
            if ( _Type.Get() == NULL )
                return E_MAGOEE_BAD_TYPES_FOR_OP;
        }

        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT MinExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        Type*   ltype = Left->_Type.Get();
        Type*   rtype = Right->_Type.Get();

        if ( (ltype->IsImaginary() && (rtype->IsReal() || rtype->IsIntegral()))
            || ((ltype->IsReal() || ltype->IsIntegral()) && rtype->IsImaginary()) )
        {
            return EvaluateMakeComplex( mode, evalData, binder, obj );
        }
        else if ( ltype->IsImaginary() && rtype->IsComplex() )
        {
            return EvaluateSpecialCase( mode, evalData, binder, obj );
        }
        else if ( ltype->IsPointer() && rtype->IsPointer() )
        {
            return EvaluatePtrDiff( mode, evalData, binder, obj );
        }
        else if ( _Type->IsPointer() )
        {
            return EvaluatePtrSub( mode, evalData, binder, obj );
        }
        else
        {
            return ArithmeticBinExpr::Evaluate( mode, evalData, binder, obj );
        }
    }

    HRESULT MinExpr::EvaluateMakeComplex( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;
        DataObject  left = { 0 };
        DataObject  right = { 0 };
        Type*       ltype = Left->_Type.Get();

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, left );
        if ( FAILED( hr ) )
            return hr;

        hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
        if ( FAILED( hr ) )
            return hr;

        if ( ltype->IsReal() || ltype->IsIntegral() )
        {
            Real10  y = ConvertToFloat( left );
            obj.Value.Complex80Value.RealPart = y;
            obj.Value.Complex80Value.ImaginaryPart.Negate( right.Value.Float80Value );
        }
        else
        {
            Real10  y = ConvertToFloat( right );
            obj.Value.Complex80Value.RealPart.Negate( y );
            obj.Value.Complex80Value.ImaginaryPart = left.Value.Float80Value;
        }

        // TODO: check all of this
        // if we're dealing with a -= b, where ifloat a, float b; or float a, ifloat b
        // then that's an illegal operation, we can't return an address for the left side here,
        // we shouldn't have gotten this far, and Semantic should have caught that
        _ASSERT( LeftAddr == 0 );

        _ASSERT( _Type->IsComplex() );
        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }

    HRESULT MinExpr::EvaluatePtrSub( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;
        DataObject  left = { 0 };
        DataObject  right = { 0 };

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, left );
        if ( FAILED( hr ) )
            return hr;

        hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
        if ( FAILED( hr ) )
            return hr;

        RefPtr<Type>    pointed = _Type->AsTypeNext()->GetNext();
        uint32_t        size = pointed->GetSize();
        doffset_t       offset = 0;
        Address         addr = 0;

        _ASSERT( Left->_Type->IsPointer() );
        _ASSERT( !Right->_Type->IsPointer() );

        addr = left.Value.Addr;
        offset = right.Value.Int64Value;

        LeftAddr = left.Addr;
        LeftValue = left.Value;

        obj.Value.Addr = addr - (size * offset);

        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }

    HRESULT MinExpr::EvaluatePtrDiff( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;
        DataObject  left = { 0 };
        DataObject  right = { 0 };

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, left );
        if ( FAILED( hr ) )
            return hr;

        hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
        if ( FAILED( hr ) )
            return hr;

        RefPtr<Type>    pointed = left._Type->AsTypeNext()->GetNext();
        uint32_t        size = pointed->GetSize();

        _ASSERT( Left->_Type->IsPointer() );
        _ASSERT( Right->_Type->IsPointer() );

        LeftAddr = left.Addr;
        LeftValue = left.Value;

        obj.Value.Int64Value = (left.Value.Addr - right.Value.Addr);
        obj.Value.Int64Value /= size;                               // make sure it's a signed divide

        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }

    // I don't like this special case code for (imaginary - complex).
    // It should work the same as (real - complex), in that it's really like
    // (complex - complex) with one of the parts on the left set to 0.

    HRESULT MinExpr::EvaluateSpecialCase( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        _ASSERT( Left->_Type->IsImaginary() );
        _ASSERT( Right->_Type->IsComplex() );

        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;
        DataObject  left = { 0 };
        DataObject  right = { 0 };
        Complex10&  c = obj.Value.Complex80Value;

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, left );
        if ( FAILED( hr ) )
            return hr;

        hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
        if ( FAILED( hr ) )
            return hr;

        c.RealPart.Negate( right.Value.Complex80Value.RealPart );
        c.ImaginaryPart.Sub( left.Value.Float80Value, right.Value.Complex80Value.ImaginaryPart );

        // obj.Value.Complex80Value already assigned to, because it's been aliased with c
        LeftAddr = left.Addr;
        LeftValue = left.Value;

        _ASSERT( _Type->IsComplex() );
        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }

    HRESULT MinExpr::Int64Op( int64_t left, int64_t right, int64_t& result )
    {
        result = left - right;
        return S_OK;
    }

    HRESULT MinExpr::UInt64Op( uint64_t left, uint64_t right, uint64_t& result )
    {
        result = left - right;
        return S_OK;
    }

    HRESULT MinExpr::Float80Op( const Real10& left, const Real10& right, Real10& result )
    {
        result.Sub( left, right );
        return S_OK;
    }

    HRESULT MinExpr::Complex80Op( const Complex10& left, const Complex10& right, Complex10& result )
    {
        result.Sub( left, right );
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  MulExpr
    //----------------------------------------------------------------------------

    HRESULT MulExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = SemanticVerifyChildren( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;

        _Type = GetMulCommonType( typeEnv, Left->_Type.Get(), Right->_Type.Get() );
        if ( _Type.Get() == NULL )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT MulExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        HRESULT hr = S_OK;
        Type*   ltype = Left->_Type.Get();
        Type*   rtype = Right->_Type.Get();

        if ( ((ltype->IsImaginary() || ltype->IsReal() || ltype->IsIntegral()) && rtype->IsComplex())
            || (ltype->IsComplex() && (rtype->IsImaginary() || rtype->IsReal() || rtype->IsIntegral())) )
        {
            return EvaluateShortcutComplex( mode, evalData, binder, obj );
        }

        hr = ArithmeticBinExpr::Evaluate( mode, evalData, binder, obj );
        if ( FAILED( hr ) )
            return hr;

        if ( ltype->IsImaginary() && rtype->IsImaginary() )
        {
            // (a + bi)(c + di) = (0 + bi)(0 + di) = bi * di = -b*d
            obj.Value.Float80Value.Negate( obj.Value.Float80Value );
        }

        // ArithmeticBinExpr::Evaluate took care of the rest
        return S_OK;
    }

    // For the (imaginary * complex) or (complex * imaginary) cases, I prefer a 
    // way that doesn't depend on the order of the arguments (where the NaN is).
    // But, this is how DMD does it.

    HRESULT MulExpr::EvaluateShortcutComplex( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;
        DataObject  left = { 0 };
        DataObject  right = { 0 };
        Type*       ltype = Left->_Type.Get();
        Type*       rtype = Right->_Type.Get();
        Complex10&  c = obj.Value.Complex80Value;

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, left );
        if ( FAILED( hr ) )
            return hr;

        hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
        if ( FAILED( hr ) )
            return hr;

        if ( ltype->IsReal() || ltype->IsIntegral() )
        {
            Real10  r = ConvertToFloat( left );
            c.RealPart.Mul( r, right.Value.Complex80Value.RealPart );
            c.ImaginaryPart.Mul( r, right.Value.Complex80Value.ImaginaryPart );
        }
        else if ( ltype->IsImaginary() )
        {
            Real10  r = ConvertToFloat( left );
            Real10  nri;
            nri.Negate( right.Value.Complex80Value.ImaginaryPart );
            c.RealPart.Mul( r, nri );
            c.ImaginaryPart.Mul( r, right.Value.Complex80Value.RealPart );
        }
        else if ( rtype->IsReal() || rtype->IsIntegral() )
        {
            Real10  r = ConvertToFloat( right );
            c.RealPart.Mul( r, left.Value.Complex80Value.RealPart );
            c.ImaginaryPart.Mul( r, left.Value.Complex80Value.ImaginaryPart );
        }
        else if ( rtype->IsImaginary() )
        {
            Real10  r = ConvertToFloat( right );
            Real10  nli;
            nli.Negate( left.Value.Complex80Value.ImaginaryPart );
            c.RealPart.Mul( r, nli );
            c.ImaginaryPart.Mul( r, left.Value.Complex80Value.RealPart );
        }
        else
        {
            _ASSERT( false );
            return E_MAGOEE_BAD_TYPES_FOR_OP;
        }

        // obj.Value.Complex80Value already assigned to, because it's been aliased with c
        LeftAddr = left.Addr;
        LeftValue = left.Value;

        _ASSERT( _Type->IsComplex() );
        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }

    HRESULT MulExpr::Int64Op( int64_t left, int64_t right, int64_t& result )
    {
        result = left * right;
        return S_OK;
    }

    HRESULT MulExpr::UInt64Op( uint64_t left, uint64_t right, uint64_t& result )
    {
        result = left * right;
        return S_OK;
    }

    HRESULT MulExpr::Float80Op( const Real10& left, const Real10& right, Real10& result )
    {
        result.Mul( left, right );
        return S_OK;
    }

    HRESULT MulExpr::Complex80Op( const Complex10& left, const Complex10& right, Complex10& result )
    {
        result.Mul( left, right );
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  DivExpr
    //----------------------------------------------------------------------------

    HRESULT DivExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = SemanticVerifyChildren( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;

        _Type = GetMulCommonType( typeEnv, Left->_Type.Get(), Right->_Type.Get() );
        if ( _Type.Get() == NULL )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT DivExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        HRESULT hr = S_OK;
        Type*   ltype = Left->_Type.Get();
        Type*   rtype = Right->_Type.Get();

        if ( ltype->IsComplex() && (rtype->IsReal() || rtype->IsIntegral() || rtype->IsImaginary()) )
        {
            return EvaluateShortcutComplex( mode, evalData, binder, obj );
        }

        hr = ArithmeticBinExpr::Evaluate( mode, evalData, binder, obj );
        if ( FAILED( hr ) )
            return hr;

        if ( (ltype->IsReal() || ltype->IsIntegral()) && rtype->IsImaginary() )
        {
            // (a + bi)(c + di) = (a + 0i)(0 + di) = -a/d
            obj.Value.Float80Value.Negate( obj.Value.Float80Value );
        }

        // ArithmeticBinExpr::Evaluate took care of the rest
        return S_OK;
    }

    HRESULT DivExpr::EvaluateShortcutComplex( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        _ASSERT( Left->_Type->IsComplex() );

        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;
        DataObject  left = { 0 };
        DataObject  right = { 0 };
        Type*       rtype = Right->_Type.Get();
        Complex10&  c = obj.Value.Complex80Value;

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, left );
        if ( FAILED( hr ) )
            return hr;

        hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
        if ( FAILED( hr ) )
            return hr;

        if ( rtype->IsReal() || rtype->IsIntegral() )
        {
            Real10      r = ConvertToFloat( right );
            c.RealPart.Div( left.Value.Complex80Value.RealPart, r );
            c.ImaginaryPart.Div( left.Value.Complex80Value.ImaginaryPart, r );
        }
        else if ( rtype->IsImaginary() )
        {
            Real10      r = right.Value.Float80Value;
            Real10      ncleft;
            c.RealPart.Div( left.Value.Complex80Value.ImaginaryPart, r );
            ncleft.Negate( left.Value.Complex80Value.RealPart );
            c.ImaginaryPart.Div( ncleft, r );
        }
        else
        {
            _ASSERT( false );
            return E_MAGOEE_BAD_TYPES_FOR_OP;
        }

        // obj.Value.Complex80Value already assigned to, because it's been aliased with c
        LeftAddr = left.Addr;
        LeftValue = left.Value;

        _ASSERT( _Type->IsComplex() );
        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }

    HRESULT DivExpr::Int64Op( int64_t left, int64_t right, int64_t& result )
    {
        if ( right == 0 )
            return E_MAGOEE_DIVIDE_BY_ZERO;

        result = left / right;
        return S_OK;
    }

    HRESULT DivExpr::UInt64Op( uint64_t left, uint64_t right, uint64_t& result )
    {
        if ( right == 0 )
            return E_MAGOEE_DIVIDE_BY_ZERO;

        result = left / right;
        return S_OK;
    }

    HRESULT DivExpr::Float80Op( const Real10& left, const Real10& right, Real10& result )
    {
        result.Div( left, right );
        return S_OK;
    }

    HRESULT DivExpr::Complex80Op( const Complex10& left, const Complex10& right, Complex10& result )
    {
        result.Div( left, right );
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  ModExpr
    //----------------------------------------------------------------------------

    HRESULT ModExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = SemanticVerifyChildren( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;

        if ( Right->_Type->IsComplex() )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        _Type = GetModCommonType( typeEnv, Left->_Type.Get(), Right->_Type.Get() );
        if ( _Type.Get() == NULL )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT ModExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        Type*   ltype = Left->_Type.Get();
        Type*   rtype = Right->_Type.Get();

        if ( ltype->IsComplex() && (rtype->IsReal() || rtype->IsIntegral() || rtype->IsImaginary()) )
        {
            return EvaluateShortcutComplex( mode, evalData, binder, obj );
        }

        return ArithmeticBinExpr::Evaluate( mode, evalData, binder, obj );
    }

    HRESULT ModExpr::EvaluateShortcutComplex( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        _ASSERT( Left->_Type->IsComplex() && !Right->_Type->IsComplex() );

        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;
        DataObject  left = { 0 };
        DataObject  right = { 0 };
        Complex10&  c = obj.Value.Complex80Value;

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, left );
        if ( FAILED( hr ) )
            return hr;

        hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
        if ( FAILED( hr ) )
            return hr;

        Real10      r = ConvertToFloat( right );
        c.RealPart.Rem( left.Value.Complex80Value.RealPart, r );
        c.ImaginaryPart.Rem( left.Value.Complex80Value.ImaginaryPart, r );

        // obj.Value.Complex80Value already assigned to, because it's been aliased with c
        LeftAddr = left.Addr;
        LeftValue = left.Value;

        _ASSERT( _Type->IsComplex() );
        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }

    HRESULT ModExpr::Int64Op( int64_t left, int64_t right, int64_t& result )
    {
        if ( right == 0 )
            return E_MAGOEE_DIVIDE_BY_ZERO;

        result = left % right;
        return S_OK;
    }

    HRESULT ModExpr::UInt64Op( uint64_t left, uint64_t right, uint64_t& result )
    {
        if ( right == 0 )
            return E_MAGOEE_DIVIDE_BY_ZERO;

        result = left % right;
        return S_OK;
    }

    HRESULT ModExpr::Float80Op( const Real10& left, const Real10& right, Real10& result )
    {
        result.Rem( left, right );
        return S_OK;
    }

    HRESULT ModExpr::Complex80Op( const Complex10& left, const Complex10& right, Complex10& result )
    {
        UNREFERENCED_PARAMETER( left );
        UNREFERENCED_PARAMETER( right );
        UNREFERENCED_PARAMETER( result );
        _ASSERT( false );
        return E_NOTIMPL;
    }


    //----------------------------------------------------------------------------
    //  PowExpr
    //----------------------------------------------------------------------------

    HRESULT PowExpr::Int64Op( int64_t left, int64_t right, int64_t& result )
    {
        // TODO:
        UNREFERENCED_PARAMETER( left );
        UNREFERENCED_PARAMETER( right );
        UNREFERENCED_PARAMETER( result );
        _ASSERT( false );
        return E_NOTIMPL;
    }

    HRESULT PowExpr::UInt64Op( uint64_t left, uint64_t right, uint64_t& result )
    {
        // TODO:
        UNREFERENCED_PARAMETER( left );
        UNREFERENCED_PARAMETER( right );
        UNREFERENCED_PARAMETER( result );
        _ASSERT( false );
        return E_NOTIMPL;
    }

    HRESULT PowExpr::Float80Op( const Real10& left, const Real10& right, Real10& result )
    {
        // TODO:
        UNREFERENCED_PARAMETER( left );
        UNREFERENCED_PARAMETER( right );
        UNREFERENCED_PARAMETER( result );
        _ASSERT( false );
        return E_NOTIMPL;
    }

    HRESULT PowExpr::Complex80Op( const Complex10& left, const Complex10& right, Complex10& result )
    {
        // TODO:
        UNREFERENCED_PARAMETER( left );
        UNREFERENCED_PARAMETER( right );
        UNREFERENCED_PARAMETER( result );
        _ASSERT( false );
        return E_NOTIMPL;
    }


    //----------------------------------------------------------------------------
    //  NegateExpr
    //----------------------------------------------------------------------------

    HRESULT NegateExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
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

        if ( !Child->_Type->IsIntegral() && !Child->_Type->IsFloatingPoint() )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        _Type = Child->_Type;
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT NegateExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        HRESULT     hr = S_OK;

        hr = Child->Evaluate( EvalMode_Value, evalData, binder, obj );
        if ( FAILED( hr ) )
            return hr;

        if ( _Type->IsComplex() )
        {
            obj.Value.Complex80Value.Negate( obj.Value.Complex80Value );
        }
        else if ( _Type->IsFloatingPoint() )
        {
            obj.Value.Float80Value.Negate( obj.Value.Float80Value );
        }
        else if ( _Type->IsIntegral() )
        {
            obj.Value.UInt64Value = -obj.Value.Int64Value;

            PromoteInPlace( obj );
        }
        else
        {
            _ASSERT( false );
            return E_MAGOEE_BAD_TYPES_FOR_OP;
        }

        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  UnaryAddExpr
    //----------------------------------------------------------------------------

    HRESULT UnaryAddExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
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

        if ( !Child->_Type->IsIntegral() && !Child->_Type->IsFloatingPoint() )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        _Type = Child->_Type;
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT UnaryAddExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        return Child->Evaluate( mode, evalData, binder, obj );
    }


    //----------------------------------------------------------------------------
    //  AndExpr
    //----------------------------------------------------------------------------

    bool AndExpr::AllowOnlyIntegral()
    {
        return true;
    }

    HRESULT AndExpr::Int64Op( int64_t left, int64_t right, int64_t& result )
    {
        result = left & right;
        return S_OK;
    }

    HRESULT AndExpr::UInt64Op( uint64_t left, uint64_t right, uint64_t& result )
    {
        result = left & right;
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  OrExpr
    //----------------------------------------------------------------------------

    bool OrExpr::AllowOnlyIntegral()
    {
        return true;
    }

    HRESULT OrExpr::Int64Op( int64_t left, int64_t right, int64_t& result )
    {
        result = left | right;
        return S_OK;
    }

    HRESULT OrExpr::UInt64Op( uint64_t left, uint64_t right, uint64_t& result )
    {
        result = left | right;
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  XorExpr
    //----------------------------------------------------------------------------

    bool XorExpr::AllowOnlyIntegral()
    {
        return true;
    }

    HRESULT XorExpr::Int64Op( int64_t left, int64_t right, int64_t& result )
    {
        result = left ^ right;
        return S_OK;
    }

    HRESULT XorExpr::UInt64Op( uint64_t left, uint64_t right, uint64_t& result )
    {
        result = left ^ right;
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  BitNotExpr
    //----------------------------------------------------------------------------

    HRESULT BitNotExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
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

        if ( !Child->_Type->IsIntegral() )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        _Type = Child->_Type;
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT BitNotExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;

        hr = Child->Evaluate( EvalMode_Value, evalData, binder, obj );
        if ( FAILED( hr ) )
            return hr;

        obj.Value.UInt64Value = ~obj.Value.UInt64Value;

        obj._Type = _Type;
        obj.Addr = 0;

        PromoteInPlace( obj );

        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  ShiftBinExpr
    //----------------------------------------------------------------------------

    HRESULT ShiftBinExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;
        ClearEvalData();

        hr = SemanticVerifyChildren( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;

        if ( !Left->_Type->IsIntegral() || !Right->_Type->IsIntegral() )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        if ( Left->_Type->GetSize() < 4 )
            _Type = typeEnv->GetType( Tint32 );
        else
            _Type = Left->_Type;

        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT ShiftBinExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        _ASSERT( Left->_Type->IsIntegral() && Right->_Type->IsIntegral() );
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT     hr = S_OK;
        DataObject  left = { 0 };
        DataObject  right = { 0 };

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, left );
        if ( FAILED( hr ) )
            return hr;

        hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
        if ( FAILED( hr ) )
            return hr;

        uint32_t    shiftAmount = (uint32_t) right.Value.UInt64Value;

        // can't shift all the bits out
        if ( _Type->GetSize() == 8 )
            shiftAmount &= 0x3F;
        else
            shiftAmount &= 0x1F;

        obj.Value.UInt64Value = IntOp( left.Value.UInt64Value, shiftAmount, _Type.Get() );
        obj._Type = _Type;
        obj.Addr = 0;

        PromoteInPlace( obj );

        LeftAddr = left.Addr;
        LeftValue = left.Value;

        _ASSERT( _Type->IsIntegral() );
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  ShiftLeftExpr
    //----------------------------------------------------------------------------

    uint64_t        ShiftLeftExpr::IntOp( uint64_t left, uint32_t right, Type* type )
    {
        UNREFERENCED_PARAMETER( type );
        return left << right;
    }


    //----------------------------------------------------------------------------
    //  ShiftRightExpr
    //----------------------------------------------------------------------------

    uint64_t        ShiftRightExpr::IntOp( uint64_t left, uint32_t right, Type* type )
    {
        // C, C++, C# behavior
        // the original value in its original size was already sign or zero extended to 64 bits
        if ( type->IsSigned() )
            return ((int64_t) left) >> right;
        else
            return ((uint64_t) left) >> right;
    }


    //----------------------------------------------------------------------------
    //  UShiftLeftExpr
    //----------------------------------------------------------------------------

    uint64_t        UShiftRightExpr::IntOp( uint64_t left, uint32_t right, Type* type )
    {
        switch ( type->GetBackingTy() )
        {
        case Tint8:
        case Tuns8:
            return ((uint8_t) left) >> right;
        case Tint16:
        case Tuns16:
            return ((uint16_t) left) >> right;
        case Tint32:
        case Tuns32:
            return ((uint32_t) left) >> right;
        case Tint64:
        case Tuns64:
            return ((uint64_t) left) >> right;
        default:
            return 0;
        }
    }


    //----------------------------------------------------------------------------
    //  CompareExpr
    //----------------------------------------------------------------------------

    HRESULT CompareExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;

        hr = SemanticVerifyChildren( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return hr;

        Type*   ltype = Left->_Type;
        Type*   rtype = Right->_Type;

        if ( (!ltype->IsScalar() && !ltype->IsSArray() && !ltype->IsDArray() && !ltype->IsAArray() && !ltype->IsDelegate())
            || (!rtype->IsScalar() && !rtype->IsSArray() && !rtype->IsDArray() && !rtype->IsAArray() && !rtype->IsDelegate()) )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        // if one is null, then try to set it to the other's type
        if ( Left->TrySetType( rtype ) )
            ltype = rtype;
        else if ( Right->TrySetType( ltype ) )
            rtype = ltype;

        if ( ltype->IsPointer() != rtype->IsPointer() )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        if ( ltype->IsAArray() != rtype->IsAArray() )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        if ( ltype->IsDelegate() != rtype->IsDelegate() )
            return E_MAGOEE_BAD_TYPES_FOR_OP;

        // you can mix and match S- and D-array, but it can't be anything else
        if ( (ltype->IsSArray() || ltype->IsDArray()) != (rtype->IsSArray() || rtype->IsDArray()) )
                return E_MAGOEE_BAD_TYPES_FOR_OP;
        if ( ltype->IsSArray() || ltype->IsDArray() )
        {
            _ASSERT( rtype->IsSArray() || rtype->IsDArray() );
            // the element types have to match
            if ( !ltype->AsTypeNext()->GetNext()->Equals( rtype->AsTypeNext()->GetNext() ) )
                return E_MAGOEE_BAD_TYPES_FOR_OP;
        }

        if ( ltype->IsAArray() )
        {
            if ( !ltype->AsTypeAArray()->GetElement()->Equals( rtype->AsTypeAArray()->GetElement() )
                || !ltype->AsTypeAArray()->GetIndex()->Equals( rtype->AsTypeAArray()->GetIndex() ) )
                return E_MAGOEE_BAD_TYPES_FOR_OP;
        }

        // TODO: in restrictions below (like for S- and D-arrays) also include delegate and A-array, 
        //       and class (actually ptr to class)
        //       except that for delegates, "==" is the same as "is"
        switch ( OpCode )
        {
        case TOKequal:
        case TOKnotequal:
            if ( ltype->IsSArray() || rtype->IsSArray() || ltype->IsDArray() || rtype->IsDArray() 
                || ltype->IsAArray() || rtype->IsAArray() )
                return E_MAGOEE_BAD_TYPES_FOR_OP;
            break;

            // TODO: can you really compare a real and an imaginary?
        case TOKlt:
        case TOKle:
        case TOKgt:
        case TOKge:
            // we'll allow comparing signed to unsigned integrals

        case TOKunord:
        case TOKlg:
        case TOKleg:
        case TOKule:
        case TOKul:
        case TOKuge:
        case TOKug:
        case TOKue:
            if ( ltype->IsComplex() || rtype->IsComplex()
                || ltype->IsSArray() || rtype->IsSArray()
                || ltype->IsDArray() || rtype->IsDArray()
                || ltype->IsAArray() || rtype->IsAArray()
                || ltype->IsDelegate() || rtype->IsDelegate() )
                return E_MAGOEE_BAD_TYPES_FOR_OP;
            break;
        }

        _Type = typeEnv->GetType( Tbool );
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT CompareExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        HRESULT         hr = S_OK;
        DataObject      left = { 0 };
        DataObject      right = { 0 };
        RefPtr<Type>    commonType;

        hr = Left->Evaluate( EvalMode_Value, evalData, binder, left );
        if ( FAILED( hr ) )
            return hr;

        hr = Right->Evaluate( EvalMode_Value, evalData, binder, right );
        if ( FAILED( hr ) )
            return hr;

        if ( Left->_Type->IsPointer() || Left->_Type->IsAArray() )
        {
            obj.Value.UInt64Value = IntegerOp( OpCode, left.Value.Addr, right.Value.Addr ) ? 1 : 0;
        }
        else if ( Left->_Type->IsSArray() || Left->_Type->IsDArray() )
        {
            obj.Value.UInt64Value = ArrayRelational( OpCode, left, right ) ? 1 : 0;
        }
        else if ( Left->_Type->IsDelegate() )
        {
            obj.Value.UInt64Value = DelegateRelational( OpCode, left, right ) ? 1 : 0;
        }
        else
        {
            // TODO: clear this up, which way do we do it?
#if 1
            commonType = GetCommonType( evalData.TypeEnv, left._Type.Get(), right._Type.Get() );
#else
            if ( (OpCode == TOKequal) || (OpCode == TOKnotequal) 
                || (OpCode == TOKidentity) || (OpCode == TOKnotidentity) )
                commonType = GetCommonType( typeEnv, left._Type.Get(), right._Type.Get() );
            else
                commonType = GetModCommonType( typeEnv, left._Type.Get(), right._Type.Get() );
#endif

            if ( commonType->IsComplex() )
            {
                obj.Value.UInt64Value = ComplexRelational( OpCode, left, right ) ? 1 : 0;
            }
            else if ( commonType->IsFloatingPoint() )
            {
                obj.Value.UInt64Value = FloatingRelational( OpCode, left, right ) ? 1 : 0;
            }
            else
            {
                obj.Value.UInt64Value = IntegerRelational( OpCode, commonType, left, right ) ? 1 : 0;
            }
        }

        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }

    bool CompareExpr::IntegerRelational( TOK code, Type* exprType, DataObject& left, DataObject& right )
    {
        PromoteInPlace( left, exprType );
        PromoteInPlace( right, exprType );

        if ( exprType->IsSigned() )
        {
            return IntegerOp( code, left.Value.Int64Value, right.Value.Int64Value );
        }
        else
        {
            return IntegerOp( code, left.Value.UInt64Value, right.Value.UInt64Value );
        }
    }

    bool CompareExpr::FloatingRelational( TOK code, DataObject& leftObj, DataObject& rightObj )
    {
        if ( ((leftObj._Type->IsReal() || leftObj._Type->IsIntegral()) && rightObj._Type->IsImaginary())
            || (leftObj._Type->IsImaginary() && (rightObj._Type->IsReal() || rightObj._Type->IsIntegral())) )
        {
            rightObj.Value.Float80Value.Zero();
        }

        Real10      leftVal = ConvertToFloat( leftObj );
        Real10      rightVal = ConvertToFloat( rightObj );
        uint16_t    status = Real10::Compare( leftVal, rightVal );

        return FloatingRelational( code, status );
    }

    bool CompareExpr::ComplexRelational( TOK code, DataObject& leftObj, DataObject& rightObj )
    {
        Complex10   leftVal = ConvertToComplex( leftObj );
        Complex10   rightVal = ConvertToComplex( rightObj );
        uint16_t    status = Complex10::Compare( leftVal, rightVal );

        return FloatingRelational( code, status );
    }

    bool CompareExpr::FloatingRelational( TOK code, uint16_t status )
    {
        switch ( code )
        {
        case TOKidentity:
        case TOKequal:   return Real10::IsEqual( status );
        case TOKnotidentity:
        case TOKnotequal:return !Real10::IsEqual( status );

        case TOKlt:      return Real10::IsLess( status );
        case TOKle:      return Real10::IsLess( status ) || Real10::IsEqual( status );
        case TOKgt:      return Real10::IsGreater( status );
        case TOKge:      return Real10::IsGreater( status ) || Real10::IsEqual( status );

        case TOKunord:   return Real10::IsUnordered( status );
        case TOKlg:      return Real10::IsLess( status ) || Real10::IsGreater( status );
        case TOKleg:     return Real10::IsLess( status ) || Real10::IsGreater( status ) || Real10::IsEqual( status );
        case TOKule:     return Real10::IsUnordered( status ) || Real10::IsLess( status ) || Real10::IsEqual( status );
        case TOKul:      return Real10::IsUnordered( status ) || Real10::IsLess( status );
        case TOKuge:     return Real10::IsUnordered( status ) || Real10::IsGreater( status ) || Real10::IsEqual( status );
        case TOKug:      return Real10::IsUnordered( status ) || Real10::IsGreater( status );
        case TOKue:      return Real10::IsUnordered( status ) || Real10::IsEqual( status );
        default:
            _ASSERT_EXPR( false, L"Relational operator not allowed on integers." );
        }
        return false;
    }

    bool CompareExpr::ArrayRelational( TOK code, DataObject& leftObj, DataObject& rightObj )
    {
        Type*               leftType = leftObj._Type;
        Type*               rightType = rightObj._Type;
        Address             leftAddr = 0;
        dlength_t           leftLen = 0;
        Address             rightAddr = 0;
        dlength_t           rightLen = 0;

        if ( leftType->IsSArray() )
        {
            leftAddr = leftObj.Addr;
            leftLen = leftType->AsTypeSArray()->GetLength();
        }
        else if ( leftType->IsDArray() )
        {
            leftAddr = leftObj.Value.Array.Addr;
            leftLen = leftObj.Value.Array.Length;
        }

        if ( rightType->IsSArray() )
        {
            rightAddr = rightObj.Addr;
            rightLen = rightType->AsTypeSArray()->GetLength();
        }
        else if ( rightType->IsDArray() )
        {
            rightAddr = rightObj.Value.Array.Addr;
            rightLen = rightObj.Value.Array.Length;
        }

        if ( code == TOKidentity )
        {
            return (leftAddr == rightAddr) && (leftLen == rightLen);
        }
        else if ( code == TOKnotidentity )
        {
            return (leftAddr != rightAddr) || (leftLen != rightLen);
        }

        _ASSERT( false );
        return false;
    }

    bool CompareExpr::DelegateRelational( TOK code, DataObject& left, DataObject& right )
    {
        if ( (code == TOKidentity) || (code == TOKequal) )
        {
            return (left.Value.Delegate.ContextAddr == right.Value.Delegate.ContextAddr)
                && (left.Value.Delegate.FuncAddr == right.Value.Delegate.FuncAddr);
        }
        else if ( (code == TOKnotidentity) || (code == TOKnotequal) )
        {
            return (left.Value.Delegate.ContextAddr != right.Value.Delegate.ContextAddr)
                || (left.Value.Delegate.FuncAddr != right.Value.Delegate.FuncAddr);
        }

        _ASSERT( false );
        return false;
    }
}
