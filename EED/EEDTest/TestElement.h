/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "Element.h"


namespace MagoEE
{
    class Type;
    class Declaration;
    class ITypeEnv;
}

class IScope;
class DataObj;


// TODO: should be called IDataEnv
class IValueEnv
{
public:
    virtual std::shared_ptr<DataObj> GetValue( MagoEE::Declaration* decl ) = 0;
    virtual std::shared_ptr<DataObj> GetValue( MagoEE::Address address, MagoEE::Type* type ) = 0;
    virtual void SetValue( MagoEE::Address address, DataObj* obj ) = 0;
    virtual RefPtr<MagoEE::Declaration> GetThis() = 0;
    virtual RefPtr<MagoEE::Declaration> GetSuper() = 0;
    virtual bool GetArrayLength( MagoEE::dlength_t& length ) = 0;
};


//----------------------------------------------------------------------------


class TestFactory : public ElementFactory
{
public:
    Element* NewElement( const wchar_t* name );
};


class TestElement : public Element
{
protected:
    RefPtr<MagoEE::Type>    mType;

public:
    MagoEE::Type* GetType();
    virtual RefPtr<MagoEE::Declaration> GetDeclaration();
    virtual bool TrySetType( MagoEE::Type* type );
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    // Element
    virtual void PrintElement();

protected:
    static void VerifyCompareValues( 
        MagoEE::ITypeEnv* typeEnv,
        const std::shared_ptr<DataObj>& val, 
        const std::shared_ptr<DataObj>& refVal );
    static bool FloatEqual( const Real10& left, const Real10& right, MagoEE::Type* type );
};


class IntTestElement : public TestElement
{
    std::wstring            mText;
    uint64_t                mVal;

public:
    IntTestElement();

    // TestElement
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    // Element
    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
};


class RealTestElement : public TestElement
{
    std::wstring            mText;
    Real10                  mVal;

public:
    RealTestElement();

    // TestElement
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    // Element
    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
};


class NullTestElement : public TestElement
{
public:
    // TestElement
    virtual bool TrySetType( MagoEE::Type* type );
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    // Element
    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
};


class ContainerTestElement : public TestElement
{
public:
    typedef std::vector< RefPtr<TestElement> >  List;

protected:
    List                    mChildren;

public:
    // new
    const List& GetChildren() { return mChildren; }

    // TestElement
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );

    // Element
    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
};


class DArrayTestElement : public ContainerTestElement
{
    MagoEE::DArray  mVal;

public:
    DArrayTestElement();

    // TestElement
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    // Element
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
};


class CombinableTestElement : public ContainerTestElement
{
public:
    const wchar_t* GetOperator()
    {
        return OpStr();
    }
    virtual const wchar_t* GetPreOrPostOperator();

protected:
    virtual const wchar_t*  OpStr() = 0;
};


class ArithmeticTestElement : public CombinableTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    static RefPtr<MagoEE::Type> GetCommonType( MagoEE::ITypeEnv* typeEnv, MagoEE::Type* left, MagoEE::Type* right );
    static RefPtr<MagoEE::Type> GetMulCommonType( MagoEE::ITypeEnv* typeEnv, MagoEE::Type* left, MagoEE::Type* right );
    static RefPtr<MagoEE::Type> GetModCommonType( MagoEE::ITypeEnv* typeEnv, MagoEE::Type* left, MagoEE::Type* right );
    static RefPtr<MagoEE::Type> PromoteComplexType( MagoEE::ITypeEnv* typeEnv, MagoEE::Type* t );
    static RefPtr<MagoEE::Type> PromoteImaginaryType( MagoEE::ITypeEnv* typeEnv, MagoEE::Type* t );
    static RefPtr<MagoEE::Type> PromoteFloatType( MagoEE::ITypeEnv* typeEnv, MagoEE::Type* t );
    static RefPtr<MagoEE::Type> PromoteIntType( MagoEE::ITypeEnv* typeEnv, MagoEE::Type* t );
    static Complex10 ConvertToComplex( DataObj* x );
    static Real10 ConvertToFloat( DataObj* x );
    static void PromoteInPlace( DataObj* x );
    static void PromoteInPlace( DataObj* x, MagoEE::Type* targetType );

