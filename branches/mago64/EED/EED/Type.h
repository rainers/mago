/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "Object.h"
#include "TypeCommon.h"


namespace MagoEE
{
    class Type;
    class ITypeNext;
    class ITypeStruct;
    class ITypeEnum;
    class ITypeFunction;
    class ITypeSArray;
    class ITypeDArray;
    class ITypeAArray;
    enum ENUMTY;
    class Declaration;
    class ITypeEnv;
    class IValueBinder;
    struct EvalData;
    class StdProperty;


    class Parameter : public Object
    {
    public:
        StorageClass    Storage;
        RefPtr<Type>    _Type;

        Parameter( StorageClass storage, Type* type );
        virtual ObjectKind  GetObjectKind();
    };


    class ParameterList : public Object
    {
    public:
        typedef std::list< RefPtr<Parameter> > ListType;

        ListType List;

        virtual ObjectKind GetObjectKind();
    };


    class Type : public Object
    {
    public:
        ENUMTY  Ty;
        MOD     Mod;

        Type( ENUMTY ty );

        // Object
        virtual ObjectKind GetObjectKind();

        // Type
        virtual RefPtr<Type>    Copy() = 0;

        virtual RefPtr<Type>    MakeMutable();
        virtual RefPtr<Type>    MakeShared();
        virtual RefPtr<Type>    MakeSharedConst();
        virtual RefPtr<Type>    MakeConst();
        virtual RefPtr<Type>    MakeInvariant();

        bool IsConst();
        bool IsInvariant();
        bool IsMutable();
        bool IsShared();
        bool IsSharedConst();

        bool CanImplicitCastToBool();

        virtual bool IsBasic();
        virtual bool IsPointer();
        virtual bool IsReference();
        virtual bool IsSArray();
        virtual bool IsDArray();
        virtual bool IsAArray();
        virtual bool IsFunction();
        virtual bool IsDelegate();

        virtual bool IsScalar();
        virtual bool IsBool();
        virtual bool IsChar();
        virtual bool IsIntegral();
        virtual bool IsFloatingPoint();
        virtual bool IsSigned();
        virtual bool IsReal();
        virtual bool IsImaginary();
        virtual bool IsComplex();
        virtual bool CanRefMember();

        virtual RefPtr<Declaration> GetDeclaration();
        virtual ENUMTY GetBackingTy();
        virtual uint32_t GetSize();
        virtual bool Equals( Type* other );
        virtual void ToString( std::wstring& str ) = 0;

        virtual RefPtr<Type> Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual StdProperty* FindProperty( const wchar_t* name );

        virtual ITypeNext* AsTypeNext();
        virtual ITypeStruct* AsTypeStruct();
        virtual ITypeEnum* AsTypeEnum();
        virtual ITypeFunction* AsTypeFunction();
        virtual ITypeSArray* AsTypeSArray();
        virtual ITypeDArray* AsTypeDArray();
        virtual ITypeAArray* AsTypeAArray();
    };


    class TypeBasic : public Type
    {
    public:
        TypeBasic( ENUMTY ty );

        virtual RefPtr<Type>    Copy();

        virtual bool IsBasic();

        virtual bool IsScalar();
        virtual bool IsBool();
        virtual bool IsChar();
        virtual bool IsIntegral();
        virtual bool IsFloatingPoint();
        virtual bool IsSigned();
        virtual bool IsReal();
        virtual bool IsImaginary();
        virtual bool IsComplex();

        virtual ENUMTY GetBackingTy();
        virtual uint32_t GetSize();
        virtual bool Equals( Type* other );
        virtual void ToString( std::wstring& str );

        virtual StdProperty* FindProperty( const wchar_t* name );

        static bool IsSigned( ENUMTY ty );
        static const wchar_t* GetTypeName( ENUMTY ty );
    };


    class ITypeNext
    {
    public:
        virtual RefPtr<Type>    GetNext() = 0;
    };


    class TypeNext : public Type, public ITypeNext
    {
    protected:
        RefPtr<Type>    Next;

    public:
        TypeNext( ENUMTY ty, Type* next );

        // Type
        virtual RefPtr<Type>    MakeShared();
        virtual RefPtr<Type>    MakeSharedConst();
        virtual RefPtr<Type>    MakeConst();
        virtual RefPtr<Type>    MakeInvariant();

        virtual ITypeNext*      AsTypeNext();

        // ITypeNext
        virtual RefPtr<Type>    GetNext();
    };


