/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Type.h"
#include "TypeUnresolved.h"
#include "Expression.h"
#include "TypeCommon.h"
#include "Declaration.h"
#include "NameTable.h"
#include "ITypeEnv.h"


namespace MagoEE
{
    //----------------------------------------------------------------------------
    //  NamePart
    //----------------------------------------------------------------------------

    ObjectKind NamePart::GetObjectKind()
    {
        return ObjectKind_NamePart;
    }

    IdPart* NamePart::AsId()
    {
        return NULL;
    }

    TemplateInstancePart* NamePart::AsTemplateInstance()
    {
        return NULL;
    }


    //----------------------------------------------------------------------------
    //  IdPart
    //----------------------------------------------------------------------------

    IdPart::IdPart( Utf16String* id )
        :   Id( id )
    {
    }

    IdPart* IdPart::AsId()
    {
        return this;
    }


    //----------------------------------------------------------------------------
    //  TemplateInstancePart
    //----------------------------------------------------------------------------

    TemplateInstancePart::TemplateInstancePart( Utf16String* id )
        :   Id( id )
    {
    }

    TemplateInstancePart* TemplateInstancePart::AsTemplateInstance()
    {
        return this;
    }


    //----------------------------------------------------------------------------
    //  TypeQualified
    //----------------------------------------------------------------------------

    TypeQualified::TypeQualified( ENUMTY ty )
        :   Type( ty )
    {
    }

    RefPtr<Type> TypeQualified::ResolveTypeChain( Declaration* head )
    {
        HRESULT hr = S_OK;
        RefPtr<Declaration> curDecl = head;
        RefPtr<Type>        type;

        for ( std::vector< RefPtr<NamePart> >::iterator it = Parts.begin();
            it != Parts.end();
            it++ )
        {
            RefPtr<Declaration> newDecl;

            if ( (*it)->AsId() != NULL )
            {
                IdPart* idPart = (*it)->AsId();

                hr = curDecl->FindObject( idPart->Id->Str, newDecl.Ref() );
                if ( FAILED( hr ) )
                    return NULL;
            }
            else
            {
                TemplateInstancePart*   templatePart = (*it)->AsTemplateInstance();
                std::wstring            fullId;

                fullId.append( templatePart->Id->Str );
                fullId.append( templatePart->ArgumentString->Str );

                hr = curDecl->FindObject( fullId.c_str(), newDecl.Ref() );
                if ( FAILED( hr ) )
                    return NULL;
            }

            curDecl = newDecl;
        }

        if ( !curDecl->IsType() )
            return NULL;

        hr = curDecl->GetType( type.Ref() );
        if ( FAILED( hr ) )
            return NULL;

        return type;
    }

    RefPtr<Type> TypeQualified::ResolveNamePath( const wchar_t* headName, IValueBinder* binder )
    {
        HRESULT             hr = S_OK;
        std::wstring        fullName;
        RefPtr<Declaration> decl;
        RefPtr<Type>        type;

        fullName.append( headName );

        for ( std::vector< RefPtr<NamePart> >::iterator it = Parts.begin();
            it != Parts.end();
            it++ )
        {
            if ( (*it)->AsId() != NULL )
            {
                IdPart* idPart = (*it)->AsId();

                fullName.append( 1, L'.' );
                fullName.append( idPart->Id->Str );
            }
            else
            {
                TemplateInstancePart*   templatePart = (*it)->AsTemplateInstance();

                fullName.append( 1, L'.' );
                fullName.append( templatePart->Id->Str );
                fullName.append( templatePart->ArgumentString->Str );
            }
        }

        hr = binder->FindObject( fullName.c_str(), decl.Ref(), IValueBinder::FindObjectAny );
        if ( FAILED( hr ) )
            return NULL;

        if ( !decl->IsType() )
            return NULL;

        hr = decl->GetType( type.Ref() );
        if ( FAILED( hr ) )
            return NULL;

        return type;
    }


    //----------------------------------------------------------------------------
    //  TypeReturn
    //----------------------------------------------------------------------------

    TypeReturn::TypeReturn()
        :   TypeQualified( Treturn )
    {
    }

    RefPtr<Type> TypeReturn::Copy()
    {
        RefPtr<TypeQualified>    type = new TypeReturn();
        type->Parts = Parts;
        return type.Get();
    }

    RefPtr<Type> TypeReturn::Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( typeEnv );

        HRESULT hr = S_OK;
        RefPtr<Type>    retType;

        hr = binder->GetReturnType( retType.Ref() );
        if ( FAILED( hr ) )
            return NULL;

        // TODO: is this the only type that can do this?
        //       is it only inner types that work with typeof( return ).T?
        //       or is it also enum members, or other things?
        if ( retType->AsTypeStruct() == NULL )
        {
            if ( Parts.size() == 0 )
                return retType;
            else
                return NULL;
        }

