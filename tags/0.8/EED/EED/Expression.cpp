/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Expression.h"
#include "Type.h"
#include "TypeUnresolved.h"
#include "Declaration.h"
#include "Property.h"
#include "SharedString.h"
#include "NameTable.h"


namespace MagoEE
{
    Expression::Expression()
        :   Kind( DataKind_None )
    {
    }

    ObjectKind Expression::GetObjectKind()
    {
        return ObjectKind_Expression;
    }


    ObjectKind ExpressionList::GetObjectKind()
    {
        return ObjectKind_ExpressionList;
    }

    bool Expression::TrySetType( Type* type )
    {
        UNREFERENCED_PARAMETER( type );
        return false;
    }

    NamingExpression* Expression::AsNamingExpression()
    {
        return NULL;
    }

    void Expression::ClearEvalData()
    {
        _Type = NULL;
        Kind = DataKind_None;
    }

    HRESULT Expression::MakeName( uint32_t capacity, RefPtr<SharedString>& namePath )
    {
        UNREFERENCED_PARAMETER( capacity );
        UNREFERENCED_PARAMETER( namePath );
        return E_MAGOEE_SYMBOL_NOT_FOUND;
    }


    //----------------------------------------------------------------------------
    //  BinExpr
    //----------------------------------------------------------------------------

    BinExpr::BinExpr( Expression* left, Expression* right )
        :   Left( left ),
            Right( right )
    {
    }


    //----------------------------------------------------------------------------
    //  CommaExpr
    //----------------------------------------------------------------------------

    CommaExpr::CommaExpr( Expression* left, Expression* right )
        :   BinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  AssignExpr
    //----------------------------------------------------------------------------

    AssignExpr::AssignExpr( Expression* left, Expression* right )
        :   BinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  CombinedAssignExpr
    //----------------------------------------------------------------------------

    CombinedAssignExpr::CombinedAssignExpr( CombinableBinExpr* child, bool postOp )
        :   Child( child ),
            IsPostOp( postOp )
    {
    }


    //----------------------------------------------------------------------------
    //  ConditionalExpr
    //----------------------------------------------------------------------------

    ConditionalExpr::ConditionalExpr( Expression* predicate, Expression* trueExpr, Expression* falseExpr )
        :   PredicateExpr( predicate ),
            TrueExpr( trueExpr ),
            FalseExpr( falseExpr )
    {
    }


    //----------------------------------------------------------------------------
    //  OrOrExpr
    //----------------------------------------------------------------------------

    OrOrExpr::OrOrExpr( Expression* left, Expression* right )
        :   BinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  AndAndExpr
    //----------------------------------------------------------------------------

    AndAndExpr::AndAndExpr( Expression* left, Expression* right )
        :   BinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  OrExpr
    //----------------------------------------------------------------------------

    OrExpr::OrExpr( Expression* left, Expression* right )
        :   ArithmeticBinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  XorExpr
    //----------------------------------------------------------------------------

    XorExpr::XorExpr( Expression* left, Expression* right )
        :   ArithmeticBinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  AndExpr
    //----------------------------------------------------------------------------

    AndExpr::AndExpr( Expression* left, Expression* right )
        :   ArithmeticBinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  CmpExpr
    //----------------------------------------------------------------------------

    CmpExpr::CmpExpr( TOK opCode, Expression* left, Expression* right )
        :   BinExpr( left, right ),
            OpCode( opCode )
    {
    }


    //----------------------------------------------------------------------------
    //  CompareExpr
    //----------------------------------------------------------------------------

    CompareExpr::CompareExpr( TOK opCode, Expression* left, Expression* right )
        :   BinExpr( left, right ),
            OpCode( opCode )
    {
    }


    //----------------------------------------------------------------------------
    //  EqualExpr
    //----------------------------------------------------------------------------

