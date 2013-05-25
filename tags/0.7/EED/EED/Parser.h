/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

enum TOK;
struct Token;


namespace MagoEE
{
    class Scanner;
    class ITypeEnv;
    class Expression;
    class ExpressionList;
    class Type;
    class TypeQualified;
    class ObjectList;
    struct Utf16String;
    class ParameterList;


    class Parser
    {
        enum DeclIdMode
        {
            DeclId_None,
            DeclId_Optional,
            DeclId_Needed
        };

        Scanner*        mScanner;
        ITypeEnv*       mTypeEnv;
        uint32_t        mDVer;
        int             mBracketCount;

    public:
        Parser( Scanner* scanner, ITypeEnv* typeEnv );

        RefPtr<Expression> ParseExpression();
        RefPtr<Expression> ParseCommaExpr();
        RefPtr<Expression> ParseAssignExpr();
        RefPtr<Expression> ParseConditionalExpr();
        RefPtr<Expression> ParseOrOrExpr();
        RefPtr<Expression> ParseAndAndExpr();
        RefPtr<Expression> ParseOrExpr();
        RefPtr<Expression> ParseXorExpr();
        RefPtr<Expression> ParseAndExpr();
        RefPtr<Expression> ParseCmpExpr();
        RefPtr<Expression> ParseEqualExpr();
        RefPtr<Expression> ParseRelExpr();
        RefPtr<Expression> ParseShiftExpr();
        RefPtr<Expression> ParseAddExpr();
        RefPtr<Expression> ParseMulExpr();
        RefPtr<Expression> ParseUnaryExpr();
        RefPtr<Expression> ParsePostfixExpr( Expression* child );
        RefPtr<Expression> ParsePrimaryExpr();
        
        RefPtr<ExpressionList>  ParseCallArguments();
        RefPtr<TypeQualified>   ParseTypeof();
        RefPtr<Type>            ParseType();
        RefPtr<Type>            ParseBasicType();
        RefPtr<Type>            ParseBasicType2( Type* type );
        RefPtr<Type>            ParseDeclarator( Type* type, Utf16String** id );
        RefPtr<TypeQualified>   ParseTypeName( TypeQualified* typeName );
        RefPtr<ObjectList>      ParseTemplateArg();
        RefPtr<ObjectList>      ParseTemplateArgList();
        StorageClass            ParseAttribute();
        RefPtr<ParameterList>   ParseParams( int& varArgs );

        bool    IsDeclaration( const Token* curToken, DeclIdMode idMode, TOK endCode, const Token** endToken );
        bool    IsBasicType( const Token** curToken );
        bool    IsDeclarator( const Token** curToken, bool& haveId, TOK endCode );
        bool    IsPrimitiveType( TOK code );
        bool    IsBasicLiteral( TOK code );
        bool    IsTypeNameIdSequence( const Token* curToken, const Token** endToken );
        bool    IsExpression( const Token** curToken );
        bool    IsParameters( const Token** curToken );
        bool    SkipParens( const Token* curToken, const Token** endToken );

    private:
        void Match( TOK code ) { ReadToken( code ); }
        void ReadToken( TOK code );
        void NextToken();
        const ::Token& GetToken();
        TOK GetTokenCode();
        TOK PeekTokenCode( int index = 1 );
        const Token* PeekToken( const Token* curToken );

        void CheckStorageClass( StorageClass stc );
        RefPtr<Type>    FindBasicType( TOK code );
    };
}
