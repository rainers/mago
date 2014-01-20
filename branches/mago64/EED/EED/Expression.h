/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "Token.h"
#include "Object.h"
#include "Eval.h"
#include "Strings.h"


namespace MagoEE
{
    struct String;
    class Type;
    class TemplateInstancePart;
    class Declaration;
    class NamingExpression;
    class StdProperty;
    class SharedString;


    enum EvalMode
    {
        EvalMode_Value,
        EvalMode_Address,
    };


    enum DataKind
    {
        DataKind_None,
        DataKind_Declaration,
        DataKind_Type,
        DataKind_Value,
    };


    struct EvalData
    {
        EvalOptions Options;
        ITypeEnv*   TypeEnv;
        bool        HasArrayLength;
        dlength_t   ArrayLength;
    };


    class Expression : public Object
    {
    public:
        RefPtr<Type>    _Type;
        DataKind        Kind;
        // Kind = 
        //  Declaration : NamingExpression::Decl is set
        //  Type : Expression::_Type is set
        //  Value : Expression::_Type is set, NamingExpression::Decl might be set
        //          Decl can be set if there is a declaration.
        //          It's used for its value, address, and other properties,
        //          like offset. But its type must be copied to the _Type member.

        Expression();
        virtual ObjectKind GetObjectKind();
        // TODO: abstract
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
        virtual bool TrySetType( Type* type );
        virtual NamingExpression* AsNamingExpression();

        // returns E_MAGOEE_SYMBOL_NOT_FOUND 
        //          if this node does not support making up a dotted name
        virtual HRESULT MakeName( uint32_t capacity, RefPtr<SharedString>& namePath );

        static bool ConvertToBool( const DataObject& obj );
        static Complex10 ConvertToComplex( const DataObject& x );
        static Complex10 ConvertToComplex( Type* commonType, const DataObject& x );
        static Real10 ConvertToFloat( const DataObject& x );
        static Real10 ConvertToFloat( Type* commonType, const DataObject& x );
        static void ConvertToDArray( const DataObject& source, DataObject& dest );
        static void PromoteInPlace( DataObject& x );
        static void PromoteInPlace( DataObject& x, Type* targetType );
        static RefPtr<Type> PromoteComplexType( ITypeEnv* typeEnv, Type* t );
        static RefPtr<Type> PromoteImaginaryType( ITypeEnv* typeEnv, Type* t );
        static RefPtr<Type> PromoteFloatType( ITypeEnv* typeEnv, Type* t );
        static RefPtr<Type> PromoteIntType( ITypeEnv* typeEnv, Type* t );
        static RefPtr<Type> GetCommonType( ITypeEnv* typeEnv, Type* left, Type* right );
        static RefPtr<Type> GetMulCommonType( ITypeEnv* typeEnv, Type* left, Type* right );
        static RefPtr<Type> GetModCommonType( ITypeEnv* typeEnv, Type* left, Type* right );

    protected:
        virtual void ClearEvalData();
    };


    class ExpressionList : public Object
    {
    public:
        std::list< RefPtr<Expression> > List;

        virtual ObjectKind GetObjectKind();
    };


    class BinExpr : public Expression
    {
    public:
        RefPtr<Expression>  Left;
        RefPtr<Expression>  Right;

        BinExpr( Expression* left, Expression* right );

    protected:
        // run Semantic on Left and Right, and verify that they're values
        HRESULT SemanticVerifyChildren( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
    };


    class CommaExpr : public BinExpr
    {
    public:
        CommaExpr( Expression* left, Expression* right );
    };


    class AssignExpr : public BinExpr
    {
    public:
        AssignExpr( Expression* left, Expression* right );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class CombinableBinExpr;

    class CombinedAssignExpr : public Expression
    {
    public:
        RefPtr<CombinableBinExpr>   Child;
        bool                        IsPostOp;
        // IsPostOp says that we're a post-increment or decrement instead of a pre-inc or dec

