/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Parser.h"
#include "Scanner.h"
#include "Expression.h"
#include "NameTable.h"
#include "Type.h"
#include "TypeUnresolved.h"
#include "TypeEnv.h"
#include "Declaration.h"

using namespace std;


namespace MagoEE
{
    Parser::Parser( Scanner* scanner, ITypeEnv* typeEnv )
    :   mScanner( scanner ),
        mTypeEnv( typeEnv ),
        mDVer( 2 ),
        mBracketCount( 0 )
    {
        _ASSERT( scanner != NULL );
    }


    RefPtr<Expression> Parser::ParseExpression()
    {
        // there's no need to accept these in a debugger's expression evaluator
        // so, don't bother, and go straight to the next kind of expression
        return ParseAssignExpr();
    }

    RefPtr<Expression> Parser::ParseCommaExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;

        e = ParseAssignExpr();

        while ( GetTokenCode() == TOKcomma )
        {
            NextToken();
            e2 = ParseAssignExpr();
            e = new CommaExpr( e.Get(), e2.Get() );
        }

        return e;
    }

    RefPtr<Expression> Parser::ParseAssignExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;

        e = ParseConditionalExpr();

        for ( ; ; )
        {
            switch ( GetTokenCode() )
            {
            case TOKassign: NextToken(); e2 = ParseAssignExpr(); e = new AssignExpr( e.Get(), e2.Get() ); break;

            case TOKaddass: NextToken(); e2 = ParseAssignExpr(); e = new CombinedAssignExpr( new AddExpr( e.Get(), e2.Get() ) ); break;
            case TOKminass: NextToken(); e2 = ParseAssignExpr(); e = new CombinedAssignExpr( new MinExpr( e.Get(), e2.Get() ) ); break;
            case TOKmulass: NextToken(); e2 = ParseAssignExpr(); e = new CombinedAssignExpr( new MulExpr( e.Get(), e2.Get() ) ); break;
            case TOKdivass: NextToken(); e2 = ParseAssignExpr(); e = new CombinedAssignExpr( new DivExpr( e.Get(), e2.Get() ) ); break;
            case TOKmodass: NextToken(); e2 = ParseAssignExpr(); e = new CombinedAssignExpr( new ModExpr( e.Get(), e2.Get() ) ); break;
            //case TOKpowas: NextToken(); e2 = ParseAssignExpr(); e = new CombinedAssignExpr( new PowExpr( e.Get(), e2.Get() ) ); break;
            case TOKandass: NextToken(); e2 = ParseAssignExpr(); e = new CombinedAssignExpr( new AndExpr( e.Get(), e2.Get() ) ); break;
            case TOKorass:  NextToken(); e2 = ParseAssignExpr(); e = new CombinedAssignExpr( new OrExpr( e.Get(), e2.Get() ) ); break;
            case TOKxorass: NextToken(); e2 = ParseAssignExpr(); e = new CombinedAssignExpr( new XorExpr( e.Get(), e2.Get() ) ); break;
            case TOKshlass: NextToken(); e2 = ParseAssignExpr(); e = new CombinedAssignExpr( new ShiftLeftExpr( e.Get(), e2.Get() ) ); break;
            case TOKshrass: NextToken(); e2 = ParseAssignExpr(); e = new CombinedAssignExpr( new ShiftRightExpr( e.Get(), e2.Get() ) ); break;
            case TOKushrass:NextToken(); e2 = ParseAssignExpr(); e = new CombinedAssignExpr( new UShiftRightExpr( e.Get(), e2.Get() ) ); break;
            case TOKcatass: NextToken(); e2 = ParseAssignExpr(); e = new CombinedAssignExpr( new CatExpr( e.Get(), e2.Get() ) ); break;
            default:
                goto Done;
            }
        }

