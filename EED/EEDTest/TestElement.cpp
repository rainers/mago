/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "TestElement.h"
#include "DataValue.h"
#include <math.h>
#include <limits>
#include "DataElement.h"
#include "AppSettings.h"
#include "ErrorStr.h"
#include "DataEnv.h"

using namespace std;
using MagoEE::Type;
using MagoEE::Declaration;
using MagoEE::ITypeEnv;


//----------------------------------------------------------------------------
//  TestFactory
//----------------------------------------------------------------------------

Element* TestFactory::NewElement( const wchar_t* name )
{
    auto_ptr<Element>   elem;

    if ( _wcsicmp( name, L"intvalue" ) == 0 )
    {
        elem.reset( new IntTestElement() );
    }
    else if ( _wcsicmp( name, L"realvalue" ) == 0 )
    {
        elem.reset( new RealTestElement() );
    }
    else if ( _wcsicmp( name, L"null" ) == 0 )
    {
        elem.reset( new NullTestElement() );
    }
    else if ( _wcsicmp( name, L"darray" ) == 0 )
    {
        elem.reset( new DArrayTestElement() );
    }
    else if ( _wcsicmp( name, L"add" ) == 0 )
    {
        elem.reset( new AddTestElement() );
    }
    else if ( _wcsicmp( name, L"sub" ) == 0 )
    {
        elem.reset( new SubTestElement() );
    }
    else if ( _wcsicmp( name, L"mul" ) == 0 )
    {
        elem.reset( new MulTestElement() );
    }
    else if ( _wcsicmp( name, L"div" ) == 0 )
    {
        elem.reset( new DivTestElement() );
    }
    else if ( _wcsicmp( name, L"mod" ) == 0 )
    {
        elem.reset( new ModTestElement() );
    }
    else if ( _wcsicmp( name, L"pow" ) == 0 )
    {
        elem.reset( new PowTestElement() );
    }
    else if ( _wcsicmp( name, L"negate" ) == 0 )
    {
        elem.reset( new NegateTestElement() );
    }
    else if ( _wcsicmp( name, L"unaryadd" ) == 0 )
    {
        elem.reset( new UnaryAddTestElement() );
    }
    else if ( _wcsicmp( name, L"bitand" ) == 0 )
    {
        elem.reset( new AndTestElement() );
    }
    else if ( _wcsicmp( name, L"bitor" ) == 0 )
    {
        elem.reset( new OrTestElement() );
    }
    else if ( _wcsicmp( name, L"bitxor" ) == 0 )
    {
        elem.reset( new XorTestElement() );
    }
    else if ( _wcsicmp( name, L"bitnot" ) == 0 )
    {
        elem.reset( new BitNotTestElement() );
    }
    else if ( _wcsicmp( name, L"shiftleft" ) == 0 )
    {
        elem.reset( new ShiftLeftTestElement() );
    }
    else if ( _wcsicmp( name, L"shiftright" ) == 0 )
    {
        elem.reset( new ShiftRightTestElement() );
    }
    else if ( _wcsicmp( name, L"ushiftright" ) == 0 )
    {
        elem.reset( new UShiftRightTestElement() );
    }
    else if ( _wcsicmp( name, L"and" ) == 0 )
    {
        elem.reset( new AndAndTestElement() );
    }
    else if ( _wcsicmp( name, L"or" ) == 0 )
    {
        elem.reset( new OrOrTestElement() );
    }
    else if ( _wcsicmp( name, L"not" ) == 0 )
    {
        elem.reset( new NotTestElement() );
    }
    else if ( _wcsicmp( name, L"cmp" ) == 0 )
    {
        elem.reset( new CompareTestElement() );
    }
    else if ( _wcsicmp( name, L"ptradd" ) == 0 )
    {
        elem.reset( new PtrAddTestElement() );
    }
    else if ( _wcsicmp( name, L"ptrsub" ) == 0 )
    {
        elem.reset( new PtrSubTestElement() );
    }
    else if ( _wcsicmp( name, L"ptrdiff" ) == 0 )
    {
        elem.reset( new PtrDiffTestElement() );
    }
    else if ( _wcsicmp( name, L"group" ) == 0 )
    {
        elem.reset( new GroupTestElement() );
    }
    else if ( _wcsicmp( name, L"cast" ) == 0 )
    {
        elem.reset( new CastTestElement() );
    }
    else if ( _wcsicmp( name, L"basictype" ) == 0 )
    {
        elem.reset( new BasicTypeTestElement() );
    }
    else if ( _wcsicmp( name, L"reftype" ) == 0 )
    {
        elem.reset( new RefTypeTestElement() );
    }
    else if ( _wcsicmp( name, L"pointertype" ) == 0 )
    {
        elem.reset( new PointerTypeTestElement() );
    }
    else if ( _wcsicmp( name, L"sarraytype" ) == 0 )
    {
        elem.reset( new SArrayTypeTestElement() );
    }
    else if ( _wcsicmp( name, L"darraytype" ) == 0 )
    {
        elem.reset( new DArrayTypeTestElement() );
    }
    else if ( _wcsicmp( name, L"aarraytype" ) == 0 )
    {
        elem.reset( new AArrayTypeTestElement() );
    }
    else if ( _wcsicmp( name, L"test" ) == 0 )
    {
        elem.reset( new TestTestElement() );
    }
    else if ( _wcsicmp( name, L"verify" ) == 0 )
    {
        elem.reset( new VerifyTestElement() );
    }
    else if ( _wcsicmp( name, L"typedvalue" ) == 0 )
    {
        elem.reset( new TypedValueTestElement() );
    }
    else if ( _wcsicmp( name, L"memoryvalue" ) == 0 )
    {
        elem.reset( new MemoryValueTestElement() );
    }
    else if ( _wcsicmp( name, L"id" ) == 0 )
    {
        elem.reset( new IdTestElement() );
    }
    else if ( _wcsicmp( name, L"member" ) == 0 )
    {
        elem.reset( new MemberTestElement() );
    }
    else if ( _wcsicmp( name, L"stdproperty" ) == 0 )
    {
        elem.reset( new StdPropertyTestElement() );
    }
    else if ( _wcsicmp( name, L"index" ) == 0 )
    {
        elem.reset( new IndexTestElement() );
    }
    else if ( _wcsicmp( name, L"slice" ) == 0 )
    {
        elem.reset( new SliceTestElement() );
    }
    else if ( _wcsicmp( name, L"address" ) == 0 )
    {
        elem.reset( new AddressTestElement() );
    }
    else if ( _wcsicmp( name, L"pointer" ) == 0 )
    {
        elem.reset( new PointerTestElement() );
    }
    else if ( _wcsicmp( name, L"dollar" ) == 0 )
    {
        elem.reset( new DollarTestElement() );
    }
    else if ( _wcsicmp( name, L"assign" ) == 0 )
    {
        elem.reset( new AssignTestElement() );
    }
    else if ( _wcsicmp( name, L"preassign" ) == 0 )
    {
        elem.reset( new PreAssignTestElement() );
    }
    else if ( _wcsicmp( name, L"postassign" ) == 0 )
    {
        elem.reset( new PostAssignTestElement() );
    }
    else if ( _wcsicmp( name, L"combinedassign" ) == 0 )
    {
        elem.reset( new CombinedAssignTestElement() );
    }

    return elem.release();
}


//----------------------------------------------------------------------------
//  TestElement
//----------------------------------------------------------------------------

Type* TestElement::GetType()
{
    return mType.Get();
}

RefPtr<MagoEE::Declaration> TestElement::GetDeclaration()
{
    return NULL;
}

bool TestElement::TrySetType( MagoEE::Type* type )
{
    return false;
}

void TestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
}

std::shared_ptr<DataObj> TestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    throw L"Can't evaluate.";
}

void TestElement::ToDText( std::wostringstream& stream )
{
}

void TestElement::PrintElement()
{
}


//----------------------------------------------------------------------------
//  IntTestElement
//----------------------------------------------------------------------------

IntTestElement::IntTestElement()
:   mVal( 0 )
{
}

void IntTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    // TODO: use the D Scanner for this
    if ( wcscmp( mText.c_str(), L"true" ) == 0 )
    {
        mVal = 1;
    }
    else if ( wcscmp( mText.c_str(), L"false" ) == 0 )
    {
        mVal = 0;
    }
    else if ( (mText.size() > 2) && (mText[0] == L'\'') )
    {
        mVal = mText.c_str()[1];
    }
    else if ( (mText.size() > 2) && (mText[0] == L'0') && (towlower( mText[1] ) == L'x') )
    {
        mVal = _wcstoui64( mText.c_str() + 2, NULL, 16 );
    }
    else
    {
        mVal = _wcstoui64( mText.c_str(), NULL, 10 );
    }
    mType = typeEnv->GetType( MagoEE::Tint32 );
}

std::shared_ptr<DataObj> IntTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<RValueObj>   val( new RValueObj() );

    val->SetType( mType );
    val->Value.UInt64Value = mVal;

    return val;
}

void IntTestElement::ToDText( std::wostringstream& stream )
{
    stream << mText;
}

void IntTestElement::AddChild( Element* elem )
{
    throw L"IntValue can't have children.";
}

void IntTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"value" ) == 0 )
    {
        mText = value;
    }
}

void IntTestElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}


//----------------------------------------------------------------------------
//  RealTestElement
//----------------------------------------------------------------------------

RealTestElement::RealTestElement()
{
    mVal.Zero();
}

void RealTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    // TODO: use the D Scanner for this
#if 0
    double  d = wcstod( mText.c_str(), NULL );
    mVal.FromDouble( d );
    mType = typeEnv->GetType( MagoEE::Tfloat80 );
#else
    errno_t err = Real10::Parse( mText.c_str(), mVal );
    if ( err != 0 )
        throw L"Couldn't parse real value.";

    if ( mText[ mText.length() - 1 ] == L'i' )
        mType = typeEnv->GetType( MagoEE::Timaginary80 );
    else
        mType = typeEnv->GetType( MagoEE::Tfloat80 );
#endif
}

std::shared_ptr<DataObj> RealTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<RValueObj>   val( new RValueObj() );

    val->SetType( mType );
    val->Value.Float80Value = mVal;

    return val;
}

void RealTestElement::ToDText( std::wostringstream& stream )
{
    bool    isImaginary = false;
    bool    isNegative = false;
    const wchar_t*  str = mText.c_str();

    if ( mText.size() >= 1 )
    {
        if ( mText[ mText.size() - 1 ] == L'i' )
            isImaginary = true;
        if ( mText[0] == L'-' )
        {
            isNegative = true;
            str++;              // skip dash when testing value name below
        }
    }

    if ( _wcsnicmp( str, L"inf", 3 ) == 0 )
    {
        if ( isNegative )
            stream << L"-";
        if ( isImaginary )
            stream << L"ireal.";
        else
            stream << L"real.";
        stream << L"infinity";
    }
    else if ( _wcsnicmp( str, L"nan", 3 ) == 0 )
    {
        if ( isNegative )
            stream << L"-";
        if ( isImaginary )
            stream << L"ireal.";
        else
            stream << L"real.";
        stream << L"nan";
    }
    else
    {
        if ( isImaginary )
            stream.write( mText.c_str(), (streamsize) mText.size() - 1 );
        else
            stream << mText;

        if ( !mVal.FitsInDouble() )
            stream << L'L';
        if ( isImaginary )
            stream << L'i';
    }
}

void RealTestElement::AddChild( Element* elem )
{
    throw L"RealValue can't have children.";
}

void RealTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"value" ) == 0 )
    {
        mText = value;
    }
}

void RealTestElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}


//----------------------------------------------------------------------------
//  NullTestElement
//----------------------------------------------------------------------------

bool NullTestElement::TrySetType( MagoEE::Type* type )
{
    // TODO: A-array, delegate
    if ( type->IsDArray() )
    {
        mType = type;
        return true;
    }

    return false;
}

void NullTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    mType = typeEnv->GetVoidPointerType();
}

std::shared_ptr<DataObj> NullTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<RValueObj>   val( new RValueObj() );

    val->SetType( mType );

    // TODO: A-array, delegate
    if ( mType->IsPointer() )
    {
        val->Value.Addr = 0;
    }
    else if ( mType->IsDArray() )
    {
        val->Value.Array.Addr = 0;
        val->Value.Array.Length = 0;
    }

    return val;
}

void NullTestElement::ToDText( std::wostringstream& stream )
{
    stream << L"null";
}

void NullTestElement::AddChild( Element* elem )
{
    throw L"Null can't have children.";
}

void NullTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
}

void NullTestElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}


//----------------------------------------------------------------------------
//  DArrayTestElement
//----------------------------------------------------------------------------

DArrayTestElement::DArrayTestElement()
{
    memset( &mVal, 0, sizeof mVal );
}

void DArrayTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( mChildren.size() != 1 )
        throw L"DArray value needs a element type child.";

    RefPtr<Type>    elemType = mChildren[0]->GetType();

    HRESULT hr = typeEnv->NewDArray( elemType, mType.Ref() );
    if ( FAILED( hr ) )
        throw L"Failed making new D-Array type.";
}

std::shared_ptr<DataObj> DArrayTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<RValueObj>   val( new RValueObj() );

    val->SetType( mType );
    val->Value.Array = mVal;

    return val;
}

void DArrayTestElement::ToDText( std::wostringstream& stream )
{
    // actually we can do something like (cast(T*) addr)[0..length], but we don't need to
}

void DArrayTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"address" ) == 0 )
    {
        if ( (value[0] == L'0') && (value[1] == L'x') )
            mVal.Addr = wcstoul( value + 2, NULL, 16 );
        else
            mVal.Addr = wcstoul( value, NULL, 10 );
    }
    else if ( _wcsicmp( name, L"length" ) == 0 )
    {
        if ( (value[0] == L'0') && (value[1] == L'x') )
            mVal.Length = wcstoul( value + 2, NULL, 16 );
        else
            mVal.Length = wcstoul( value, NULL, 10 );
    }

    ContainerTestElement::SetAttribute( name, value );
}


//----------------------------------------------------------------------------
//  ContainerTestElement
//----------------------------------------------------------------------------

void ContainerTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    for ( List::iterator it = mChildren.begin();
        it != mChildren.end();
        it++ )
    {
        (*it)->Bind( typeEnv, scope, dataEnv );
    }
}

void ContainerTestElement::AddChild( Element* elem )
{
    TestElement*    testElem = dynamic_cast<TestElement*>( elem );

    if ( testElem == NULL )
        throw L"Can only add TestElements to a TestElement.";

    mChildren.push_back( testElem );
}

void ContainerTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
}

void ContainerTestElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}


//----------------------------------------------------------------------------
//  CombinableTestElement
//----------------------------------------------------------------------------

const wchar_t* CombinableTestElement::GetPreOrPostOperator()
{
    throw L"Test element doesn't support getting operator.";
}


//----------------------------------------------------------------------------
//  ArithmeticTestElement
//----------------------------------------------------------------------------

void ArithmeticTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( mChildren.size() != 2 )
        throw L"Two children expected for arithmetic operations.";

    if ( AllowOnlyIntegral() )
    {
        if ( !mChildren[0]->GetType()->IsIntegral()
            || !mChildren[1]->GetType()->IsIntegral() )
            throw L"Only integrals allowed.";
    }

    mType = ArithmeticTestElement::GetCommonType( 
        typeEnv, 
        mChildren[0]->GetType(), 
        mChildren[1]->GetType() );
}

std::shared_ptr<DataObj> ArithmeticTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<RValueObj>   val( new RValueObj() );
    std::shared_ptr<DataObj>     left( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );
    std::shared_ptr<DataObj>     right( mChildren[1]->Evaluate( typeEnv, scope, dataEnv ) );

    val->SetType( mType );

    if ( mType->IsComplex() )
    {
        Complex10   leftVal = ConvertToComplex( left.get() );
        Complex10   rightVal = ConvertToComplex( right.get() );

        val->Value.Complex80Value = ComplexOp( leftVal, rightVal );
    }
    else if ( mType->IsFloatingPoint() )
    {
        // same operation, no matter if it's real or imaginary
        Real10  leftVal = ConvertToFloat( left.get() );
        Real10  rightVal = ConvertToFloat( right.get() );

        val->Value.Float80Value = FloatOp( leftVal, rightVal );
    }
    else
    {
        _ASSERT( mType->IsIntegral() );
        PromoteInPlace( left.get(), mType.Get() );
        PromoteInPlace( right.get(), mType.Get() );

        if ( mType->IsSigned() )
        {
            int64_t    leftVal = left->Value.Int64Value;
            int64_t    rightVal = right->Value.Int64Value;

            val->Value.Int64Value = IntOp( leftVal, rightVal );
        }
        else
        {
            uint64_t    leftVal = left->Value.UInt64Value;
            uint64_t    rightVal = right->Value.UInt64Value;

            val->Value.UInt64Value = UIntOp( leftVal, rightVal );
        }

        PromoteInPlace( val.get() );
    }

    return val;
}

void ArithmeticTestElement::ToDText( std::wostringstream& stream )
{
    mChildren[0]->ToDText( stream );
    stream << ' ' << OpStr() << ' ';
    mChildren[1]->ToDText( stream );
}