        CombinedAssignExpr( CombinableBinExpr* child, bool postOp = false );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class ConditionalExpr : public Expression
    {
    public:
        RefPtr<Expression>  PredicateExpr;
        RefPtr<Expression>  TrueExpr;
        RefPtr<Expression>  FalseExpr;

        ConditionalExpr( Expression* predicate, Expression* trueExpr, Expression* falseExpr );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class OrOrExpr : public BinExpr
    {
    public:
        OrOrExpr( Expression* left, Expression* right );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class AndAndExpr : public BinExpr
    {
    public:
        AndAndExpr( Expression* left, Expression* right );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class CombinableBinExpr : public BinExpr
    {
    public:
        Address     LeftAddr;       // for combined assign
        DataValue   LeftValue;      // for post-increment

        CombinableBinExpr( Expression* left, Expression* right )
            :   BinExpr( left, right ),
                LeftAddr( 0 )
        {
            memset( &LeftValue, 0, sizeof LeftValue );
        }

    protected:
        virtual void ClearEvalData()
        {
            BinExpr::ClearEvalData();
            LeftAddr = 0;
            memset( &LeftValue, 0, sizeof LeftValue );
        }
    };


    class ArithmeticBinExpr : public CombinableBinExpr
    {
    public:
        ArithmeticBinExpr( Expression* left, Expression* right )
            :   CombinableBinExpr( left, right )
        {
        }

        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );

    protected:
        virtual bool    AllowOnlyIntegral();
        virtual HRESULT UInt64Op( uint64_t left, uint64_t right, uint64_t& result );
        virtual HRESULT Int64Op( int64_t left, int64_t right, int64_t& result );
        virtual HRESULT Float80Op( const Real10& left, const Real10& right, Real10& result );
        virtual HRESULT Complex80Op( const Complex10& left, const Complex10& right, Complex10& result );
    };


    class OrExpr : public ArithmeticBinExpr
    {
    public:
        OrExpr( Expression* left, Expression* right );

    protected:
        virtual bool    AllowOnlyIntegral();
        virtual HRESULT UInt64Op( uint64_t left, uint64_t right, uint64_t& result );
        virtual HRESULT Int64Op( int64_t left, int64_t right, int64_t& result );
    };


    class XorExpr : public ArithmeticBinExpr
    {
    public:
        XorExpr( Expression* left, Expression* right );

    protected:
        virtual bool    AllowOnlyIntegral();
        virtual HRESULT UInt64Op( uint64_t left, uint64_t right, uint64_t& result );
        virtual HRESULT Int64Op( int64_t left, int64_t right, int64_t& result );
    };


    class AndExpr : public ArithmeticBinExpr
    {
    public:
        AndExpr( Expression* left, Expression* right );

    protected:
        virtual bool    AllowOnlyIntegral();
        virtual HRESULT UInt64Op( uint64_t left, uint64_t right, uint64_t& result );
        virtual HRESULT Int64Op( int64_t left, int64_t right, int64_t& result );
    };


    // D front end only makes CmpExp and their RelExp seems deprecated
    // it's the other way around here
    class CmpExpr : public BinExpr
    {
    public:
        TOK OpCode;

        CmpExpr( TOK opCode, Expression* left, Expression* right );
    };


    class CompareExpr : public BinExpr
    {
    public:
        TOK OpCode;

        CompareExpr( TOK opCode, Expression* left, Expression* right );

        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );

        template <class T>
        static bool IntegerOp( TOK code, T left, T right )
        {
            switch ( code )
            {
            case TOKidentity:
            case TOKequal:   return left == right;   break;
            case TOKnotidentity:
            case TOKnotequal:return left != right;   break;
            case TOKlt:      return left < right;    break;
            case TOKle:      return left <= right;   break;
            case TOKgt:      return left > right;    break;
            case TOKge:      return left >= right;   break;
            case TOKunord:   return false;
            case TOKlg:      return left != right;
            case TOKleg:     return true;
            case TOKule:     return left <= right;
            case TOKul:      return left < right;
            case TOKuge:     return left >= right;
            case TOKug:      return left > right;
            case TOKue:      return left == right;
            default:
                _ASSERT_EXPR( false, L"Relational operator not allowed on integers." );
            }
            return false;
        }

        static bool IntegerRelational( TOK code, Type* exprType, DataObject& left, DataObject& right );
        static bool FloatingRelational( TOK code, Type* exprType, DataObject& left, DataObject& right );
        static bool ComplexRelational( TOK code, Type* exprType, DataObject& left, DataObject& right );
        static bool FloatingRelational( TOK code, uint16_t status );
        static bool ArrayRelational( TOK code, DataObject& left, DataObject& right );
        static bool DelegateRelational( TOK code, DataObject& left, DataObject& right );
    };


    class EqualExpr : public CompareExpr
    {
    public:
        EqualExpr( TOK opCode, Expression* left, Expression* right );
    };


    class IdentityExpr : public CompareExpr
    {
    public:
        IdentityExpr( TOK opCode, Expression* left, Expression* right );
    };


    class RelExpr : public CompareExpr
    {
    public:
        RelExpr( TOK opCode, Expression* left, Expression* right );
    };


    class InExpr : public BinExpr
    {
    public:
        InExpr( Expression* left, Expression* right );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class ShiftBinExpr : public CombinableBinExpr
    {
    public:
        ShiftBinExpr( Expression* left, Expression* right );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );

    protected:
        virtual uint64_t        IntOp( uint64_t left, uint32_t right, Type* type ) = 0;
    };


    class ShiftLeftExpr : public ShiftBinExpr
    {
    public:
        ShiftLeftExpr( Expression* left, Expression* right );

    protected:
        virtual uint64_t        IntOp( uint64_t left, uint32_t right, Type* type );
    };


    class ShiftRightExpr : public ShiftBinExpr
    {
    public:
        ShiftRightExpr( Expression* left, Expression* right );

    protected:
        virtual uint64_t        IntOp( uint64_t left, uint32_t right, Type* type );
    };


    class UShiftRightExpr : public ShiftBinExpr
    {
    public:
        UShiftRightExpr( Expression* left, Expression* right );

    protected:
        virtual uint64_t        IntOp( uint64_t left, uint32_t right, Type* type );
    };


    class AddExpr : public ArithmeticBinExpr
    {
    public:
        AddExpr( Expression* left, Expression* right );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );

    protected:
        virtual HRESULT UInt64Op( uint64_t left, uint64_t right, uint64_t& result );
        virtual HRESULT Int64Op( int64_t left, int64_t right, int64_t& result );
        virtual HRESULT Float80Op( const Real10& left, const Real10& right, Real10& result );
        virtual HRESULT Complex80Op( const Complex10& left, const Complex10& right, Complex10& result );

    private:
        HRESULT EvaluateMakeComplex( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
        HRESULT EvaluatePtrAdd( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class MinExpr : public ArithmeticBinExpr
    {
    public:
        MinExpr( Expression* left, Expression* right );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );

    protected:
        virtual HRESULT UInt64Op( uint64_t left, uint64_t right, uint64_t& result );
        virtual HRESULT Int64Op( int64_t left, int64_t right, int64_t& result );
        virtual HRESULT Float80Op( const Real10& left, const Real10& right, Real10& result );
        virtual HRESULT Complex80Op( const Complex10& left, const Complex10& right, Complex10& result );

    private:
        HRESULT EvaluateMakeComplex( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
        HRESULT EvaluatePtrSub( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
        HRESULT EvaluatePtrDiff( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
        HRESULT EvaluateSpecialCase( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class CatExpr : public CombinableBinExpr
    {
    public:
        CatExpr( Expression* left, Expression* right );
    };


    class MulExpr : public ArithmeticBinExpr
    {
    public:
        MulExpr( Expression* left, Expression* right );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );

    protected:
        virtual HRESULT UInt64Op( uint64_t left, uint64_t right, uint64_t& result );
        virtual HRESULT Int64Op( int64_t left, int64_t right, int64_t& result );
        virtual HRESULT Float80Op( const Real10& left, const Real10& right, Real10& result );
        virtual HRESULT Complex80Op( const Complex10& left, const Complex10& right, Complex10& result );

    private:
        HRESULT EvaluateShortcutComplex( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class DivExpr : public ArithmeticBinExpr
    {
    public:
        DivExpr( Expression* left, Expression* right );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );

    protected:
        virtual HRESULT UInt64Op( uint64_t left, uint64_t right, uint64_t& result );
        virtual HRESULT Int64Op( int64_t left, int64_t right, int64_t& result );
        virtual HRESULT Float80Op( const Real10& left, const Real10& right, Real10& result );
        virtual HRESULT Complex80Op( const Complex10& left, const Complex10& right, Complex10& result );

    private:
        HRESULT EvaluateShortcutComplex( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class ModExpr : public ArithmeticBinExpr
    {
    public:
        ModExpr( Expression* left, Expression* right );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );

    protected:
        virtual HRESULT UInt64Op( uint64_t left, uint64_t right, uint64_t& result );
        virtual HRESULT Int64Op( int64_t left, int64_t right, int64_t& result );
        virtual HRESULT Float80Op( const Real10& left, const Real10& right, Real10& result );
        virtual HRESULT Complex80Op( const Complex10& left, const Complex10& right, Complex10& result );

    private:
        HRESULT EvaluateShortcutComplex( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class PowExpr : public ArithmeticBinExpr
    {
    public:
        PowExpr( Expression* left, Expression* right );

    protected:
        virtual HRESULT UInt64Op( uint64_t left, uint64_t right, uint64_t& result );
        virtual HRESULT Int64Op( int64_t left, int64_t right, int64_t& result );
        virtual HRESULT Float80Op( const Real10& left, const Real10& right, Real10& result );
        virtual HRESULT Complex80Op( const Complex10& left, const Complex10& right, Complex10& result );
    };


    class AddressOfExpr : public Expression
    {
    public:
        RefPtr<Expression>  Child;

        AddressOfExpr( Expression* child );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class PointerExpr : public Expression
    {
    public:
        RefPtr<Expression>  Child;

        PointerExpr( Expression* child );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class NegateExpr : public Expression
    {
    public:
        RefPtr<Expression>  Child;

        NegateExpr( Expression* child );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class UnaryAddExpr : public Expression
    {
    public:
        RefPtr<Expression>  Child;

        UnaryAddExpr( Expression* child );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class NotExpr : public Expression
    {
    public:
        RefPtr<Expression>  Child;

        NotExpr( Expression* child );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class BitNotExpr : public Expression
    {
    public:
        RefPtr<Expression>  Child;

        BitNotExpr( Expression* child );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class NewExpr : public Expression
    {
    public:
        NewExpr();
    };


    class DeleteExpr : public Expression
    {
    public:
        RefPtr<Expression>  Child;

        DeleteExpr( Expression* child );
    };


    class CastExpr : public Expression
    {
    public:
        RefPtr<Expression>  Child;
        MOD                 FlagsTo;
        RefPtr<Type>        _TypeTo;

        CastExpr( Expression* child, MOD flags );
        CastExpr( Expression* child, Type* type );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );

        static bool CanImplicitCast( Type* source, Type* dest );
        static bool CanCast( Type* source, Type* dest );
        static void AssignValue( const DataObject& source, DataObject& dest );
    };


    class NamingExpression : public Expression
    {
    public:
        RefPtr<Declaration> Decl;

        virtual NamingExpression* AsNamingExpression() { return this; }

    protected:
        virtual void ClearEvalData()
        {
            Expression::ClearEvalData();
            Decl = NULL;
        }
    };


    class DotExpr : public NamingExpression
    {
        RefPtr<SharedString>    mNamePath;
        // the length of the name path from the root to this node
        // we need to assign this value right after we append to the name path
        // because our parent might append its own name and change the length
        uint32_t                mNamePathLen;

    public:
        RefPtr<Expression>  Child;
        Utf16String*        Id;
        StdProperty*        Property;

        DotExpr( Expression* child, Utf16String* id );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );

    protected:
        virtual HRESULT MakeName( uint32_t capacity, RefPtr<SharedString>& namePath );

    private:
        HRESULT SemanticStdProperty( ITypeEnv* typeEnv );
        HRESULT EvaluateStdProperty( const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class DotTemplateInstanceExpr : public NamingExpression
    {
        RefPtr<SharedString>    mNamePath;
        // the length of the name path from the root to this node
        // we need to assign this value right after we append to the name path
        // because our parent might append its own name and change the length
        uint32_t                mNamePathLen;

    public:
        RefPtr<Expression>  Child;
        RefPtr<TemplateInstancePart>    Instance;

        DotTemplateInstanceExpr( Expression* child, TemplateInstancePart* instance );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );

    protected:
        virtual HRESULT MakeName( uint32_t capacity, RefPtr<SharedString>& namePath );
    };


    class CallExpr : public Expression
    {
    public:
        RefPtr<Expression>      Child;
        RefPtr<ExpressionList>  Args;

        CallExpr( Expression* child, ExpressionList* args );
    };


    class PostExpr : public Expression
    {
    public:
        RefPtr<Expression>  Child;
        TOK                 Operator;

        PostExpr( Expression* child, TOK op );
    };


    class IndexExpr : public Expression
    {
    public:
        RefPtr<Expression>      Child;
        RefPtr<ExpressionList>  Args;

        IndexExpr( Expression* child, ExpressionList* args );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class SliceExpr : public Expression
    {
    public:
        RefPtr<Expression>  Child;
        RefPtr<Expression>  From;
        RefPtr<Expression>  To;

        SliceExpr( Expression* child, Expression* from, Expression* to );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class IdExpr : public NamingExpression
    {
        RefPtr<SharedString>    mNamePath;

    public:
        Utf16String*    Id;

        IdExpr( Utf16String* id );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );

    protected:
        virtual HRESULT MakeName( uint32_t capacity, RefPtr<SharedString>& namePath );

    private:
        HRESULT FindObject( const wchar_t* name, IValueBinder* binder, Declaration*& decl );
        HRESULT GetThisAddress( IValueBinder* binder, Address& addr );
    };


    class ScopeExpr : public NamingExpression
    {
        RefPtr<SharedString>    mNamePath;

    public:
        RefPtr<TemplateInstancePart>    Instance;

        ScopeExpr( TemplateInstancePart* instance );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );

    protected:
        virtual HRESULT MakeName( uint32_t capacity, RefPtr<SharedString>& namePath );
    };


    class ThisExpr : public NamingExpression
    {
    public:
        ThisExpr();
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class SuperExpr : public NamingExpression
    {
    public:
        SuperExpr();
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class DollarExpr : public Expression
    {
    public:
        DollarExpr();
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class TypeidExpr : public Expression
    {
    public:
        RefPtr<Object>  Child;

        TypeidExpr( Object* child );
    };


    class IsExpr : public Expression
    {
    public:
        IsExpr();
    };


    class TraitsExpr : public Expression
    {
    public:
        TraitsExpr();
    };


    class IntExpr : public Expression
    {
    public:
        uint64_t    Value;

        IntExpr( uint64_t value, Type* type );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class RealExpr : public Expression
    {
    public:
        Real10      Value;

        RealExpr( Real10 value, Type* type );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };


    class StringExpr : public Expression
    {
        class AlternateStrings
        {
            Utf16String     mNewUtf16Str;
            Utf32String     mNewUtf32Str;

            UniquePtr<wchar_t[]>    mStrBuf16;
            UniquePtr<dchar_t[]>    mStrBuf32;

        public:
            AlternateStrings();

            Utf16String* GetUtf16String();
            Utf32String* GetUtf32String();

            HRESULT SetUtf16String( const char* utf8Str, int utf8Length );
            HRESULT SetUtf32String( const char* utf8Str, int utf8Length );
        };

        ByteString* mUntypedStr;
        std::auto_ptr<AlternateStrings> mAlternates;

    public:
        String*     Value;
        bool        IsSpecificType;

        StringExpr( String* value, bool isSpecificType );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
        virtual bool TrySetType( Type* type );
    };


    class ArrayLiteralExpr : public Expression
    {
    public:
        RefPtr<ExpressionList>  Values;

        ArrayLiteralExpr( ExpressionList* values );
    };


    class AssocArrayLiteralExpr : public Expression
    {
    public:
        RefPtr<ExpressionList>  Keys;
        RefPtr<ExpressionList>  Values;

        AssocArrayLiteralExpr( ExpressionList* keys, ExpressionList* values );
    };


    class NullExpr : public Expression
    {
    public:
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
        virtual bool TrySetType( Type* type );
    };


    class TypeExpr : public Expression
    {
    public:
        RefPtr<Type>    UnresolvedType;

        TypeExpr( Type* type );
        virtual HRESULT Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder );
        virtual HRESULT Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj );
    };
}