    EqualExpr::EqualExpr( TOK opCode, Expression* left, Expression* right )
        :   CompareExpr( opCode, left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  IdentityExpr
    //----------------------------------------------------------------------------

    IdentityExpr::IdentityExpr( TOK opCode, Expression* left, Expression* right )
        :   CompareExpr( opCode, left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  RelExpr
    //----------------------------------------------------------------------------

    RelExpr::RelExpr( TOK opCode, Expression* left, Expression* right )
        :   CompareExpr( opCode, left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  InExpr
    //----------------------------------------------------------------------------

    InExpr::InExpr( Expression* left, Expression* right )
        :   BinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  ShiftBinExpr
    //----------------------------------------------------------------------------

    ShiftBinExpr::ShiftBinExpr( Expression* left, Expression* right )
        :   CombinableBinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  ShiftLeftExpr
    //----------------------------------------------------------------------------

    ShiftLeftExpr::ShiftLeftExpr( Expression* left, Expression* right )
        :   ShiftBinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  ShiftRightExpr
    //----------------------------------------------------------------------------

    ShiftRightExpr::ShiftRightExpr( Expression* left, Expression* right )
        :   ShiftBinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  UShiftRightExpr
    //----------------------------------------------------------------------------

    UShiftRightExpr::UShiftRightExpr( Expression* left, Expression* right )
        :   ShiftBinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  AddExpr
    //----------------------------------------------------------------------------

    AddExpr::AddExpr( Expression* left, Expression* right )
        :   ArithmeticBinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  MinExpr
    //----------------------------------------------------------------------------

    MinExpr::MinExpr( Expression* left, Expression* right )
        :   ArithmeticBinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  CatExpr
    //----------------------------------------------------------------------------

    CatExpr::CatExpr( Expression* left, Expression* right )
        :   CombinableBinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  MulExpr
    //----------------------------------------------------------------------------

    MulExpr::MulExpr( Expression* left, Expression* right )
        :   ArithmeticBinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  DivExpr
    //----------------------------------------------------------------------------

    DivExpr::DivExpr( Expression* left, Expression* right )
        :   ArithmeticBinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  ModExpr
    //----------------------------------------------------------------------------

    ModExpr::ModExpr( Expression* left, Expression* right )
        :   ArithmeticBinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  PowExpr
    //----------------------------------------------------------------------------

    PowExpr::PowExpr( Expression* left, Expression* right )
        :   ArithmeticBinExpr( left, right )
    {
    }


    //----------------------------------------------------------------------------
    //  AddressOfExpr
    //----------------------------------------------------------------------------

    AddressOfExpr::AddressOfExpr( Expression* child )
        :   Child( child )
    {
    }

    //----------------------------------------------------------------------------
    //  PointerExpr
    //----------------------------------------------------------------------------
    
    PointerExpr::PointerExpr( Expression* child )
        :   Child( child )
    {
    }


    //----------------------------------------------------------------------------
    //  NegateExpr
    //----------------------------------------------------------------------------
    
    NegateExpr::NegateExpr( Expression* child )
        :   Child( child )
    {
    }


    //----------------------------------------------------------------------------
    //  UnaryAddExpr
    //----------------------------------------------------------------------------
    
    UnaryAddExpr::UnaryAddExpr( Expression* child )
        :   Child( child )
    {
    }


    //----------------------------------------------------------------------------
    //  NotExpr
    //----------------------------------------------------------------------------
    
    NotExpr::NotExpr( Expression* child )
        :   Child( child )
    {
    }


    //----------------------------------------------------------------------------
    //  BitNotExpr
    //----------------------------------------------------------------------------
    
    BitNotExpr::BitNotExpr( Expression* child )
        :   Child( child )
    {
    }


    //----------------------------------------------------------------------------
    //  NewExpr
    //----------------------------------------------------------------------------

    NewExpr::NewExpr()
    {
    }


    //----------------------------------------------------------------------------
    //  DeleteExpr
    //----------------------------------------------------------------------------

    DeleteExpr::DeleteExpr( Expression* child )
        :   Child( child )
    {
    }


    //----------------------------------------------------------------------------
    //  CastExpr
    //----------------------------------------------------------------------------

    CastExpr::CastExpr( Expression* child, MOD flags )
        :   Child( child ),
            FlagsTo( flags )
    {
    }

    CastExpr::CastExpr( Expression* child, Type* type )
        :   Child( child ),
            _TypeTo( type ),
            FlagsTo( MODnone )
    {
    }


    //----------------------------------------------------------------------------
    //  DotExpr
    //----------------------------------------------------------------------------

    DotExpr::DotExpr( Expression* child, Utf16String* id )
        :   Child( child ),
            Id( id ),
            Property( NULL ),
            mNamePathLen( 0 )
    {
    }

    HRESULT DotExpr::MakeName( uint32_t capacity, RefPtr<SharedString>& namePath )
    {
        HRESULT hr = S_OK;

        if ( mNamePath == NULL )
        {
            hr = Child->MakeName( capacity + Id->Length + 1, mNamePath );
            if ( FAILED( hr ) )
                return hr;

            mNamePath->Append( L"." );
            mNamePath->Append( Id->Str );

            mNamePathLen = mNamePath->GetLength();
        }

        namePath = mNamePath;

        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  DotTemplateInstanceExpr
    //----------------------------------------------------------------------------

    DotTemplateInstanceExpr::DotTemplateInstanceExpr( Expression* child, TemplateInstancePart* instance )
        :   Child( child ),
            Instance( instance ),
            mNamePathLen( 0 )
    {
    }

    HRESULT DotTemplateInstanceExpr::MakeName( uint32_t capacity, RefPtr<SharedString>& namePath )
    {
        HRESULT hr = S_OK;

        if ( mNamePath == NULL )
        {
            const uint32_t  ourLen = Instance->Id->Length + Instance->ArgumentString->Length;

            hr = Child->MakeName( capacity + ourLen + 1, mNamePath );
            if ( FAILED( hr ) )
                return hr;

            mNamePath->Append( L"." );
            mNamePath->Append( Instance->Id->Str );
            mNamePath->Append( Instance->ArgumentString->Str );

            mNamePathLen = mNamePath->GetLength();
        }

        namePath = mNamePath;

        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  CallExpr
    //----------------------------------------------------------------------------

    CallExpr::CallExpr( Expression* child, ExpressionList* args )
        :   Child( child ),
            Args( args )
    {
    }


    //----------------------------------------------------------------------------
    //  PostExpr
    //----------------------------------------------------------------------------

    PostExpr::PostExpr( Expression* child, TOK op )
        :   Child( child ),
            Operator( op )
    {
    }


    //----------------------------------------------------------------------------
    //  IndexExpr
    //----------------------------------------------------------------------------

    IndexExpr::IndexExpr( Expression* child, ExpressionList* args )
        :   Child( child ),
            Args( args )
    {
    }


    //----------------------------------------------------------------------------
    //  SliceExpr
    //----------------------------------------------------------------------------

    SliceExpr::SliceExpr( Expression* child, Expression* from, Expression* to )
        :   Child( child ),
            From( from ),
            To( to )
    {
    }


    //----------------------------------------------------------------------------
    //  IdExpr
    //----------------------------------------------------------------------------

    IdExpr::IdExpr( Utf16String* id )
        :   Id( id )
    {
    }

    HRESULT IdExpr::MakeName( uint32_t capacity, RefPtr<SharedString>& namePath )
    {
        if ( mNamePath == NULL )
        {
            if ( !SharedString::Make( capacity + Id->Length, mNamePath ) )
                return E_OUTOFMEMORY;

            mNamePath->Append( Id->Str );
        }

        namePath = mNamePath;

        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  ScopeExpr
    //----------------------------------------------------------------------------

    ScopeExpr::ScopeExpr( TemplateInstancePart* instance )
        :   Instance( instance )
    {
    }

    HRESULT ScopeExpr::MakeName( uint32_t capacity, RefPtr<SharedString>& namePath )
    {
        if ( mNamePath == NULL )
        {
            const uint32_t  SelfCapacity = 
                Instance->Id->Length + Instance->ArgumentString->Length;

            if ( !SharedString::Make( capacity + SelfCapacity, mNamePath ) )
                return E_OUTOFMEMORY;

            mNamePath->Append( Instance->Id->Str );
            mNamePath->Append( Instance->ArgumentString->Str );
        }

        namePath = mNamePath;

        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  ThisExpr
    //----------------------------------------------------------------------------

    ThisExpr::ThisExpr()
    {
    }


    //----------------------------------------------------------------------------
    //  SuperExpr
    //----------------------------------------------------------------------------

    SuperExpr::SuperExpr()
    {
    }


    //----------------------------------------------------------------------------
    //  DollarExpr
    //----------------------------------------------------------------------------

    DollarExpr::DollarExpr()
    {
    }


    //----------------------------------------------------------------------------
    //  TypeidExpr
    //----------------------------------------------------------------------------

    TypeidExpr::TypeidExpr( Object* child )
        :   Child( child )
    {
    }


    //----------------------------------------------------------------------------
    //  IsExpr
    //----------------------------------------------------------------------------

    IsExpr::IsExpr()
    {
    }


    //----------------------------------------------------------------------------
    //  TraitsExpr
    //----------------------------------------------------------------------------

    TraitsExpr::TraitsExpr()
    {
    }


    //----------------------------------------------------------------------------
    //  IntExpr
    //----------------------------------------------------------------------------

    IntExpr::IntExpr( uint64_t value, Type* type )
        :   Value( value )
    {
        _Type = type;
    }


    //----------------------------------------------------------------------------
    //  RealExpr
    //----------------------------------------------------------------------------

    RealExpr::RealExpr( Real10 value, Type* type )
        :   Value( value )
    {
        _Type = type;
    }


    //----------------------------------------------------------------------------
    //  StringExpr
    //----------------------------------------------------------------------------

    StringExpr::StringExpr( String* value, bool isSpecificType )
        :   Value( value ),
            IsSpecificType( isSpecificType ),
            mUntypedStr( NULL )
    {
        _ASSERT( value != NULL );

        if ( !isSpecificType )
        {
            _ASSERT( value->Kind == StringKind_Byte );
            mUntypedStr = (ByteString*) value;
        }
    }


    //----------------------------------------------------------------------------
    //  ArrayLiteralExpr
    //----------------------------------------------------------------------------

    ArrayLiteralExpr::ArrayLiteralExpr( ExpressionList* values )
        :   Values( values )
    {
    }


    //----------------------------------------------------------------------------
    //  AssocArrayLiteralExpr
    //----------------------------------------------------------------------------

    AssocArrayLiteralExpr::AssocArrayLiteralExpr( ExpressionList* keys, ExpressionList* values )
        :   Keys( keys ),
            Values( values )
    {
    }


    //----------------------------------------------------------------------------
    //  TypeExpr
    //----------------------------------------------------------------------------

    TypeExpr::TypeExpr( Type* type )
        :   UnresolvedType( type )
    {
    }
}
