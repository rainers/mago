/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "Object.h"
#include "TypeCommon.h"
#include "Type.h"


// these are all types that need to be resolved during evaluation

namespace MagoEE
{
    class Expression;
    class IdPart;
    class TemplateInstancePart;
    struct Utf16String;
    class Type;
    class TypeNext;
    class TypeStruct;
    enum ENUMTY;
    class Declaration;


    class NamePart : public Object
    {
    public:
        virtual ObjectKind  GetObjectKind();

        virtual IdPart*                 AsId();
        virtual TemplateInstancePart*   AsTemplateInstance();
    };


    class IdPart : public NamePart
    {
    public:
        Utf16String*                    Id;

        IdPart( Utf16String* str );

        virtual IdPart*                 AsId();
    };


    class TemplateInstancePart : public NamePart
    {
    public:
        Utf16String*                    Id;
        RefPtr<ObjectList>              Params;

        TemplateInstancePart( Utf16String* str );

        virtual TemplateInstancePart*   AsTemplateInstance();
    };


    class TypeQualified : public Type
    {
    public:
        std::vector< RefPtr<NamePart> > Parts;

        TypeQualified( ENUMTY ty );

    protected:
        RefPtr<Type> ResolveTypeChain( Declaration* head );
    };


    class TypeInstance : public TypeQualified
    {
    public:
        RefPtr<TemplateInstancePart>    Instance;

        TypeInstance( TemplateInstancePart* instance );
        virtual RefPtr<Type>    Copy();
        virtual RefPtr<Type> Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual void ToString( std::wstring& str );
    };


    class TypeIdentifier : public TypeQualified
    {
    public:
        //RefPtr<IdPart>  Id;
        Utf16String*    Id;

        TypeIdentifier( Utf16String* id );
        virtual RefPtr<Type>    Copy();
        virtual RefPtr<Type> Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual void ToString( std::wstring& str );
    };


    class TypeReturn : public TypeQualified
    {
    public:
        TypeReturn();

        virtual RefPtr<Type>    Copy();
        virtual RefPtr<Type> Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual void ToString( std::wstring& str );
    };


    class TypeTypeof : public TypeQualified
    {
    public:
        RefPtr<Expression>  Expr;

        TypeTypeof( Expression* expr );
        virtual RefPtr<Type>    Copy();
        virtual RefPtr<Type> Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual void ToString( std::wstring& str );
    };


    class TypeSArrayUnresolved : public TypeNext
    {
    public:
        RefPtr<Expression>  Expr;

        TypeSArrayUnresolved( Type* elem, Expression* expr );
        virtual RefPtr<Type>    Copy();
        virtual RefPtr<Type> Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual void ToString( std::wstring& str );
    };


    class TypeSlice : public TypeNext
    {
    public:
        RefPtr<Expression>  ExprLow;
        RefPtr<Expression>  ExprHigh;

        TypeSlice( Type* elem, Expression* exprLow, Expression* exprHigh );
        virtual RefPtr<Type>    Copy();
        virtual RefPtr<Type> Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual void ToString( std::wstring& str );
    };
}
