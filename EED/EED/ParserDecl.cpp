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
    bool    Parser::IsDeclaration( const Token* t, DeclIdMode idMode, TOK endCode, const Token** endToken )
    {
        bool    haveId = false;

        if ( ((t->Code == TOKconst) ||
            (t->Code == TOKinvariant) ||
            (t->Code == TOKimmutable) ||
            (t->Code == TOKshared))
            && (PeekToken( t )->Code != TOKlparen) )
        {
            t = PeekToken( t );
        }

        if ( !IsBasicType( &t ) )
            return false;

        if ( !IsDeclarator( &t, haveId, endCode ) )
            return false;

        if ( (idMode == DeclId_Optional)
            || ((idMode == DeclId_None) && !haveId)
            || ((idMode == DeclId_Needed) && haveId) )
        {
            if ( endToken != NULL )
                *endToken = t;

            return true;
        }

        return false;
    }

    bool    Parser::IsBasicType( const Token** curToken )
    {
        const Token*    t = *curToken;

        if ( IsPrimitiveType( t->Code ) )
        {
            t = PeekToken( t );
        }
        else if ( (t->Code == TOKconst) 
            || (t->Code == TOKinvariant) 
            || (t->Code == TOKimmutable) 
            || (t->Code == TOKshared) )
        {
        // const(type)  or  immutable(type)  or  shared(type)
            t = PeekToken( t );
            if ( t->Code != TOKlparen )
                return false;
            t = PeekToken( t );
            if ( !IsDeclaration( t, DeclId_None, TOKrparen, &t ) )
                return false;
            t = PeekToken( t );
        }
        else if ( t->Code == TOKdot )
        {
            t = PeekToken( t );
            if ( !IsTypeNameIdSequence( t, &t ) )
                return false;
        }
        else if ( t->Code == TOKtypeof )
        {
            t = PeekToken( t );
            if ( t->Code != TOKlparen )
                return false;
            if ( !SkipParens( t, &t ) )
                return false;
            if ( t->Code == TOKdot )
            {
                t = PeekToken( t );
                if ( !IsTypeNameIdSequence( t, &t ) )
                    return false;
            }
        }
        else
        {
            if ( !IsTypeNameIdSequence( t, &t ) )
                return false;
        }

        *curToken = t;

        return true;
    }

    bool    Parser::IsTypeNameIdSequence( const Token* curToken, const Token** endToken )
    {
        const Token*    t = curToken;

        for ( ; ; )
        {
            if ( t->Code != TOKidentifier )
                return false;
            t = PeekToken( t );         // skip ID

            if ( t->Code == TOKdot )
            {
                t = PeekToken( t );
            }
            else if ( t->Code == TOKnot )
            {
                t = PeekToken( t );

                if ( t->Code == TOKlparen )
                {
                    if ( !SkipParens( t, &t ) )
                        return false;
                }
                else if ( IsPrimitiveType( t->Code )
                    || IsBasicLiteral( t->Code ) )
                {
                    t = PeekToken( t );
                }
                else
                    continue;

                if ( t->Code == TOKdot )
                {
                    t = PeekToken( t );
                }
                else
                    break;
            }
            else
                break;
        }

        if ( endToken != NULL )
            *endToken = t;

        return true;
    }

    bool    Parser::SkipParens( const Token* curToken, const Token** endToken )
    {
        const Token*    t = curToken;
        int     parenCount = 0;
        bool    done = false;

        for ( ; !done ; )
        {
            switch ( t->Code )
            {
            case TOKlparen:
                parenCount++;
                break;

            case TOKrparen:
                parenCount--;
                if ( parenCount < 0 )
                    return false;
                if ( parenCount == 0 )
                    done = true;
                break;

            case TOKeof:
            case TOKsemicolon:
                return false;

            default:
                break;
            }

            t = PeekToken( t );
        }

        if ( endToken != NULL )
            *endToken = t;

        return true;
    }

    bool    Parser::IsPrimitiveType( TOK code )
    {
        switch ( code )
        {
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
        case TOKchar:
        case TOKwchar:
        case TOKdchar:
        case TOKbit:
        case TOKbool:
            return true;
        }

        return false;
    }

    bool    Parser::IsBasicLiteral( TOK code )
    {
        switch ( code )
        {
        case TOKint32v:
        case TOKuns32v:
        case TOKint64v:
        case TOKuns64v:
        case TOKfloat32v:
        case TOKfloat64v:
        case TOKfloat80v:
        case TOKimaginary32v:
        case TOKimaginary64v:
        case TOKimaginary80v:
        case TOKnull:
        case TOKtrue:
        case TOKfalse:
        case TOKcharv:
        case TOKwcharv:
        case TOKdcharv:
        case TOKstring:
        case TOKfile:
        case TOKline:
            return true;
        }

        return false;
    }

    bool    Parser::IsDeclarator( const Token** curToken, bool& haveId, TOK endCode )
    {
        const Token*    t = *curToken;

        haveId = false;

        if ( t->Code == TOKassign )
            return false;

        for ( ; ; )
        {
            switch ( t->Code )
            {
            case TOKmul:
                t = PeekToken( t );
                continue;

            case TOKlbracket:
                t = PeekToken( t );
                if ( t->Code == TOKrbracket )
                {
                    t = PeekToken( t );
                }
                else if ( (t->Code == TOKnew) && (PeekToken( t )->Code == TOKrbracket) )
                {
                    t = PeekToken( t );
                    t = PeekToken( t );
                }
                else if ( IsDeclaration( t, DeclId_None, TOKrbracket, &t ) )
                {
                    // associative array
                    t = PeekToken( t );
                }
                else
                {
                    if ( !IsExpression( &t ) )
                        return false;
                    if ( t->Code == TOKslice )
                    {
                        t = PeekToken( t );
                        if ( !IsExpression( &t ) )
                            return false;
                    }
                    if ( t->Code != TOKrbracket )
                        return false;
                    t = PeekToken( t );
                }
                continue;

            case TOKidentifier:
                t = PeekToken( t );
                haveId = true;
                break;

            case TOKlparen:
                // leave out C-style function pointers
                return false;

            case TOKdelegate:
            case TOKfunction:
                t = PeekToken( t );
                if ( !IsParameters( &t ) )
                    return false;
                continue;
            }
            break;
        }

        switch ( t->Code )
        {
            // Valid tokens that follow a declaration
        case TOKrparen:
        case TOKrbracket:
        case TOKassign:
        case TOKcomma:
        case TOKsemicolon:
        case TOKlcurly:
        case TOKin:
            if ( (endCode == TOKreserved) || (endCode == t->Code) )
            {
                *curToken = t;
                return true;
            }
            break;
        }

        return false;
    }

    bool    Parser::IsExpression( const Token** curToken )
    {
        const Token*    t = *curToken;
        int bracketNest = 0;
        int parenNest = 0;
        int curlyNest = 0;

        for ( ; ; t = PeekToken( t ) )
        {
            switch ( t->Code )
            {
            case TOKlbracket:
                bracketNest++;
                continue;

            case TOKrbracket:
                bracketNest--;
                if ( bracketNest >= 0 )
                    continue;
                break;

            case TOKlparen:
                parenNest++;
                continue;

            case TOKcomma:
                if ( (bracketNest > 0) || (parenNest > 0) )
                    continue;
                break;

            case TOKrparen:
                parenNest--;
                if ( parenNest >= 0 )
                    continue;
                break;

            case TOKlcurly:
                curlyNest++;
                continue;

            case TOKrcurly:
                curlyNest--;
                if ( curlyNest >= 0 )
                    continue;
                return false;

            case TOKslice:
                if ( bracketNest > 0 )
                    continue;
                break;

            case TOKsemicolon:
                if ( curlyNest > 0 )
                    continue;
                return false;

            case TOKeof:
                return false;

            default:
                continue;
            }
            break;
        }

        *curToken = t;
        return true;
    }

    bool    Parser::IsParameters( const Token** curToken )
    {
        const Token*    t = *curToken;

        if ( t->Code != TOKlparen )
            return false;
        t = PeekToken( t );

        for ( ; ; t = PeekToken( t ) )
        {
            switch ( t->Code )
            {
            case TOKrparen:
                goto Done;

            case TOKdotdotdot:
                t = PeekToken( t );
                goto Done;

            case TOKin:
            case TOKout:
            case TOKinout:
            case TOKref:
            case TOKlazy:
            case TOKfinal:
                continue;

            case TOKconst:
            case TOKinvariant:
            case TOKimmutable:
            case TOKshared:
                if ( PeekToken( t )->Code == TOKlparen )
                {
                    t = PeekToken( t );
                    t = PeekToken( t );
                    if ( !IsDeclaration( t, DeclId_None, TOKrparen, &t ) )
                        return false;
                    t = PeekToken( t ); // skip past closing ')'
                    break;
                }
                continue;

            default:
                if ( !IsBasicType( &t ) )
                    return false;
                break;
            }

            bool    haveId;
            if ( (t->Code != TOKdotdotdot)
                && !IsDeclarator( &t, haveId, TOKreserved ) )
                return false;

            if ( t->Code == TOKassign )
            {
                t = PeekToken( t );
                if ( !IsExpression( &t ) )
                    return false;
            }
            if ( t->Code == TOKdotdotdot )
            {
                t = PeekToken( t );
                break;
            }

            if ( t->Code == TOKcomma )
            {
                if ( PeekToken( t )->Code == TOKrparen )
                    return false;
            }
        }
Done:

        if ( t->Code != TOKrparen )
            return false;
        t = PeekToken( t );

        *curToken = t;
        return true;
    }

    RefPtr<Type>        Parser::ParseType()
    {
        RefPtr<Type>    type;
        const Token&    token = GetToken();

        // first, storage class prefixes that serve as type attributes:
        // const shared, shared const, const, invariant, shared

        if ( token.Code == TOKconst && PeekTokenCode() == TOKshared && PeekTokenCode( 2 ) != TOKlparen ||
            token.Code == TOKshared && PeekTokenCode() == TOKconst && PeekTokenCode( 2 ) != TOKlparen )
        {
            NextToken();
            NextToken();
            /* shared const type
            */
            type = ParseType();
            type = type->MakeSharedConst();
            return type;
        }
        else if ( token.Code == TOKconst && PeekTokenCode() != TOKlparen )
        {
            NextToken();
            /* const type
            */
            type = ParseType();
            type = type->MakeConst();
            return type;
        }
        else if ( (token.Code == TOKinvariant || token.Code == TOKimmutable) &&
            PeekTokenCode() != TOKlparen )
        {
            NextToken();
            /* invariant type
            */
            type = ParseType();
            type = type->MakeInvariant();
            return type;
        }
        else if ( token.Code == TOKshared && PeekTokenCode() != TOKlparen )
        {
            NextToken();
            /* shared type
            */
            type = ParseType();
            type = type->MakeShared();
            return type;
        }
        else
            type = ParseBasicType();

        type = ParseDeclarator( type.Get(), NULL );
        return type;
    }

    RefPtr<Type>        Parser::ParseBasicType()
    {
        RefPtr<Type>            type;
        RefPtr<TypeQualified>   tid;

        switch ( GetTokenCode() )
        {
        case TOKidentifier:
            tid = ParseTypeName( NULL );
            type.Attach( tid.Detach() );
            break;

        case TOKdot:
            tid = new TypeIdentifier( mScanner->GetNameTable()->GetEmpty() );
            tid = ParseTypeName( tid.Get() );
            type.Attach( tid.Detach() );
            break;

        case TOKtypeof:
            tid = ParseTypeof();
            if ( GetTokenCode() == TOKdot )
            {
                NextToken();
                tid = ParseTypeName( tid.Get() );
            }
            type.Attach( tid.Detach() );
            break;

        case TOKconst:
            // const(type)
            NextToken();
            Match( TOKlparen );
            type = ParseType();
            Match( TOKrparen );
            if ( type->IsShared() )
                type = type->MakeSharedConst();
            else
                type = type->MakeConst();
            break;

        case TOKinvariant:
        case TOKimmutable:
            // invariant(type)
            NextToken();
            Match( TOKlparen );
            type = ParseType();
            Match( TOKrparen );
            type = type->MakeInvariant();
            break;

        case TOKshared:
            // shared(type)
            NextToken();
            Match( TOKlparen );
            type = ParseType();
            Match( TOKrparen );
            if ( type->IsConst() )
                type = type->MakeSharedConst();
            else
                type = type->MakeShared();
            break;

        default:
            type = FindBasicType( GetTokenCode() );
            if ( type == NULL )
                throw 25;
            NextToken();
            break;
        }

        return type;
    }

    RefPtr<Type>        Parser::ParseBasicType2( Type* type )
    {
        RefPtr<Type>    type2 = type;
        int             ptrSize = mTypeEnv->GetVoidPointerType()->GetSize();

        for ( ; ; )
        {
            switch ( GetTokenCode() )
            {
            case TOKmul:
                type2 = new TypePointer( type2.Get(), ptrSize );
                NextToken();
                break;

            case TOKlbracket:
                NextToken();
                if ( GetTokenCode() == TOKrbracket )
                {
                    RefPtr<Type>    newType;
                    HRESULT hr = mTypeEnv->NewDArray( type2, newType.Ref() );
                    if ( FAILED( hr ) )
                        throw 90;
                    NextToken();
                    type2 = newType;
                }
                else if ( IsDeclaration( &GetToken(), DeclId_None, TOKrbracket, NULL ) )
                {
                    RefPtr<Type>    newType;
                    RefPtr<Type>    index = ParseType();
                    HRESULT hr = mTypeEnv->NewAArray( type2, index, newType.Ref() );
                    if ( FAILED( hr ) )
                        throw 90;
                    Match( TOKrbracket );
                    type2 = newType;
                }
                else
                {
                    mBracketCount++;
                    RefPtr<Expression>  e = ParseAssignExpr();
                    if ( GetTokenCode() == TOKslice )
                    {
                        NextToken();
                        RefPtr<Expression>  e2 = ParseAssignExpr();
                        type2 = new TypeSlice( type2.Get(), e.Get(), e2.Get() );
                    }
                    else
                    {
                        type2 = new TypeSArrayUnresolved( type2.Get(), e.Get() );
                    }
                    mBracketCount--;
                    Match( TOKrbracket );
                }
                break;

            case TOKfunction:
            case TOKdelegate:
                {
                    // Handle delegate declaration:
                    //  t delegate(parameter list) nothrow pure
                    //  t function(parameter list) nothrow pure
                    RefPtr<ParameterList>   params;
                    TOK tokCode = GetTokenCode();
                    bool ispure = false;
                    bool isnothrow = false;
                    bool isproperty = false;
                    TRUST trust = TRUSTdefault;
                    int varArgs = 0;

                    NextToken();
                    params = ParseParams( varArgs );
                    for ( ; ; )
                    {
                        TOK code = GetTokenCode();

                        // Postfixes
                        if ( code == TOKpure )
                            ispure = true;
                        else if ( code == TOKnothrow )
                            isnothrow = true;
                        else if ( code == TOKat )
                        {
                            StorageClass stc = ParseAttribute();
                            switch ((unsigned)(stc >> 32))
                            {
                            case STCproperty >> 32:
                                isproperty = true;
                                break;
                            case STCsafe >> 32:
                                trust = TRUSTsafe;
                                break;
                            case STCsystem >> 32:
                                trust = TRUSTsystem;
                                break;
                            case STCtrusted >> 32:
                                trust = TRUSTtrusted;
                                break;
                            case 0:
                                break;
                            default:
                                _ASSERT( false );
                            }
                        }
                        else
                            break;
                        NextToken();
                    }

                    RefPtr<TypeFunction>    funcType = new TypeFunction( params.Get(), type2.Get(), varArgs );
                    funcType->SetPure( ispure );
                    funcType->SetNoThrow( isnothrow );
                    funcType->SetProperty( isproperty );
                    funcType->SetTrust( trust );
                    if ( tokCode == TOKdelegate )
                    {
                        RefPtr<TypePointer> ptrType = new TypePointer( funcType, ptrSize );
                        type2 = new TypeDelegate( ptrType );
                    }
                    else
                        type2 = new TypePointer( funcType.Get(), ptrSize );  // pointer to function
                }
                break;

            default:
                return type2;
            }
        }
    }

    RefPtr<Type>        Parser::ParseDeclarator( Type* type, Utf16String** id )
    {
        RefPtr<Type>    type2 = ParseBasicType2( type );

        if ( GetTokenCode() == TOKidentifier )
        {
            if ( id == NULL )
                throw 23;

            *id = GetToken().Utf16Str;
            NextToken();
        }

        return type2;
    }

    RefPtr<TypeQualified>        Parser::ParseTypeName( TypeQualified* typeName )
    {
        TOK                     curTokCode = TOKreserved;
        Utf16String*            id = NULL;
        RefPtr<TypeQualified>   qualified = typeName;

        do
        {
            if ( GetTokenCode() != TOKidentifier )
                throw 20;

            id = GetToken().Utf16Str;
            NextToken();

            if ( GetTokenCode() != TOKnot )
            {
                if ( qualified == NULL )
                    qualified = new TypeIdentifier( id );
                else
                    qualified->Parts.push_back( new IdPart( id ) );
            }
            else
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

                if ( qualified == NULL )
                    qualified = new TypeInstance( instance.Get() );
                else
                    qualified->Parts.push_back( instance.Get() );
            }

            curTokCode = GetTokenCode();
            if ( curTokCode == TOKdot )
                // get it ready for looping around again, which expects an ID
                NextToken();
        } while ( curTokCode == TOKdot );

        return qualified;
    }

    RefPtr<ObjectList>      Parser::ParseTemplateArg()
    {
        RefPtr<ObjectList>  tiArgs = new ObjectList();
        RefPtr<Type>        type;

        switch ( GetTokenCode() )
        {
        case TOKidentifier:
            type = new TypeIdentifier( GetToken().Utf16Str );
            tiArgs->List.push_back( type.Get() );
            NextToken();
            break;

        case TOKint32v:
        case TOKuns32v:
        case TOKint64v:
        case TOKuns64v:
        case TOKfloat32v:
        case TOKfloat64v:
        case TOKfloat80v:
        case TOKimaginary32v:
        case TOKimaginary64v:
        case TOKimaginary80v:
        case TOKnull:
        case TOKtrue:
        case TOKfalse:
        case TOKcharv:
        case TOKwcharv:
        case TOKdcharv:
        case TOKstring:
        case TOKfile:
        case TOKline:
            {
                // Template argument is an expression
                RefPtr<Expression>  e = ParsePrimaryExpr();
                tiArgs->List.push_back( e.Get() );
                break;
            }

        default:
            type = FindBasicType( GetTokenCode() );
            if ( type == NULL )
                throw 25;
            tiArgs->List.push_back( type.Get() );
            NextToken();
            break;
        }

        if ( GetTokenCode() == TOKnot )
            throw 21;

        return tiArgs;
    }

    RefPtr<ObjectList>      Parser::ParseTemplateArgList()
    {
        RefPtr<ObjectList>  tiArgs = new ObjectList();
        RefPtr<Type>        type;

        if ( GetTokenCode() != TOKlparen )
            throw 22;
        NextToken();

        if ( GetTokenCode() != TOKrparen )
        {
            for ( ; ; )
            {
                const Token*    curTok = &GetToken();

                if ( IsDeclaration( curTok, DeclId_None, TOKreserved, NULL ) )
                {
                    RefPtr<Type>    type = ParseType();
                    tiArgs->List.push_back( type.Get() );
                }
                else
                {
                    RefPtr<Expression>  e = ParseAssignExpr();
                    // leave out function literals
                    tiArgs->List.push_back( e.Get() );
                }

                if ( GetTokenCode() != TOKcomma )
                    break;
                NextToken();
            }
        }

        Match( TOKrparen );

        return tiArgs;
    }

    StorageClass Parser::ParseAttribute()
    {
        NextToken();
        StorageClass stc = 0;
        Utf16String*    id = NULL;

        if ( GetTokenCode() != TOKidentifier )
            throw 22;

        id = GetToken().Utf16Str;

        // TODO: constant strings
        if ( wcscmp( id->Str, L"property" ) == 0 )
            stc = STCproperty;
        else if ( wcscmp( id->Str, L"safe" ) == 0 )
            stc = STCsafe;
        else if ( wcscmp( id->Str, L"trusted" ) == 0 )
            stc = STCtrusted;
        else if ( wcscmp( id->Str, L"system" ) == 0 )
            stc = STCsystem;
        else
            throw 23;

        return stc;
    }

    RefPtr<ParameterList>   Parser::ParseParams( int& varArgs )
    {
        RefPtr<ParameterList>   params = new ParameterList();

        varArgs = 0;    // TODO: why not bool?
        Match( TOKlparen );
        
        for ( ; ; )
        {
            RefPtr<Type>    type;
            StorageClass    storageClass = 0;
            StorageClass    stc = 0;
            RefPtr<Parameter>   param;

            for ( ; ; NextToken() )
            {
                switch ( GetTokenCode() )
                {
                case TOKrparen:
                    goto Done;

                case TOKdotdotdot:
                    varArgs = 1;
                    NextToken();
                    goto Done;

                case TOKconst:
                    if ( PeekTokenCode() == TOKlparen )
                        goto LDefault;
                    stc = STCconst;
                    goto L2;

                case TOKinvariant:
                case TOKimmutable:
                    if ( PeekTokenCode() == TOKlparen )
                        goto LDefault;
                    stc = STCimmutable;
                    goto L2;

                case TOKshared:
                    if ( PeekTokenCode() == TOKlparen )
                        goto LDefault;
                    stc = STCshared;
                    goto L2;

                case TOKin:     stc = STCin;    goto L2;
                case TOKout:    stc = STCout;   goto L2;
                case TOKinout:
                case TOKref:    stc = STCref;   goto L2;
                case TOKlazy:   stc = STClazy;  goto L2;
                case TOKscope:  stc = STCscope; goto L2;
                case TOKfinal:  stc = STCfinal; goto L2;
L2:
                    if ( storageClass & stc ||
                        (storageClass & STCin && stc & (STCconst | STCscope)) ||
                        (stc & STCin && storageClass & (STCconst | STCscope))
                        )
                        //error("redundant storage class %s", Token::toChars(token.value));
                        throw 23;
                    storageClass |= stc;
                    CheckStorageClass( storageClass );
                    break;

                default:
LDefault:
                    stc = storageClass & (STCin | STCout | STCref | STClazy);
                    if (stc & (stc - 1))    // if stc is not a power of 2
                        //error("incompatible parameter storage classes");
                        throw 24;
                    if ((storageClass & (STCconst | STCout)) == (STCconst | STCout))
                        //error("out cannot be const");
                        throw 24;
                    if ((storageClass & (STCimmutable | STCout)) == (STCimmutable | STCout))
                        //error("out cannot be immutable");
                        throw 24;
                    if ((storageClass & STCscope) &&
                        (storageClass & (STCref | STCout)))
                        //error("scope cannot be ref or out");
                        throw 24;
                    type = ParseType();
                    if ( GetTokenCode() == TOKdotdotdot )
                    {
                        /* This is:
                        *   at ai ...
                        */

                        if ( storageClass & (STCout | STCref) )
                            //error("variadic argument cannot be out or ref");
                            throw 24;
                        varArgs = 2;
                        param = new Parameter( storageClass, type.Get() );
                        params->List.push_back( param );
                        NextToken();
                        goto Done;
                    }
                    param = new Parameter( storageClass, type.Get() );
                    params->List.push_back( param );
                    if ( GetTokenCode() == TOKcomma )
                    {
                        NextToken();
                        goto LNextParam;
                    }
                }
            }
LNextParam: ;
        }
Done:
        
        Match( TOKrparen );
        return params;
    }

    // Give error on conflicting storage classes
    void Parser::CheckStorageClass( StorageClass stc )
    {
        StorageClass u = stc;
        u &= STCconst | STCimmutable | STCmanifest;
        if ( u & (u - 1) )
            //error("conflicting storage class %s", Token::toChars(token.value));
            throw 24;
        u = stc;
        u &= STCgshared | STCshared | STCtls;
        if ( u & (u - 1) )
            //error("conflicting storage class %s", Token::toChars(token.value));
            throw 24;
        u = stc;
        u &= STCsafe | STCsystem | STCtrusted;
        if ( u & (u - 1) )
            //error("conflicting attribute @%s", token.toChars());
            throw 24;
    }

    RefPtr<Type>    Parser::FindBasicType( TOK code )
    {
        ENUMTY  ty = TMAX;

        switch ( code )
        {
        case TOKvoid:   ty = Tvoid; break;
        case TOKint8:   ty = Tint8; break;
        case TOKuns8:   ty = Tuns8; break;
        case TOKint16:  ty = Tint16; break;
        case TOKuns16:  ty = Tuns16; break;
        case TOKint32:  ty = Tint32; break;
        case TOKuns32:  ty = Tuns32; break;
        case TOKint64:  ty = Tint64; break;
        case TOKuns64:  ty = Tuns64; break;
        case TOKfloat32:    ty = Tfloat32; break;
        case TOKfloat64:    ty = Tfloat64; break;
        case TOKfloat80:    ty = Tfloat80; break;
        case TOKimaginary32: ty = Timaginary32; break;
        case TOKimaginary64: ty = Timaginary64; break;
        case TOKimaginary80: ty = Timaginary80; break;
        case TOKcomplex32:  ty = Tcomplex32; break;
        case TOKcomplex64:  ty = Tcomplex64; break;
        case TOKcomplex80:  ty = Tcomplex80; break;
        case TOKchar:   ty = Tchar; break;
        case TOKwchar:  ty = Twchar; break;
        case TOKdchar:  ty = Tdchar; break;
        case TOKbit:    ty = Tbit; break;
        case TOKbool:   ty = Tbool; break;
        default:
            return NULL;
        }

        return mTypeEnv->GetType( ty );
    }
}