Done:
        return e;
    }

    RefPtr<Expression> Parser::ParseConditionalExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;
        RefPtr<Expression>  e3;

        e = ParseOrOrExpr();

        if ( GetTokenCode() == TOKquestion )
        {
            e2 = ParseExpression();
            ReadToken( TOKcolon );
            e3 = ParseConditionalExpr();
            e = new ConditionalExpr( e.Get(), e2.Get(), e3.Get() );
        }

        return e;
    }

    RefPtr<Expression> Parser::ParseOrOrExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;

        e = ParseAndAndExpr();

        while ( GetTokenCode() == TOKoror )
        {
            NextToken();
            e2 = ParseAndAndExpr();
            e = new OrOrExpr( e.Get(), e2.Get() );
        }

        return e;
    }

    RefPtr<Expression> Parser::ParseAndAndExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;

        e = ParseOrExpr();

        while ( GetTokenCode() == TOKandand )
        {
            NextToken();
            e2 = ParseOrExpr();
            e = new AndAndExpr( e.Get(), e2.Get() );
        }

        return e;
    }

    RefPtr<Expression> Parser::ParseOrExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;

        e = ParseXorExpr();

        while ( GetTokenCode() == TOKor )
        {
            NextToken();
            e2 = ParseXorExpr();
            e = new OrExpr( e.Get(), e2.Get() );
        }

        return e;
    }

    RefPtr<Expression> Parser::ParseXorExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;

        e = ParseAndExpr();

        while ( GetTokenCode() == TOKxor )
        {
            NextToken();
            e2 = ParseAndExpr();
            e = new XorExpr( e.Get(), e2.Get() );
        }

        return e;
    }

    RefPtr<Expression> Parser::ParseAndExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;

        // D v1
        if ( mDVer < 2 )
        {
            e = ParseEqualExpr();

            while ( GetTokenCode() == TOKand )
            {
                NextToken();
                e2 = ParseEqualExpr();
                e = new AndExpr( e.Get(), e2.Get() );
            }
        }
        else
        {
            e = ParseCmpExpr();

            while ( GetTokenCode() == TOKand )
            {
                NextToken();
                e2 = ParseCmpExpr();
                e = new AndExpr( e.Get(), e2.Get() );
            }
        }

        return e;
    }

    RefPtr<Expression> Parser::ParseCmpExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;

        e = ParseShiftExpr();

        for ( ; ; )
        {
            TOK opCode = GetTokenCode();
    
            switch ( GetTokenCode() )
            {
            case TOKequal:
            case TOKnotequal:
                NextToken();
                e2 = ParseShiftExpr();
                e = new EqualExpr( opCode, e.Get(), e2.Get() );
                break;

            case TOKis:
                NextToken();
                e2 = ParseShiftExpr();
                e = new IdentityExpr( TOKidentity, e.Get(), e2.Get() );
                break;

            case TOKnot:
                if ( PeekTokenCode( 1 ) != TOKis )
                    break;
                NextToken();
                NextToken();
                e2 = ParseShiftExpr();
                e = new IdentityExpr( TOKnotidentity, e.Get(), e2.Get() );
                break;

            case TOKlt:
            case TOKle:
            case TOKgt:
            case TOKge:
            case TOKunord:
            case TOKlg:
            case TOKleg:
            case TOKule:
            case TOKul:
            case TOKuge:
            case TOKug:
            case TOKue:
                NextToken();
                e2 = ParseShiftExpr();
                // D front end only makes CmpExp, and their RelExp seems deprecated
                // it's the other way around here
                e = new RelExpr( opCode, e.Get(), e2.Get() );
                break;

            case TOKin:
                NextToken();
                e2 = ParseShiftExpr();
                e = new InExpr( e.Get(), e2.Get() );
                break;

            default:
                goto Done;
            }
        }

Done:
        return e;
    }

    RefPtr<Expression> Parser::ParseEqualExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;

        e = ParseRelExpr();

        for ( ; ; )
        {
            TOK opCode = GetTokenCode();
    
            switch ( GetTokenCode() )
            {
            case TOKequal:
            case TOKnotequal:
                NextToken();
                e2 = ParseRelExpr();
                e = new EqualExpr( opCode, e.Get(), e2.Get() );
                break;

            case TOKidentity:
            case TOKnotidentity:
            case TOKis:
                if ( opCode == TOKis )
                    opCode = TOKidentity;
                NextToken();
                e2 = ParseRelExpr();
                e = new IdentityExpr( opCode, e.Get(), e2.Get() );
                break;

            case TOKnot:
                if ( PeekTokenCode( 1 ) != TOKis )
                    break;
                NextToken();
                NextToken();
                e2 = ParseRelExpr();
                e = new IdentityExpr( TOKnotidentity, e.Get(), e2.Get() );
                break;

            default:
                goto Done;
            }
        }