    class TypePointer : public TypeNext
    {
    public:
        TypePointer( Type* child );
        virtual RefPtr<Type>    Copy();
        virtual bool IsPointer();
        virtual bool IsScalar();
        virtual bool CanRefMember();
        virtual uint32_t GetSize();
        virtual bool Equals( Type* other );
        virtual void ToString( std::wstring& str );
        virtual RefPtr<Type> Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
    };


    class TypeReference : public TypeNext
    {
    public:
        TypeReference( Type* child );
        virtual RefPtr<Type>    Copy();
        virtual bool IsPointer();
        virtual bool IsReference();
        virtual bool IsScalar();
        virtual bool CanRefMember();
        virtual uint32_t GetSize();
        virtual bool Equals( Type* other );
        virtual void ToString( std::wstring& str );
        virtual RefPtr<Type> Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
    };


    class ITypeDArray
    {
    public:
        virtual RefPtr<Type> GetLengthType() = 0;
        virtual RefPtr<Type> GetPointerType() = 0;
        virtual Type*        GetElement() = 0;
    };


    class TypeDArray : public TypeNext, public ITypeDArray
    {
        RefPtr<Type>    mLenType;
        RefPtr<Type>    mPtrType;

    public:
        TypeDArray( Type* element, Type* lenType, Type* ptrType );
        virtual RefPtr<Type>    Copy();
        virtual bool IsDArray();
        virtual uint32_t GetSize();
        virtual bool Equals( Type* other );
        virtual RefPtr<Type> Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual void ToString( std::wstring& str );
        virtual StdProperty* FindProperty( const wchar_t* name );
        virtual ITypeDArray* AsTypeDArray();

        // ITypeDArray
        virtual RefPtr<Type> GetLengthType();
        virtual RefPtr<Type> GetPointerType();
        virtual Type*        GetElement();
    };


    class ITypeAArray
    {
    public:
        virtual Type* GetElement() = 0;
        virtual Type* GetIndex() = 0;
    };


    class TypeAArray : public TypeNext, public ITypeAArray
    {
        uint32_t        mSize;
        RefPtr<Type>    Index;

    public:
        TypeAArray( Type* elem, Type* index, uint32_t size );
        virtual RefPtr<Type>    Copy();
        virtual bool IsAArray();
        virtual uint32_t GetSize();
        virtual bool Equals( Type* other );
        virtual void ToString( std::wstring& str );
        virtual RefPtr<Type> Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual ITypeAArray* AsTypeAArray();

        // ITypeAArray
        virtual Type* GetElement();
        virtual Type* GetIndex();
    };


    class ITypeSArray
    {
    public:
        virtual uint32_t    GetLength() = 0;
        virtual Type*       GetElement() = 0;
    };


    class TypeSArray : public TypeNext, public ITypeSArray
    {
        uint32_t            Length;

    public:
        TypeSArray( Type* elem, uint32_t len );
        virtual RefPtr<Type>    Copy();
        virtual bool IsSArray();
        virtual uint32_t GetSize();
        virtual bool Equals( Type* other );
        virtual RefPtr<Type> Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual void ToString( std::wstring& str );
        virtual StdProperty* FindProperty( const wchar_t* name );
        virtual ITypeSArray* AsTypeSArray();

        // ITypeSArray
        virtual uint32_t    GetLength();
        virtual Type*       GetElement();
    };


    class ITypeFunction
    {
    public:
        virtual Type*   GetReturnType() = 0;
        virtual int     GetVarArgs() = 0;
        virtual ParameterList*  GetParams() = 0;

        virtual bool    IsPure() = 0;
        virtual bool    IsNoThrow() = 0;
        virtual bool    IsProperty() = 0;
        virtual TRUST   GetTrust() = 0;

        virtual void    SetPure( bool value ) = 0;
        virtual void    SetNoThrow( bool value ) = 0;
        virtual void    SetProperty( bool value ) = 0;
        virtual void    SetTrust( TRUST value ) = 0;
    };


    // TODO: should there be a calling convention?

    // in DMD source this is also a TypeNext
    class TypeFunction : public TypeNext, public ITypeFunction
    {
        bool                    mIsPure;
        bool                    mIsNoThrow;
        bool                    mIsProperty;
        TRUST                   mTrust;
        RefPtr<ParameterList>   Params;
        int                     VarArgs;

    public:
        TypeFunction( ParameterList* params, Type* retType, int varArgs );
        virtual RefPtr<Type>    Copy();
        virtual bool IsFunction();
        virtual bool Equals( Type* other );
        virtual void ToString( std::wstring& str );
        virtual ITypeFunction* AsTypeFunction();

