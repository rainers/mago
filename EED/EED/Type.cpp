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
#include "Properties.h"
#include "PropTables.h"


namespace MagoEE
{
    //----------------------------------------------------------------------------
    //  Parameter
    //----------------------------------------------------------------------------

    Parameter::Parameter( StorageClass storage, Type* type )
        :   Storage( storage ),
            _Type( type )
    {
    }

    ObjectKind Parameter::GetObjectKind()
    {
        return ObjectKind_Parameter;
    }


    //----------------------------------------------------------------------------
    //  ParameterList
    //----------------------------------------------------------------------------

    ObjectKind ParameterList::GetObjectKind()
    {
        return ObjectKind_ParameterList;
    }


    //----------------------------------------------------------------------------
    //  Type
    //----------------------------------------------------------------------------

    Type::Type( ENUMTY ty )
        :   Ty( ty ),
            Mod( MODnone )
    {
    }

    ObjectKind Type::GetObjectKind()
    {
        return ObjectKind_Type;
    }

    RefPtr<Type> Type::MakeMutable()
    {
        RefPtr<Type>    type = Copy();
        type->Mod = (MOD) (Mod & MODshared);
        return type;
    }

    RefPtr<Type> Type::MakeConst()
    {
        RefPtr<Type>    type = Copy();
        type->Mod = MODconst;
        return type;
    }

    RefPtr<Type> Type::MakeSharedConst()
    {
        RefPtr<Type>    type = Copy();
        type->Mod = (MOD) (MODconst | MODshared);
        return type;
    }

    RefPtr<Type> Type::MakeShared()
    {
        RefPtr<Type>    type = Copy();
        type->Mod = MODshared;
        return type;
    }

    RefPtr<Type> Type::MakeInvariant()
    {
        RefPtr<Type>    type = Copy();
        type->Mod = MODinvariant;
        return type;
    }

    RefPtr<Type> Type::MakeMod( MOD m )
    {
        RefPtr<Type>    type = Copy();
        type->Mod = m;
        return type;
    }

    bool Type::IsConst()        { return (Mod & MODconst) != 0; }
    bool Type::IsInvariant()    { return (Mod & MODinvariant) != 0; }
    bool Type::IsMutable()      { return !(Mod & (MODconst | MODinvariant)); }
    bool Type::IsShared()       { return (Mod & MODshared) != 0; }
    bool Type::IsSharedConst()  { return Mod == (MODshared | MODconst); }

    void Type::ToString( std::wstring& str )
    {
        size_t len = str.length();
        _ToString( str );
        if ( Mod == 0 )
            return;
        std::wstring sub = str.substr(len);
        if ( Mod & MODinvariant )
            sub = L"immutable(" + sub + L")";
        else if ( Mod & MODconst )
            sub = L"const(" + sub + L")";
        if ( Mod & MODshared )
            sub = L"shared(" + sub + L")";
        str = str.substr( 0, len ) + sub;
    }

    bool Type::CanImplicitCastToBool()
    {
        return IsScalar() || IsDArray() || IsSArray() || IsAArray() || IsDelegate();
    }

    bool Type::IsBasic()
    {
        return false;
    }

    bool Type::IsPointer()
    {
        return false;
    }

    bool Type::IsReference()
    {
        return false;
    }

    bool Type::IsSArray()
    {
        return false;
    }

    bool Type::IsDArray()
    {
        return false;
    }

    bool Type::IsAArray()
    {
        return false;
    }

    bool Type::IsFunction()
    {
        return false;
    }

    bool Type::IsDelegate()
    {
        return false;
    }

    bool Type::IsScalar()
    {
        return false;
    }

    bool Type::IsBool()
    {
        return false;
    }

    bool Type::IsChar()
    {
        return false;
    }

    bool Type::IsIntegral()
    {
        return false;
    }

    bool Type::IsFloatingPoint()
    {
        return false;
    }

    bool Type::IsSigned()
    {
        return false;
    }

    bool Type::IsReal()
    {
        return false;
    }

    bool Type::IsImaginary()
    {
        return false;
    }

    bool Type::IsComplex()
    {
        return false;
    }

    uint32_t Type::GetSize()
    {
        return 0;
    }

    ITypeNext* Type::AsTypeNext()
    {
        return NULL;
    }

    ITypeStruct* Type::AsTypeStruct()
    {
        return NULL;
    }

    ITypeEnum* Type::AsTypeEnum()
    {
        return NULL;
    }

    ITypeFunction* Type::AsTypeFunction()
    {
        return NULL;
    }

    ITypeSArray* Type::AsTypeSArray()
    {
        return NULL;
    }

    ITypeAArray* Type::AsTypeAArray()
    {
        return NULL;
    }

    ITypeDArray* Type::AsTypeDArray()
    {
        return NULL;
    }

    ITypeTuple* Type::AsTypeTuple()
    {
        return NULL;
    }

    Type* Type::Unaliased()
    {
        return this;
    }

    bool Type::CanRefMember()
    {
        return false;
    }

    RefPtr<Declaration> Type::GetDeclaration()
    {
        return NULL;
    }

    ENUMTY Type::GetBackingTy()
    {
        return Ty;
    }

    bool Type::Equals( Type* other )
    {
        UNREFERENCED_PARAMETER( other );
        return false;
    }