protected:
    virtual bool            AllowOnlyIntegral();
    virtual uint64_t        UIntOp( uint64_t left, uint64_t right ) = 0;
    virtual int64_t         IntOp( int64_t left, int64_t right ) = 0;
    virtual Real10          FloatOp( const Real10& left, const Real10& right ) = 0;
    virtual Complex10       ComplexOp( const Complex10& left, const Complex10& right ) = 0;
};


class AddTestElement : public ArithmeticTestElement
{
public:
    virtual const wchar_t* GetPreOrPostOperator();
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );

protected:
    virtual uint64_t        UIntOp( uint64_t left, uint64_t right );
    virtual int64_t         IntOp( int64_t left, int64_t right );
    virtual Real10          FloatOp( const Real10& left, const Real10& right );
    virtual Complex10       ComplexOp( const Complex10& left, const Complex10& right );
    virtual const wchar_t*  OpStr();
};


class SubTestElement : public ArithmeticTestElement
{
public:
    virtual const wchar_t* GetPreOrPostOperator();
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );

protected:
    virtual uint64_t        UIntOp( uint64_t left, uint64_t right );
    virtual int64_t         IntOp( int64_t left, int64_t right );
    virtual Real10          FloatOp( const Real10& left, const Real10& right );
    virtual Complex10       ComplexOp( const Complex10& left, const Complex10& right );
    virtual const wchar_t*  OpStr();
};


class MulTestElement : public ArithmeticTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );

protected:
    virtual uint64_t        UIntOp( uint64_t left, uint64_t right );
    virtual int64_t         IntOp( int64_t left, int64_t right );
    virtual Real10          FloatOp( const Real10& left, const Real10& right );
    virtual Complex10       ComplexOp( const Complex10& left, const Complex10& right );
    virtual const wchar_t*  OpStr();
};


class DivTestElement : public ArithmeticTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );

protected:
    virtual uint64_t        UIntOp( uint64_t left, uint64_t right );
    virtual int64_t         IntOp( int64_t left, int64_t right );
    virtual Real10          FloatOp( const Real10& left, const Real10& right );
    virtual Complex10       ComplexOp( const Complex10& left, const Complex10& right );
    virtual const wchar_t*  OpStr();
};


class ModTestElement : public ArithmeticTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );

protected:
    virtual uint64_t        UIntOp( uint64_t left, uint64_t right );
    virtual int64_t         IntOp( int64_t left, int64_t right );
    virtual Real10          FloatOp( const Real10& left, const Real10& right );
    virtual Complex10       ComplexOp( const Complex10& left, const Complex10& right );
    virtual const wchar_t*  OpStr();
};


class PowTestElement : public ArithmeticTestElement
{
protected:
    virtual uint64_t        UIntOp( uint64_t left, uint64_t right );
    virtual int64_t         IntOp( int64_t left, int64_t right );
    virtual Real10          FloatOp( const Real10& left, const Real10& right );
    virtual Complex10       ComplexOp( const Complex10& left, const Complex10& right );
    virtual const wchar_t*  OpStr();
};


class NegateTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};


class UnaryAddTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};


class AndTestElement : public ArithmeticTestElement
{
protected:
    virtual bool            AllowOnlyIntegral();
    virtual uint64_t        UIntOp( uint64_t left, uint64_t right );
    virtual int64_t         IntOp( int64_t left, int64_t right );
    virtual Real10          FloatOp( const Real10& left, const Real10& right );
    virtual Complex10       ComplexOp( const Complex10& left, const Complex10& right );
    virtual const wchar_t*  OpStr();
};