        // ITypeFunction
        virtual Type*   GetReturnType();
        virtual int     GetVarArgs();
        virtual ParameterList*  GetParams();

        virtual bool    IsPure();
        virtual bool    IsNoThrow();
        virtual bool    IsProperty();
        virtual TRUST   GetTrust();

        virtual void    SetPure( bool value );
        virtual void    SetNoThrow( bool value );
        virtual void    SetProperty( bool value );
        virtual void    SetTrust( TRUST value );
    };


    class TypeDelegate : public TypeNext
    {
    public:
        TypeDelegate( Type* ptrToFunction );
        virtual RefPtr<Type>    Copy();
        virtual bool IsDelegate();
        virtual uint32_t GetSize();
        virtual bool Equals( Type* other );
        virtual void ToString( std::wstring& str );
        virtual StdProperty* FindProperty( const wchar_t* name );
    };


    class ITypeStruct
    {
    public:
        virtual RefPtr<Declaration> FindObject( const wchar_t* name ) = 0;

        // TODO: where do we put the method that returns all the members?

        virtual UdtKind GetUdtKind() = 0;
        virtual bool GetBaseClassOffset( Type* baseClass, int& offset ) = 0;
    };


    class TypeStruct : public Type, public ITypeStruct
    {
        RefPtr<Declaration>  mDecl;

    public:
        TypeStruct( Declaration* decl );
        RefPtr<Declaration> GetDeclaration();
        virtual RefPtr<Type>    Copy();
        virtual bool CanRefMember();
        virtual uint32_t GetSize();
        virtual bool Equals( Type* other );
        virtual void ToString( std::wstring& str );
        virtual ITypeStruct* AsTypeStruct();

        // ITypeStruct
        virtual RefPtr<Declaration> FindObject( const wchar_t* name );
        virtual UdtKind GetUdtKind();
        virtual bool GetBaseClassOffset( Type* baseClass, int& offset );
    };


    class ITypeEnum
    {
    public:
        virtual RefPtr<Declaration> FindObject( const wchar_t* name ) = 0;
        virtual RefPtr<Declaration> FindObjectByValue( uint64_t intVal ) = 0;
    };


    class TypeEnum : public Type, public ITypeEnum
    {
        RefPtr<Declaration> mDecl;
        ENUMTY              mBackingTy;

    public:
        TypeEnum( Declaration* decl );
        virtual ENUMTY GetBackingTy();
        RefPtr<Declaration> GetDeclaration();
        virtual RefPtr<Type>    Copy();
        virtual bool IsIntegral();
        virtual bool IsSigned();
        virtual bool CanRefMember();
        virtual uint32_t GetSize();
        virtual bool Equals( Type* other );
        virtual void ToString( std::wstring& str );
        virtual ITypeEnum* AsTypeEnum();

        // ITypeEnum
        virtual RefPtr<Declaration> FindObject( const wchar_t* name );
        virtual RefPtr<Declaration> FindObjectByValue( uint64_t intVal );
    };


    class TypeTypedef : public Type
    {
    protected:
        RefPtr<Type>    mAliased;
        std::wstring    mName;

    public:
        TypeTypedef( const wchar_t* name, Type* aliasedType );

        // Type
        virtual RefPtr<Type>    Copy();

        virtual bool IsBasic();
        virtual bool IsPointer();
        virtual bool IsReference();
        virtual bool IsSArray();
        virtual bool IsDArray();

        virtual bool IsScalar();
        virtual bool IsBool();
        virtual bool IsChar();
        virtual bool IsIntegral();
        virtual bool IsFloatingPoint();
        virtual bool IsSigned();
        virtual bool IsReal();
        virtual bool IsImaginary();
        virtual bool IsComplex();
        virtual bool CanRefMember();

        virtual RefPtr<Declaration> GetDeclaration();
        virtual ENUMTY GetBackingTy();
        virtual uint32_t GetSize();
        virtual bool Equals( Type* other );
        virtual void ToString( std::wstring& str );

        virtual StdProperty* FindProperty( const wchar_t* name );

        virtual ITypeNext* AsTypeNext();
        virtual ITypeStruct* AsTypeStruct();
        virtual ITypeEnum* AsTypeEnum();
        virtual ITypeFunction* AsTypeFunction();
        virtual ITypeSArray* AsTypeSArray();
        virtual ITypeDArray* AsTypeDArray();
        virtual ITypeAArray* AsTypeAArray();
    };
}