RefPtr<Type> ArithmeticTestElement::GetCommonType( ITypeEnv* typeEnv, Type* left, Type* right )
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

        if ( ltype->GetSize() > rtype->GetSize() )
            type = ltype;
        else
            type = rtype;
    }
    else if ( left->IsImaginary() && right->IsImaginary() )
    {
        ltype = PromoteImaginaryType( typeEnv, left );
        rtype = PromoteImaginaryType( typeEnv, right );

        if ( ltype->GetSize() > rtype->GetSize() )
            type = ltype;
        else
            type = rtype;
    }
    else if ( left->IsReal() || right->IsReal() )
    {
        ltype = PromoteFloatType( typeEnv, left );
        rtype = PromoteFloatType( typeEnv, right );

        if ( ltype->GetSize() > rtype->GetSize() )
            type = ltype;
        else
            type = rtype;
    }
    else
    {
        _ASSERT( left->IsIntegral() && right->IsIntegral() );
        ltype = PromoteIntType( typeEnv, left );
        rtype = PromoteIntType( typeEnv, right );

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

    return type;
}

RefPtr<Type> ArithmeticTestElement::GetMulCommonType( ITypeEnv* typeEnv, Type* left, Type* right )
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

        if ( ltype->GetSize() > rtype->GetSize() )
            type = ltype;
        else
            type = rtype;
    }
    else if ( left->IsImaginary() && right->IsImaginary() )
    {
        ltype = PromoteFloatType( typeEnv, left );
        rtype = PromoteFloatType( typeEnv, right );

        if ( ltype->GetSize() > rtype->GetSize() )
            type = ltype;
        else
            type = rtype;
    }
    else if ( left->IsImaginary() || right->IsImaginary() )
    {
        ltype = PromoteImaginaryType( typeEnv, left );
        rtype = PromoteImaginaryType( typeEnv, right );

        if ( ltype->GetSize() > rtype->GetSize() )
            type = ltype;
        else
            type = rtype;
    }
    else if ( left->IsReal() || right->IsReal() )
    {
        ltype = PromoteFloatType( typeEnv, left );
        rtype = PromoteFloatType( typeEnv, right );

        if ( ltype->GetSize() > rtype->GetSize() )
            type = ltype;
        else
            type = rtype;
    }
    else
    {
        _ASSERT( left->IsIntegral() && right->IsIntegral() );
        ltype = PromoteIntType( typeEnv, left );
        rtype = PromoteIntType( typeEnv, right );

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

    return type;
}

RefPtr<Type> ArithmeticTestElement::GetModCommonType( ITypeEnv* typeEnv, Type* left, Type* right )
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

        if ( ltype->GetSize() > rtype->GetSize() )
            type = ltype;
        else
            type = rtype;
    }
    else if ( left->IsImaginary() )
    {
        ltype = PromoteImaginaryType( typeEnv, left );
        rtype = PromoteImaginaryType( typeEnv, right );

        if ( ltype->GetSize() > rtype->GetSize() )
            type = ltype;
        else
            type = rtype;
    }
    else if ( left->IsReal() || right->IsReal() || right->IsImaginary() )
    {
        ltype = PromoteFloatType( typeEnv, left );
        rtype = PromoteFloatType( typeEnv, right );

        if ( ltype->GetSize() > rtype->GetSize() )
            type = ltype;
        else
            type = rtype;
    }
    else
    {
        _ASSERT( left->IsIntegral() && right->IsIntegral() );
        ltype = PromoteIntType( typeEnv, left );
        rtype = PromoteIntType( typeEnv, right );

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

    return type;
}

RefPtr<Type> ArithmeticTestElement::PromoteComplexType( ITypeEnv* typeEnv, Type* t )
{
    RefPtr<Type>    type;

    if ( t->IsIntegral() )
        type = typeEnv->GetType( MagoEE::Tcomplex32 );
    else if ( t->IsComplex() )
    {
        switch ( t->GetSize() )
        {
        case 8: type = typeEnv->GetType( MagoEE::Tcomplex32 );    break;
        case 16: type = typeEnv->GetType( MagoEE::Tcomplex64 );    break;
        case 20: type = typeEnv->GetType( MagoEE::Tcomplex80 );    break;
        default:
            throw L"Unknown float size.";
        }
    }
    else if ( t->IsFloatingPoint() )
    {
        // real or imaginary, same size
        switch ( t->GetSize() )
        {
        case 4: type = typeEnv->GetType( MagoEE::Tcomplex32 );    break;
        case 8: type = typeEnv->GetType( MagoEE::Tcomplex64 );    break;
        case 10: type = typeEnv->GetType( MagoEE::Tcomplex80 );    break;
        default:
            throw L"Unknown float size.";
        }
    }
    else
        throw L"Can't cast to float.";

    return type;
}

RefPtr<Type> ArithmeticTestElement::PromoteImaginaryType( ITypeEnv* typeEnv, Type* t )
{
    RefPtr<Type>    type;

    if ( t->IsIntegral() )
        type = typeEnv->GetType( MagoEE::Timaginary32 );
    else if ( t->IsFloatingPoint() )
    {
        switch ( t->GetSize() )
        {
        case 4: type = typeEnv->GetType( MagoEE::Timaginary32 );    break;
        case 8: type = typeEnv->GetType( MagoEE::Timaginary64 );    break;
        case 10: type = typeEnv->GetType( MagoEE::Timaginary80 );    break;
        default:
            throw L"Unknown float size.";
        }
    }
    else
        throw L"Can't cast to float.";

    return type;
}

RefPtr<Type> ArithmeticTestElement::PromoteFloatType( ITypeEnv* typeEnv, Type* t )
{
    RefPtr<Type>    type;

    if ( t->IsIntegral() )
        type = typeEnv->GetType( MagoEE::Tfloat32 );
    else if ( t->IsFloatingPoint() )
    {
        switch ( t->GetSize() )
        {
        case 4: type = typeEnv->GetType( MagoEE::Tfloat32 );    break;
        case 8: type = typeEnv->GetType( MagoEE::Tfloat64 );    break;
        case 10: type = typeEnv->GetType( MagoEE::Tfloat80 );    break;
        default:
            throw L"Unknown float size.";
        }
    }
    else
        throw L"Can't cast to float.";

    return type;
}

RefPtr<MagoEE::Type> ArithmeticTestElement::PromoteIntType( ITypeEnv* typeEnv, MagoEE::Type* t )
{
    RefPtr<Type>    intType = typeEnv->GetType( MagoEE::Tint32 );
    RefPtr<Type>    type;

    if ( t->GetSize() >= intType->GetSize() )
    {
        if ( t->GetSize() == 8 )
        {
            if ( t->IsSigned() )
                type = typeEnv->GetType( MagoEE::Tint64 );
            else
                type = typeEnv->GetType( MagoEE::Tuns64 );
        }
        else
        {
            if ( t->IsSigned() )
                type = typeEnv->GetType( MagoEE::Tint32 );
            else
                type = typeEnv->GetType( MagoEE::Tuns32 );
        }
    }
    else
    {
        type = intType;
    }

    return type;
}

Complex10 ArithmeticTestElement::ConvertToComplex( DataObj* x )
{
    Complex10  result;

    Type*   type = x->GetType();

    if ( type->IsImaginary() )
    {
        result.RealPart.Zero();
        result.ImaginaryPart = x->Value.Float80Value;
    }
    else if ( type->IsReal() )
    {
        result.RealPart = x->Value.Float80Value;
        result.ImaginaryPart.Zero();
    }
    else if ( type->IsIntegral() )
    {
        if ( type->IsSigned() )
            result.RealPart.FromInt64( x->Value.Int64Value );
        else
            result.RealPart.FromUInt64( x->Value.UInt64Value );
        result.ImaginaryPart.Zero();
    }
    else if ( type->IsPointer() )
    {
        result.RealPart.FromUInt64( x->Value.Addr );
        result.ImaginaryPart.Zero();
    }
    else
    {
        _ASSERT( type->IsComplex() );
        result = x->Value.Complex80Value;
    }

    return result;
}

Real10 ArithmeticTestElement::ConvertToFloat( DataObj* x )
{
    Real10  result;

    Type*   type = x->GetType();

    if ( type->IsIntegral() )
    {
        if ( type->IsSigned() )
            result.FromInt64( x->Value.Int64Value );
        else
            result.FromUInt64( x->Value.UInt64Value );
    }
    else
    {
        result = x->Value.Float80Value;
    }

    return result;
}

void ArithmeticTestElement::PromoteInPlace( DataObj* x )
{
    _ASSERT( x->GetType()->IsIntegral() );

    if ( x->GetType()->GetSize() == 4 )
    {
        if ( x->GetType()->IsSigned() )
            x->Value.UInt64Value = (int32_t) x->Value.UInt64Value;
        else
            x->Value.UInt64Value = (uint32_t) x->Value.UInt64Value;
    }
    else if ( x->GetType()->GetSize() == 2 )
    {
        if ( x->GetType()->IsSigned() )
            x->Value.UInt64Value = (int16_t) x->Value.UInt64Value;
        else
            x->Value.UInt64Value = (uint16_t) x->Value.UInt64Value;
    }
    else if ( x->GetType()->GetSize() == 1 )
    {
        if ( x->GetType()->IsSigned() )
            x->Value.UInt64Value = (int8_t) x->Value.UInt64Value;
        else
            x->Value.UInt64Value = (uint8_t) x->Value.UInt64Value;
    }
    else
        _ASSERT( x->GetType()->GetSize() == 8 );
}

void ArithmeticTestElement::PromoteInPlace( DataObj* x, Type* targetType )
{
    _ASSERT( x->GetType()->IsIntegral() );
    _ASSERT( targetType->IsIntegral() );

    if ( (targetType->GetSize() == 8) || targetType->IsSigned() )
    {
        PromoteInPlace( x );
    }
    else
    {
        _ASSERT( targetType->GetSize() == 4 );
        _ASSERT( x->GetType()->GetSize() <= 4 );

        uint32_t    y = 0;

        if ( x->GetType()->GetSize() == 4 )
        {
            if ( x->GetType()->IsSigned() )
                y = (int32_t) x->Value.UInt64Value;
            else
                y = (uint32_t) x->Value.UInt64Value;
        }
        else if ( x->GetType()->GetSize() == 2 )
        {
            if ( x->GetType()->IsSigned() )
                y = (int16_t) x->Value.UInt64Value;
            else
                y = (uint16_t) x->Value.UInt64Value;
        }
        else if ( x->GetType()->GetSize() == 1 )
        {
            if ( x->GetType()->IsSigned() )
                y = (int8_t) x->Value.UInt64Value;
            else
                y = (uint8_t) x->Value.UInt64Value;
        }

        x->Value.UInt64Value = y;
    }
}

bool ArithmeticTestElement::AllowOnlyIntegral()
{
    return false;
}


//----------------------------------------------------------------------------
//  AddTestElement
//----------------------------------------------------------------------------

const wchar_t* AddTestElement::GetPreOrPostOperator()
{
    return L"++";
}

std::shared_ptr<DataObj> AddTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    Type*   ltype = mChildren[0]->GetType();
    Type*   rtype = mChildren[1]->GetType();

    if ( (ltype->IsImaginary() && (rtype->IsReal() || rtype->IsIntegral()))
        || ((ltype->IsReal() || ltype->IsIntegral()) && rtype->IsImaginary()) )
    {
        // making up a complex, not actually subtracting
        std::shared_ptr<DataObj>     val( new RValueObj() );
        std::shared_ptr<DataObj>     left( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );
        std::shared_ptr<DataObj>     right( mChildren[1]->Evaluate( typeEnv, scope, dataEnv ) );
        Complex10   c;

        if ( ltype->IsReal() || ltype->IsIntegral() )
        {
            Real10  r = ConvertToFloat( left.get() );
            c.RealPart = r;
            c.ImaginaryPart = right->Value.Float80Value;
        }
        else
        {
            Real10  r = ConvertToFloat( right.get() );
            c.ImaginaryPart = left->Value.Float80Value;
            c.RealPart = r;
        }

        val->Value.Complex80Value = c;
        val->SetType( mType );
        return val;
    }
    else
    {
        return ArithmeticTestElement::Evaluate( typeEnv, scope, dataEnv );
    }
}

uint64_t        AddTestElement::UIntOp( uint64_t left, uint64_t right )
{
    return left + right;
}

int64_t         AddTestElement::IntOp( int64_t left, int64_t right )
{
    return left + right;
}

Real10          AddTestElement::FloatOp( const Real10& left, const Real10& right )
{
    Real10  result;
    result.Add( left, right );
    return result;
}

Complex10       AddTestElement::ComplexOp( const Complex10& left, const Complex10& right )
{
    Complex10   result;
    result.Add( left, right );
    return result;
}

const wchar_t*  AddTestElement::OpStr()
{
    return L"+";
}


//----------------------------------------------------------------------------
//  SubTestElement
//----------------------------------------------------------------------------

const wchar_t* SubTestElement::GetPreOrPostOperator()
{
    return L"--";
}

std::shared_ptr<DataObj> SubTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    Type*   ltype = mChildren[0]->GetType();
    Type*   rtype = mChildren[1]->GetType();

    if ( (ltype->IsImaginary() && (rtype->IsReal() || rtype->IsIntegral()))
        || ((ltype->IsReal() || ltype->IsIntegral()) && rtype->IsImaginary()) )
    {
        // making up a complex, not actually subtracting
        std::shared_ptr<DataObj>     val( new RValueObj() );
        std::shared_ptr<DataObj>     left( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );
        std::shared_ptr<DataObj>     right( mChildren[1]->Evaluate( typeEnv, scope, dataEnv ) );
        Complex10   c;

        if ( ltype->IsReal() || ltype->IsIntegral() )
        {
            Real10  r = ConvertToFloat( left.get() );
            c.RealPart = r;
            c.ImaginaryPart.Negate( right->Value.Float80Value );
        }
        else
        {
            Real10  r = ConvertToFloat( right.get() );
            c.ImaginaryPart = left->Value.Float80Value;
            c.RealPart.Negate( r );
        }

        val->Value.Complex80Value = c;
        val->SetType( mType );
        return val;
    }
    else if ( ltype->IsImaginary()  && rtype->IsComplex() )
    {
        // this is how DMD actually does (imaginary - complex)
        // I would prefer if it was (0 + Xi) - complex, in other words, 
        // let ArithmeticTestElement::Evaluate handle it by promoting the imaginary to a complex

        std::shared_ptr<DataObj>     val( new RValueObj() );
        std::shared_ptr<DataObj>     left( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );
        std::shared_ptr<DataObj>     right( mChildren[1]->Evaluate( typeEnv, scope, dataEnv ) );
        Complex10   c;

        Real10  r = ConvertToFloat( left.get() );
        c.RealPart.Negate( right->Value.Complex80Value.RealPart );
        c.ImaginaryPart.Sub( r, right->Value.Complex80Value.ImaginaryPart );

        val->Value.Complex80Value = c;
        val->SetType( mType );
        return val;
    }
    else
    {
        return ArithmeticTestElement::Evaluate( typeEnv, scope, dataEnv );
    }
}

uint64_t        SubTestElement::UIntOp( uint64_t left, uint64_t right )
{
    return left - right;
}

int64_t         SubTestElement::IntOp( int64_t left, int64_t right )
{
    return left - right;
}

Real10          SubTestElement::FloatOp( const Real10& left, const Real10& right )
{
    Real10  result;
    result.Sub( left, right );
    return result;
}

Complex10       SubTestElement::ComplexOp( const Complex10& left, const Complex10& right )
{
    Complex10   result;
    result.Sub( left, right );
    return result;
}

const wchar_t*  SubTestElement::OpStr()
{
    return L"-";
}


//----------------------------------------------------------------------------
//  MulTestElement
//----------------------------------------------------------------------------

void MulTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( mChildren.size() != 2 )
        throw L"Two children expected for arithmetic operations.";

    mType = ArithmeticTestElement::GetMulCommonType( 
        typeEnv, 
        mChildren[0]->GetType(), 
        mChildren[1]->GetType() );
}

std::shared_ptr<DataObj> MulTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>   val;

    Type*   ltype = mChildren[0]->GetType();
    Type*   rtype = mChildren[1]->GetType();

    if ( ((ltype->IsImaginary() || ltype->IsReal() || ltype->IsIntegral()) && rtype->IsComplex())
        || (ltype->IsComplex() && (rtype->IsImaginary() || rtype->IsReal() || rtype->IsIntegral())) )
    {
        val.reset( new RValueObj() );
        std::shared_ptr<DataObj>     left( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );
        std::shared_ptr<DataObj>     right( mChildren[1]->Evaluate( typeEnv, scope, dataEnv ) );
        Complex10   c;

        if ( ltype->IsReal() || ltype->IsIntegral() )
        {
            Real10  r = ConvertToFloat( left.get() );
            c.RealPart.Mul( r, right->Value.Complex80Value.RealPart );
            c.ImaginaryPart.Mul( r, right->Value.Complex80Value.ImaginaryPart );
        }
        else if ( ltype->IsImaginary() )
        {
#if 0
            Real10  r = ConvertToFloat( left.get() );
            Real10  nr;
            nr.Negate( r );
            c.RealPart.Mul( nr, right->Value.Complex80Value.ImaginaryPart );
            c.ImaginaryPart.Mul( r, right->Value.Complex80Value.RealPart );
#else
            // this is how DMD actually does it
            // I prefer a way that doesn't depend on the order of the arguments (where the NaN is)
            Real10  r = ConvertToFloat( left.get() );
            Real10  nri;
            nri.Negate( right->Value.Complex80Value.ImaginaryPart );
            c.RealPart.Mul( r, nri );
            c.ImaginaryPart.Mul( r, right->Value.Complex80Value.RealPart );
#endif
        }
        else if ( rtype->IsReal() || rtype->IsIntegral() )
        {
            Real10  r = ConvertToFloat( right.get() );
            c.RealPart.Mul( r, left->Value.Complex80Value.RealPart );
            c.ImaginaryPart.Mul( r, left->Value.Complex80Value.ImaginaryPart );
        }
        else if ( rtype->IsImaginary() )
        {
#if 0
            Real10  r = ConvertToFloat( right.get() );
            Real10  nr;
            nr.Negate( r );
            c.RealPart.Mul( nr, left->Value.Complex80Value.ImaginaryPart );
            c.ImaginaryPart.Mul( r, left->Value.Complex80Value.RealPart );
#else
            // this is how DMD actually does it
            // I prefer a way that doesn't depend on the order of the arguments (where the NaN is)
            Real10  r = ConvertToFloat( right.get() );
            Real10  nli;
            nli.Negate( left->Value.Complex80Value.ImaginaryPart );
            c.RealPart.Mul( r, nli );
            c.ImaginaryPart.Mul( r, left->Value.Complex80Value.RealPart );
#endif
        }

        val->Value.Complex80Value = c;
        val->SetType( mType );
        return val;
    }

    val = ArithmeticTestElement::Evaluate( typeEnv, scope, dataEnv );

    if ( mChildren[0]->GetType()->IsImaginary() && mChildren[1]->GetType()->IsImaginary() )
    {
        // (a + bi)(c + di) = (0 + bi)(0 + di) = bi * di = -b*d
        val->Value.Float80Value.Negate( val->Value.Float80Value );
    }

    return val;
}