        return ResolveTypeChain( retType->GetDeclaration() );
    }

    void TypeReturn::_ToString( std::wstring& str )
    {
        // TODO: do we really need to define this method or can we have Type say "unresolved"?
        str.append( L"typeof(return)" );
    }


    //----------------------------------------------------------------------------
    //  TypeTypeof
    //----------------------------------------------------------------------------

    TypeTypeof::TypeTypeof( Expression* expr )
        :   TypeQualified( Ttypeof ),
            Expr( expr )
    {
    }

    RefPtr<Type> TypeTypeof::Copy()
    {
        RefPtr<TypeQualified>    type = new TypeTypeof( Expr.Get() );
        type->Parts = Parts;
        return type.Get();
    }

    RefPtr<Type> TypeTypeof::Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;

        hr = Expr->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return NULL;

        // TODO: is this the only type that can do this?
        if ( Expr->_Type->AsTypeStruct() == NULL )
        {
            if ( Parts.size() == 0 )
                return Expr->_Type;
            else
                return NULL;
        }

        return ResolveTypeChain( Expr->_Type->GetDeclaration() );
    }

    void TypeTypeof::_ToString( std::wstring& str )
    {
        // TODO: do we really need to define this method or can we have Type say "unresolved"?
        str.append( L"typeof()" );
    }


    //----------------------------------------------------------------------------
    //  TypeInstance
    //----------------------------------------------------------------------------

    TypeInstance::TypeInstance( TemplateInstancePart* instance )
        :   TypeQualified( Tinstance ),
            Instance( instance )
    {
    }

    RefPtr<Type> TypeInstance::Copy()
    {
        RefPtr<TypeQualified>    type = new TypeInstance( Instance.Get() );
        type->Parts = Parts;
        return type.Get();
    }

    RefPtr<Type> TypeInstance::Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( typeEnv );

        HRESULT hr = S_OK;
        RefPtr<Declaration> decl;
        RefPtr<Type>        resolvedType;
        std::wstring        fullId;

        fullId.reserve( Instance->Id->Length + Instance->ArgumentString->Length );
        fullId.append( Instance->Id->Str );
        fullId.append( Instance->ArgumentString->Str );

        resolvedType = ResolveNamePath( fullId.c_str(), binder );
        if ( resolvedType != NULL )
            return resolvedType;

        hr = binder->FindObject( fullId.c_str(), decl.Ref(), IValueBinder::FindObjectAny );
        if ( FAILED( hr ) )
            return NULL;

        return ResolveTypeChain( decl );
    }

    void TypeInstance::_ToString( std::wstring& str )
    {
        // TODO: do we really need to define this method or can we have Type say "unresolved"?
        str.append( L"instance" );
    }


    //----------------------------------------------------------------------------
    //  TypeIdentifier
    //----------------------------------------------------------------------------

    TypeIdentifier::TypeIdentifier( Utf16String* id )
        :   TypeQualified( Tident ),
            Id( id )
    {
    }

    RefPtr<Type> TypeIdentifier::Copy()
    {
        RefPtr<TypeQualified>    type = new TypeIdentifier( Id );
        type->Parts = Parts;
        return type.Get();
    }

    RefPtr<Type> TypeIdentifier::Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( typeEnv );

        HRESULT hr = S_OK;
        RefPtr<Declaration> decl;
        RefPtr<Type>        resolvedType;

        resolvedType = ResolveNamePath( Id->Str, binder );
        if ( resolvedType != NULL )
            return resolvedType;

        uint32_t flags = IValueBinder::FindObjectAny | ( Parts.empty() ? IValueBinder::FindObjectTryFQN : 0 );
        hr = binder->FindObject( Id->Str, decl.Ref(), flags );
        if ( FAILED( hr ) )
            return NULL;

        return ResolveTypeChain( decl );
    }

    void TypeIdentifier::_ToString( std::wstring& str )
    {
        // TODO: do we really need to define this method or can we have Type say "unresolved"?
        str.append( L"identifier" );
    }


    //----------------------------------------------------------------------------
    //  TypeSArrayUnresolved
    //----------------------------------------------------------------------------

    TypeSArrayUnresolved::TypeSArrayUnresolved( Type* element, Expression* expr )
        :   TypeNext( Tsarray, element ),
            Expr( expr )
    {
    }

    RefPtr<Type> TypeSArrayUnresolved::Copy()
    {
        RefPtr<Type>    type = new TypeSArrayUnresolved( Next.Get(), Expr.Get() );
        return type;
    }

    RefPtr<Type> TypeSArrayUnresolved::Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        HRESULT hr = S_OK;

        RefPtr<Type>    resolvedNext = Next->Resolve( evalData, typeEnv, binder );
        if ( resolvedNext == NULL )
            return NULL;

        hr = Expr->Semantic( evalData, typeEnv, binder );
        if ( FAILED( hr ) )
            return NULL;

        if ( !Expr->_Type->IsIntegral() )
            return NULL;

        // TODO: we should say that we can only evaluate constants and no variables
        DataObject  exprVal = { 0 };
        hr = Expr->Evaluate( EvalMode_Value, evalData, binder, exprVal );
        if ( FAILED( hr ) )
            return NULL;

        RefPtr<Type>    resolvedThis;
        hr = typeEnv->NewSArray( 
            resolvedNext, 
            // for now chopping it to 32 bits is OK
            (uint32_t) exprVal.Value.UInt64Value,
            resolvedThis.Ref() );
        if ( FAILED( hr ) )
            return NULL;

        return resolvedThis;
    }

    void TypeSArrayUnresolved::_ToString( std::wstring& str )
    {
        // TODO: do we really need to define this method or can we have Type say "unresolved"?
        str.append( L"unresolved_Sarray" );
    }


    //----------------------------------------------------------------------------
    //  TypeSlice
    //----------------------------------------------------------------------------

    TypeSlice::TypeSlice( Type* element, Expression* exprLow, Expression* exprHigh )
        :   TypeNext( Tslice, element ),
            ExprLow( exprLow ),
            ExprHigh( exprHigh )
    {
    }

    RefPtr<Type> TypeSlice::Copy()
    {
        RefPtr<Type>    type = new TypeSlice( Next.Get(), ExprLow.Get(), ExprHigh.Get() );
        return type;
    }

    RefPtr<Type> TypeSlice::Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( binder );
        // TODO:
        _ASSERT( false );
        return NULL;
    }

    void TypeSlice::_ToString( std::wstring& str )
    {
        // TODO: do we really need to define this method or can we have Type say "unresolved"?
        str.append( L"slice" );
    }
}