    RefPtr<Type> Type::Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( binder );
        return this;
    }


    //----------------------------------------------------------------------------
    //  TypeBasic
    //----------------------------------------------------------------------------

    TypeBasic::TypeBasic( ENUMTY ty )
        :   Type( ty )
    {
    }

    RefPtr<Type> TypeBasic::Copy()
    {
        RefPtr<Type>    type = new TypeBasic( Ty );
        return type;
    }

    uint32_t TypeBasic::GetSize()
    {
        return GetTypeSize( Ty );
    }

    uint32_t TypeBasic::GetTypeSize( ENUMTY ty )
    {
        switch ( ty )
        {
        case Tvoid:     return 0;
        case Tint8:     case Tuns8:     return 1;
        case Tint16:    case Tuns16:    return 2;
        case Tint32:    case Tuns32:    return 4;
        case Tint64:    case Tuns64:    return 8;
        case Tfloat32:  case Timaginary32:  return 4;
        case Tfloat64:  case Timaginary64:  return 8;
        case Tfloat80:  case Timaginary80:  return 10;
        case Tcomplex32:    return 8;
        case Tcomplex64:    return 16;
        case Tcomplex80:    return 20;
        case Tbit:      case Tbool:     return 1;
        case Tchar:     return 1;
        case Twchar:    return 2;
        case Tdchar:    return 4;
        }

        _ASSERT( false );
        return 0;
    }

    bool TypeBasic::IsBasic()
    {
        return true;
    }

    bool TypeBasic::IsScalar()
    {
        switch ( Ty )
        {
        case Tint8:     case Tuns8:
        case Tint16:    case Tuns16:
        case Tint32:    case Tuns32:
        case Tint64:    case Tuns64:
        case Tbit:      case Tbool:
        case Tchar:     case Twchar:        case Tdchar:
        case Tfloat32:  case Timaginary32:  case Tcomplex32:
        case Tfloat64:  case Timaginary64:  case Tcomplex64:
        case Tfloat80:  case Timaginary80:  case Tcomplex80:
            return true;
        }
        return false;
    }

    bool TypeBasic::IsBool()
    {
        return Ty == Tbool;
    }

    bool TypeBasic::IsChar()
    {
        switch ( Ty )
        {
        case Tchar:
        case Twchar:
        case Tdchar:
            return true;
        }
        return false;
    }

    bool TypeBasic::IsIntegral()
    {
        switch ( Ty )
        {
        case Tint8:     case Tuns8:
        case Tint16:    case Tuns16:
        case Tint32:    case Tuns32:
        case Tint64:    case Tuns64:
        case Tbit:      case Tbool:
        case Tchar:     case Twchar:    case Tdchar:
            return true;
        }
        return false;
    }

    bool TypeBasic::IsFloatingPoint()
    {
        switch ( Ty )
        {
        case Tfloat32:  case Timaginary32:  case Tcomplex32:
        case Tfloat64:  case Timaginary64:  case Tcomplex64:
        case Tfloat80:  case Timaginary80:  case Tcomplex80:
            return true;
        }
        return false;
    }

    bool TypeBasic::IsSigned()
    {
        return IsSigned( Ty );
    }

    bool TypeBasic::IsSigned( ENUMTY ty )
    {
        switch ( ty )
        {
        case Tint8:
        case Tint16:
        case Tint32:
        case Tint64:
            return true;
        }
        return false;
    }

    bool TypeBasic::IsReal()
    {
        switch ( Ty )
        {
        case Tfloat32:
        case Tfloat64:
        case Tfloat80:
            return true;
        }
        return false;
    }

    bool TypeBasic::IsImaginary()
    {
        switch ( Ty )
        {
        case Timaginary32:
        case Timaginary64:
        case Timaginary80:
            return true;
        }
        return false;
    }

    bool TypeBasic::IsComplex()
    {
        switch ( Ty )
        {
        case Tcomplex32:
        case Tcomplex64:
        case Tcomplex80:
            return true;
        }
        return false;
    }

    ENUMTY TypeBasic::GetBackingTy()
    {
        switch ( Ty )
        {
        case Tbit:      
        case Tbool:     
        case Tchar:
            return Tuns8;
        case Twchar:        return Tuns16;
        case Tdchar:        return Tuns32;
        case Timaginary32:  return Tfloat32;
        case Timaginary64:  return Tfloat64;
        case Timaginary80:  return Tfloat80;
        }
        return Ty;
    }

    bool TypeBasic::Equals( Type* other )
    {
        if ( other->IsBasic() )
            return other->Ty == Ty;

        return false;
    }

    void TypeBasic::_ToString( std::wstring& str )
    {
        str.append( GetTypeName( Ty ) );
    }

    const wchar_t* TypeBasic::GetTypeName( ENUMTY ty )
    {
        switch ( ty )
        {
        case Tvoid:     return L"void";
        case Tint8:     return L"byte";
        case Tuns8:     return L"ubyte";
        case Tint16:    return L"short";
        case Tuns16:    return L"ushort";
        case Tint32:    return L"int";
        case Tuns32:    return L"uint";
        case Tint64:    return L"long";
        case Tuns64:    return L"ulong";
        case Tfloat32:  return L"float";
        case Timaginary32:  return L"ifloat";
        case Tfloat64:  return L"double";
        case Timaginary64:  return L"idouble";
        case Tfloat80:  return L"real";
        case Timaginary80:  return L"ireal";
        case Tcomplex32:    return L"cfloat";
        case Tcomplex64:    return L"cdouble";
        case Tcomplex80:    return L"creal";
        case Tbit:      case Tbool:     return L"bool";
        case Tchar:     return L"char";
        case Twchar:    return L"wchar";
        case Tdchar:    return L"dchar";
        }

        _ASSERT( false );
        return NULL;
    }


    //----------------------------------------------------------------------------
    //  TypeNext
    //----------------------------------------------------------------------------

    TypeNext::TypeNext( ENUMTY ty, Type* next )
        :   Type( ty ),
            Next( next )
    {
    }

    RefPtr<Type> TypeNext::MakeConst()
    {
        RefPtr<TypeNext>    type = (TypeNext*) Type::MakeConst().Get();
        if ( (Ty != Tfunction) 
            && (Ty != Tdelegate)
            && !Next->IsInvariant()
            && !Next->IsConst() )
        {
            if ( Next->IsShared() )
                type->Next = Next->MakeSharedConst();
            else
                type->Next = Next->MakeConst();
        }
        return type.Get();
    }

    RefPtr<Type> TypeNext::MakeSharedConst()
    {
        RefPtr<TypeNext>    type = (TypeNext*) Type::MakeSharedConst().Get();
        if ( (Ty != Tfunction) 
            && (Ty != Tdelegate)
            && !Next->IsInvariant()
            && !Next->IsSharedConst() )
        {
            type->Next = Next->MakeSharedConst();
        }
        return type.Get();
    }

    RefPtr<Type> TypeNext::MakeShared()
    {
        RefPtr<TypeNext>    type = (TypeNext*) Type::MakeShared().Get();
        if ( (Ty != Tfunction) 
            && (Ty != Tdelegate)
            && !Next->IsInvariant()
            && !Next->IsShared() )
        {
            if ( Next->IsConst() )
                type->Next = Next->MakeSharedConst();
            else
                type->Next = Next->MakeShared();
        }
        return type.Get();
    }

    RefPtr<Type> TypeNext::MakeInvariant()
    {
        RefPtr<TypeNext>    type = (TypeNext*) Type::MakeInvariant().Get();
        if ( (Ty != Tfunction) 
            && (Ty != Tdelegate)
            && !Next->IsInvariant() )
        {
            type->Next = Next->MakeInvariant();
        }
        return type.Get();
    }

    ITypeNext* TypeNext::AsTypeNext()
    {
        return this;
    }

    RefPtr<Type>    TypeNext::GetNext()
    {
        return Next;
    }


    //----------------------------------------------------------------------------
    //  TypePointer
    //----------------------------------------------------------------------------

    TypePointer::TypePointer( Type* child, int ptrSize )
        :   TypeNext( Tpointer, child ),
            mPtrSize( ptrSize )
    {
    }

    uint32_t TypePointer::GetSize()
    {
        return mPtrSize;
    }

    RefPtr<Type> TypePointer::Copy()
    {
        RefPtr<Type>    type = new TypePointer( Next.Get(), mPtrSize );
        return type;
    }

    bool TypePointer::IsPointer()
    {
        return true;
    }

    bool TypePointer::IsScalar()
    {
        return true;
    }

    bool TypePointer::CanRefMember()
    {
        _ASSERT( Next.Get() != NULL );
        return Next->AsTypeStruct() != NULL;
    }

    bool TypePointer::Equals( Type* other )
    {
        if ( other->Ty != Tpointer )
            return false;

        return Next->Equals( other->AsTypeNext()->GetNext() );
    }

    void TypePointer::_ToString( std::wstring& str )
    {
        Next->ToString( str );
        str.append( L"*" );
    }

    RefPtr<Type> TypePointer::Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        RefPtr<Type>    resolvedNext = Next->Resolve( evalData, typeEnv, binder );
        if ( resolvedNext == NULL )
            return NULL;

        RefPtr<Type>    type = new TypePointer( resolvedNext, mPtrSize );
        type->Mod = Mod;
        return type;
    }


    //----------------------------------------------------------------------------
    //  TypeReference
    //----------------------------------------------------------------------------

    TypeReference::TypeReference( Type* child, int ptrSize )
        :   TypeNext( Treference, child ),
            mPtrSize( ptrSize )
    {
    }

    uint32_t TypeReference::GetSize()
    {
        return mPtrSize;
    }

    RefPtr<Type> TypeReference::Copy()
    {
        RefPtr<Type>    type = new TypeReference( Next.Get(), mPtrSize );
        return type;
    }

    bool TypeReference::IsPointer()
    {
        return true;
    }

    bool TypeReference::IsReference()
    {
        return true;
    }

    bool TypeReference::IsScalar()
    {
        return true;
    }

    bool TypeReference::CanRefMember()
    {
        _ASSERT( Next.Get() != NULL );
        return Next->AsTypeStruct() != NULL;
    }

    bool TypeReference::Equals( Type* other )
    {
        if ( other->Ty != Treference )
            return false;

        return Next->Equals( other->AsTypeNext()->GetNext() );
    }

    void TypeReference::_ToString( std::wstring& str )
    {
        Next->ToString( str );
    }

    RefPtr<Type> TypeReference::Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        RefPtr<Type>    resolvedNext = Next->Resolve( evalData, typeEnv, binder );
        if ( resolvedNext == NULL )
            return NULL;

        RefPtr<Type>    type = new TypeReference( resolvedNext, mPtrSize );
        type->Mod = Mod;
        return type;
    }


    //----------------------------------------------------------------------------
    //  TypeDArray
    //----------------------------------------------------------------------------

    TypeDArray::TypeDArray( Type* element, Type* lenType, Type* ptrType )
        :   TypeNext( Tarray, element ),
            mLenType( lenType ),
            mPtrType( ptrType )
    {
    }

    RefPtr<Type> TypeDArray::GetLengthType()
    {
        return mLenType;
    }

    RefPtr<Type> TypeDArray::GetPointerType()
    {
        return mPtrType;
    }

    Type*       TypeDArray::GetElement()
    {
        return Next;
    }

    RefPtr<Type> TypeDArray::Copy()
    {
        RefPtr<Type>    type = new TypeDArray( Next.Get(), mLenType, mPtrType );
        return type;
    }

    bool TypeDArray::IsDArray()
    {
        return true;
    }

    uint32_t TypeDArray::GetSize()
    {
        return mLenType->GetSize() + mPtrType->GetSize();
    }

    bool TypeDArray::Equals( Type* other )
    {
        if ( other->Ty != Tarray )
            return false;

        return Next->Equals( other->AsTypeNext()->GetNext() );
    }

    void TypeDArray::_ToString( std::wstring& str )
    {
        size_t len = str.length();
        Next->ToString( str );
        std::wstring sub = str.substr(len);
        if ( sub == L"immutable(char)" )
            str = str.substr( 0, len ) + L"string";
        else if ( sub == L"immutable(wchar)" )
            str = str.substr( 0, len ) + L"wstring";
        else if ( sub == L"immutable(dchar)" )
            str = str.substr( 0, len ) + L"dstring";
        else
            str.append( L"[]" );
    }

    RefPtr<Type> TypeDArray::Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        RefPtr<Type>    resolvedNext = Next->Resolve( evalData, typeEnv, binder );
        if ( resolvedNext == NULL )
            return NULL;

        RefPtr<Type>    type = new TypeDArray( resolvedNext, mLenType, mPtrType );
        type->Mod = Mod;
        return type;
    }

    ITypeDArray* TypeDArray::AsTypeDArray()
    {
        return this;
    }


    //----------------------------------------------------------------------------
    //  TypeAArray
    //----------------------------------------------------------------------------

    TypeAArray::TypeAArray( Type* element, Type* index, uint32_t size )
        :   TypeNext( Taarray, element ),
            mSize( size ),
            Index( index )
    {
        _ASSERT( size > 0 );
    }

    RefPtr<Type> TypeAArray::Copy()
    {
        RefPtr<Type>    type = new TypeAArray( Next.Get(), Index.Get(), mSize );
        return type;
    }

    bool TypeAArray::IsAArray()
    {
        return true;
    }

    uint32_t TypeAArray::GetSize()
    {
        return mSize;
    }

    bool TypeAArray::Equals( Type* other )
    {
        if ( other->Ty != Taarray )
            return false;

        if ( !Next->Equals( other->AsTypeNext()->GetNext() ) )
            return false;

        return Index->Equals( ((TypeAArray*) other)->Index.Get() );
    }

    void TypeAArray::_ToString( std::wstring& str )
    {
        Next->ToString( str );
        str.append( L"[" );
        Index->ToString( str );
        str.append( L"]" );
    }

    RefPtr<Type> TypeAArray::Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        RefPtr<Type>    resolvedNext = Next->Resolve( evalData, typeEnv, binder );
        if ( resolvedNext == NULL )
            return NULL;

        RefPtr<Type>    resolvedIndex = Index->Resolve( evalData, typeEnv, binder );
        if ( resolvedIndex == NULL )
            return NULL;

        RefPtr<Type>    type = new TypeAArray( resolvedNext, resolvedIndex, mSize );
        type->Mod = Mod;
        return type;
    }

    ITypeAArray* TypeAArray::AsTypeAArray()
    {
        return this;
    }

    Type* TypeAArray::GetElement()
    {
        return Next;
    }

    Type* TypeAArray::GetIndex()
    {
        return Index;
    }


    //----------------------------------------------------------------------------
    //  TypeSArray
    //----------------------------------------------------------------------------

    TypeSArray::TypeSArray( Type* element, uint32_t length )
        :   TypeNext( Tsarray, element ),
            Length( length )
    {
    }

    RefPtr<Type> TypeSArray::Copy()
    {
        RefPtr<Type>    type = new TypeSArray( Next.Get(), Length );
        return type;
    }

    bool TypeSArray::IsSArray()
    {
        return true;
    }

    uint32_t TypeSArray::GetSize()
    {
        return Next->GetSize() * Length;
    }

    bool TypeSArray::Equals( Type* other )
    {
        if ( other->Ty != Tsarray )
            return false;

        if ( !Next->Equals( other->AsTypeNext()->GetNext() ) )
            return false;

        return Length == ((TypeSArray*) other)->Length;
    }

    void TypeSArray::_ToString( std::wstring& str )
    {
        // we're using space for a ulong's digits, but we only need that for a uint
        const int UlongDigits = 20;
        wchar_t numStr[ UlongDigits + 1 ] = L"";
        errno_t err = 0;

        err = _ultow_s( Length, numStr, 10 );
        _ASSERT( err == 0 );

        Next->ToString( str );
        str.append( L"[" );
        str.append( numStr );
        str.append( L"]" );
    }

    RefPtr<Type> TypeSArray::Resolve( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        RefPtr<Type>    resolvedNext = Next->Resolve( evalData, typeEnv, binder );
        if ( resolvedNext == NULL )
            return NULL;

        RefPtr<Type>    type = new TypeSArray( resolvedNext, Length );
        type->Mod = Mod;
        return type;
    }

    ITypeSArray* TypeSArray::AsTypeSArray()
    {
        return this;
    }

    uint32_t    TypeSArray::GetLength()
    {
        return Length;
    }

    Type*       TypeSArray::GetElement()
    {
        return Next;
    }

    //----------------------------------------------------------------------------
    //  TypeTuple
    //----------------------------------------------------------------------------

    TypeTuple::TypeTuple( const std::vector<RefPtr<Declaration>>& fields, bool ambiguousGlobals )
        : Type( Ttuple ),
          mFields( fields ),
          mAmbiguousGlobals( ambiguousGlobals )
    {
    }

    RefPtr<Type> TypeTuple::Copy()
    {
        RefPtr<Type> type = new TypeTuple( mFields, mAmbiguousGlobals );
        return type;
    }

    uint32_t TypeTuple::GetSize()
    {
        uint32_t s = 0;
        for( auto& f : mFields )
        {
            RefPtr<Type> type;
            if( SUCCEEDED( f->GetType( type.Ref() ) ) )
                s += type->GetSize();
        }
        return s;
    }

    bool TypeTuple::Equals( Type* other )
    {
        if ( other->Ty != Ttuple )
            return false;

        ITypeTuple* tpl = other->AsTypeTuple();
        if( tpl->GetLength() != mFields.size() )
            return false;

        uint32_t i = 0;
        for ( auto& f : mFields )
        {
            RefPtr<Type> type;
            if( SUCCEEDED( f->GetType( type.Ref() ) ) )
                if ( !type->Equals( tpl->GetElementType( i ) ) )
                    return false;
            i++;
        }
        return true;
    }

    void TypeTuple::_ToString( std::wstring& str )
    {
        if ( mAmbiguousGlobals )
        {
            str.append( L"<ambiguous globals>" );
            return;
        }
        str.append( L"tuple!(" );
        auto len = str.length();
        for ( auto& f : mFields )
        {
            RefPtr<Type> type;
            if ( SUCCEEDED( f->GetType( type.Ref() ) ) )
            {
                if( str.length() > len )
                    str.append(L",");
                type->ToString( str );
            }
        }
        str.append(L")");
    }

    ITypeTuple* TypeTuple::AsTypeTuple()
    {
        return this;
    }

    uint32_t    TypeTuple::GetLength()
    {
        return mFields.size();
    }

    Declaration* TypeTuple::GetElementDecl( uint32_t idx )
    {
        return mFields[idx];
    }

    Type* TypeTuple::GetElementType( uint32_t idx )
    {
        RefPtr<Type> type;
        if( FAILED( mFields[idx]->GetType( type.Ref() ) ) )
            return nullptr;
        return type;
    }

    bool TypeTuple::IsAmbiguousGlobals()
    {
        return mAmbiguousGlobals;
    }

    //----------------------------------------------------------------------------
    //  TypeFunction
    //----------------------------------------------------------------------------

    TypeFunction::TypeFunction( ParameterList* params, Type* retType, Type* thisPtrType, uint8_t callConv, int varArgs )
        :   TypeNext( Tfunction, retType ),
            mThisPointerType( thisPtrType ),
            Params( params ),
            VarArgs( varArgs ),
            mCallConv( callConv ),
            mIsPure( false ),
            mIsNoThrow( false ),
            mIsProperty( false ),
            mIsConst( false ),
            mTrust( TRUSTdefault )
    {
    }

    RefPtr<Type> TypeFunction::Copy()
    {
        RefPtr<Type>    type = new TypeFunction( Params.Get(), Next.Get(), mThisPointerType.Get(), mCallConv, VarArgs);
        ITypeFunction* funcType = type->AsTypeFunction();
        funcType->SetPure( mIsPure );
        funcType->SetNoThrow( mIsNoThrow );
        funcType->SetProperty( mIsProperty );
        funcType->SetConst( mIsConst );
        funcType->SetTrust( mTrust );
        return type;
    }

    bool TypeFunction::IsFunction()
    {
        return true;
    }

    bool TypeFunction::Equals( Type* other )
    {
        if ( other->Ty != Ty )
            return false;

        ITypeFunction*  otherFunc = other->AsTypeFunction();

        if ( !Next->Equals( otherFunc->GetReturnType() ) )
            return false;

        if ( VarArgs != otherFunc->GetVarArgs() )
            return false;

        if ( mCallConv != otherFunc->GetCallConv() )
            return false;

        ParameterList*  otherParams = otherFunc->GetParams();

        if ( Params->List.size() != otherParams->List.size() )
            return false;

        for ( ParameterList::ListType::iterator it = Params->List.begin(), otherIt = otherParams->List.begin();
            it != Params->List.end();
            it++, otherIt++ )
        {
            Parameter*  param = *it;
            Parameter*  otherParam = *otherIt;

            if ( param->Storage != otherParam->Storage )
                return false;

            if ( !param->_Type->Equals( otherParam->_Type ) )
                return false;
        }

        // TODO: do we have to check pure, trust, and the rest?
        // TODO: does this class really need to store those attributes?

        return true;
    }

    void StorageClassToString( StorageClass storage, std::wstring& str )
    {
        if ( (storage & STCconst) != 0 )
            str.append( L"const " );

        if ( (storage & STCimmutable) != 0 )
            str.append( L"immutable " );

        if ( (storage & STCshared) != 0 )
            str.append( L"shared " );

        if ( (storage & STCin) != 0 )
            str.append( L"in " );

        if ( (storage & STCout) != 0 )
            str.append( L"out " );

        // it looks like ref is preferred over inout
        if ( (storage & STCref) != 0 )
            str.append( L"ref " );

        if ( (storage & STClazy) != 0 )
            str.append( L"lazy " );

        if ( (storage & STCscope) != 0 )
            str.append( L"scope " );

        if ( (storage & STCfinal) != 0 )
            str.append( L"final " );

        // TODO: any more storage classes?
    }

    void FunctionToString( const wchar_t* keyword, ITypeFunction* funcType, std::wstring& str )
    {
        funcType->GetReturnType()->ToString( str );
        str.append( L" " );
        str.append( keyword );
        str.append( L"(" );

        ParameterList*  params = funcType->GetParams();

        for ( ParameterList::ListType::iterator it = params->List.begin(); 
            it != params->List.end(); 
            it++ )
        {
            if ( it != params->List.begin() )
                str.append( L", " );

            StorageClassToString( (*it)->Storage, str );

            (*it)->_Type->ToString( str );
        }

        if ( funcType->GetVarArgs() == 1 )
        {
            str.append( L", ..." );
        }
        else if ( funcType->GetVarArgs() == 2 )
        {
            str.append( L"..." );
        }

        str.append( L")" );
    }

    void TypeFunction::_ToString( std::wstring& str )
    {
        FunctionToString( L"function", AsTypeFunction(), str );
    }

    ITypeFunction* TypeFunction::AsTypeFunction()
    {
        return this;
    }

    Type* TypeFunction::GetReturnType()
    {
        return Next;
    }

    Type* TypeFunction::GetThisPointerType()
    {
        return mThisPointerType;
    }

    int TypeFunction::GetVarArgs()
    {
        return VarArgs;
    }

    ParameterList*  TypeFunction::GetParams()
    {
        return Params;
    }

    bool TypeFunction::IsPure()
    {
        return mIsPure;
    }

    bool TypeFunction::IsNoThrow()
    {
        return mIsNoThrow;
    }

    bool TypeFunction::IsProperty()
    {
        return mIsProperty;
    }

    bool TypeFunction::IsConst()
    {
        return mIsConst;
    }

    TRUST TypeFunction::GetTrust()
    {
        return mTrust;
    }

    uint8_t TypeFunction::GetCallConv()
    {
        return mCallConv;
    }

    void TypeFunction::SetCallConv( uint8_t value )
    {
        mCallConv = value;
    }


    void TypeFunction::SetPure( bool value )
    {
        mIsPure = value;
    }

    void TypeFunction::SetNoThrow( bool value )
    {
        mIsNoThrow = value;
    }

    void TypeFunction::SetProperty( bool value )
    {
        mIsProperty = value;
    }

    void TypeFunction::SetConst( bool value )
    {
        mIsConst = value;
    }

    void TypeFunction::SetTrust( TRUST value )
    {
        mTrust = value;
    }


    //----------------------------------------------------------------------------
    //  TypeDelegate
    //----------------------------------------------------------------------------

    TypeDelegate::TypeDelegate( Type* ptrToFunction )
        :   TypeNext( Tdelegate, ptrToFunction )
    {
        ITypeNext*  ptrToFunc = ptrToFunction->AsTypeNext();
        _ASSERT( ptrToFunc != NULL );
        UNREFERENCED_PARAMETER( ptrToFunc );
    }

    RefPtr<Type> TypeDelegate::Copy()
    {
        RefPtr<Type>    type = new TypeDelegate( Next.Get() );
        return type;
    }

    bool TypeDelegate::IsDelegate()
    {
        return true;
    }

    uint32_t TypeDelegate::GetSize()
    {
        return Next->GetSize() * 2;
    }

    bool TypeDelegate::Equals( Type* other )
    {
        if ( other->Ty != Ty )
            return false;

        return Next->Equals( other->AsTypeNext()->GetNext() );
    }

    void TypeDelegate::_ToString( std::wstring& str )
    {
        ITypeNext* ptrToFunc = Next->AsTypeNext();
        ITypeFunction* funcType = ptrToFunc ? ptrToFunc->GetNext()->AsTypeFunction() : nullptr;

        if ( funcType == NULL )
        {
            str.append( L"delegate" );
            return;
        }

        FunctionToString( L"delegate", funcType, str );
    }


    //----------------------------------------------------------------------------
    //  TypeStruct
    //----------------------------------------------------------------------------

    TypeStruct::TypeStruct( Declaration* decl )
        :   Type( Tstruct ),
            mDecl( decl )
    {
    }

    RefPtr<Type> TypeStruct::Copy()
    {
        RefPtr<Type>    type = new TypeStruct( mDecl.Get() );
        return type;
    }

    uint32_t TypeStruct::GetSize()
    {
        // TODO: assert size and decl is a type
        uint32_t    size = 0;
        mDecl->GetSize( size );
        return size;
    }

    bool TypeStruct::CanRefMember()
    {
        return true;
    }

    RefPtr<Declaration> TypeStruct::GetDeclaration()
    {
        return mDecl;
    }

    RefPtr<Declaration> TypeStruct::FindObject( const wchar_t* name )
    {
        HRESULT hr = S_OK;
        RefPtr<Declaration> childDecl;

        if ( wcscmp( name, L"__vfptr" ) == 0 )
            if ( GetUdtKind() == Udt_Class )
            {
                if( mDecl->GetVTableShape( childDecl.Ref() ) )
                    return childDecl;
            }

        hr = mDecl->FindObject( name, childDecl.Ref() );
        if ( FAILED( hr ) )
            return NULL;

        return childDecl;
    }

    UdtKind TypeStruct::GetUdtKind()
    {
        UdtKind kind = Udt_Struct;

        mDecl->GetUdtKind( kind );

        return kind;
    }

    bool TypeStruct::GetBaseClassOffset( Type* baseClass, int& offset )
    {
        _ASSERT( baseClass != NULL );
        if ( (baseClass == NULL) || (baseClass->AsTypeStruct() == NULL) )
            return false;

        return mDecl->GetBaseClassOffset( baseClass->GetDeclaration(), offset );
    }

    bool TypeStruct::IsPOD()
	{
		RefPtr<IEnumDeclarationMembers> members;
		if ( !mDecl->EnumMembers( members.Ref() ) )
			return false; // undefined?

		uint32_t cnt = members->GetCount();
		for ( int i = 0; i < cnt; i++ )
		{
			RefPtr<Declaration> decl;
			if ( !members->Next( decl.Ref() ) )
				return false;
			if ( decl->IsStaticField() )
				continue;
			if ( decl->IsBaseClass() )
				return false;
			if( decl->IsFunction() )
				if( wcscmp( decl->GetName(), L"__dtor" ) == 0 )
					return false;
			if( decl->IsField() )
			{
				if ( wcscmp( decl->GetName(), L"this" ) == 0 )
					return false;
				RefPtr<Type> ftype;
				if ( decl->GetType( ftype.Ref() ) )
					if( auto s = ftype->AsTypeStruct() )
						if( !s->IsPOD() )
							return false;
			}
		}
		return true;
	}

    bool TypeStruct::Equals( Type* other )
    {
        if ( Ty != other->Ty )
            return false;

        return wcscmp( mDecl->GetName(), other->GetDeclaration()->GetName() ) == 0;
    }

    void TypeStruct::_ToString( std::wstring& str )
    {
        str.append( mDecl->GetName() );
    }

    ITypeStruct* TypeStruct::AsTypeStruct()
    {
        return this;
    }

    const wchar_t* TypeStruct::GetName()
    {
        return mDecl->GetName();
    }

    //----------------------------------------------------------------------------
    //  TypeEnum
    //----------------------------------------------------------------------------

    TypeEnum::TypeEnum( Declaration* decl )
        :   Type( Tenum ),
            mDecl( decl ),
            mBackingTy( Tint32 )
    {
        _ASSERT( decl->IsType() );
        bool    found = decl->GetBackingTy( mBackingTy );
        UNREFERENCED_PARAMETER( found );
        _ASSERT( found );
    }

    ENUMTY TypeEnum::GetBackingTy()
    {
        return mBackingTy;
    }

    RefPtr<Type> TypeEnum::Copy()
    {
        RefPtr<TypeEnum>    type = new TypeEnum( mDecl );
        type->mBackingTy = mBackingTy;
        return type.Get();
    }

    uint32_t TypeEnum::GetSize()
    {
        uint32_t    size = 0;
        mDecl->GetSize( size );
        return size;
    }

    bool TypeEnum::IsIntegral()
    {
        return true;
    }

    bool TypeEnum::IsSigned()
    {
        return TypeBasic::IsSigned( mBackingTy );
    }

    bool TypeEnum::CanRefMember()
    {
        return true;
    }

    RefPtr<Declaration> TypeEnum::GetDeclaration()
    {
        return mDecl;
    }

    RefPtr<Declaration> TypeEnum::FindObject( const wchar_t* name )
    {
        HRESULT hr = S_OK;
        RefPtr<Declaration> childDecl;

        hr = mDecl->FindObject( name, childDecl.Ref() );
        if ( FAILED( hr ) )
            return NULL;

        return childDecl;
    }

    RefPtr<Declaration> TypeEnum::FindObjectByValue( uint64_t intVal )
    {
        HRESULT hr = S_OK;
        RefPtr<Declaration> childDecl;

        hr = mDecl->FindObjectByValue( intVal, childDecl.Ref() );
        if ( FAILED( hr ) )
            return NULL;

        return childDecl;
    }

    bool TypeEnum::Equals( Type* other )
    {
        if ( Ty != other->Ty )
            return false;

        return wcscmp( mDecl->GetName(), other->GetDeclaration()->GetName() ) == 0;
    }

    void TypeEnum::_ToString( std::wstring& str )
    {
        str.append( mDecl->GetName() );
    }

    ITypeEnum* TypeEnum::AsTypeEnum()
    {
        return this;
    }


    //------------------------------------------------------------------------
    //  Properties
    //------------------------------------------------------------------------

    StdProperty* Type::FindProperty( const wchar_t* name )
    {
        return FindBaseProperty( name );
    }

    StdProperty* TypeBasic::FindProperty( const wchar_t* name )
    {
        StdProperty*    prop = NULL;

        if ( IsIntegral() )
        {
            prop = FindIntProperty( name );
            if ( prop != NULL )
                return prop;
        }

        if ( IsFloatingPoint() )
        {
            prop = FindFloatProperty( name );
            if ( prop != NULL )
                return prop;
        }

        return Type::FindProperty( name );
    }

    StdProperty* TypeDArray::FindProperty( const wchar_t* name )
    {
        StdProperty*    prop = FindDArrayProperty( name );
        if ( prop != NULL )
            return prop;

        return Type::FindProperty( name );
    }

    StdProperty* TypeSArray::FindProperty( const wchar_t* name )
    {
        StdProperty*    prop = FindSArrayProperty( name );
        if ( prop != NULL )
            return prop;

        return Type::FindProperty( name );
    }

    StdProperty* TypeTuple::FindProperty( const wchar_t* name )
    {
        StdProperty*    prop = FindTupleProperty( name );
        if ( prop != NULL )
            return prop;

        return Type::FindProperty( name );
    }

    StdProperty* TypeDelegate::FindProperty( const wchar_t* name )
    {
        StdProperty*    prop = FindDelegateProperty( name );
        if ( prop != NULL )
            return prop;

        return Type::FindProperty( name );
    }

    //----------------------------------------------------------------------------
    //  TypeTypedef
    //----------------------------------------------------------------------------

    TypeTypedef::TypeTypedef( const wchar_t* name, Type* aliasedType )
        :   Type( Ttypedef ),
            mName( name ),
            mAliased( aliasedType )
    {
        Mod = aliasedType->Mod;
    }

    RefPtr<Type> TypeTypedef::Copy()
    {
        RefPtr<Type>    type = new TypeTypedef( mName.c_str(), mAliased );
        return type;
    }

    bool TypeTypedef::IsBasic()
    {
        return mAliased->IsBasic();
    }

    bool TypeTypedef::IsPointer()
    {
        return mAliased->IsPointer();
    }

    bool TypeTypedef::IsReference()
    {
        return mAliased->IsReference();
    }

    bool TypeTypedef::IsSArray()
    {
        return mAliased->IsSArray();
    }

    bool TypeTypedef::IsDArray()
    {
        return mAliased->IsDArray();
    }

    bool TypeTypedef::IsScalar()
    {
        return mAliased->IsSArray();
    }

    bool TypeTypedef::IsBool()
    {
        return mAliased->IsBool();
    }

    bool TypeTypedef::IsChar()
    {
        return mAliased->IsChar();
    }

    bool TypeTypedef::IsIntegral()
    {
        return mAliased->IsIntegral();
    }

    bool TypeTypedef::IsFloatingPoint()
    {
        return mAliased->IsFloatingPoint();
    }

    bool TypeTypedef::IsSigned()
    {
        return mAliased->IsSigned();
    }

    bool TypeTypedef::IsReal()
    {
        return mAliased->IsReal();
    }

    bool TypeTypedef::IsImaginary()
    {
        return mAliased->IsImaginary();
    }

    bool TypeTypedef::IsComplex()
    {
        return mAliased->IsComplex();
    }

    uint32_t TypeTypedef::GetSize()
    {
        return mAliased->GetSize();
    }

    bool TypeTypedef::CanRefMember()
    {
        return mAliased->CanRefMember();
    }

    RefPtr<Declaration> TypeTypedef::GetDeclaration()
    {
        return mAliased->GetDeclaration();
    }

    ENUMTY TypeTypedef::GetBackingTy()
    {
        return mAliased->GetBackingTy();
    }

    bool TypeTypedef::Equals( Type* other )
    {
        if ( other->Ty != Ty )
            return false;

        return wcscmp( ((TypeTypedef*) other)->mName.c_str(), mName.c_str() ) == 0;
    }

    StdProperty* TypeTypedef::FindProperty( const wchar_t* name )
    {
        return mAliased->FindProperty( name );
    }

    void TypeTypedef::_ToString( std::wstring& str )
    {
        str.append( mName );
    }

    ITypeNext* TypeTypedef::AsTypeNext()
    {
        return mAliased->AsTypeNext();
    }

    ITypeStruct* TypeTypedef::AsTypeStruct()
    {
        return mAliased->AsTypeStruct();
    }

    ITypeEnum* TypeTypedef::AsTypeEnum()
    {
        return mAliased->AsTypeEnum();
    }

    ITypeFunction* TypeTypedef::AsTypeFunction()
    {
        return mAliased->AsTypeFunction();
    }

    ITypeSArray* TypeTypedef::AsTypeSArray()
    {
        return mAliased->AsTypeSArray();
    }

    ITypeDArray* TypeTypedef::AsTypeDArray()
    {
        return mAliased->AsTypeDArray();
    }

    ITypeAArray* TypeTypedef::AsTypeAArray()
    {
        return mAliased->AsTypeAArray();
    }

    Type* TypeTypedef::Unaliased()
    {
        return mAliased->Unaliased();
    }

}