Done:
        return e;
    }

    RefPtr<Expression> Parser::ParseRelExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;

        e = ParseShiftExpr();

        for ( ; ; )
        {
            TOK opCode = GetTokenCode();
    
            switch ( GetTokenCode() )
            {
            case TOKlt:
            case TOKle:
            case TOKgt:
            case TOKge:
            case TOKunord:
            case TOKlg:
            case TOKleg:
            case TOKule:
            case TOKul:
            case TOKuge:
            case TOKug:
            case TOKue:
                NextToken();
                e2 = ParseShiftExpr();
                e = new RelExpr( opCode, e.Get(), e2.Get() );
                break;

            case TOKin:
                NextToken();
                e2 = ParseShiftExpr();
                e = new InExpr( e.Get(), e2.Get() );
                break;

            default:
                goto Done;
            }
        }

Done:
        return e;
    }

    RefPtr<Expression> Parser::ParseShiftExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;

        e = ParseAddExpr();

        for ( ; ; )
        {
            switch ( GetTokenCode() )
            {
            case TOKshl:
                NextToken();
                e2 = ParseAddExpr();
                e = new ShiftLeftExpr( e.Get(), e2.Get() );
                break;

            case TOKshr:
                NextToken();
                e2 = ParseAddExpr();
                e = new ShiftRightExpr( e.Get(), e2.Get() );
                break;

            case TOKushr:
                NextToken();
                e2 = ParseAddExpr();
                e = new UShiftRightExpr( e.Get(), e2.Get() );
                break;

            default:
                goto Done;
            }
        }

Done:
        return e;
    }

    RefPtr<Expression> Parser::ParseAddExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;

        e = ParseMulExpr();

        for ( ; ; )
        {
            switch ( GetTokenCode() )
            {
            case TOKadd:
                NextToken();
                e2 = ParseMulExpr();
                e = new AddExpr( e.Get(), e2.Get() );
                break;

            case TOKmin:
                NextToken();
                e2 = ParseMulExpr();
                e = new MinExpr( e.Get(), e2.Get() );
                break;

            case TOKtilde:
                NextToken();
                e2 = ParseMulExpr();
                e = new CatExpr( e.Get(), e2.Get() );
                break;

            default:
                goto Done;
            }
        }

Done:
        return e;
    }

    RefPtr<Expression> Parser::ParseMulExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;

        e = ParseUnaryExpr();

        for ( ; ; )
        {
            switch ( GetTokenCode() )
            {
            case TOKmul:
                NextToken();
                e2 = ParseUnaryExpr();
                e = new MulExpr( e.Get(), e2.Get() );
                break;

            case TOKdiv:
                NextToken();
                e2 = ParseUnaryExpr();
                e = new DivExpr( e.Get(), e2.Get() );
                break;

            case TOKmod:
                NextToken();
                e2 = ParseUnaryExpr();
                e = new ModExpr( e.Get(), e2.Get() );
                break;

            case TOKpow:
                NextToken();
                e2 = ParseUnaryExpr();
                e = new PowExpr( e.Get(), e2.Get() );
                break;

            default:
                goto Done;
            }
        }