uint64_t        MulTestElement::UIntOp( uint64_t left, uint64_t right )
{
    return left * right;
}

int64_t         MulTestElement::IntOp( int64_t left, int64_t right )
{
    return left * right;
}

Real10          MulTestElement::FloatOp( const Real10& left, const Real10& right )
{
    Real10  result;
    result.Mul( left, right );
    return result;
}

Complex10       MulTestElement::ComplexOp( const Complex10& left, const Complex10& right )
{
    Complex10   result;
    result.Mul( left, right );
    return result;
}

const wchar_t*  MulTestElement::OpStr()
{
    return L"*";
}


//----------------------------------------------------------------------------
//  DivTestElement
//----------------------------------------------------------------------------

void DivTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( mChildren.size() != 2 )
        throw L"Two children expected for arithmetic operations.";

    mType = ArithmeticTestElement::GetMulCommonType( 
        typeEnv, 
        mChildren[0]->GetType(), 
        mChildren[1]->GetType() );
}

std::shared_ptr<DataObj> DivTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>   val;

    Type*   ltype = mChildren[0]->GetType();
    Type*   rtype = mChildren[1]->GetType();

    if ( ltype->IsComplex() && (rtype->IsReal() || rtype->IsIntegral() || rtype->IsImaginary()) )
    {
        val.reset( new RValueObj() );
        std::shared_ptr<DataObj>     left( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );
        std::shared_ptr<DataObj>     right( mChildren[1]->Evaluate( typeEnv, scope, dataEnv ) );
        Complex10   c;

        if ( rtype->IsReal() || rtype->IsIntegral() )
        {
            Real10      r = ConvertToFloat( right.get() );
            c.RealPart.Div( left->Value.Complex80Value.RealPart, r );
            c.ImaginaryPart.Div( left->Value.Complex80Value.ImaginaryPart, r );
        }
        else if ( rtype->IsImaginary() )
        {
            Real10      r = right->Value.Float80Value;
            Real10      ncleft;
            c.RealPart.Div( left->Value.Complex80Value.ImaginaryPart, r );
            ncleft.Negate( left->Value.Complex80Value.RealPart );
            c.ImaginaryPart.Div( ncleft, r );
        }
        else
            _ASSERT( false );

        val->Value.Complex80Value = c;
        val->SetType( mType );
        return val;
    }

    val = ArithmeticTestElement::Evaluate( typeEnv, scope, dataEnv );

    if ( (mChildren[0]->GetType()->IsReal() || mChildren[0]->GetType()->IsIntegral())
        && mChildren[1]->GetType()->IsImaginary() )
    {
        // (a + bi)(c + di) = (a + 0i)(0 + di) = -a/d
        val->Value.Float80Value.Negate( val->Value.Float80Value );
    }

    return val;
}

uint64_t        DivTestElement::UIntOp( uint64_t left, uint64_t right )
{
    if ( right == 0 )
        throw L"Div by zero.";

    return left / right;
}

int64_t         DivTestElement::IntOp( int64_t left, int64_t right )
{
    if ( right == 0 )
        throw L"Div by zero.";

    return left / right;
}

Real10          DivTestElement::FloatOp( const Real10& left, const Real10& right )
{
    Real10  result;
    result.Div( left, right );
    return result;
}

Complex10       DivTestElement::ComplexOp( const Complex10& left, const Complex10& right )
{
    Complex10   result;
    result.Div( left, right );
    return result;
}

const wchar_t*  DivTestElement::OpStr()
{
    return L"/";
}


//----------------------------------------------------------------------------
//  ModTestElement
//----------------------------------------------------------------------------

void ModTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( mChildren[1]->GetType()->IsComplex() )
    {
        throw L"Can't perform modulo complex arithmetic.";
    }

    if ( mChildren.size() != 2 )
        throw L"Two children expected for arithmetic operations.";

    mType = ArithmeticTestElement::GetModCommonType( 
        typeEnv, 
        mChildren[0]->GetType(), 
        mChildren[1]->GetType() );
}

uint64_t        ModTestElement::UIntOp( uint64_t left, uint64_t right )
{
    if ( right == 0 )
        throw L"Mod by zero.";

    return left % right;
}

int64_t         ModTestElement::IntOp( int64_t left, int64_t right )
{
    if ( right == 0 )
        throw L"Mod by zero.";

    return left % right;
}

Real10          ModTestElement::FloatOp( const Real10& left, const Real10& right )
{
    Real10  result;
    result.Rem( left, right );
    return result;
}

Complex10       ModTestElement::ComplexOp( const Complex10& left, const Complex10& right )
{
    // ModTestElement::Bind already took care of this, but we can't really take mod of complex and complex.
    // So, right will actually be a real or imaginary in the form (x + 0i) or (0 + yi). In that case we 
    // take the non-zero one and divide both parts of left by it.

    Complex10   result;

    if ( !right.RealPart.IsZero() && !right.ImaginaryPart.IsZero() )
        throw L"Can't perform modulo complex arithmetic.";

    // only one of these will be non-zero, since our right side value had to have come from a real or imaginary
    if ( !right.RealPart.IsZero() )
    {
        result.RealPart.Rem( left.RealPart, right.RealPart );
        result.ImaginaryPart.Rem( left.ImaginaryPart, right.RealPart );
    }
    else
    {
        result.RealPart.Rem( left.RealPart, right.ImaginaryPart );
        result.ImaginaryPart.Rem( left.ImaginaryPart, right.ImaginaryPart );
    }

    return result;
}

const wchar_t*  ModTestElement::OpStr()
{
    return L"%";
}


//----------------------------------------------------------------------------
//  PowTestElement
//----------------------------------------------------------------------------

uint64_t        PowTestElement::UIntOp( uint64_t left, uint64_t right )
{
    throw L"Can't pow ints.";
}

int64_t         PowTestElement::IntOp( int64_t left, int64_t right )
{
    throw L"Can't pow ints.";
}

Real10          PowTestElement::FloatOp( const Real10& left, const Real10& right )
{
    // TODO: implement
    // TODO: can't work for imaginaries
    Real10  result;
    result.Zero();
    return result;
}

Complex10       PowTestElement::ComplexOp( const Complex10& left, const Complex10& right )
{
    throw L"Can't pow complexes.";
}

const wchar_t*  PowTestElement::OpStr()
{
    return L"^^";
}


//----------------------------------------------------------------------------
//  NegateTestElement
//----------------------------------------------------------------------------

void NegateTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    mType = mChildren[0]->GetType();

    if ( !mType->IsIntegral() && !mType->IsFloatingPoint() )
        throw L"Can only negate integrals and floats.";
}

std::shared_ptr<DataObj> NegateTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>   val( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );

    if ( mType->IsComplex() )
    {
        val->Value.Complex80Value.Negate( val->Value.Complex80Value );
    }
    else if ( mType->IsFloatingPoint() )
    {
        val->Value.Float80Value.Negate( val->Value.Float80Value );
    }
    else
    {
        _ASSERT( mType->IsIntegral() );

        val->Value.UInt64Value = -val->Value.Int64Value;

        ArithmeticTestElement::PromoteInPlace( val.get() );
    }

    return val;
}

void NegateTestElement::ToDText( std::wostringstream& stream )
{
    stream << '-';
    mChildren[0]->ToDText( stream );
}


//----------------------------------------------------------------------------
//  UnaryAddTestElement
//----------------------------------------------------------------------------

void UnaryAddTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    mType = mChildren[0]->GetType();

    if ( !mType->IsIntegral() && !mType->IsFloatingPoint() )
        throw L"Can only UnaryAdd integrals and floats.";
}

std::shared_ptr<DataObj> UnaryAddTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    return mChildren[0]->Evaluate( typeEnv, scope, dataEnv );
}

void UnaryAddTestElement::ToDText( std::wostringstream& stream )
{
    stream << '+';
    mChildren[0]->ToDText( stream );
}


//----------------------------------------------------------------------------
//  AndTestElement
//----------------------------------------------------------------------------

bool AndTestElement::AllowOnlyIntegral()
{
    return true;
}

uint64_t        AndTestElement::UIntOp( uint64_t left, uint64_t right )
{
    return left & right;
}

int64_t         AndTestElement::IntOp( int64_t left, int64_t right )
{
    return left & right;
}

Real10          AndTestElement::FloatOp( const Real10& left, const Real10& right )
{
    throw L"Can't And floats.";
}

Complex10       AndTestElement::ComplexOp( const Complex10& left, const Complex10& right )
{
    throw L"Can't And floats.";
}

const wchar_t*  AndTestElement::OpStr()
{
    return L"&";
}


//----------------------------------------------------------------------------
//  OrTestElement
//----------------------------------------------------------------------------

bool OrTestElement::AllowOnlyIntegral()
{
    return true;
}

uint64_t        OrTestElement::UIntOp( uint64_t left, uint64_t right )
{
    return left | right;
}

int64_t         OrTestElement::IntOp( int64_t left, int64_t right )
{
    return left | right;
}

Real10          OrTestElement::FloatOp( const Real10& left, const Real10& right )
{
    throw L"Can't And floats.";
}

Complex10       OrTestElement::ComplexOp( const Complex10& left, const Complex10& right )
{
    throw L"Can't And floats.";
}

const wchar_t*  OrTestElement::OpStr()
{
    return L"|";
}


//----------------------------------------------------------------------------
//  XorTestElement
//----------------------------------------------------------------------------

bool XorTestElement::AllowOnlyIntegral()
{
    return true;
}

uint64_t        XorTestElement::UIntOp( uint64_t left, uint64_t right )
{
    return left ^ right;
}

int64_t         XorTestElement::IntOp( int64_t left, int64_t right )
{
    return left ^ right;
}

Real10          XorTestElement::FloatOp( const Real10& left, const Real10& right )
{
    throw L"Can't And floats.";
}

Complex10       XorTestElement::ComplexOp( const Complex10& left, const Complex10& right )
{
    throw L"Can't And floats.";
}

const wchar_t*  XorTestElement::OpStr()
{
    return L"^";
}


//----------------------------------------------------------------------------
//  BitNotTestElement
//----------------------------------------------------------------------------

void BitNotTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    mType = mChildren[0]->GetType();

    if ( !mType->IsIntegral() )
        throw L"Can only BitNot integrals.";
}

std::shared_ptr<DataObj> BitNotTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>   val( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );

    _ASSERT( mType->IsIntegral() );

    val->Value.UInt64Value = ~val->Value.Int64Value;

    ArithmeticTestElement::PromoteInPlace( val.get() );

    return val;
}

void BitNotTestElement::ToDText( std::wostringstream& stream )
{
    stream << '~';
    mChildren[0]->ToDText( stream );
}


//----------------------------------------------------------------------------
//  ShiftTestElement
//----------------------------------------------------------------------------

void ShiftTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( !mChildren[0]->GetType()->IsIntegral() )
        throw L"Can only Shift integrals.";
    if ( !mChildren[1]->GetType()->IsIntegral() )
        throw L"Can only Shift with integrals.";

    if ( mChildren[0]->GetType()->GetSize() >= 4 )
        mType = mChildren[0]->GetType();
    else
        mType = typeEnv->GetType( MagoEE::Tint32 );
}

std::shared_ptr<DataObj> ShiftTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<RValueObj>   val( new RValueObj() );
    std::shared_ptr<DataObj>     left( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );
    std::shared_ptr<DataObj>     right( mChildren[1]->Evaluate( typeEnv, scope, dataEnv ) );

    val->SetType( mType );

    _ASSERT( mType->IsIntegral() );
    uint64_t    leftVal = left->Value.UInt64Value;
    uint64_t    rightVal = right->Value.UInt64Value;

    // can't shift all the bits out
    if ( mType->GetSize() == 8 )
        rightVal &= 0x3F;
    else
        rightVal &= 0x1F;

    val->Value.UInt64Value = IntOp( leftVal, rightVal, mType.Get() );

    ArithmeticTestElement::PromoteInPlace( val.get() );

    return val;
}

void ShiftTestElement::ToDText( std::wostringstream& stream )
{
    mChildren[0]->ToDText( stream );
    stream << " " << OpStr() << " ";
    mChildren[1]->ToDText( stream );
}


//----------------------------------------------------------------------------
//  ShiftLeftTestElement
//----------------------------------------------------------------------------

uint64_t        ShiftLeftTestElement::IntOp( uint64_t left, uint64_t right, Type* type )
{
    return left << right;
}

const wchar_t*  ShiftLeftTestElement::OpStr()
{
    return L"<<";
}


//----------------------------------------------------------------------------
//  ShiftRightTestElement
//----------------------------------------------------------------------------

uint64_t        ShiftRightTestElement::IntOp( uint64_t left, uint64_t right, Type* type )
{
    // C, C++, C# behavior
    // I think this can be even simpler (int64 >> right) or (uint64 >> right)
    if ( type->GetSize() == 8 )
    {
        if ( type->IsSigned() )
            return ((int64_t) left) >> right;
        else
            return ((uint64_t) left) >> right;
    }
    else
    {
        if ( type->IsSigned() )
            return ((int32_t) left) >> right;
        else
            return ((uint32_t) left) >> right;
    }
}

const wchar_t*  ShiftRightTestElement::OpStr()
{
    return L">>";
}


//----------------------------------------------------------------------------
//  UShiftRightTestElement
//----------------------------------------------------------------------------

uint64_t        UShiftRightTestElement::IntOp( uint64_t left, uint64_t right, Type* type )
{
    switch ( type->GetBackingTy() )
    {
    case MagoEE::Tint8:
    case MagoEE::Tuns8:
        return ((uint8_t) left) >> right;
    case MagoEE::Tint16:
    case MagoEE::Tuns16:
        return ((uint16_t) left) >> right;
    case MagoEE::Tint32:
    case MagoEE::Tuns32:
        return ((uint32_t) left) >> right;
    case MagoEE::Tint64:
    case MagoEE::Tuns64:
        return ((uint64_t) left) >> right;
    default:
        return 0;
    }
}

const wchar_t*  UShiftRightTestElement::OpStr()
{
    return L">>>";
}


//----------------------------------------------------------------------------
//  AndAndTestElement
//----------------------------------------------------------------------------

void AndAndTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( !mChildren[0]->GetType()->CanImplicitCastToBool() )
        throw L"Can only AndAnd with integrals.";
    if ( !mChildren[1]->GetType()->CanImplicitCastToBool() )
        throw L"Can only AndAnd with integrals.";

    mType = typeEnv->GetType( MagoEE::Tbool );
}

std::shared_ptr<DataObj> AndAndTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<RValueObj>   val( new RValueObj() );
    std::shared_ptr<DataObj>     left( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );

    val->SetType( mType );

    if ( EvalBool( left.get() ) )
    {
        std::shared_ptr<DataObj>     right( mChildren[1]->Evaluate( typeEnv, scope, dataEnv ) );

        val->Value.UInt64Value = EvalBool( right.get() ) ? 1 : 0;
    }
    else
    {
        val->Value.UInt64Value = 0;
    }

    return val;
}

void AndAndTestElement::ToDText( std::wostringstream& stream )
{
    mChildren[0]->ToDText( stream );
    stream << " " << "&&" << " ";
    mChildren[1]->ToDText( stream );
}

bool AndAndTestElement::EvalBool( DataObj* val )
{
    if ( val->GetType()->IsComplex() )
    {
        // TODO: this needs to be cleared up
        //       In DMD, explicitly casting to bool treats all imaginaries as a 0 zero, known as false.
        //       While implicit casting (for example in and && expr) treats non-zero as true, and zero as false.
#if 0
        return !val->Value.Complex80Value.RealPart.IsZero();
#else
        return !val->Value.Complex80Value.RealPart.IsZero()
            || !val->Value.Complex80Value.ImaginaryPart.IsZero();
#endif
    }
    else if ( val->GetType()->IsImaginary() )
    {
        // TODO: this needs to be cleared up
        //       In DMD, explicitly casting to bool treats all imaginaries as a 0 zero, known as false.
        //       While implicit casting (for example in and && expr) treats non-zero as true, and zero as false.
#if 0
        return false;
#else
        return !val->Value.Float80Value.IsZero();
#endif
    }
    else if ( val->GetType()->IsFloatingPoint() )
    {
        return !val->Value.Float80Value.IsZero();
    }
    else if ( val->GetType()->IsIntegral() )
    {
        return val->Value.UInt64Value != 0;
    }
    else if ( val->GetType()->IsPointer() )
    {
        return val->Value.Addr != 0;
    }
    else if ( val->GetType()->IsDArray() )
    {
        return val->Value.Array.Addr != 0;
    }
    else if ( val->GetType()->IsSArray() )
    {
        MagoEE::Address addr = 0;
        return val->GetAddress( addr ) && (addr != 0);
    }
    // TODO: delegate, A-array

    throw L"Can't evaluate to bool.";
}