class OrTestElement : public ArithmeticTestElement
{
protected:
    virtual bool            AllowOnlyIntegral();
    virtual uint64_t        UIntOp( uint64_t left, uint64_t right );
    virtual int64_t         IntOp( int64_t left, int64_t right );
    virtual Real10          FloatOp( const Real10& left, const Real10& right );
    virtual Complex10       ComplexOp( const Complex10& left, const Complex10& right );
    virtual const wchar_t*  OpStr();
};


class XorTestElement : public ArithmeticTestElement
{
protected:
    virtual bool            AllowOnlyIntegral();
    virtual uint64_t        UIntOp( uint64_t left, uint64_t right );
    virtual int64_t         IntOp( int64_t left, int64_t right );
    virtual Real10          FloatOp( const Real10& left, const Real10& right );
    virtual Complex10       ComplexOp( const Complex10& left, const Complex10& right );
    virtual const wchar_t*  OpStr();
};


class BitNotTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};


class ShiftTestElement : public CombinableTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

protected:
    virtual uint64_t        IntOp( uint64_t left, uint64_t right, MagoEE::Type* type ) = 0;
};


class ShiftLeftTestElement : public ShiftTestElement
{
protected:
    virtual uint64_t        IntOp( uint64_t left, uint64_t right, MagoEE::Type* type );
    virtual const wchar_t*  OpStr();
};


class ShiftRightTestElement : public ShiftTestElement
{
protected:
    virtual uint64_t        IntOp( uint64_t left, uint64_t right, MagoEE::Type* type );
    virtual const wchar_t*  OpStr();
};


class UShiftRightTestElement : public ShiftTestElement
{
protected:
    virtual uint64_t        IntOp( uint64_t left, uint64_t right, MagoEE::Type* type );
    virtual const wchar_t*  OpStr();
};


class AndAndTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    static bool EvalBool( DataObj* val );
};


class OrOrTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};


class NotTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};


class CompareTestElement : public ContainerTestElement
{
    enum OpCode
    {
        O_none,
        O_equal,  
        O_notequal,
        O_identity,
        O_notidentity,

        O_lt,     
        O_le,     
        O_gt,     
        O_ge,     

        O_unord,  
        O_lg,     
        O_leg,    
        O_ule,    
        O_ul,     
        O_uge,    
        O_ug,     
        O_ue,     
    };

    std::wstring    mOpStr;
    OpCode          mOpCode;

public:
    CompareTestElement();

    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );

    template <class T>
    static bool IntegerOp( OpCode code, T left, T right )
    {
        switch ( code )
        {
        case O_identity:
        case O_equal:   return left == right;   break;
        case O_notidentity:
        case O_notequal:return left != right;   break;
        case O_lt:      return left < right;    break;
        case O_le:      return left <= right;   break;
        case O_gt:      return left > right;    break;
        case O_ge:      return left >= right;   break;
        case O_unord:   return false;
        case O_lg:      return left != right;
        case O_leg:     return true;
        case O_ule:     return left <= right;
        case O_ul:      return left < right;
        case O_uge:     return left >= right;
        case O_ug:      return left > right;
        case O_ue:      return left == right;
        default:
            throw L"Relational operator not allowed on integers.";
        }
    }

    static bool IntegerRelational( OpCode code, MagoEE::Type* exprType, DataObj* left, DataObj* right );
    static bool FloatingRelational( OpCode code, DataObj* left, DataObj* right );
    static bool ComplexRelational( OpCode code, DataObj* left, DataObj* right );
    static bool FloatingRelational( OpCode code, uint16_t status );
    static bool ArrayRelational( OpCode code, DataObj* left, DataObj* right );
};


class PtrArithmeticTestElement : public CombinableTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

protected:
    virtual MagoEE::Address PtrOp( MagoEE::Address addr, int64_t offset ) = 0;
};


class PtrAddTestElement : public PtrArithmeticTestElement
{
public:
    virtual const wchar_t* GetPreOrPostOperator();

protected:
    virtual MagoEE::Address PtrOp( MagoEE::Address addr, int64_t offset );
    virtual const wchar_t* OpStr();
};