Done:
        return e;
    }

    RefPtr<Expression> Parser::ParseUnaryExpr()
    {
        RefPtr<Expression>  e;
        RefPtr<Expression>  e2;

        switch ( GetTokenCode() )
        {
        case TOKand:
            NextToken();
            e = ParseUnaryExpr();
            e = new AddressOfExpr( e.Get() );
            break;

        case TOKplusplus:
            NextToken();
            e = ParseUnaryExpr();
            e2 = new IntExpr( 1, mTypeEnv->GetType( Tint32 ) );
            e = new CombinedAssignExpr( new AddExpr( e.Get(), e2.Get() ) );
            break;

        case TOKminusminus:
            NextToken();
            e = ParseUnaryExpr();
            e2 = new IntExpr( 1, mTypeEnv->GetType( Tint32 ) );
            e = new CombinedAssignExpr( new MinExpr( e.Get(), e2.Get() ) );
            break;

        case TOKmul:
            NextToken();
            e = ParseUnaryExpr();
            e = new PointerExpr( e.Get() );
            break;

        case TOKmin:
            NextToken();
            e = ParseUnaryExpr();
            e = new NegateExpr( e.Get() );
            break;

        case TOKadd:
            NextToken();
            e = ParseUnaryExpr();
            e = new UnaryAddExpr( e.Get() );
            break;

        case TOKnot:
            NextToken();
            e = ParseUnaryExpr();
            e = new NotExpr( e.Get() );
            break;

        case TOKtilde:
            NextToken();
            e = ParseUnaryExpr();
            e = new BitNotExpr( e.Get() );
            break;

        case TOKnew:
            throw 15;
            break;

        case TOKdelete:
            NextToken();
            e = ParseUnaryExpr();
            e = new DeleteExpr( e.Get() );
            break;

        case TOKcast:
            {
                MOD flags = (MOD) -1;
                // none is also a valid way to cast, so use a value outside MOD's range
                
                NextToken();
                ReadToken( TOKlparen );

                if ( GetTokenCode() == TOKrparen )
                    flags = MODnone;
                else if ( (GetTokenCode() == TOKconst) && (PeekTokenCode( 1 ) == TOKrparen) )
                {
                    flags = MODconst;
                    NextToken();
                }
                else if ( (GetTokenCode() == TOKshared) && (PeekTokenCode( 1 ) == TOKrparen) )
                {
                    flags = MODshared;
                    NextToken();
                }
                else if ( ((GetTokenCode() == TOKinvariant) || (GetTokenCode() == TOKimmutable))
                    && (PeekTokenCode( 1 ) == TOKrparen) )
                {
                    flags = MODinvariant;
                    NextToken();
                }
                else if ( (((GetTokenCode() == TOKconst) && (PeekTokenCode( 1 ) == TOKshared))
                    || ((GetTokenCode() == TOKshared) && (PeekTokenCode( 1 ) == TOKconst)))
                    && (PeekTokenCode( 2 ) == TOKrparen) )
                {
                    flags = (MOD) (MODconst | MODshared);
                    NextToken();
                    NextToken();
                }

                if ( flags >= MODnone )
                {
                    NextToken();
                    e = ParseUnaryExpr();
                    e = new CastExpr( e.Get(), flags );
                }
                else
                {
                    RefPtr<Type>    type = ParseType();
                    ReadToken( TOKrparen );
                    e = ParseUnaryExpr();
                    e = new CastExpr( e.Get(), type.Get() );
                }
                break;
            }

        // don't worry about C-style casts
        default:
            e = ParsePrimaryExpr();
            e = ParsePostfixExpr( e.Get() );
            break;
        }

        return e;
    }

    RefPtr<Expression> Parser::ParsePostfixExpr( Expression* child )
    {
        RefPtr<Expression>     e = child;
        const Token*    token = NULL;

        for ( ; ; )
        {
            token = &GetToken();
            switch ( token->Code )
            {
            case TOKarrow: // allow "->" just aswell to improve debugging C/C++
            case TOKdot:
                NextToken();
                // leave out New expression (instead of ID)
                if ( GetTokenCode() == TOKidentifier )
                {
                    Utf16String*    id = GetToken().Utf16Str;
                    NextToken();
                    if ( (GetTokenCode() == TOKnot) && (PeekTokenCode( 1 ) != TOKis) )
                    {
                        RefPtr<TemplateInstancePart>    instance = new TemplateInstancePart( id );
                        const wchar_t*  startPtr = mScanner->GetToken().TextStartPtr;
                        NextToken();

                        if ( GetTokenCode() == TOKlparen )
                            instance->Params = ParseTemplateArgList();
                        else
                            instance->Params = ParseTemplateArg();

                        // get the template args as a string starting from '!'
                        const wchar_t*  endPtr = mScanner->GetToken().TextStartPtr;
                        instance->ArgumentString = mScanner->GetNameTable()->AddString( startPtr, (endPtr - startPtr) );

                        e = new DotTemplateInstanceExpr( e.Get(), instance.Get() );
                    }
                    else
                    {
                        e = new DotExpr( e.Get(), id );
                    }
                }
                else
                    throw 13;
                break;

            case TOKplusplus:
            case TOKminusminus:
                {
                    RefPtr<Expression>  e2 = new IntExpr( 1, mTypeEnv->GetType( Tint32 ) );
                    RefPtr<CombinableBinExpr>   e3;
                    if ( token->Code == TOKplusplus )
                        e3 = new AddExpr( e, e2 );
                    else
                        e3 = new MinExpr( e, e2 );
                    e = new CombinedAssignExpr( e3, true );
                    //e = new PostExpr( e.Get(), token->Code );
                    NextToken();
                }
                break;

            case TOKlparen:
                {
                    RefPtr<ExpressionList>  args = ParseCallArguments();
                    e = new CallExpr( e.Get(), args.Get() );
                }
                break;

            case TOKlbracket:
                mBracketCount++;
                
                NextToken();
                
                if ( GetTokenCode() == TOKrbracket )
                {
                    e = new SliceExpr( e.Get(), NULL, NULL );
                }
                else
                {
                    RefPtr<Expression>  index = ParseAssignExpr();

                    if ( GetTokenCode() == TOKslice )
                    {
                        NextToken();

                        RefPtr<Expression>  limit = ParseAssignExpr();
                        e = new SliceExpr( e.Get(), index.Get(), limit.Get() );
                    }
                    else
                    {
                        RefPtr<ExpressionList>  args = new ExpressionList();

                        args->List.push_back( index );

                        while ( GetTokenCode() == TOKcomma )
                        {
                            NextToken();

                            RefPtr<Expression>  arg = ParseAssignExpr();

                            args->List.push_back( arg );
                        }

                        e = new IndexExpr( e.Get(), args.Get() );
                    }
                }

                ReadToken( TOKrbracket );
                mBracketCount--;
                break;

            default:
                goto Done;
            }
        }

Done:
        return e;
    }

    RefPtr<ExpressionList> Parser::ParseCallArguments()
    {
        RefPtr<ExpressionList>  args = new ExpressionList();

        NextToken();

        for ( ; ; NextToken() )
        {
            RefPtr<Expression>  arg = ParseAssignExpr();

            args->List.push_back( arg );

            if ( GetTokenCode() == TOKrparen )
                break;

            ReadToken( TOKcomma );
        }

        ReadToken( TOKrparen );

        return args;
    }

    RefPtr<Expression> Parser::ParsePrimaryExpr()
    {
        const Token*    token = &GetToken();
        RefPtr<Expression>     e;

        switch ( token->Code )
        {
        case TOKint32v:
            e = new IntExpr( token->UInt64Value, mTypeEnv->GetType( Tint32 ) );
            NextToken();
            break;

        case TOKuns32v:
            e = new IntExpr( token->UInt64Value, mTypeEnv->GetType( Tuns32 ) );
            NextToken();
            break;

        case TOKint64v:
            e = new IntExpr( token->UInt64Value, mTypeEnv->GetType( Tint64 ) );
            NextToken();
            break;

        case TOKuns64v:
            e = new IntExpr( token->UInt64Value, mTypeEnv->GetType( Tuns64 ) );
            NextToken();
            break;

        case TOKfloat32v:
            e = new RealExpr( token->Float80Value, mTypeEnv->GetType( Tfloat32 ) );
            NextToken();
            break;

        case TOKfloat64v:
            e = new RealExpr( token->Float80Value, mTypeEnv->GetType( Tfloat64 ) );
            NextToken();
            break;

        case TOKfloat80v:
            e = new RealExpr( token->Float80Value, mTypeEnv->GetType( Tfloat80 ) );
            NextToken();
            break;

        case TOKimaginary32v:
            e = new RealExpr( token->Float80Value, mTypeEnv->GetType( Timaginary32 ) );
            NextToken();
            break;

        case TOKimaginary64v:
            e = new RealExpr( token->Float80Value, mTypeEnv->GetType( Timaginary64 ) );
            NextToken();
            break;

        case TOKimaginary80v:
            e = new RealExpr( token->Float80Value, mTypeEnv->GetType( Timaginary80 ) );
            NextToken();
            break;

        case TOKcharv:
            e = new IntExpr( token->UInt64Value, mTypeEnv->GetType( Tchar ) );
            NextToken();
            break;

        case TOKwcharv:
            e = new IntExpr( token->UInt64Value, mTypeEnv->GetType( Twchar ) );
            NextToken();
            break;

        case TOKdcharv:
            e = new IntExpr( token->UInt64Value, mTypeEnv->GetType( Tdchar ) );
            NextToken();
            break;

        case TOKstring:
        case TOKcstring:
        case TOKwstring:
        case TOKdstring:
            // TODO: combine strings next to each other
            e = new StringExpr( token->Utf16Str, token->Code != TOKstring );
            NextToken();
            break;

        case TOKtrue:
            e = new IntExpr( 1, mTypeEnv->GetType( Tbool ) );
            NextToken();
            break;

        case TOKfalse:
            e = new IntExpr( 0, mTypeEnv->GetType( Tbool ) );
            NextToken();
            break;

        case TOKnull:
            e = new NullExpr();
            NextToken();
            break;

        case TOKdollar:
            if ( mBracketCount == 0 )
                throw 16;
            e = new DollarExpr();
            NextToken();
            break;

        case TOKdot:
#if defined( KEEP_GLOBAL_SCOPE_EXPRESSION )
            e = new IdExpr( NULL );
            // don't eat the dot, because we want PostExpr to handle it
#else
            NextToken();
            if ( GetTokenCode() != TOKidentifier )
                throw 13;
            goto Lidentifier;
#endif
            break;

        case TOKthis:
            e = new ThisExpr();
            NextToken();
            break;

        case TOKsuper:
            e = new SuperExpr();
            NextToken();
            break;

#if !defined( KEEP_GLOBAL_SCOPE_EXPRESSION )
Lidentifier:
#endif
        case TOKidentifier:
            {
                Utf16String*    id = GetToken().Utf16Str;
                NextToken();
                if ( (GetTokenCode() == TOKnot) && (PeekTokenCode( 1 ) != TOKis) )
                {
                    RefPtr<TemplateInstancePart>    instance = new TemplateInstancePart( id );
                    const wchar_t*  startPtr = mScanner->GetToken().TextStartPtr;
                    NextToken();

                    if ( GetTokenCode() == TOKlparen )
                        instance->Params = ParseTemplateArgList();
                    else
                        instance->Params = ParseTemplateArg();

                    // get the template args as a string starting from '!'
                    const wchar_t*  endPtr = mScanner->GetToken().TextStartPtr;
                    instance->ArgumentString = mScanner->GetNameTable()->AddString( startPtr, (endPtr - startPtr) );

                    e = new ScopeExpr( instance.Get() );
                }
                else
                {
                    e = new IdExpr( id );
                }
            }
            break;

        case TOKlparen:
            // leave out delegate starting with params: ( params ) { body }
            NextToken();
            e = ParseExpression();
            ReadToken( TOKrparen );
            break;

        case TOKtypeof:
            {
                RefPtr<TypeQualified>   t = ParseTypeof();
                e = new TypeExpr( t.Get() );
            }
            break;

        case TOKtypeof2:
            {
                NextToken();
                ReadToken( TOKlparen );
                RefPtr<Type>    t = ParseType();
                ReadToken( TOKrparen );
                e = new TypeExpr( t );
            }
            break;

        case TOKtypeid:
            {
                NextToken();
                ReadToken( TOKlparen );
                RefPtr<Object>  obj;
                if ( IsDeclaration( &GetToken(), DeclId_None, TOKreserved, NULL ) )
                {
                    obj = ParseType().Get();
                }
                else
                {
                    obj = ParseAssignExpr().Get();
                }
                ReadToken( TOKrparen );
                e = new TypeidExpr( obj.Get() );
            }
            break;

        case TOKis:
            throw 16;
            break;

        case TOKtraits:
            throw 16;
            break;

        case TOKlbracket:
            {
                RefPtr<ExpressionList>  keys;
                RefPtr<ExpressionList>  values = new ExpressionList();

                NextToken();

                while ( GetTokenCode() != TOKeof )
                {
                    RefPtr<Expression>  expr = ParseAssignExpr();

                    if ( (GetTokenCode() == TOKcolon) && ((keys.Get() != NULL) || (values->List.size() == 0)) )
                    {
                        NextToken();
                        if ( keys.Get() == NULL )
                            keys = new ExpressionList();
                        keys->List.push_back( expr );
                        expr = ParseAssignExpr();
                    }
                    // we have keys, but current element doesn't have a key
                    else if ( keys.Get() != NULL )
                    {
                        throw 17;
                    }

                    values->List.push_back( expr );

                    if ( GetTokenCode() == TOKrbracket )
                        break;
                    ReadToken( TOKcomma );
                }

                ReadToken( TOKrbracket );

                if ( keys.Get() != NULL )
                    e = new AssocArrayLiteralExpr( keys.Get(), values.Get() );
                else
                    e = new ArrayLiteralExpr( values.Get() );
            }
            break;

        case TOKvoid:
        case TOKint8:
        case TOKuns8:
        case TOKint16:
        case TOKuns16:
        case TOKint32:
        case TOKuns32:
        case TOKint64:
        case TOKuns64:
        case TOKfloat32:
        case TOKfloat64:
        case TOKfloat80:
        case TOKimaginary32:
        case TOKimaginary64:
        case TOKimaginary80:
        case TOKcomplex32:
        case TOKcomplex64:
        case TOKcomplex80:
        case TOKbit:
        case TOKbool:
        case TOKchar:
        case TOKwchar:
        case TOKdchar:
            {
                RefPtr<Type>    type = FindBasicType( GetTokenCode() );
                NextToken();
                Match( TOKdot );
                if ( GetTokenCode() != TOKidentifier )
                    throw 26;
                RefPtr<TypeExpr>    typeExpr = new TypeExpr( type.Get() );
                RefPtr<DotExpr>     dotExpr = new DotExpr( typeExpr.Get(), GetToken().Utf16Str );
                e = dotExpr.Get();
                NextToken();
            }
            break;

        case TOKfunction:
        case TOKdelegate:
        case TOKlcurly:
        case TOKassert:
        case TOKmixin:
        case TOKimport:
            throw 12;
            break;

        default:
            throw 11;
        }

        return e;
    }

    RefPtr<TypeQualified>  Parser::ParseTypeof()
    {
        RefPtr<TypeQualified>   t;

        NextToken();
        ReadToken( TOKlparen );

        if ( GetTokenCode() == TOKreturn )
        {
            t = new TypeReturn();
        }
        else
        {
            RefPtr<Expression>  e = ParseExpression();
            t = new TypeTypeof( e.Get() );
        }

        ReadToken( TOKrparen );

        return t;
    }

    void Parser::ReadToken( TOK code )
    {
        if ( mScanner->GetToken().Code != code )
            throw 13;

        mScanner->NextToken();
    }

    void Parser::NextToken()
    {
        mScanner->NextToken();
    }

    const Token& Parser::GetToken()
    {
        return mScanner->GetToken();
    }

    TOK Parser::GetTokenCode()
    {
        return mScanner->GetToken().Code;
    }

    TOK Parser::PeekTokenCode( int index )
    {
        return mScanner->PeekToken( index ).Code;
    }

    const Token* Parser::PeekToken( const Token* curToken )
    {
        return mScanner->PeekToken( curToken );
    }
}