//----------------------------------------------------------------------------
//  OrOrTestElement
//----------------------------------------------------------------------------

void OrOrTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( !mChildren[0]->GetType()->CanImplicitCastToBool() )
        throw L"Can only OrOr with integrals.";
    if ( !mChildren[1]->GetType()->CanImplicitCastToBool() )
        throw L"Can only OrOr with integrals.";

    mType = typeEnv->GetType( MagoEE::Tbool );
}

std::shared_ptr<DataObj> OrOrTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<RValueObj>   val( new RValueObj() );
    std::shared_ptr<DataObj>     left( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );

    val->SetType( mType );

    if ( !AndAndTestElement::EvalBool( left.get() ) )
    {
        std::shared_ptr<DataObj>     right( mChildren[1]->Evaluate( typeEnv, scope, dataEnv ) );

        val->Value.UInt64Value = AndAndTestElement::EvalBool( right.get() ) ? 1 : 0;
    }
    else
    {
        val->Value.UInt64Value = 1;
    }

    return val;
}

void OrOrTestElement::ToDText( std::wostringstream& stream )
{
    mChildren[0]->ToDText( stream );
    stream << " " << "||" << " ";
    mChildren[1]->ToDText( stream );
}


//----------------------------------------------------------------------------
//  NotTestElement
//----------------------------------------------------------------------------

void NotTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( !mChildren[0]->GetType()->CanImplicitCastToBool() )
        throw L"Can only Not with integrals.";

    mType = typeEnv->GetType( MagoEE::Tbool );
}

std::shared_ptr<DataObj> NotTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<RValueObj>   val( new RValueObj() );
    std::shared_ptr<DataObj>     left( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );

    val->SetType( mType );

    val->Value.UInt64Value = AndAndTestElement::EvalBool( left.get() ) ? 0 : 1;

    return val;
}

void NotTestElement::ToDText( std::wostringstream& stream )
{
    stream << "!";
    mChildren[0]->ToDText( stream );
}


//----------------------------------------------------------------------------
//  CompareTestElement
//----------------------------------------------------------------------------

CompareTestElement::CompareTestElement()
:   mOpCode( O_none )
{
}

void CompareTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    if ( mOpCode == O_none )
        throw L"Unknown relational operator.";

    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    Type*   ltype = mChildren[0]->GetType();
    Type*   rtype = mChildren[1]->GetType();

    if ( (!ltype->IsScalar() && !ltype->IsSArray() && !ltype->IsDArray())
        || (!rtype->IsScalar() && !rtype->IsSArray() && !rtype->IsDArray()) )
        throw L"Can only Compare with integrals, floats, and pointers.";

    // if one is null, then try to set it to the other's type
    RefPtr<Type>    voidType = typeEnv->GetVoidPointerType();

    if ( ltype->Equals( voidType ) || rtype->Equals( voidType ) )
    {
        if ( ltype->Equals( voidType ) )
        {
            if ( mChildren[0]->TrySetType( rtype ) )
                ltype = rtype;
        }
        else if ( rtype->Equals( voidType ) )
        {
            if ( mChildren[1]->TrySetType( ltype ) )
                rtype = ltype;
        }
    }

    if ( ltype->IsPointer() != rtype->IsPointer() )
        throw L"Both or neither must be pointers.";

    if ( (ltype->IsSArray() || ltype->IsDArray()) != (rtype->IsSArray() || rtype->IsDArray()) )
        throw L"Can only mix and match arrays for comparison.";
    if ( ltype->IsSArray() || rtype->IsDArray() )
    {
        _ASSERT( rtype->IsSArray() || rtype->IsDArray() );
        if ( !ltype->AsTypeNext()->GetNext()->Equals( rtype->AsTypeNext()->GetNext() ) )
            throw L"Array element types must match for comparison.";
    }

    // TODO: in restrictions below (like for S- and D-arrays) also include delegate and A-array
    //       and class (actually ptr to class)
    //       except that for delegates, "==" is the same as "is"
    switch ( mOpCode )
    {
    case O_equal:
    case O_notequal:
        if ( ltype->IsSArray() || rtype->IsSArray() || ltype->IsDArray() || rtype->IsDArray() )
            throw L"Arrays have restricted relational operators.";
        break;

    //  not all supported types support these
    case O_lt:
    case O_le:
    case O_gt:
    case O_ge:

    case O_unord:
    case O_lg:
    case O_leg:
    case O_ule:
    case O_ul:
    case O_uge:
    case O_ug:
    case O_ue:
        if ( ltype->IsComplex() || rtype->IsComplex() 
            || ltype->IsSArray() || rtype->IsSArray() 
            || ltype->IsDArray() || rtype->IsDArray() )
            throw L"Complex types have restricted relational operators.";
        break;
    }

    mType = typeEnv->GetType( MagoEE::Tbool );
}

std::shared_ptr<DataObj> CompareTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<RValueObj>   val( new RValueObj() );
    std::shared_ptr<DataObj>     left( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );
    std::shared_ptr<DataObj>     right( mChildren[1]->Evaluate( typeEnv, scope, dataEnv ) );
    RefPtr<Type>            commonType;

    val->SetType( mType );

    if ( left->GetType()->IsPointer() )
    {
        val->Value.UInt64Value = IntegerOp( mOpCode, left->Value.Addr, right->Value.Addr ) ? 1 : 0;
    }
    else if ( left->GetType()->IsSArray() || left->GetType()->IsDArray() )
    {
        val->Value.UInt64Value = ArrayRelational( mOpCode, left.get(), right.get() ) ? 1 : 0;
    }
    else
    {
#if 1
        commonType = ArithmeticTestElement::GetCommonType( typeEnv, left->GetType(), right->GetType() );
#else
        if ( (mOpCode == O_equal) || (mOpCode == O_notequal) 
            || (mOpCode == O_identity) || (mOpCode == O_notidentity) )
            commonType = ArithmeticTestElement::GetCommonType( typeEnv, left->GetType(), right->GetType() );
        else
            commonType = ArithmeticTestElement::GetModCommonType( typeEnv, left->GetType(), right->GetType() );
#endif

        if ( commonType->IsComplex() )
        {
            val->Value.UInt64Value = ComplexRelational( mOpCode, left.get(), right.get() ) ? 1 : 0;
        }
        else if ( commonType->IsFloatingPoint() )
        {
            val->Value.UInt64Value = FloatingRelational( mOpCode, left.get(), right.get() ) ? 1 : 0;
        }
        else
        {
            val->Value.UInt64Value = IntegerRelational( mOpCode, commonType.Get(), left.get(), right.get() ) ? 1 : 0;
        }
    }

    return val;
}

void CompareTestElement::ToDText( std::wostringstream& stream )
{
    mChildren[0]->ToDText( stream );
    stream << " " << mOpStr.c_str() << " ";
    mChildren[1]->ToDText( stream );
}

void CompareTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"op" ) == 0 )
    {
        mOpStr = value;

        if ( wcscmp( value, L"==" ) == 0 )
            mOpCode = O_equal;
        else if ( wcscmp( value, L"!=" ) == 0 )
            mOpCode = O_notequal;
        else if ( wcscmp( value, L"is" ) == 0 )
            mOpCode = O_identity;
        else if ( wcscmp( value, L"!is" ) == 0 )
            mOpCode = O_notidentity;
        else if ( wcscmp( value, L"<" ) == 0 )
            mOpCode = O_lt;
        else if ( wcscmp( value, L"<=" ) == 0 )
            mOpCode = O_le;
        else if ( wcscmp( value, L">" ) == 0 )
            mOpCode = O_gt;
        else if ( wcscmp( value, L">=" ) == 0 )
            mOpCode = O_ge;
        else if ( wcscmp( value, L"!<>=" ) == 0 )
            mOpCode = O_unord;
        else if ( wcscmp( value, L"<>" ) == 0 )
            mOpCode = O_lg;
        else if ( wcscmp( value, L"<>=" ) == 0 )
            mOpCode = O_leg;
        else if ( wcscmp( value, L"!<=" ) == 0 )
            mOpCode = O_ug;
        else if ( wcscmp( value, L"!<" ) == 0 )
            mOpCode = O_uge;
        else if ( wcscmp( value, L"!>=" ) == 0 )
            mOpCode = O_ul;
        else if ( wcscmp( value, L"!>" ) == 0 )
            mOpCode = O_ule;
        else if ( wcscmp( value, L"!<>" ) == 0 )
            mOpCode = O_ue;
        else
            throw L"Unknown relational operator.";
    }
}

bool CompareTestElement::IntegerRelational( OpCode code, Type* exprType, DataObj* left, DataObj* right )
{
    ArithmeticTestElement::PromoteInPlace( left, exprType );
    ArithmeticTestElement::PromoteInPlace( right, exprType );

    if ( exprType->IsSigned() )
    {
        return IntegerOp( code, left->Value.Int64Value, right->Value.Int64Value );
    }
    else
    {
        return IntegerOp( code, left->Value.UInt64Value, right->Value.UInt64Value );
    }
}

bool CompareTestElement::FloatingRelational( OpCode code, DataObj* leftObj, DataObj* rightObj )
{
    if ( ((leftObj->GetType()->IsReal() || leftObj->GetType()->IsIntegral()) && rightObj->GetType()->IsImaginary())
        || (leftObj->GetType()->IsImaginary() && (rightObj->GetType()->IsReal() || rightObj->GetType()->IsIntegral())) )
    {
        rightObj->Value.Float80Value.Zero();
    }

    Real10  leftVal = ArithmeticTestElement::ConvertToFloat( leftObj );
    Real10  rightVal = ArithmeticTestElement::ConvertToFloat( rightObj );
    uint16_t    status = Real10::Compare( leftVal, rightVal );

    return FloatingRelational( code, status );
}

bool CompareTestElement::ComplexRelational( OpCode code, DataObj* leftObj, DataObj* rightObj )
{
    Complex10   leftVal = ArithmeticTestElement::ConvertToComplex( leftObj );
    Complex10   rightVal = ArithmeticTestElement::ConvertToComplex( rightObj );
    uint16_t    status = Complex10::Compare( leftVal, rightVal );

    return FloatingRelational( code, status );
}

bool CompareTestElement::FloatingRelational( OpCode code, uint16_t status )
{
    switch ( code )
    {
    case O_identity:
    case O_equal:   return Real10::IsEqual( status );
    case O_notidentity:
    case O_notequal:return !Real10::IsEqual( status );

    case O_lt:      return Real10::IsLess( status );
    case O_le:      return Real10::IsLess( status ) || Real10::IsEqual( status );
    case O_gt:      return Real10::IsGreater( status );
    case O_ge:      return Real10::IsGreater( status ) || Real10::IsEqual( status );

    case O_unord:   return Real10::IsUnordered( status );
    case O_lg:      return Real10::IsLess( status ) || Real10::IsGreater( status );
    case O_leg:     return Real10::IsLess( status ) || Real10::IsGreater( status ) || Real10::IsEqual( status );
    case O_ule:     return Real10::IsUnordered( status ) || Real10::IsLess( status ) || Real10::IsEqual( status );
    case O_ul:      return Real10::IsUnordered( status ) || Real10::IsLess( status );
    case O_uge:     return Real10::IsUnordered( status ) || Real10::IsGreater( status ) || Real10::IsEqual( status );
    case O_ug:      return Real10::IsUnordered( status ) || Real10::IsGreater( status );
    case O_ue:      return Real10::IsUnordered( status ) || Real10::IsEqual( status );
    default:
        throw L"Relational operator not allowed on integers.";
    }
}

bool CompareTestElement::ArrayRelational( OpCode code, DataObj* left, DataObj* right )
{
    MagoEE::Address     leftAddr = 0;
    MagoEE::dlength_t   leftLen = 0;
    MagoEE::Address     rightAddr = 0;
    MagoEE::dlength_t   rightLen = 0;

    if ( left->GetType()->IsSArray() )
    {
        if ( !left->GetAddress( leftAddr ) )
            throw L"Couldn't get S-array address.";
        leftLen = left->GetType()->AsTypeSArray()->GetLength();
    }
    else if ( left->GetType()->IsDArray() )
    {
        leftAddr = left->Value.Array.Addr;
        leftLen = left->Value.Array.Length;
    }
    else
        throw L"Expected to compare arrays.";

    if ( right->GetType()->IsSArray() )
    {
        if ( !right->GetAddress( rightAddr ) )
            throw L"Couldn't get S-array address.";
        rightLen = right->GetType()->AsTypeSArray()->GetLength();
    }
    else if ( right->GetType()->IsDArray() )
    {
        rightAddr = right->Value.Array.Addr;
        rightLen = right->Value.Array.Length;
    }
    else
        throw L"Expected to compare arrays.";

    bool    equal = (leftAddr == rightAddr) && (leftLen == rightLen);

    if ( code == O_identity )
        return equal;
    else if ( code == O_notidentity )
        return !equal;

    throw L"Unknown array comparison.";
}


//----------------------------------------------------------------------------
//  PtrArithmeticTestElement
//----------------------------------------------------------------------------

void PtrArithmeticTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    RefPtr<Type>    ltype = mChildren[0]->GetType();
    RefPtr<Type>    rtype = mChildren[1]->GetType();

    if ( ltype->IsPointer() )
    {
        if ( !rtype->IsIntegral() )
            throw L"Can only PtrArithmetic pointers with integrals.";

        mType = ltype;
    }
    else if ( rtype->IsPointer() )
    {
        if ( !ltype->IsIntegral() )
            throw L"Can only PtrArithmetic pointers with integrals.";

        mType = rtype;
    }
    else
        throw L"Can only PtrArithmetic pointers with integrals.";
}

std::shared_ptr<DataObj> PtrArithmeticTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<RValueObj>   val( new RValueObj() );
    std::shared_ptr<DataObj>     left( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );
    std::shared_ptr<DataObj>     right( mChildren[1]->Evaluate( typeEnv, scope, dataEnv ) );

    val->SetType( mType );

    RefPtr<Type>    pointed = mType->AsTypeNext()->GetNext();
    uint32_t        size = pointed->GetSize();
    int64_t         offset = 0;
    MagoEE::Address addr = 0;

    if ( left->GetType()->IsPointer() )
    {
        addr = left->Value.Addr;
        offset = right->Value.Int64Value;
    }
    else
    {
        addr = right->Value.Addr;
        offset = left->Value.Int64Value;
    }

    val->Value.Addr = PtrOp( addr, (size * offset) );

    return val;
}

void PtrArithmeticTestElement::ToDText( std::wostringstream& stream )
{
    mChildren[0]->ToDText( stream );
    stream << " " << OpStr() << " ";
    mChildren[1]->ToDText( stream );
}


//----------------------------------------------------------------------------
//  PtrAddTestElement
//----------------------------------------------------------------------------

const wchar_t* PtrAddTestElement::GetPreOrPostOperator()
{
    return L"++";
}

MagoEE::Address PtrAddTestElement::PtrOp( MagoEE::Address addr, int64_t offset )
{
    return addr + offset;
}

const wchar_t* PtrAddTestElement::OpStr()
{
    return L"+";
}


//----------------------------------------------------------------------------
//  PtrSubTestElement
//----------------------------------------------------------------------------

const wchar_t* PtrSubTestElement::GetPreOrPostOperator()
{
    return L"--";
}

MagoEE::Address PtrSubTestElement::PtrOp( MagoEE::Address addr, int64_t offset )
{
    return addr - offset;
}

const wchar_t* PtrSubTestElement::OpStr()
{
    return L"-";
}


//----------------------------------------------------------------------------
//  PtrDiffTestElement
//----------------------------------------------------------------------------

void PtrDiffTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( !mChildren[0]->GetType()->IsPointer()
        && !mChildren[1]->GetType()->IsPointer() )
        throw L"Can only PtrDiff with pointers.";

    if ( !mChildren[0]->GetType()->Equals( mChildren[1]->GetType() ) )
        throw L"Operand types bad for this operation.";

    mType = typeEnv->GetAliasType( MagoEE::Tptrdiff_t );
}

std::shared_ptr<DataObj> PtrDiffTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<RValueObj>   val( new RValueObj() );
    std::shared_ptr<DataObj>     left( mChildren[0]->Evaluate( typeEnv, scope, dataEnv ) );
    std::shared_ptr<DataObj>     right( mChildren[1]->Evaluate( typeEnv, scope, dataEnv ) );

    Type*    elemType = left->GetType()->AsTypeNext()->GetNext();

    val->SetType( mType );

    val->Value.Int64Value = left->Value.Addr - right->Value.Addr;
    val->Value.Int64Value /= elemType->GetSize();          // make sure it's a signed divide

    return val;
}

void PtrDiffTestElement::ToDText( std::wostringstream& stream )
{
    mChildren[0]->ToDText( stream );
    stream << " - ";
    mChildren[1]->ToDText( stream );
}


//----------------------------------------------------------------------------
//  GroupTestElement
//----------------------------------------------------------------------------

void GroupTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    mType = mChildren[0]->GetType();
}