class PtrSubTestElement : public PtrArithmeticTestElement
{
public:
    virtual const wchar_t* GetPreOrPostOperator();

protected:
    virtual MagoEE::Address PtrOp( MagoEE::Address addr, int64_t offset );
    virtual const wchar_t* OpStr();
};


class PtrDiffTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};


class GroupTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};


class CastTestElement : public ContainerTestElement
{
    MOD     mMod;

public:
    CastTestElement();

    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );

    static bool CanCast( MagoEE::Type* fromType, MagoEE::Type* toType );
    static bool CanImplicitCast( MagoEE::Type* fromType, MagoEE::Type* toType );
    static void AssignValue( DataObj* source, DataObj* dest );
    static bool ConvertToBool( DataObj* source );
    static Real10 ConvertToReal( DataObj* source );
    static Real10 ConvertToImaginary( DataObj* source );
    static MagoEE::Address ConvertToPointer( DataObj* source, MagoEE::Type* ptrType );
    static void ConvertToDArray( DataObj* source, DataObj* dest );
};


class TypeTestElement
{
};


class BasicTypeTestElement : public TestElement, public TypeTestElement
{
    std::wstring    mName;
    MOD             mMod;

public:
    BasicTypeTestElement();

    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    // Element
    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
};


class RefTypeTestElement : public TestElement, public TypeTestElement
{
    std::wstring    mName;
    MOD             mMod;

public:
    RefTypeTestElement();

    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    // Element
    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
};


class PointerTypeTestElement : public ContainerTestElement, public TypeTestElement
{
    MOD             mMod;

public:
    PointerTypeTestElement();

    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    // Element
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
};


class SArrayTypeTestElement : public ContainerTestElement, public TypeTestElement
{
    MOD             mMod;
    uint32_t        mLen;

public:
    SArrayTypeTestElement();

    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    // Element
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
};


class DArrayTypeTestElement : public ContainerTestElement, public TypeTestElement
{
    MOD             mMod;

public:
    DArrayTypeTestElement();

    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    // Element
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
};


class AArrayTypeTestElement : public ContainerTestElement, public TypeTestElement
{
    MOD             mMod;

public:
    AArrayTypeTestElement();

    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    // Element
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
};


class TestTestElement : public ContainerTestElement
{
public:
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};


class VerifyTestElement : public ContainerTestElement
{
    std::wstring    mId;

public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );

private:
    std::shared_ptr<DataObj> EvaluateSelf( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    std::shared_ptr<DataObj> EvaluateEED( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );

    std::shared_ptr<DataObj> EvaluateText( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );

    void ThrowError( const wchar_t* msg );
};


class TypedValueTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};


class MemoryValueTestElement : public ContainerTestElement
{
    size_t  mAddr;

public:
    MemoryValueTestElement();

    size_t GetAddress();

    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
};


class IdTestElement : public TestElement
{
    std::wstring    mName;
    RefPtr<MagoEE::Declaration> mDecl;
    bool            mIsGlobal;

public:
    IdTestElement();

    virtual RefPtr<MagoEE::Declaration> GetDeclaration();
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
};


class MemberTestElement : public ContainerTestElement
{
    std::wstring    mName;
    RefPtr<MagoEE::Declaration> mDecl;

public:
    virtual RefPtr<MagoEE::Declaration> GetDeclaration();
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
};


class StdPropertyTestElement : public ContainerTestElement
{
    std::wstring    mName;

public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
};


class IndexTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};


class SliceTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};


class AddressTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};


class PointerTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};


class DollarTestElement : public TestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
};


class AssignTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};


class PreAssignTestElement : public ContainerTestElement
{
    std::wstring        mOp;
    RefPtr<TestElement> mBinElem;

public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
};


class PostAssignTestElement : public ContainerTestElement
{
    std::wstring        mOp;
    RefPtr<TestElement> mBinElem;

public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );

    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
};


class CombinedAssignTestElement : public ContainerTestElement
{
public:
    virtual void Bind( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::ITypeEnv* typeEnv, IScope* scope, IValueEnv* dataEnv );
    virtual void ToDText( std::wostringstream& stream );
};
