/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Expression.h"
#include "Declaration.h"
#include "Eval.h"
#include "Type.h"
#include "ITypeEnv.h"


namespace MagoEE
{
    //----------------------------------------------------------------------------
    //  IntExpr
    //----------------------------------------------------------------------------

    HRESULT IntExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( binder );

        Kind = DataKind_Value;
        _ASSERT( _Type.Get() != NULL );
        return S_OK;
    }

    HRESULT IntExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( binder );

        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        obj._Type = _Type;
        obj.Addr = 0;
        obj.Value.UInt64Value = Value;
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  RealExpr
    //----------------------------------------------------------------------------

    HRESULT RealExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( binder );

        Kind = DataKind_Value;
        _ASSERT( _Type.Get() != NULL );
        return S_OK;
    }

    HRESULT RealExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( binder );

        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        obj._Type = _Type;
        obj.Addr = 0;
        obj.Value.Float80Value = Value;
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  NullExpr
    //----------------------------------------------------------------------------

    HRESULT NullExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( binder );

        _Type = typeEnv->GetVoidPointerType();
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT NullExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( binder );

        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        if ( _Type->IsPointer() )
        {
            obj.Value.Addr = 0;
        }
        else if ( _Type->IsDArray() )
        {
            obj.Value.Array.Addr = 0;
            obj.Value.Array.Length = 0;
        }
        else if ( _Type->IsAArray() )
        {
            obj.Value.Addr = 0;
        }
        else if ( _Type->IsDelegate() )
        {
            obj.Value.Delegate.ContextAddr = 0;
            obj.Value.Delegate.FuncAddr = 0;
        }
        else
            return E_FAIL;

        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }

    bool NullExpr::TrySetType( Type* type )
    {
        if ( type->IsDArray() || type->IsAArray() || type->IsDelegate() )
        {
            _Type = type;
            return true;
        }

        return false;
    }
}