std::shared_ptr<DataObj> GroupTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    return mChildren[0]->Evaluate( typeEnv, scope, dataEnv );
}

void GroupTestElement::ToDText( std::wostringstream& stream )
{
    stream << "(";
    mChildren[0]->ToDText( stream );
    stream << ")";
}


//----------------------------------------------------------------------------
//  CastTestElement
//----------------------------------------------------------------------------

CastTestElement::CastTestElement()
:   mMod( MODnone )
{
}

    // TODO: is this all?
    // down to across
    //          integral    floating    pointer
    // integral X           X           X
    // floating X           X           ?
    // pointer  X           X           X

bool CastTestElement::CanImplicitCast( MagoEE::Type* typeFrom, MagoEE::Type* typeTo )
{
    return CanCast( typeFrom, typeTo );
}

bool CastTestElement::CanCast( MagoEE::Type* typeFrom, MagoEE::Type* typeTo )
{
    if (    (typeTo->IsIntegral() && typeFrom->IsIntegral())
        ||  (typeTo->IsIntegral() && typeFrom->IsFloatingPoint())
        ||  (typeTo->IsIntegral() && typeFrom->IsPointer())

        ||  (typeTo->IsFloatingPoint() && typeFrom->IsIntegral())
        ||  (typeTo->IsFloatingPoint() && typeFrom->IsFloatingPoint())
        ||  (typeTo->IsFloatingPoint() && typeFrom->IsPointer())

        ||  (typeTo->IsPointer() && typeFrom->IsIntegral())
        ||  (typeTo->IsPointer() && typeFrom->IsPointer())
        ||  (typeTo->IsPointer() && typeFrom->IsFloatingPoint())
        ||  (typeTo->IsPointer() && typeFrom->IsSArray())
        ||  (typeTo->IsPointer() && typeFrom->IsDArray())

        ||  (typeTo->IsDArray() && typeFrom->IsSArray())
        ||  (typeTo->IsDArray() && typeFrom->IsDArray())

        ||  (typeTo->IsBool() && typeFrom->IsSArray())
        ||  (typeTo->IsBool() && typeFrom->IsDArray())
        )
        return true;

    return false;
}

void CastTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( mChildren.size() > 1 )
    {
        if ( mMod != MODnone )
            throw L"Conflicting cast arguments.";

        RefPtr<Type>    typeTo = mChildren[0]->GetType();
        RefPtr<Type>    childType = mChildren[1]->GetType();

        if ( mChildren[1]->TrySetType( typeTo ) )
        {
            mType = typeTo;
        }
        else if ( CanCast( childType, typeTo ) )
        {
            mType = typeTo;
        }
        else
            throw L"Bad cast.";
    }
    else
    {
        RefPtr<Type>    childType = mChildren[0]->GetType();

        if ( mMod == childType->Mod )
        {
            // no change
            mType = childType;
        }
        else if ( (mMod & (MODshared | MODconst)) == (MODshared | MODconst) )
        {
            mType = childType->MakeSharedConst();
        }
        else if ( (mMod & MODshared) == MODshared )
        {
            mType = childType->MakeShared();
        }
        else if ( (mMod & MODconst) == MODconst )
        {
            mType = childType->MakeConst();
        }
        else if ( (mMod & MODinvariant) == MODinvariant )
        {
            mType = childType->MakeInvariant();
        }
        else
        {
            mType = childType->MakeMutable();
        }
    }
}

std::shared_ptr<DataObj> CastTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj> child;
    std::shared_ptr<DataObj> val( new RValueObj() );

    if ( mChildren.size() > 1 )
        child = mChildren[1]->Evaluate( typeEnv, scope, dataEnv );
    else
        child = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );

    val->SetType( mType );

    // TODO: it's not an l-value unless *casting to original type* and child was an l-value
    //      the point is that we're saying it's an r-value only, even though it could be an l-value
    //      see where we initialize val above
    AssignValue( child.get(), val.get() );

    return val;
}

void CastTestElement::ToDText( std::wostringstream& stream )
{
    stream << "cast(";

    if ( (mMod & MODshared) != 0 )
        stream << "shared ";
    if ( (mMod & MODconst) != 0 )
        stream << "const ";
    if ( (mMod & MODconst) != 0 )
        stream << "invariant ";

    if ( mChildren.size() > 1 )
        mChildren[0]->ToDText( stream );

    stream << ") ";

    if ( mChildren.size() > 1 )
        mChildren[1]->ToDText( stream );
    else
        mChildren[0]->ToDText( stream );
}

void CastTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"shared" ) == 0 )
        mMod = (MOD) (mMod | MODshared);
    else if ( _wcsicmp( name, L"const" ) == 0 )
        mMod = (MOD) (mMod | MODconst);
    else if ( _wcsicmp( name, L"invariant" ) == 0 )
        mMod = (MOD) (mMod | MODinvariant);
}

Real10 CastTestElement::ConvertToReal( DataObj* source )
{
    RefPtr<Type>    srcType = source->GetType();

    if ( srcType->IsComplex() )
    {
        return source->Value.Complex80Value.RealPart;
    }
    else if ( srcType->IsImaginary() )
    {
        Real10  v;
        v.Zero();
        return v;
    }
    else if ( srcType->IsReal() )
    {
        return source->Value.Float80Value;
    }
    else if ( srcType->IsIntegral() )
    {
        Real10  v;

        if ( srcType->IsSigned() )
            v.FromInt64( source->Value.Int64Value );
        else
            v.FromUInt64( source->Value.UInt64Value );

        return v;
    }
    else if ( srcType->IsPointer() )
    {
        Real10  v;
        v.FromUInt64( source->Value.Addr );
        return v;
    }
    else if ( srcType->IsSArray() )
    {
        MagoEE::Address addr = 0;
        if ( !source->GetAddress( addr ) )
            throw L"Couldn't get address of S-array.";

        Real10  v;
        v.FromUInt64( addr );
        return v;
    }
    else if ( srcType->IsDArray() )
    {
        Real10  v;
        v.FromUInt64( source->Value.Array.Addr );
        return v;
    }

    throw L"Can't evaluate to real.";
}

Real10 CastTestElement::ConvertToImaginary( DataObj* source )
{
    RefPtr<Type>    srcType = source->GetType();

    if ( srcType->IsComplex() )
    {
        return source->Value.Complex80Value.ImaginaryPart;
    }
    else if ( srcType->IsImaginary() )
    {
        return source->Value.Float80Value;
    }
    else
    {
        _ASSERT( srcType->IsPointer()
            || srcType->IsIntegral()
            || srcType->IsReal() );

        Real10  v;
        v.Zero();
        return v;
    }
}

bool CastTestElement::ConvertToBool( DataObj* source )
{
    return AndAndTestElement::EvalBool( source );
}

MagoEE::Address CastTestElement::ConvertToPointer( DataObj* source, Type* ptrType )
{
    RefPtr<Type>    srcType = source->GetType();
    MagoEE::Address addr = 0;

    if ( srcType->IsComplex() )
    {
        addr = source->Value.Complex80Value.RealPart.ToUInt64();
    }
    else if ( srcType->IsImaginary() )
    {
        addr = 0;
    }
    else if ( srcType->IsReal() )
    {
        addr = source->Value.Float80Value.ToUInt64();
    }
    else if ( srcType->IsIntegral() )
    {
        addr = source->Value.UInt64Value;
    }
    else if ( srcType->IsPointer() )
    {
        addr = source->Value.Addr;
    }
    else if ( srcType->IsSArray() )
    {
        if ( !source->GetAddress( addr ) )
            throw L"Couldn't get address of S-array.";
    }
    else if ( srcType->IsDArray() )
    {
        addr = source->Value.Array.Addr;
    }
    else
        throw L"Can't evaluate to pointer.";

    if ( ptrType->GetSize() == 4 )
        addr &= 0xFFFFFFFF;

    return addr;
}

void CastTestElement::ConvertToDArray( DataObj* source, DataObj* dest )
{
    RefPtr<Type>        destType = dest->GetType();
    RefPtr<Type>        srcType = source->GetType();
    MagoEE::Address     addr = 0;
    MagoEE::dlength_t   srcLen = 0;

    if ( !srcType->IsSArray() && !srcType->IsDArray() )
        throw L"Can't evaluate to D-array.";

    if ( srcType->IsSArray() )
    {
        if ( !source->GetAddress( addr ) )
            throw L"Couldn't get address of S-array.";

        srcLen = srcType->AsTypeSArray()->GetLength();
    }
    else if ( srcType->IsDArray() )
    {
        addr = source->Value.Array.Addr;
        srcLen = source->Value.Array.Length;
    }

    Type*               srcElemType = srcType->AsTypeNext()->GetNext();
    Type*               destElemType = destType->AsTypeNext()->GetNext();
    MagoEE::dlength_t   totalSize = srcLen * srcElemType->GetSize();
    MagoEE::dlength_t   destLen = 0;

    if ( (totalSize % destElemType->GetSize()) != 0 )
        throw L"Sizes don't line up for casting to D-array.";

    destLen = totalSize / destElemType->GetSize();

    if ( destLen == 0 )
        addr = 0;

    dest->Value.Array.Addr = addr;
    dest->Value.Array.Length = destLen;
}

void CastTestElement::AssignValue( DataObj* source, DataObj* dest )
{
    _ASSERT( source != NULL );
    _ASSERT( dest != NULL );

    RefPtr<Type>    destType = dest->GetType();
    RefPtr<Type>    srcType = source->GetType();

    if ( destType->IsBool() )
    {
        dest->Value.UInt64Value = ConvertToBool( source ) ? 1 : 0;
    }
    else if ( destType->IsComplex() )
    {
        dest->Value.Complex80Value = ArithmeticTestElement::ConvertToComplex( source );
    }
    else if ( destType->IsImaginary() )
    {
        dest->Value.Float80Value = ConvertToImaginary( source );
    }
    else if ( destType->IsReal() )
    {
        dest->Value.Float80Value = ConvertToReal( source );
    }
    else if ( destType->IsIntegral() )
    {
        if ( srcType->IsComplex() )
        {
            if ( (destType->GetSize() == 8) && !destType->IsSigned() )
                dest->Value.UInt64Value = source->Value.Complex80Value.RealPart.ToUInt64();
            else if ( (destType->GetSize() == 8) || (!destType->IsSigned() && destType->GetSize() == 4) )
                dest->Value.Int64Value = source->Value.Complex80Value.RealPart.ToInt64();
            else if ( (destType->GetSize() == 4) || (!destType->IsSigned() && destType->GetSize() == 2) )
                dest->Value.Int64Value = source->Value.Complex80Value.RealPart.ToInt32();
            else
                dest->Value.Int64Value = source->Value.Complex80Value.RealPart.ToInt16();
        }
        else if ( srcType->IsImaginary() )
        {
            dest->Value.Int64Value = 0;
        }
        else if ( srcType->IsReal() )
        {
            if ( (destType->GetSize() == 8) && !destType->IsSigned() )
                dest->Value.UInt64Value = source->Value.Float80Value.ToUInt64();
            else if ( (destType->GetSize() == 8) || (!destType->IsSigned() && destType->GetSize() == 4) )
                dest->Value.Int64Value = source->Value.Float80Value.ToInt64();
            else if ( (destType->GetSize() == 4) || (!destType->IsSigned() && destType->GetSize() == 2) )
                dest->Value.Int64Value = source->Value.Float80Value.ToInt32();
            else
                dest->Value.Int64Value = source->Value.Float80Value.ToInt16();
        }
        else if ( srcType->IsIntegral() )
        {
            dest->Value.UInt64Value = source->Value.UInt64Value;
        }
        else if ( srcType->IsPointer() )
        {
            dest->Value.UInt64Value = source->Value.Addr;
        }
        else if ( srcType->IsSArray() )
        {
            MagoEE::Address addr = 0;

            if ( !source->GetAddress( addr ) )
                throw L"Couldn't get address of S-array.";

            dest->Value.UInt64Value = addr;
        }
        else if ( srcType->IsDArray() )
        {
            dest->Value.UInt64Value = source->Value.Array.Addr;
        }

        ArithmeticTestElement::PromoteInPlace( dest );
    }
    else if ( destType->IsPointer() )
    {
        dest->Value.Addr = ConvertToPointer( source, destType );
    }
    else if ( destType->IsDArray() )
    {
        ConvertToDArray( source, dest );
    }
}


//----------------------------------------------------------------------------
//  BasicTypeTestElement
//----------------------------------------------------------------------------

BasicTypeTestElement::BasicTypeTestElement()
:   mMod( MODnone )
{
}

void BasicTypeTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    MagoEE::ENUMTY  ty = MagoEE::Tnone;

    if ( wcscmp( mName.c_str(), L"void" ) == 0 )
        ty = MagoEE::Tvoid;
    else if ( wcscmp( mName.c_str(), L"bool" ) == 0 )
        ty = MagoEE::Tbool;
    else if ( wcscmp( mName.c_str(), L"byte" ) == 0 )
        ty = MagoEE::Tint8;
    else if ( wcscmp( mName.c_str(), L"ubyte" ) == 0 )
        ty = MagoEE::Tuns8;
    else if ( wcscmp( mName.c_str(), L"short" ) == 0 )
        ty = MagoEE::Tint16;
    else if ( wcscmp( mName.c_str(), L"ushort" ) == 0 )
        ty = MagoEE::Tuns16;
    else if ( wcscmp( mName.c_str(), L"int" ) == 0 )
        ty = MagoEE::Tint32;
    else if ( wcscmp( mName.c_str(), L"uint" ) == 0 )
        ty = MagoEE::Tuns32;
    else if ( wcscmp( mName.c_str(), L"long" ) == 0 )
        ty = MagoEE::Tint64;
    else if ( wcscmp( mName.c_str(), L"ulong" ) == 0 )
        ty = MagoEE::Tuns64;
    else if ( wcscmp( mName.c_str(), L"float" ) == 0 )
        ty = MagoEE::Tfloat32;
    else if ( wcscmp( mName.c_str(), L"double" ) == 0 )
        ty = MagoEE::Tfloat64;
    else if ( wcscmp( mName.c_str(), L"real" ) == 0 )
        ty = MagoEE::Tfloat80;
    else if ( wcscmp( mName.c_str(), L"ifloat" ) == 0 )
        ty = MagoEE::Timaginary32;
    else if ( wcscmp( mName.c_str(), L"idouble" ) == 0 )
        ty = MagoEE::Timaginary64;
    else if ( wcscmp( mName.c_str(), L"ireal" ) == 0 )
        ty = MagoEE::Timaginary80;
    else if ( wcscmp( mName.c_str(), L"cfloat" ) == 0 )
        ty = MagoEE::Tcomplex32;
    else if ( wcscmp( mName.c_str(), L"cdouble" ) == 0 )
        ty = MagoEE::Tcomplex64;
    else if ( wcscmp( mName.c_str(), L"creal" ) == 0 )
        ty = MagoEE::Tcomplex80;
    else if ( wcscmp( mName.c_str(), L"char" ) == 0 )
        ty = MagoEE::Tchar;
    else if ( wcscmp( mName.c_str(), L"wchar" ) == 0 )
        ty = MagoEE::Twchar;
    else if ( wcscmp( mName.c_str(), L"dchar" ) == 0 )
        ty = MagoEE::Tdchar;
    else
        throw L"Unknown basic type.";

    mType = typeEnv->GetType( ty );

    _ASSERT( mType.Get() != NULL );
}

std::shared_ptr<DataObj> BasicTypeTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    throw L"Can't evaluate a type.";
}

void BasicTypeTestElement::ToDText( std::wostringstream& stream )
{
    if ( (mMod & MODshared) != 0 )
        stream << "shared ";
    if ( (mMod & MODconst) != 0 )
        stream << "const ";
    if ( (mMod & MODconst) != 0 )
        stream << "invariant ";

    stream << mName;
}

void BasicTypeTestElement::AddChild( Element* elem )
{
    throw L"BasicType can't have children.";
}

void BasicTypeTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"name" ) == 0 )
        mName = value;
    else if ( _wcsicmp( name, L"shared" ) == 0 )
        mMod = (MOD) (mMod | MODshared);
    else if ( _wcsicmp( name, L"const" ) == 0 )
        mMod = (MOD) (mMod | MODconst);
    else if ( _wcsicmp( name, L"invariant" ) == 0 )
        mMod = (MOD) (mMod | MODinvariant);
}

void BasicTypeTestElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}


//----------------------------------------------------------------------------
//  RefTypeTestElement
//----------------------------------------------------------------------------

RefTypeTestElement::RefTypeTestElement()
:   mMod( MODnone )
{
}

void RefTypeTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    HRESULT hr = S_OK;
    RefPtr<Declaration> decl;

    hr = scope->FindObject( mName.c_str(), decl.Ref() );
    if ( FAILED( hr ) )
        throw L"Couldn't find type.";

    if ( !decl->IsType() )
        throw L"Symbol is not a type.";

    if ( !decl->GetType( mType.Ref() ) )
        throw L"Couldn't get type.";
}

std::shared_ptr<DataObj> RefTypeTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    throw L"Can't evaluate a type.";
}

void RefTypeTestElement::ToDText( std::wostringstream& stream )
{
    if ( (mMod & MODshared) != 0 )
        stream << "shared ";
    if ( (mMod & MODconst) != 0 )
        stream << "const ";
    if ( (mMod & MODconst) != 0 )
        stream << "invariant ";

    stream << mName;
}

void RefTypeTestElement::AddChild( Element* elem )
{
    throw L"RefTypeTestElement can't have children.";
}

void RefTypeTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"shared" ) == 0 )
        mMod = (MOD) (mMod | MODshared);
    else if ( _wcsicmp( name, L"const" ) == 0 )
        mMod = (MOD) (mMod | MODconst);
    else if ( _wcsicmp( name, L"invariant" ) == 0 )
        mMod = (MOD) (mMod | MODinvariant);

    if ( _wcsicmp( name, L"name" ) == 0 )
    {
        mName = value;
    }
}

void RefTypeTestElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}


//----------------------------------------------------------------------------
//  PointerTypeTestElement
//----------------------------------------------------------------------------

PointerTypeTestElement::PointerTypeTestElement()
:   mMod( MODnone )
{
}

void PointerTypeTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    Type*   childType = mChildren[0]->GetType();

    HRESULT hr = typeEnv->NewPointer( childType, mType.Ref() );
    if ( FAILED( hr ) )
        throw L"Failed making new pointer type.";

    _ASSERT( mType.Get() != NULL );
}

std::shared_ptr<DataObj> PointerTypeTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    throw L"Can't evaluate a type.";
}

void PointerTypeTestElement::ToDText( std::wostringstream& stream )
{
    if ( (mMod & MODshared) != 0 )
        stream << "shared ";
    if ( (mMod & MODconst) != 0 )
        stream << "const ";
    if ( (mMod & MODconst) != 0 )
        stream << "invariant ";

    mChildren[0]->ToDText( stream );
    stream << "*";
}

void PointerTypeTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"shared" ) == 0 )
        mMod = (MOD) (mMod | MODshared);
    else if ( _wcsicmp( name, L"const" ) == 0 )
        mMod = (MOD) (mMod | MODconst);
    else if ( _wcsicmp( name, L"invariant" ) == 0 )
        mMod = (MOD) (mMod | MODinvariant);
}


//----------------------------------------------------------------------------
//  SArrayTypeTestElement
//----------------------------------------------------------------------------

SArrayTypeTestElement::SArrayTypeTestElement()
:   mMod( MODnone ),
    mLen( 0 )
{
}

void SArrayTypeTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    Type*   childType = mChildren[0]->GetType();

    HRESULT hr = typeEnv->NewSArray( childType, mLen, mType.Ref() );
    if ( FAILED( hr ) )
        throw L"Failed making new S-Array type.";

    _ASSERT( mType.Get() != NULL );
}

std::shared_ptr<DataObj> SArrayTypeTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    throw L"Can't evaluate a type.";
}

void SArrayTypeTestElement::ToDText( std::wostringstream& stream )
{
    if ( (mMod & MODshared) != 0 )
        stream << "shared ";
    if ( (mMod & MODconst) != 0 )
        stream << "const ";
    if ( (mMod & MODconst) != 0 )
        stream << "invariant ";

    mChildren[0]->ToDText( stream );
    stream << "[" << mLen << "]";
}

void SArrayTypeTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"shared" ) == 0 )
        mMod = (MOD) (mMod | MODshared);
    else if ( _wcsicmp( name, L"const" ) == 0 )
        mMod = (MOD) (mMod | MODconst);
    else if ( _wcsicmp( name, L"invariant" ) == 0 )
        mMod = (MOD) (mMod | MODinvariant);

    if ( _wcsicmp( name, L"length" ) == 0 )
        mLen = wcstoul( value, NULL, 10 );
}


//----------------------------------------------------------------------------
//  DArrayTypeTestElement
//----------------------------------------------------------------------------

DArrayTypeTestElement::DArrayTypeTestElement()
:   mMod( MODnone )
{
}

void DArrayTypeTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    Type*   childType = mChildren[0]->GetType();

    HRESULT hr = typeEnv->NewDArray( childType, mType.Ref() );
    if ( FAILED( hr ) )
        throw L"Failed making new D-Array type.";

    _ASSERT( mType.Get() != NULL );
}

std::shared_ptr<DataObj> DArrayTypeTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    throw L"Can't evaluate a type.";
}

void DArrayTypeTestElement::ToDText( std::wostringstream& stream )
{
    if ( (mMod & MODshared) != 0 )
        stream << "shared ";
    if ( (mMod & MODconst) != 0 )
        stream << "const ";
    if ( (mMod & MODconst) != 0 )
        stream << "invariant ";

    mChildren[0]->ToDText( stream );
    stream << "[]";
}

void DArrayTypeTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"shared" ) == 0 )
        mMod = (MOD) (mMod | MODshared);
    else if ( _wcsicmp( name, L"const" ) == 0 )
        mMod = (MOD) (mMod | MODconst);
    else if ( _wcsicmp( name, L"invariant" ) == 0 )
        mMod = (MOD) (mMod | MODinvariant);
}


//----------------------------------------------------------------------------
//  AArrayTypeTestElement
//----------------------------------------------------------------------------

AArrayTypeTestElement::AArrayTypeTestElement()
:   mMod( MODnone )
{
}

void AArrayTypeTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    Type*   elemType = mChildren[0]->GetType();
    Type*   indexType = mChildren[1]->GetType();

    HRESULT hr = typeEnv->NewAArray( elemType, indexType, mType.Ref() );
    if ( FAILED( hr ) )
        throw L"Failed making new A-Array type.";

    _ASSERT( mType.Get() != NULL );
}

std::shared_ptr<DataObj> AArrayTypeTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    throw L"Can't evaluate a type.";
}

void AArrayTypeTestElement::ToDText( std::wostringstream& stream )
{
    if ( (mMod & MODshared) != 0 )
        stream << "shared ";
    if ( (mMod & MODconst) != 0 )
        stream << "const ";
    if ( (mMod & MODconst) != 0 )
        stream << "invariant ";

    mChildren[0]->ToDText( stream );
    stream << "[";
    mChildren[1]->ToDText( stream );
    stream << "]";
}

void AArrayTypeTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"shared" ) == 0 )
        mMod = (MOD) (mMod | MODshared);
    else if ( _wcsicmp( name, L"const" ) == 0 )
        mMod = (MOD) (mMod | MODconst);
    else if ( _wcsicmp( name, L"invariant" ) == 0 )
        mMod = (MOD) (mMod | MODinvariant);
}


//----------------------------------------------------------------------------
//  VerifyTestElement
//----------------------------------------------------------------------------

void VerifyTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    try
    {
        ContainerTestElement::Bind( typeEnv, scope, dataEnv );
    }
    catch ( const wstring& msg )
    {
        ThrowError( msg.c_str() );
    }
    catch ( const wchar_t* msg )
    {
        ThrowError( msg );
    }

    if ( mChildren.size() < 2 )
        ThrowError( L"Verify needs at least two children." );

    if ( dynamic_cast<TypedValueTestElement*>( mChildren[1].Get() ) == NULL )
        ThrowError( L"needs a TypedValue." );
}

void TestElement::VerifyCompareValues( ITypeEnv* typeEnv, const std::shared_ptr<DataObj>& val, const std::shared_ptr<DataObj>& refVal )
{
    if ( !val->GetType()->Equals( refVal->GetType() ) )
    {
        wstring text = L"Types not equal.";
        wstring typeStr;

        text.append( L" val.type = " );
        val->GetType()->ToString( typeStr );
        text.append( typeStr );

        text.append( L"; refVal.type = " );
        typeStr.clear();
        refVal->GetType()->ToString( typeStr );
        text.append( typeStr );

        throw text;
    }

    bool    equal = true;

    if ( val->GetType()->IsComplex() )
    {
        bool    imEq = false;
        bool    reEq = false;
        MagoEE::ENUMTY  ty = MagoEE::Tnone;
        RefPtr<Type>    type;

        switch ( val->GetType()->GetSize() )
        {
        case 8: ty = MagoEE::Tfloat32;  break;
        case 16: ty = MagoEE::Tfloat64;  break;
        case 20: ty = MagoEE::Tfloat80;  break;
        }

        type = typeEnv->GetType( ty );

        imEq = FloatEqual( val->Value.Complex80Value.ImaginaryPart, refVal->Value.Complex80Value.ImaginaryPart, type.Get() );
        reEq = FloatEqual( val->Value.Complex80Value.RealPart, refVal->Value.Complex80Value.RealPart, type.Get() );

        equal = imEq && reEq;
    }
    else if ( val->GetType()->IsFloatingPoint() )
    {
        equal = FloatEqual( val->Value.Float80Value, refVal->Value.Float80Value, val->GetType() );
    }
    else if ( val->GetType()->IsIntegral() )
    {
        equal = val->Value.UInt64Value == refVal->Value.UInt64Value;
    }
    else if ( val->GetType()->IsPointer() )
    {
        equal = val->Value.Addr == refVal->Value.Addr;
    }
    else if ( val->GetType()->IsDArray() )
    {
        equal = (val->Value.Array.Addr == refVal->Value.Array.Addr)
            && (val->Value.Array.Length == refVal->Value.Array.Length);
    }
    // don't worry about any other types, we can't get their values, so they can't be compared

    // TODO: other types: delegates, A-array

    if ( !equal )
    {
        wstring text = L"Values not equal.";

        text.append( L" val = " );
        text.append( val->ToString() );
        text.append( L"; refVal = " );
        text.append( refVal->ToString() );
        throw text;
    }
}

std::shared_ptr<DataObj> VerifyTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    //_ASSERT( _wcsicmp( mId.c_str(), L"37.1" ) != 0 );
    std::shared_ptr<DataObj> val;

    try
    {
        if ( gAppSettings.SelfTest )
        {
            val = EvaluateSelf( typeEnv, scope, dataEnv );
        }
        else
        {
            val = EvaluateEED( typeEnv, scope, dataEnv );
        }
    }
    catch ( const wstring& msg )
    {
        ThrowError( msg.c_str() );
    }
    catch ( const wchar_t* msg )
    {
        ThrowError( msg );
    }

    return val;
}

void VerifyCompareMemory( DataEnv* oldEnv, DataEnv* newEnv, size_t addr, size_t length )
{
    int cmp = memcmp( oldEnv->GetBuffer() + addr, newEnv->GetBuffer() + addr, length );

    if ( cmp != 0 )
    {
        wstring msg = L"Memory mismatch ";

        // memcmp can do the fast comparison, which should normally result in a match
        // now that we know it doesn't match, use a slower comparison to find where it's mismatched
        for ( size_t i = addr; i < (addr + length); i++ )
        {
            if ( oldEnv->GetBuffer()[i] != newEnv->GetBuffer()[i] )
            {
                size_t  j = 0;

                // find the next byte where they match, so we know where this non-matching part ends
                for ( j = i + 1; j < (addr + length); j++ )
                {
                    if ( oldEnv->GetBuffer()[j] == newEnv->GetBuffer()[j] )
                        break;
                }

                wchar_t buf[40 + 1] = L"";
                swprintf_s( buf, L"(%d bytes at %08x)", (j - i), i );
                msg.append( buf );

                // we want to find all the patches that don't match
                i = j + 1;  // we know that the bytes at j match or are past the end
            }
        }
        msg.append( L"." );
        throw msg;
    }
}

std::shared_ptr<DataObj> VerifyTestElement::EvaluateSelf( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj> val;
    std::shared_ptr<DataObj> refVal;
    bool    hasMemChecks = mChildren.size() > 2;

    if ( hasMemChecks && (gAppSettings.TestEvalDataEnv != NULL) )
    {
        DataEnv         envCopy( *gAppSettings.TestEvalDataEnv );
        size_t          lastAddr = 0;
        List::iterator  it = mChildren.begin();
        size_t          len = 0;
        size_t          totalBufLen = envCopy.GetBufferLimit() - envCopy.GetBuffer();
        DataEnv*        oldEnv = &envCopy;
        DataEnv*        newEnv = gAppSettings.TestEvalDataEnv;

        if ( gAppSettings.TempAssignment )
            swap( oldEnv, newEnv );

        val = mChildren[0]->Evaluate( typeEnv, scope, newEnv );

        it++;       // skip the computed value whose reference is below
        it++;       // skip the reference value for computation above

        for ( ; it != mChildren.end(); it++ )
        {
            MemoryValueTestElement* memVal = dynamic_cast<MemoryValueTestElement*>( (*it).Get() );

            if ( memVal == NULL )
                throw L"MemoryValue expected.";

            memVal->Evaluate( typeEnv, scope, newEnv );

            // mem vals to check can overlap, but the starting addrs always have to be in order
            if ( memVal->GetAddress() <= lastAddr )
            {
                size_t  newLastAddr = memVal->GetAddress() + memVal->GetType()->GetSize();
                if ( newLastAddr > lastAddr )
                    lastAddr = newLastAddr;
            }
            else
            {
                len = memVal->GetAddress() - lastAddr;

                VerifyCompareMemory( gAppSettings.TestEvalDataEnv, &envCopy, lastAddr, len );
                lastAddr = memVal->GetAddress() + memVal->GetType()->GetSize();
            }
        }

        len = totalBufLen - lastAddr;

        VerifyCompareMemory( gAppSettings.TestEvalDataEnv, &envCopy, lastAddr, len );
    }
    else
        val = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );

    refVal = mChildren[1]->Evaluate( typeEnv, scope, dataEnv );

    VerifyCompareValues( typeEnv, val, refVal );

    return val;
}

std::shared_ptr<DataObj> VerifyTestElement::EvaluateEED( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj> val;
    std::shared_ptr<DataObj> refVal;
    bool    hasMemChecks = mChildren.size() > 2;

    if ( hasMemChecks && (gAppSettings.TestEvalDataEnv != NULL) )
    {
        DataEnv             envCopy( *gAppSettings.TestEvalDataEnv );
        size_t              totalBufLen = envCopy.GetBufferLimit() - envCopy.GetBuffer();
        DataEnv*            refEnv = gAppSettings.TestEvalDataEnv;
        auto_ptr<DataEnv>   allocRefEnv;

        if ( gAppSettings.TempAssignment )
        {
            allocRefEnv.reset( new DataEnv( *gAppSettings.TestEvalDataEnv ) );
            refEnv = allocRefEnv.get();
        }

        refVal = mChildren[0]->Evaluate( typeEnv, scope, refEnv );

        val = EvaluateText( typeEnv, scope, &envCopy );

        for ( List::iterator it = mChildren.begin(); it != mChildren.end(); it++ )
        {
            MemoryValueTestElement* memVal = dynamic_cast<MemoryValueTestElement*>( (*it).Get() );

            if ( memVal != NULL )
                memVal->Evaluate( typeEnv, scope, &envCopy );
        }

        VerifyCompareMemory( refEnv, &envCopy, 0, totalBufLen );
    }
    else
    {
        refVal = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );

        val = EvaluateText( typeEnv, scope, dataEnv );
    }

    VerifyCompareValues( typeEnv, val, refVal );

    return val;
}

std::shared_ptr<DataObj> VerifyTestElement::EvaluateText( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    wostringstream      stream;

    mChildren[0]->ToDText( stream );

    HRESULT hr = S_OK;
    RefPtr<MagoEE::NameTable>           nameTable;
    RefPtr<MagoEE::IEEDParsedExpr>      expr;

    hr = MagoEE::MakeNameTable( nameTable.Ref() );
    if ( FAILED( hr ) )
        ThrowError( L"Couldn't allocate name table." );

    hr = MagoEE::ParseText( stream.str().c_str(), typeEnv, nameTable, expr.Ref() );
    if ( FAILED( hr ) )
        ThrowError( L"Couldn't parse expression text." );

    MagoEE::EvalOptions options = MagoEE::EvalOptions::defaults;
    MagoEE::EvalResult  result = { 0 };
    DataEnvBinder       binder( dataEnv, scope );

    options.AllowAssignment = gAppSettings.AllowAssignment;

    hr = expr->Bind( options, &binder );
    if ( FAILED( hr ) )
    {
        wstring msg = L"Couldn't bind expression. ";
        msg.append( GetErrorString( hr ) );
        ThrowError( msg.c_str() );
    }

    hr = expr->Evaluate( options, &binder, result );
    if ( FAILED( hr ) )
    {
        wstring msg = L"Couldn't evaluate expression. ";
        msg.append( GetErrorString( hr ) );
        ThrowError( msg.c_str() );
    }

    std::shared_ptr<DataObj> val;

    if ( result.ObjVal.Addr != 0 )
        val.reset( new LValueObj( NULL, result.ObjVal.Addr ) );
    else
        val.reset( new RValueObj() );

    val->SetType( result.ObjVal._Type );
    val->Value = result.ObjVal.Value;

    return val;
}

void VerifyTestElement::ThrowError( const wchar_t* msg )
{
    wstring text = L"Verify: ";

    text.append( mId );
    text.append( L": " );
    text.append( msg );
    throw text;
}

