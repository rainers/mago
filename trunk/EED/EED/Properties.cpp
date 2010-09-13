/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Properties.h"
#include "ITypeEnv.h"
#include "Type.h"
#include "Declaration.h"
#include "Eval.h"
#include <limits>

using namespace std;


namespace MagoEE
{
    //------------------------------------------------------------------------
    //  Base
    //------------------------------------------------------------------------

    //bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )

    bool PropertyBase::UsesParentValue()
    {
        return false;
    }

    bool PropertyBase::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );
        UNREFERENCED_PARAMETER( parentType );
        UNREFERENCED_PARAMETER( result );
        _ASSERT( false );
        return false;
    }

    bool PropertyBase::GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );
        UNREFERENCED_PARAMETER( parentType );
        UNREFERENCED_PARAMETER( parentVal );
        UNREFERENCED_PARAMETER( result );
        _ASSERT( false );
        return false;
    }


    //------------------------------------------------------------------------
    //  Size for all
    //------------------------------------------------------------------------

    bool PropertySize::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( parentType == NULL )
            return false;

        type = typeEnv->GetAliasType( Tsize_t );
        type->AddRef();
        return true;
    }

    bool PropertySize::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        result.UInt64Value = parentType->GetSize();
        return true;
    }


    //------------------------------------------------------------------------
    //  Integrals
    //------------------------------------------------------------------------

    bool PropertyIntegralMax::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( parentDecl );

        if ( parentType == NULL )
            return false;

        type = parentType;
        type->AddRef();
        return true;
    }

    bool PropertyIntegralMax::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsIntegral() )
            return false;

        switch ( parentType->GetBackingTy() )
        {
        case Tint8: result.UInt64Value = numeric_limits<char>::max(); break;
        case Tuns8: result.UInt64Value = numeric_limits<unsigned char>::max(); break;
        case Tint16: result.UInt64Value = numeric_limits<short>::max(); break;
        case Tuns16: result.UInt64Value = numeric_limits<unsigned short>::max(); break;
        case Tint32: result.UInt64Value = numeric_limits<int>::max(); break;
        case Tuns32: result.UInt64Value = numeric_limits<unsigned int>::max(); break;
        case Tint64: result.UInt64Value = numeric_limits<__int64>::max(); break;
        case Tuns64: result.UInt64Value = numeric_limits<unsigned __int64>::max(); break;
        }
        return true;
    }


    bool PropertyIntegralMin::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsIntegral() )
            return false;

        type = parentType;
        type->AddRef();
        return true;
    }

    bool PropertyIntegralMin::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsIntegral() )
            return false;

        switch ( parentType->GetBackingTy() )
        {
        case Tint8: result.UInt64Value = numeric_limits<__int8>::min(); break;
        case Tuns8: result.UInt64Value = numeric_limits<unsigned char>::min(); break;
        case Tint16: result.UInt64Value = numeric_limits<short>::min(); break;
        case Tuns16: result.UInt64Value = numeric_limits<unsigned short>::min(); break;
        case Tint32: result.UInt64Value = numeric_limits<int>::min(); break;
        case Tuns32: result.UInt64Value = numeric_limits<unsigned int>::min(); break;
        case Tint64: result.UInt64Value = numeric_limits<__int64>::min(); break;
        case Tuns64: result.UInt64Value = numeric_limits<unsigned __int64>::min(); break;
        }
        return true;
    }


    //------------------------------------------------------------------------
    //  Floats
    //------------------------------------------------------------------------

    bool PropertyFloatMax::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        type = parentType;
        type->AddRef();
        return true;
    }

    bool PropertyFloatMax::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        Real10  r;

        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:  case Timaginary32:  case Tcomplex32:  r.FromFloat( numeric_limits<float>::max() ); break;
        case Tfloat64:  case Timaginary64:  case Tcomplex64:  r.FromDouble( numeric_limits<double>::max() ); break;
        case Tfloat80:  case Timaginary80:  case Tcomplex80:  r.LoadMax(); break;
        default:
            return false;
        }

        if ( parentType->IsComplex() )
        {
            result.Complex80Value.RealPart = r;
            result.Complex80Value.ImaginaryPart = r;
        }
        else
            result.Float80Value = r;

        return true;
    }


    bool PropertyFloatMin::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        type = parentType;
        type->AddRef();
        return true;
    }

    bool PropertyFloatMin::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        Real10  r;

        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:  case Timaginary32:  case Tcomplex32:  r.FromFloat( numeric_limits<float>::min() ); break;
        case Tfloat64:  case Timaginary64:  case Tcomplex64:  r.FromDouble( numeric_limits<double>::min() ); break;
        case Tfloat80:  case Timaginary80:  case Tcomplex80:  r.LoadMinNormal(); break;
        default:
            return false;
        }

        if ( parentType->IsComplex() )
        {
            result.Complex80Value.RealPart = r;
            result.Complex80Value.ImaginaryPart = r;
        }
        else
            result.Float80Value = r;

        return true;
    }


    bool PropertyFloatEpsilon::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        type = parentType;
        type->AddRef();
        return true;
    }

    bool PropertyFloatEpsilon::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        Real10  r;

        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:  case Timaginary32:  case Tcomplex32:  r.FromFloat( numeric_limits<float>::epsilon() ); break;
        case Tfloat64:  case Timaginary64:  case Tcomplex64:  r.FromDouble( numeric_limits<double>::epsilon() ); break;
        case Tfloat80:  case Timaginary80:  case Tcomplex80:  r.LoadEpsilon(); break;
        default:
            return false;
        }

        if ( parentType->IsComplex() )
        {
            result.Complex80Value.RealPart = r;
            result.Complex80Value.ImaginaryPart = r;
        }
        else
            result.Float80Value = r;

        return true;
    }


    bool PropertyFloatNan::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        type = parentType;
        type->AddRef();
        return true;
    }

    bool PropertyFloatNan::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        Real10  r;

        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:  case Timaginary32:  case Tcomplex32:  r.FromFloat( numeric_limits<float>::quiet_NaN() ); break;
        case Tfloat64:  case Timaginary64:  case Tcomplex64:  r.FromDouble( numeric_limits<double>::quiet_NaN() ); break;
        case Tfloat80:  case Timaginary80:  case Tcomplex80:  r.LoadNan(); break;
        default:
            return false;
        }

        if ( parentType->IsComplex() )
        {
            result.Complex80Value.RealPart = r;
            result.Complex80Value.ImaginaryPart = r;
        }
        else
            result.Float80Value = r;

        return true;
    }


    bool PropertyFloatInfinity::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        type = parentType;
        type->AddRef();
        return true;
    }

    bool PropertyFloatInfinity::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        Real10  r;

        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:  case Timaginary32:  case Tcomplex32:  r.FromFloat( numeric_limits<float>::infinity() ); break;
        case Tfloat64:  case Timaginary64:  case Tcomplex64:  r.FromDouble( numeric_limits<double>::infinity() ); break;
        case Tfloat80:  case Timaginary80:  case Tcomplex80:  r.LoadInfinity(); break;
        default:
            return false;
        }

        if ( parentType->IsComplex() )
        {
            result.Complex80Value.RealPart = r;
            result.Complex80Value.ImaginaryPart = r;
        }
        else
            result.Float80Value = r;

        return true;
    }


    bool PropertyFloatDigits::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        type = typeEnv->GetType( Tint32 );
        type->AddRef();
        return true;
    }

    bool PropertyFloatDigits::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        int v = 0;
        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:  case Timaginary32:  case Tcomplex32:  v = numeric_limits<float>::digits10; break;
        case Tfloat64:  case Timaginary64:  case Tcomplex64:  v = numeric_limits<double>::digits10; break;
        case Tfloat80:  case Timaginary80:  case Tcomplex80:  v = Real10::Digits; break;
        default:
            return false;
        }

        result.UInt64Value = v;
        return true;
    }


    bool PropertyFloatMantissaDigits::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        type = typeEnv->GetType( Tint32 );
        type->AddRef();
        return true;
    }

    bool PropertyFloatMantissaDigits::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        int v = 0;
        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:  case Timaginary32:  case Tcomplex32:  v = numeric_limits<float>::digits; break;
        case Tfloat64:  case Timaginary64:  case Tcomplex64:  v = numeric_limits<double>::digits; break;
        case Tfloat80:  case Timaginary80:  case Tcomplex80:  v = Real10::MantissaDigits(); break;
        default:
            return false;
        }

        result.UInt64Value = v;
        return true;
    }


    bool PropertyFloatMax10Exp::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        type = typeEnv->GetType( Tint32 );
        type->AddRef();
        return true;
    }

    bool PropertyFloatMax10Exp::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        int v = 0;
        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:  case Timaginary32:  case Tcomplex32:  v = numeric_limits<float>::max_exponent10; break;
        case Tfloat64:  case Timaginary64:  case Tcomplex64:  v = numeric_limits<double>::max_exponent10; break;
        case Tfloat80:  case Timaginary80:  case Tcomplex80:  v = Real10::MaxExponentBase10(); break;
        default:
            return false;
        }

        result.UInt64Value = v;
        return true;
    }


    bool PropertyFloatMaxExp::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        type = typeEnv->GetType( Tint32 );
        type->AddRef();
        return true;
    }

    bool PropertyFloatMaxExp::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        int v = 0;
        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:  case Timaginary32:  case Tcomplex32:  v = numeric_limits<float>::max_exponent; break;
        case Tfloat64:  case Timaginary64:  case Tcomplex64:  v = numeric_limits<double>::max_exponent; break;
        case Tfloat80:  case Timaginary80:  case Tcomplex80:  v = Real10::MaxExponentBase2(); break;
        default:
            return false;
        }

        result.UInt64Value = v;
        return true;
    }


    bool PropertyFloatMin10Exp::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        type = typeEnv->GetType( Tint32 );
        type->AddRef();
        return true;
    }

    bool PropertyFloatMin10Exp::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        int v = 0;
        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:  case Timaginary32:  case Tcomplex32:  v = numeric_limits<float>::min_exponent10; break;
        case Tfloat64:  case Timaginary64:  case Tcomplex64:  v = numeric_limits<double>::min_exponent10; break;
        case Tfloat80:  case Timaginary80:  case Tcomplex80:  v = Real10::MinExponentBase10(); break;
        default:
            return false;
        }

        result.UInt64Value = v;
        return true;
    }


    bool PropertyFloatMinExp::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        type = typeEnv->GetType( Tint32 );
        type->AddRef();
        return true;
    }

    bool PropertyFloatMinExp::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        int v = 0;
        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:  case Timaginary32:  case Tcomplex32:  v = numeric_limits<float>::min_exponent; break;
        case Tfloat64:  case Timaginary64:  case Tcomplex64:  v = numeric_limits<double>::min_exponent; break;
        case Tfloat80:  case Timaginary80:  case Tcomplex80:  v = Real10::MinExponentBase2(); break;
        default:
            return false;
        }

        result.UInt64Value = v;
        return true;
    }


    bool PropertyFloatReal::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        ENUMTY  ty = Tnone;

        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:  case Timaginary32:  case Tcomplex32:    ty = Tfloat32;  break;
        case Tfloat64:  case Timaginary64:  case Tcomplex64:    ty = Tfloat64;  break;
        case Tfloat80:  case Timaginary80:  case Tcomplex80:    ty = Tfloat80;  break;
        default:
            return false;
        }

        type = typeEnv->GetType( ty );
        type->AddRef();
        return true;
    }

    bool PropertyFloatReal::UsesParentValue()
    {
        return true;
    }

    bool PropertyFloatReal::GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:
        case Tfloat64:
        case Tfloat80:
            result.Float80Value = parentVal.Float80Value;
            break;

        case Timaginary32:
        case Timaginary64:
        case Timaginary80:
            result.Float80Value.Zero();
            break;

        case Tcomplex32:
        case Tcomplex64:
        case Tcomplex80:
            result.Float80Value = parentVal.Complex80Value.RealPart;
            break;

        default:
            return false;
        }

        return true;
    }


    bool PropertyFloatImaginary::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        ENUMTY  ty = Tnone;

        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:  case Timaginary32:  case Tcomplex32:    ty = Tfloat32;  break;
        case Tfloat64:  case Timaginary64:  case Tcomplex64:    ty = Tfloat64;  break;
        case Tfloat80:  case Timaginary80:  case Tcomplex80:    ty = Tfloat80;  break;
        default:
            return false;
        }

        type = typeEnv->GetType( ty );
        type->AddRef();
        return true;
    }

    bool PropertyFloatImaginary::UsesParentValue()
    {
        return true;
    }

    bool PropertyFloatImaginary::GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsFloatingPoint() )
            return false;

        switch ( parentType->GetBackingTy() )
        {
        case Tfloat32:
        case Tfloat64:
        case Tfloat80:
            result.Float80Value.Zero();
            break;

        case Timaginary32:
        case Timaginary64:
        case Timaginary80:
            result.Float80Value = parentVal.Float80Value;
            break;

        case Tcomplex32:
        case Tcomplex64:
        case Tcomplex80:
            result.Float80Value = parentVal.Complex80Value.ImaginaryPart;
            break;

        default:
            return false;
        }

        return true;
    }


    //------------------------------------------------------------------------
    //  SArray
    //------------------------------------------------------------------------

    bool PropertySArrayLength::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsSArray() )
            return false;

        type = typeEnv->GetAliasType( Tsize_t );
        type->AddRef();
        return true;
    }

    bool PropertySArrayLength::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsSArray() )
            return false;

        result.UInt64Value = parentType->AsTypeSArray()->GetLength();
        return true;
    }


    bool PropertySArrayPtr::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsSArray() )
            return false;

        HRESULT hr = S_OK;
        hr = typeEnv->NewPointer( parentType->AsTypeNext()->GetNext(), type );
        return SUCCEEDED( hr );
    }

    bool PropertySArrayPtr::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        if ( (parentType == NULL) || !parentType->IsSArray() )
            return false;

        return parentDecl->GetAddress( result.Addr );
    }


    //------------------------------------------------------------------------
    //  DArray
    //------------------------------------------------------------------------

    bool PropertyDArrayLength::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsDArray() )
            return false;

        type = typeEnv->GetAliasType( Tsize_t );
        type->AddRef();
        return true;
    }

    bool PropertyDArrayLength::UsesParentValue()
    {
        return true;
    }

    bool PropertyDArrayLength::GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsDArray() )
            return false;

        result.UInt64Value = parentVal.Array.Length;
        return true;
    }


    bool PropertyDArrayPtr::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsDArray() )
            return false;

        HRESULT hr = S_OK;
        hr = typeEnv->NewPointer( parentType->AsTypeNext()->GetNext(), type );
        return SUCCEEDED( hr );
    }

    bool PropertyDArrayPtr::UsesParentValue()
    {
        return true;
    }

    bool PropertyDArrayPtr::GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsDArray() )
            return false;

        result.Addr = parentVal.Array.Addr;
        return true;
    }


    //------------------------------------------------------------------------
    //  Delegate
    //------------------------------------------------------------------------

    bool PropertyDelegatePtr::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsDelegate() )
            return false;

        type = typeEnv->GetVoidPointerType();
        type->AddRef();
        return true;
    }

    bool PropertyDelegatePtr::UsesParentValue()
    {
        return true;
    }

    bool PropertyDelegatePtr::GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsDelegate() )
            return false;

        result.Addr = parentVal.Delegate.ContextAddr;
        return true;
    }


    bool PropertyDelegateFuncPtr::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsDelegate() )
            return false;

        type = parentType->AsTypeNext()->GetNext();
        type->AddRef();
        return true;
    }

    bool PropertyDelegateFuncPtr::UsesParentValue()
    {
        return true;
    }

    bool PropertyDelegateFuncPtr::GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentDecl );

        if ( (parentType == NULL) || !parentType->IsDelegate() )
            return false;

        result.Addr = parentVal.Delegate.FuncAddr;
        return true;
    }


    //------------------------------------------------------------------------
    //  Field
    //------------------------------------------------------------------------

    bool PropertyFieldOffset::GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )
    {
        UNREFERENCED_PARAMETER( parentType );

        if ( (parentDecl == NULL) || !parentDecl->IsField() )
            return false;

        type = typeEnv->GetAliasType( Tsize_t );
        type->AddRef();
        return true;
    }

    bool PropertyFieldOffset::GetValue( Type* parentType, Declaration* parentDecl, DataValue& result )
    {
        UNREFERENCED_PARAMETER( parentType );

        if ( (parentDecl == NULL) || !parentDecl->IsField() )
            return false;

        int offset = 0;

        if ( !parentDecl->GetOffset( offset ) )
            return false;

        result.UInt64Value = offset;
        return true;
    }
}