bool TestElement::FloatEqual( const Real10& left, const Real10& right, MagoEE::Type* type )
{
    // at this point left's and right's types are the same
    bool    equal = false;

    switch ( type->GetSize() )
    {
    case 4:
        {
            float   l = left.ToFloat();
            float   r = right.ToFloat();
            float   nan = numeric_limits<float>::quiet_NaN();

            uint32_t    lSign = (*(uint32_t*) &l) & 0x80000000;
            uint32_t    rSign = (*(uint32_t*) &r) & 0x80000000;

            // clear the sign bit, because it doesn't matter for NaN comparison
            *(uint32_t*) &l &= 0x7fffffff;
            *(uint32_t*) &r &= 0x7fffffff;

            if ( lSign != rSign )
                equal = false;
            else if ( (memcmp( &l, &nan, sizeof nan ) == 0) && (memcmp( &r, &nan, sizeof nan ) == 0) )
                equal = true;
            else
                equal = l == r;
        }
        break;

    case 8:
        {
            double  l = left.ToDouble();
            double  r = right.ToDouble();
            double  nan = numeric_limits<double>::quiet_NaN();

            uint64_t    lSign = (*(uint64_t*) &l) & 0x8000000000000000;
            uint64_t    rSign = (*(uint64_t*) &r) & 0x8000000000000000;

            // clear the sign bit, because it doesn't matter for NaN comparison
            *(uint64_t*) &l &= 0x7fffffffffffffff;
            *(uint64_t*) &r &= 0x7fffffffffffffff;

            if ( lSign != rSign )
                equal = false;
            else if ( (memcmp( &l, &nan, sizeof nan ) == 0) && (memcmp( &r, &nan, sizeof nan ) == 0) )
                equal = true;
            else
                equal = l == r;
        }
        break;

    case 10:
        {
            if ( left.IsNan() && right.IsNan() && (left.GetSign() == right.GetSign()) )
                equal = true;
            else
            {
                uint16_t    status = Real10::Compare( left, right );

                equal = Real10::IsEqual( status );
            }
        }
        break;

    default:
        _ASSERT( false );
    }

    return equal;
}

void VerifyTestElement::ToDText( std::wostringstream& stream )
{
    mChildren[0]->ToDText( stream );
}

void VerifyTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"id" ) == 0 )
    {
        mId = value;
    }
}


//----------------------------------------------------------------------------
//  TypedValueTestElement
//----------------------------------------------------------------------------

void TypedValueTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( dynamic_cast<TypeTestElement*>( mChildren[0].Get() ) != NULL )
        mType = mChildren[0]->GetType();
    else if ( (mChildren.size() > 1) && (dynamic_cast<TypeTestElement*>( mChildren[1].Get() ) != NULL) )
        mType = mChildren[1]->GetType();
    else
        throw L"Type expected.";
}

std::shared_ptr<DataObj> TypedValueTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>   val;

    if ( dynamic_cast<TypeTestElement*>( mChildren[0].Get() ) == NULL )
    {
        mChildren[0]->TrySetType( mType );
        val = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );
    }
    else if ( (mChildren.size() > 1) && (dynamic_cast<TypeTestElement*>( mChildren[1].Get() ) == NULL) )
    {
        mChildren[1]->TrySetType( mType );
        val = mChildren[1]->Evaluate( typeEnv, scope, dataEnv );
    }
    else
        val.reset( new RValueObj() );

    val->SetType( mType );

    if ( gAppSettings.PromoteTypedValue )
    {
        if ( mType->IsIntegral() )
            ArithmeticTestElement::PromoteInPlace( val.get() );
    }

    return val;
}

void TypedValueTestElement::ToDText( std::wostringstream& stream )
{
    if ( dynamic_cast<TypeTestElement*>( mChildren[0].Get() ) == NULL )
        mChildren[0]->ToDText( stream );
    else if ( (mChildren.size() > 1) && (dynamic_cast<TypeTestElement*>( mChildren[1].Get() ) == NULL) )
        mChildren[1]->ToDText( stream );
}


//----------------------------------------------------------------------------
//  MemoryValueTestElement
//----------------------------------------------------------------------------

MemoryValueTestElement::MemoryValueTestElement()
:   mAddr( 0 )
{
}

size_t MemoryValueTestElement::GetAddress()
{
    return mAddr;
}

void MemoryValueTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( mAddr == 0 )
        throw L"Can't have memory value at address 0.";

    if ( mChildren.size() != 2 )
        throw L"MemoryValue must have two children.";
    if ( dynamic_cast<TypeTestElement*>( mChildren[0].Get() ) == NULL )
        throw L"Type expected.";

    mType = mChildren[0]->GetType();
}

std::shared_ptr<DataObj> MemoryValueTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>   val;
    std::shared_ptr<DataObj>   refVal;

    refVal = mChildren[1]->Evaluate( typeEnv, scope, dataEnv );

    refVal->SetType( mType );

    if ( gAppSettings.PromoteTypedValue )
    {
        if ( mType->IsIntegral() )
            ArithmeticTestElement::PromoteInPlace( refVal.get() );
    }

    val = dataEnv->GetValue( (MagoEE::Address) mAddr, mType );

    VerifyCompareValues( typeEnv, val, refVal );

    return val;
}

void MemoryValueTestElement::ToDText( std::wostringstream& stream )
{
}

void MemoryValueTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"address" ) == 0 )
    {
        if ( (value[0] == L'0') && (value[1] == L'x') )
            mAddr = wcstoul( value + 2, NULL, 16 );
        else
            mAddr = wcstoul( value, NULL, 10 );
    }
}


//----------------------------------------------------------------------------
//  TestTestElement
//----------------------------------------------------------------------------

std::shared_ptr<DataObj> TestTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>   val;

    for ( List::iterator it = mChildren.begin();
        it != mChildren.end();
        it++ )
    {
        val = (*it)->Evaluate( typeEnv, scope, dataEnv );
    }

    return val;
}

void TestTestElement::ToDText( std::wostringstream& stream )
{
    for ( List::iterator it = mChildren.begin();
        it != mChildren.end();
        it++ )
    {
        (*it)->ToDText( stream );
        stream << endl;
    }
}


//----------------------------------------------------------------------------
//  IdTestElement
//----------------------------------------------------------------------------

IdTestElement::IdTestElement()
:   mIsGlobal( false )
{
}

RefPtr<MagoEE::Declaration> IdTestElement::GetDeclaration()
{
    return mDecl;
}

void IdTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    HRESULT hr = S_OK;

    hr = scope->FindObject( mName.c_str(), mDecl.Ref() );
    if ( FAILED( hr ) )
    {
        RefPtr<Declaration> thisDecl = dataEnv->GetThis();
        if ( thisDecl == NULL )
            throw L"Couldn't find named object.";

        RefPtr<Type>    thisType;
        if ( !thisDecl->GetType( thisType.Ref() ) )
            throw L"Couldn't find named object.";

        if ( thisType->IsPointer() )
            thisType = thisType->AsTypeNext()->GetNext();

        if ( thisType->AsTypeStruct() == NULL )
            throw L"Couldn't find named object.";

        mDecl = thisType->AsTypeStruct()->FindObject( mName.c_str() );
        if ( mDecl == NULL )
            throw L"Couldn't find named object.";
    }

    bool    ok = mDecl->GetType( mType.Ref() );
}

std::shared_ptr<DataObj> IdTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>   val;

    if ( mDecl->IsConstant() || mDecl->IsVar() )
    {
        val = dataEnv->GetValue( mDecl.Get() );
    }
    else if ( mDecl->IsField() )
    {
        RefPtr<MagoEE::Declaration> thisDecl;
        std::shared_ptr<DataObj>  thisVal;
        MagoEE::Address             addr = 0;
        int                         offset = 0;
        RefPtr<Type>                type;

        thisDecl = dataEnv->GetThis();
        thisVal = dataEnv->GetValue( thisDecl.Get() );

        addr = thisVal->Value.Addr;
        mDecl->GetType( type.Ref() );

        mDecl->GetOffset( offset );
        addr += offset;

        val = dataEnv->GetValue( addr, type.Get() );
    }
    else
        throw L"Named object has no value.";

    return val;
}

void IdTestElement::ToDText( std::wostringstream& stream )
{
    if ( mIsGlobal )
        stream << ".";

    stream << mName;
}

void IdTestElement::AddChild( Element* elem )
{
    throw L"ID can't have children.";
}

void IdTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"name" ) == 0 )
    {
        mName = value;
    }
    else if ( _wcsicmp( name, L"global" ) == 0 )
    {
        mIsGlobal = true;
    }
}

void IdTestElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}


//----------------------------------------------------------------------------
//  MemberTestElement
//----------------------------------------------------------------------------

RefPtr<MagoEE::Declaration> MemberTestElement::GetDeclaration()
{
    return mDecl;
}

void MemberTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    HRESULT hr = S_OK;

    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    RefPtr<MagoEE::Declaration> parentDecl = mChildren[0]->GetDeclaration();

    if ( parentDecl.Get() != NULL )
    {
        hr = parentDecl->FindObject( mName.c_str(), mDecl.Ref() );
    }

    if ( mDecl.Get() == NULL )
    {
        Type*   parentType = mChildren[0]->GetType();

        if ( parentType != NULL )
        {
            if ( parentType->IsPointer() )
                parentType = parentType->AsTypeNext()->GetNext().Get();

            MagoEE::ITypeStruct* parentStruct = parentType->AsTypeStruct();

            if ( parentStruct == NULL )
                throw L"Type doesn't support members.";

            mDecl = parentStruct->FindObject( mName.c_str() );
        }
    }

    if ( mDecl.Get() == NULL )
        throw L"Couldn't find member.";

    bool    ok = mDecl->GetType( mType.Ref() );
}

std::shared_ptr<DataObj> MemberTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>   val;

    if ( mDecl->IsConstant() || mDecl->IsVar() )
    {
        val = dataEnv->GetValue( mDecl.Get() );
    }
    else if ( mDecl->IsField() )
    {
        std::shared_ptr<DataObj> parent = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );
        MagoEE::Address     parentAddr = 0;
        MagoEE::Address     addr = 0;
        RefPtr<Type>        type;
        int                 offset = 0;

        if ( parent->GetType()->IsPointer() )
        {
            parentAddr = parent->Value.Addr;
        }
        else
        {
            if ( !parent->GetAddress( parentAddr ) )
                throw L"Can't reference field with no address.";
        }

        if ( !mDecl->GetOffset( offset ) )
            throw L"Can't reference field with no offset.";

        mDecl->GetType( type.Ref() );

        addr = parentAddr + offset;

        val = dataEnv->GetValue( addr, type.Get() );
    }
    else
        throw L"Named object has no value.";

    return val;
}

void MemberTestElement::ToDText( std::wostringstream& stream )
{
    mChildren[0]->ToDText( stream );
    stream << "." << mName;
}

void MemberTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"name" ) == 0 )
    {
        mName = value;
    }
}


//----------------------------------------------------------------------------
//  StdPropertyTestElement
//----------------------------------------------------------------------------

void StdPropertyTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    RefPtr<Declaration> parentDecl = mChildren[0]->GetDeclaration();
    Type*               parentType = mChildren[0]->GetType();

    if ( _wcsicmp( mName.c_str(), L"sizeof" ) == 0 )
    {
        if ( (parentType != NULL) || (parentDecl.Get() != NULL && parentDecl->IsType()) )
        {
            mType = typeEnv->GetType( MagoEE::Tuns32 );
        }
        else
            throw L"sizeof property not allowed.";
    }
    else if ( _wcsicmp( mName.c_str(), L"offsetof" ) == 0 )
    {
        if ( parentDecl.Get() != NULL && parentDecl->IsField() )
        {
            mType = typeEnv->GetType( MagoEE::Tuns32 );
        }
        else
            throw L"offsetof property not allowed.";
    }
    else if ( _wcsicmp( mName.c_str(), L"re" ) == 0 )
    {
        if ( (parentType != NULL) && parentType->IsFloatingPoint() )
        {
            MagoEE::ENUMTY  ty = MagoEE::Tnone;

            switch ( parentType->GetBackingTy() )
            {
            case MagoEE::Tfloat32:  case MagoEE::Timaginary32:  case MagoEE::Tcomplex32:    ty = MagoEE::Tfloat32;  break;
            case MagoEE::Tfloat64:  case MagoEE::Timaginary64:  case MagoEE::Tcomplex64:    ty = MagoEE::Tfloat64;  break;
            case MagoEE::Tfloat80:  case MagoEE::Timaginary80:  case MagoEE::Tcomplex80:    ty = MagoEE::Tfloat80;  break;
            }

            mType = typeEnv->GetType( ty );
        }
        else
            throw L"re property not allowed.";
    }
    else if ( _wcsicmp( mName.c_str(), L"im" ) == 0 )
    {
        if ( (parentType != NULL) && parentType->IsFloatingPoint() )
        {
            MagoEE::ENUMTY  ty = MagoEE::Tnone;

            switch ( parentType->GetBackingTy() )
            {
            case MagoEE::Tfloat32:  case MagoEE::Timaginary32:  case MagoEE::Tcomplex32:    ty = MagoEE::Timaginary32;  break;
            case MagoEE::Tfloat64:  case MagoEE::Timaginary64:  case MagoEE::Tcomplex64:    ty = MagoEE::Timaginary64;  break;
            case MagoEE::Tfloat80:  case MagoEE::Timaginary80:  case MagoEE::Tcomplex80:    ty = MagoEE::Timaginary80;  break;
            }

            mType = typeEnv->GetType( ty );
        }
        else
            throw L"im property not allowed.";
    }
    else if ( _wcsicmp( mName.c_str(), L"length" ) == 0 )
    {
        if ( (parentType != NULL) && (parentType->IsSArray() || parentType->IsDArray()) )
        {
            mType = typeEnv->GetType( MagoEE::Tuns32 );
        }
        else
            throw L"length property not allowed.";
    }
    else if ( _wcsicmp( mName.c_str(), L"ptr" ) == 0 )
    {
        if ( (parentType != NULL) && (parentType->IsSArray() || parentType->IsDArray()) )
        {
            typeEnv->NewPointer( parentType->AsTypeNext()->GetNext(), mType.Ref() );
        }
        else
            throw L"length property not allowed.";
    }
    else
        throw L"Couldn't find standard property.";
}

std::shared_ptr<DataObj> StdPropertyTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>   val;

    RefPtr<Declaration> parentDecl = mChildren[0]->GetDeclaration();
    Type*               parentType = mChildren[0]->GetType();

    val.reset( new RValueObj() );
    val->SetType( mType );

    if ( _wcsicmp( mName.c_str(), L"sizeof" ) == 0 )
    {
        if ( parentType != NULL )
            val->Value.UInt64Value = parentType->GetSize();
        else if ( parentDecl.Get() != NULL && parentDecl->IsType() )
        {
            uint32_t    size = 0;
            parentDecl->GetSize( size );
            val->Value.UInt64Value = size;
        }
        else
            throw L"sizeof property not allowed.";
    }
    else if ( _wcsicmp( mName.c_str(), L"offsetof" ) == 0 )
    {
        if ( parentDecl.Get() != NULL && parentDecl->IsField() )
        {
            int offset = 0;
            parentDecl->GetOffset( offset );
            val->Value.Int64Value = offset;
        }
        else
            throw L"offsetof property not allowed.";
    }
    else if ( _wcsicmp( mName.c_str(), L"re" ) == 0 )
    {
        if ( (parentType != NULL) && parentType->IsFloatingPoint() )
        {
            std::shared_ptr<DataObj> parentVal = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );
            Real10& r = val->Value.Float80Value;

            if ( parentVal->GetType()->IsReal() )
                r = parentVal->Value.Float80Value;
            else if ( parentVal->GetType()->IsImaginary() )
                r.Zero();
            else
            {
                _ASSERT( parentVal->GetType()->IsComplex() );
                r = parentVal->Value.Complex80Value.RealPart;
            }
        }
        else
            throw L"re property not allowed.";
    }
    else if ( _wcsicmp( mName.c_str(), L"im" ) == 0 )
    {
        if ( (parentType != NULL) && parentType->IsFloatingPoint() )
        {
            std::shared_ptr<DataObj> parentVal = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );
            Real10& r = val->Value.Float80Value;

            if ( parentVal->GetType()->IsReal() )
                r.Zero();
            else if ( parentVal->GetType()->IsImaginary() )
                r = parentVal->Value.Float80Value;
            else
            {
                _ASSERT( parentVal->GetType()->IsComplex() );
                r = parentVal->Value.Complex80Value.ImaginaryPart;
            }
        }
        else
            throw L"im property not allowed.";
    }
    else if ( _wcsicmp( mName.c_str(), L"length" ) == 0 )
    {
        if ( (parentType != NULL) && (parentType->IsSArray() || parentType->IsDArray()) )
        {
            if ( parentType->IsSArray() )
                val->Value.UInt64Value = parentType->AsTypeSArray()->GetLength();
            else
            {
                std::shared_ptr<DataObj> parentVal = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );

                val->Value.UInt64Value = parentVal->Value.Array.Length;
            }
        }
        else
            throw L"length property not allowed.";
    }
    else if ( _wcsicmp( mName.c_str(), L"ptr" ) == 0 )
    {
        if ( (parentType != NULL) && (parentType->IsSArray() || parentType->IsDArray()) )
        {
            std::shared_ptr<DataObj> parentVal = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );

            if ( parentType->IsSArray() )
            {
                if ( !parentVal->GetAddress( val->Value.Addr ) )
                    throw L"Couldn't get S-Array address.";
            }
            else
            {
                val->Value.Addr = parentVal->Value.Array.Addr;
            }
        }
        else
            throw L"length property not allowed.";
    }
    else
        throw L"Couldn't find standard property.";

    return val;
}

void StdPropertyTestElement::ToDText( std::wostringstream& stream )
{
    mChildren[0]->ToDText( stream );
    stream << "." << mName;
}

void StdPropertyTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"name" ) == 0 )
    {
        mName = value;
    }
}


class IndexDataEnv : public IValueEnv
{
    IValueEnv*          mParentEnv;
    MagoEE::dlength_t   mArrayLen;

public:
    IndexDataEnv( IValueEnv* parentEnv, MagoEE::dlength_t arrayLen )
        : mParentEnv( parentEnv ), mArrayLen( arrayLen )
    { }
    virtual std::shared_ptr<DataObj> GetValue( MagoEE::Declaration* decl )
    { return mParentEnv->GetValue( decl ); }
    virtual std::shared_ptr<DataObj> GetValue( MagoEE::Address address, MagoEE::Type* type )
    { return mParentEnv->GetValue( address, type ); }
    virtual void SetValue( MagoEE::Address address, DataObj* obj )
    { mParentEnv->SetValue( address, obj ); }
    virtual RefPtr<MagoEE::Declaration> GetThis()
    { return mParentEnv->GetThis(); }
    virtual RefPtr<MagoEE::Declaration> GetSuper()
    { return mParentEnv->GetSuper(); }
    virtual bool GetArrayLength( MagoEE::dlength_t& length )
    { length = mArrayLen; return true; }
};


//----------------------------------------------------------------------------
//  IndexTestElement
//----------------------------------------------------------------------------

void IndexTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( mChildren.size() != 2 )
        throw L"Index needs two children.";

    Type*   parentType = mChildren[0]->GetType();
    Type*   indexType = mChildren[1]->GetType();

    if ( !indexType->IsIntegral() )
        throw L"Only integrals can index.";
    if ( !parentType->IsPointer() && !parentType->IsSArray() && !parentType->IsDArray() )
        throw L"Can only index arrays.";

    _ASSERT( parentType->AsTypeNext() != NULL );
    RefPtr<Type>    voidType = typeEnv->GetType( MagoEE::Tvoid );
    if ( parentType->AsTypeNext()->GetNext()->Equals( voidType ) )
        throw L"Can't index to a void.";

    mType = parentType->AsTypeNext()->GetNext();
}

std::shared_ptr<DataObj> IndexTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>     val;
    std::shared_ptr<DataObj>     parent = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );
    std::shared_ptr<DataObj>     index;
    uint32_t                elemSize = mType->GetSize();
    int64_t                 offset = 0;
    MagoEE::Address         addr = 0;
    Type*                   parentType = mChildren[0]->GetType();
    bool                    hasArrayLen = false;
    MagoEE::dlength_t       arrayLen = 0;

    if ( parentType->IsPointer() )
    {
        addr = parent->Value.Addr;
    }
    else if ( parentType->IsSArray() )
    {
        if ( !parent->GetAddress( addr ) )
            throw L"Array expression needs an address.";
        hasArrayLen = true;
        arrayLen = parentType->AsTypeSArray()->GetLength();
    }
    else if ( parentType->IsDArray() )
    {
        addr = parent->Value.Array.Addr;
        hasArrayLen = true;
        arrayLen = parent->Value.Array.Length;
    }

    if ( hasArrayLen )
    {
        IndexDataEnv    indexEnv( dataEnv, arrayLen );
        index = mChildren[1]->Evaluate( typeEnv, scope, &indexEnv );
    }
    else
    {
        index = mChildren[1]->Evaluate( typeEnv, scope, dataEnv );
    }

    offset = index->Value.Int64Value * elemSize;
    addr += offset;

    val = dataEnv->GetValue( addr, mType.Get() );

    return val;
}

void IndexTestElement::ToDText( std::wostringstream& stream )
{
    mChildren[0]->ToDText( stream );
    stream << "[";
    mChildren[1]->ToDText( stream );
    stream << "]";
}


//----------------------------------------------------------------------------
//  SliceTestElement
//----------------------------------------------------------------------------

void SliceTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( mChildren.size() != 1 && mChildren.size() != 3 )
        throw L"Slice needs one or three children.";

    Type*   parentType = mChildren[0]->GetType();

    if ( !parentType->IsPointer() && !parentType->IsSArray() && !parentType->IsDArray() )
        throw L"Can only index arrays.";

    if ( parentType->IsPointer() && (mChildren.size() != 3) )
        throw L"Slicing a pointer needs two index and limit expressions.";
    // only allow [] or [e1..e2]
    if ( (parentType->IsSArray() || parentType->IsDArray()) && (mChildren.size() == 2) )
        throw L"Slicing an array needs one or three children.";

    if ( mChildren.size() == 3 )
    {
        Type*   indexType = mChildren[1]->GetType();
        Type*   limitType = mChildren[2]->GetType();

        if ( !indexType->IsIntegral() )
            throw L"Only integrals can index.";
        if ( !limitType->IsIntegral() )
            throw L"Only integrals can index.";
    }

    _ASSERT( parentType->AsTypeNext() != NULL );
    RefPtr<Type>    voidType = typeEnv->GetType( MagoEE::Tvoid );
    if ( parentType->AsTypeNext()->GetNext()->Equals( voidType ) )
        throw L"Can't index to a void.";

    HRESULT hr = typeEnv->NewDArray( parentType->AsTypeNext()->GetNext(), mType.Ref() );
    if ( FAILED( hr ) )
        throw L"Failed making new D-array type for Slice.";
}

std::shared_ptr<DataObj> SliceTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>     val( new RValueObj() );
    std::shared_ptr<DataObj>     parent = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );
    std::shared_ptr<DataObj>     index( new RValueObj() );
    std::shared_ptr<DataObj>     limit( new RValueObj() );
    uint32_t                elemSize = mType->AsTypeNext()->GetNext()->GetSize();
    int64_t                 offset = 0;
    MagoEE::Address         addr = 0;
    Type*                   parentType = mChildren[0]->GetType();
    bool                    hasArrayLen = false;
    MagoEE::dlength_t       arrayLen = 0;

    if ( parentType->IsPointer() )
    {
        addr = parent->Value.Addr;
    }
    else if ( parentType->IsSArray() )
    {
        if ( !parent->GetAddress( addr ) )
            throw L"Array expression needs an address.";
        hasArrayLen = true;
        arrayLen = parentType->AsTypeSArray()->GetLength();
        limit->Value.Int64Value = arrayLen;
    }
    else if ( parentType->IsDArray() )
    {
        addr = parent->Value.Array.Addr;
        hasArrayLen = true;
        arrayLen = parent->Value.Array.Length;
        limit->Value.Int64Value = arrayLen;
    }

    if ( mChildren.size() == 3 )
    {
        if ( hasArrayLen )
        {
            IndexDataEnv    indexEnv( dataEnv, arrayLen );
            index = mChildren[1]->Evaluate( typeEnv, scope, &indexEnv );
            limit = mChildren[2]->Evaluate( typeEnv, scope, &indexEnv );
        }
        else
        {
            index = mChildren[1]->Evaluate( typeEnv, scope, dataEnv );
            limit = mChildren[2]->Evaluate( typeEnv, scope, dataEnv );
        }

        if ( limit->Value.Int64Value < index->Value.Int64Value )
            throw L"Can't make D-Array with a negative length.";
    }

    offset = index->Value.Int64Value * elemSize;
    addr += offset;

    _ASSERT( mType->IsDArray() );
    val->SetType( mType );
    val->Value.Array.Addr = addr;
    val->Value.Array.Length = limit->Value.Int64Value - index->Value.Int64Value;

    return val;
}

void SliceTestElement::ToDText( std::wostringstream& stream )
{
    mChildren[0]->ToDText( stream );
    stream << "[";
    if ( mChildren.size() > 1 )
    {
        mChildren[1]->ToDText( stream );
        stream << "..";
        mChildren[2]->ToDText( stream );
    }
    stream << "]";
}


//----------------------------------------------------------------------------
//  AddressTestElement
//----------------------------------------------------------------------------

void AddressTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( mChildren.size() != 1 )
        throw L"AddressOf needs one child.";

    Type*   parentType = mChildren[0]->GetType();

    HRESULT hr = typeEnv->NewPointer( parentType, mType.Ref() );
    if ( FAILED( hr ) )
        throw L"Couldn't make new pointer type.";
}

std::shared_ptr<DataObj> AddressTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>     val( new RValueObj() );
    std::shared_ptr<DataObj>     parent = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );
    MagoEE::Address         addr = 0;

    if ( !parent->GetAddress( addr ) )
        throw L"Address expression needs an address.";

    val->SetType( mType );
    val->Value.Addr = addr;

    return val;
}

void AddressTestElement::ToDText( std::wostringstream& stream )
{
    stream << "&";
    mChildren[0]->ToDText( stream );
}


//----------------------------------------------------------------------------
//  PointerTestElement
//----------------------------------------------------------------------------

void PointerTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( mChildren.size() != 1 )
        throw L"Pointer needs one child.";

    Type*   parentType = mChildren[0]->GetType();

    if ( !parentType->IsPointer() && !parentType->IsSArray() && !parentType->IsDArray() )
        throw L"Can only Pointer pointers and arrays.";

    _ASSERT( parentType->AsTypeNext() != NULL );
    RefPtr<Type>    voidType = typeEnv->GetType( MagoEE::Tvoid );
    if ( parentType->AsTypeNext()->GetNext()->Equals( voidType ) )
        throw L"Can't pointer to a void.";

    mType = parentType->AsTypeNext()->GetNext();
}

std::shared_ptr<DataObj> PointerTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>     val;
    std::shared_ptr<DataObj>     parent = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );
    MagoEE::Address         addr = 0;

    if ( mChildren[0]->GetType()->IsDArray() )
    {
        addr = parent->Value.Array.Addr;
    }
    else if ( mChildren[0]->GetType()->IsSArray() )
    {
        if ( !parent->GetAddress( addr ) )
            throw L"Couldn't get address of S Array.";
    }
    else
        addr = parent->Value.Addr;

    val = dataEnv->GetValue( addr, mType.Get() );

    return val;
}

void PointerTestElement::ToDText( std::wostringstream& stream )
{
    stream << "*";
    mChildren[0]->ToDText( stream );
}


//----------------------------------------------------------------------------
//  DollarTestElement
//----------------------------------------------------------------------------

void DollarTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    // assume we're valid in this context, 
    // let Evaluate figure out if there's actually an array length to work with

    mType = typeEnv->GetType( MagoEE::Tuns32 );
}

std::shared_ptr<DataObj> DollarTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    std::shared_ptr<DataObj>   val( new RValueObj() );
    MagoEE::dlength_t       len = 0;

    if ( !dataEnv->GetArrayLength( len ) )
        throw L"Array expected.";

    val->Value.UInt64Value = len;
    val->SetType( mType );

    return val;
}

void DollarTestElement::ToDText( std::wostringstream& stream )
{
    stream << "$";
}

void DollarTestElement::AddChild( Element* elem )
{
    throw L"Dollar can't have children.";
}

void DollarTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
}

void DollarTestElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}


//----------------------------------------------------------------------------
//  AssignTestElement
//----------------------------------------------------------------------------

void AssignTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( mChildren.size() != 2 )
        throw L"Assign needs two children.";

    if ( mChildren[1]->TrySetType( mChildren[0]->GetType() ) )
    {
        // OK
    }
    else if ( !CastTestElement::CanImplicitCast( mChildren[1]->GetType(), mChildren[0]->GetType() ) )
        throw L"Bad cast.";

    mType = mChildren[0]->GetType();
}

std::shared_ptr<DataObj> AssignTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    // TODO: we don't need to evaluate it, only get its address
    std::shared_ptr<DataObj>     val = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );
    std::shared_ptr<DataObj>     rightVal = mChildren[1]->Evaluate( typeEnv, scope, dataEnv );
    MagoEE::Address         addr = 0;

    // in this test framework, L-values always have addresses

    if ( !val->GetAddress( addr ) )
        throw L"L-value with address expected.";

    CastTestElement::AssignValue( rightVal.get(), val.get() );

    if ( gAppSettings.AllowAssignment )
        dataEnv->SetValue( addr, val.get() );

    return val;
}

void AssignTestElement::ToDText( std::wostringstream& stream )
{
    mChildren[0]->ToDText( stream );
    stream << L" = ";
    mChildren[1]->ToDText( stream );
}


//----------------------------------------------------------------------------
//  PreAssignTestElement
//----------------------------------------------------------------------------

void PreAssignTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    // don't bind the child here, because we want the binary element that we make here bind it

    if ( mChildren.size() != 1 )
        throw L"PreAssign needs one child.";

    RefPtr<TestElement> intElem = new IntTestElement();
    intElem->SetAttribute( L"value", L"1" );

    RefPtr<TestElement> binElem;

    binElem = (TestElement*) gAppSettings.TestElemFactory->NewElement( mOp.c_str() );
    if ( binElem == NULL )
        throw L"Unknown operator for pre-assign.";

    binElem->AddChild( mChildren[0] );
    binElem->AddChild( intElem );

    binElem->Bind( typeEnv, scope, dataEnv );

    mBinElem = binElem;

    mType = mBinElem->GetType();
}

std::shared_ptr<DataObj> PreAssignTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    // TODO: we don't need to evaluate it, only get its address
    std::shared_ptr<DataObj>     val = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );
    std::shared_ptr<DataObj>     rightVal = mBinElem->Evaluate( typeEnv, scope, dataEnv );
    MagoEE::Address         addr = 0;

    // in this test framework, L-values always have addresses

    if ( !val->GetAddress( addr ) )
        throw L"L-value with address expected.";

    CastTestElement::AssignValue( rightVal.get(), val.get() );

    if ( gAppSettings.AllowAssignment )
        dataEnv->SetValue( addr, val.get() );

    return val;
}

void PreAssignTestElement::ToDText( std::wostringstream& stream )
{
    CombinableTestElement*   containerElem = dynamic_cast<CombinableTestElement*>( mBinElem.Get() );
    if ( containerElem == NULL )
        throw L"ContainerTestElement expected.";

    stream << containerElem->GetPreOrPostOperator();
    mChildren[0]->ToDText( stream );
}

void PreAssignTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"op" ) == 0 )
    {
        mOp = value;
    }
}


//----------------------------------------------------------------------------
//  PostAssignTestElement
//----------------------------------------------------------------------------

void PostAssignTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    // don't bind the child here, because we want the binary element that we make here bind it

    if ( mChildren.size() != 1 )
        throw L"PostAssign needs one child.";

    RefPtr<TestElement> intElem = new IntTestElement();
    intElem->SetAttribute( L"value", L"1" );

    RefPtr<TestElement> binElem;
    
    binElem = (TestElement*) gAppSettings.TestElemFactory->NewElement( mOp.c_str() );
    if ( binElem == NULL )
        throw L"Unknown operator for post-assign.";

    binElem->AddChild( mChildren[0] );
    binElem->AddChild( intElem );

    binElem->Bind( typeEnv, scope, dataEnv );

    mBinElem = binElem;

    mType = mBinElem->GetType();
}

std::shared_ptr<DataObj> PostAssignTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    // TODO: we don't need to evaluate it, only get its address
    std::shared_ptr<DataObj>     val = mChildren[0]->Evaluate( typeEnv, scope, dataEnv );
    std::shared_ptr<DataObj>     newVal;
    std::shared_ptr<DataObj>     rightVal = mBinElem->Evaluate( typeEnv, scope, dataEnv );
    MagoEE::Address         addr = 0;

    // in this test framework, L-values always have addresses

    if ( !val->GetAddress( addr ) )
        throw L"L-value with address expected.";

    newVal.reset( new LValueObj( NULL, addr ) );
    newVal->SetType( val->GetType() );

    CastTestElement::AssignValue( rightVal.get(), newVal.get() );

    if ( gAppSettings.AllowAssignment )
        dataEnv->SetValue( addr, newVal.get() );

    return val;
}

void PostAssignTestElement::ToDText( std::wostringstream& stream )
{
    CombinableTestElement*   containerElem = dynamic_cast<CombinableTestElement*>( mBinElem.Get() );
    if ( containerElem == NULL )
        throw L"ContainerTestElement expected.";

    mChildren[0]->ToDText( stream );
    stream << containerElem->GetPreOrPostOperator();
}

void PostAssignTestElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"op" ) == 0 )
    {
        mOp = value;
    }
}


//----------------------------------------------------------------------------
//  CombinedAssignTestElement
//----------------------------------------------------------------------------

void CombinedAssignTestElement::Bind( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement::Bind( typeEnv, scope, dataEnv );

    if ( mChildren.size() != 1 )
        throw L"CombinedAssign needs one binary child.";

    ContainerTestElement*   binChild = dynamic_cast<ContainerTestElement*>( mChildren[0].Get() );

    if ( (binChild == NULL) || (binChild->GetChildren().size() != 2) )
        throw L"CombinedAssign needs one binary child.";

    if ( !CastTestElement::CanImplicitCast( mChildren[0]->GetType(), binChild->GetChildren()[0]->GetType() ) )
        throw L"Bad cast.";

    mType = binChild->GetChildren()[0]->GetType();
}

std::shared_ptr<DataObj> CombinedAssignTestElement::Evaluate( ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv )
{
    ContainerTestElement*   binChild = dynamic_cast<ContainerTestElement*>( mChildren[0].Get() );
    // TODO: we don't need to evaluate it, only get its address
    std::shared_ptr<DataObj>     val = binChild->GetChildren()[0]->Evaluate( typeEnv, scope, dataEnv );
    std::shared_ptr<DataObj>     rightVal = binChild->Evaluate( typeEnv, scope, dataEnv );
    MagoEE::Address         addr = 0;

    // in this test framework, L-values always have addresses

    if ( !val->GetAddress( addr ) )
        throw L"L-value with address expected.";

    CastTestElement::AssignValue( rightVal.get(), val.get() );

    if ( gAppSettings.AllowAssignment )
        dataEnv->SetValue( addr, val.get() );

    return val;
}

void CombinedAssignTestElement::ToDText( std::wostringstream& stream )
{
    CombinableTestElement*   binChild = dynamic_cast<CombinableTestElement*>( mChildren[0].Get() );

    binChild->GetChildren()[0]->ToDText( stream );
    stream << L" " << binChild->GetOperator() << L"= ";
    binChild->GetChildren()[1]->ToDText( stream );
}
