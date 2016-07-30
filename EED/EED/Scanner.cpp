/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Scanner.h"
#include "Token.h"
#include "Keywords.h"
#include "NamedChars.h"
#include "UniAlpha.h"
#include "NameTable.h"

using namespace std;


namespace MagoEE
{
    Scanner::Scanner( const wchar_t* inBuffer, size_t inBufferLen, NameTable* nameTable )
        :   mDVer( 2 ),
            mInBuf( inBuffer ),
            mBufLen( inBufferLen ),
            mCurPtr( inBuffer ),
            mEndPtr( inBuffer + inBufferLen ),
            mCurNodes( NULL ),
            mFreeNodes( NULL ),
            mNameTable( nameTable )
    {
        memset( &mTok, 0, sizeof mTok );
        mCurNodes = NewNode();
    }

    Scanner::~Scanner()
    {
        while ( mCurNodes != NULL )
        {
            TokenNode*  node = mCurNodes;
            mCurNodes = mCurNodes->Next;
            delete node;
        }

        while ( mFreeNodes != NULL )
        {
            TokenNode*  node = mFreeNodes;
            mFreeNodes = mFreeNodes->Next;
            delete node;
        }
    }

    NameTable* Scanner::GetNameTable()
    {
        return mNameTable;
    }

    TOK Scanner::NextToken()
    {
        if ( mCurNodes->Next != NULL )
        {
            TokenNode*  node = mCurNodes;
            mCurNodes = mCurNodes->Next;

            node->Next = mFreeNodes;
            mFreeNodes = node;

            mTok = *mCurNodes;
        }
        else
        {
            Scan();
            *mCurNodes = mTok;
        }

#ifdef SCANNER_DEBUG
        printf( "%d ", mTok.Code );
#endif
        return mTok.Code;
    }

    const Token& Scanner::PeekToken( int index )
    {
        if ( (index > TokenQueueMaxSize) || (index < 0) )
            throw 30;

        int n = 0;
        TokenNode*  node = mCurNodes;

        for ( n = 0; (n < index) && (node->Next != NULL); n++ )
            node = node->Next;

        for ( ; n < index; n++ )
        {
            Scan();
            node->Next = NewNode();
            node = node->Next;
            *node = mTok;
        }

        return *node;
    }

    const Token* Scanner::PeekToken( const Token* token )
    {
        _ASSERT( token != NULL );

        TokenNode*  node = (TokenNode*) token;

        if ( node->Next == NULL )
        {
            Scan();
            node->Next = NewNode();
            *node->Next = mTok;
        }
    
        return node->Next;
    }

    const Token& Scanner::GetToken()
    {
        return *mCurNodes;
    }

    void Scanner::Scan()
    {
        for ( ; ; )
        {
            mTok.TextStartPtr = GetCharPtr();

            switch ( GetChar() )
            {
            case L'\0':
            case 0x001A:
                mTok.Code = TOKeof;
                return;

            case L' ':
            case L'\t':
            case L'\v':
            case L'\f':
                NextChar();
                break;

            case L'\r':
            case L'\n':
            case 0x2028:
            case 0x2029:
                NextChar();
                break;

            case L'0':      case L'1':   case L'2':   case L'3':   case L'4':
            case L'5':      case L'6':   case L'7':   case L'8':   case L'9':
                ScanNumber();
                return;

            case L'\'':
                ScanCharLiteral();
                return;

            case L'"':
                ScanStringLiteral();
                return;

            case L'q':
                if ( PeekChar() == L'"' )
                {
                    NextChar();
                    ScanDelimitedStringLiteral();
                }
                else if ( PeekChar() == L'{' )
                {
                    NextChar();
                    ScanTokenStringLiteral();
                }
                else
                    goto case_ident;
                return;

            case L'r':
                if ( PeekChar() != L'"' )
                    goto case_ident;
                NextChar();
            case L'`':
                ScanWysiwygString();
                return;

            case L'x':
                if ( PeekChar() != L'"' )
                    goto case_ident;
                NextChar();
                ScanHexString();
                return;

            case L'a':      case L'b':   case L'c':   case L'd':   case L'e':
            case L'f':      case L'g':   case L'h':   case L'i':   case L'j':
            case L'k':      case L'l':   case L'm':   case L'n':   case L'o':
#if DMDV2
            case L'p':      /*case L'q': case L'r':*/ case L's':   case L't':
#else
            case L'p':      case L'q': /*case L'r':*/ case L's':   case L't':
#endif
            case L'u':      case L'v':   case L'w': /*case L'x':*/ case L'y':
            case L'z':
            case L'A':      case L'B':   case L'C':   case L'D':   case L'E':
            case L'F':      case L'G':   case L'H':   case L'I':   case L'J':
            case L'K':      case L'L':   case L'M':   case L'N':   case L'O':
            case L'P':      case L'Q':   case L'R':   case L'S':   case L'T':
            case L'U':      case L'V':   case L'W':   case L'X':   case L'Y':
            case L'Z':
            case L'_':
            case_ident:
                ScanIdentOrKeyword();
                return;

            case L'/':
                NextChar();
                if ( GetChar() == L'=' )
                {
                    mTok.Code = TOKdivass;
                    NextChar();
                }
                else
                    mTok.Code = TOKdiv;
                return;

            case L'.':
                if ( iswdigit( PeekChar() ) )
                {
                    ScanReal();
                }
                else if ( PeekChar() == L'.' )
                {
                    NextChar();
                    NextChar();
                    if ( GetChar() == L'.' )
                    {
                        NextChar();
                        mTok.Code = TOKdotdotdot;
                    }
                    else
                        mTok.Code = TOKslice;
                }
                else
                {
                    NextChar();
                    mTok.Code = TOKdot;
                }
                return;

            case L'&':
                NextChar();
                if ( GetChar() == L'&' )
                {
                    NextChar();
                    mTok.Code = TOKandand;
                }
                else if ( GetChar() == L'=' )
                {
                    NextChar();
                    mTok.Code = TOKandass;
                }
                else
                    mTok.Code = TOKand;
                return;

            case L'|':
                NextChar();
                if ( GetChar() == L'|' )
                {
                    NextChar();
                    mTok.Code = TOKoror;
                }
                else if ( GetChar() == L'=' )
                {
                    NextChar();
                    mTok.Code = TOKorass;
                }
                else
                    mTok.Code = TOKor;
                return;

            case L'-':
                NextChar();
                if ( GetChar() == L'-' )
                {
                    NextChar();
                    mTok.Code = TOKminusminus;
                }
                else if ( GetChar() == L'=' )
                {
                    NextChar();
                    mTok.Code = TOKminass;
                }
                else if ( GetChar() == L'>' )
                {
                    NextChar();
                    mTok.Code = TOKarrow;
                }
                else
                    mTok.Code = TOKmin;
                return;

            case L'+':
                NextChar();
                if ( GetChar() == L'+' )
                {
                    NextChar();
                    mTok.Code = TOKplusplus;
                }
                else if ( GetChar() == L'=' )
                {
                    NextChar();
                    mTok.Code = TOKaddass;
                }
                else
                    mTok.Code = TOKadd;
                return;

            case L'<':
                NextChar();
                if ( GetChar() == L'=' )
                {
                    NextChar();
                    mTok.Code = TOKle;
                }
                else if ( GetChar() == L'<' )
                {
                    NextChar();
                    if ( GetChar() == L'=' )
                    {
                        NextChar();
                        mTok.Code = TOKshlass;
                    }
                    else
                        mTok.Code = TOKshl;
                }
                else if ( GetChar() == L'>' )
                {
                    NextChar();
                    if ( GetChar() == L'=' )
                    {
                        NextChar();
                        mTok.Code = TOKleg;
                    }
                    else
                        mTok.Code = TOKlg;
                }
                else
                    mTok.Code = TOKlt;
                return;

            case L'>':
                NextChar();
                if ( GetChar() == L'=' )
                {
                    NextChar();
                    mTok.Code = TOKge;
                }
                else if ( GetChar() == L'>' )
                {
                    NextChar();
                    if ( GetChar() == L'=' )
                    {
                        NextChar();
                        mTok.Code = TOKshrass;
                    }
                    else if ( GetChar() == L'>' )
                    {
                        NextChar();
                        if ( GetChar() == L'=' )
                        {
                            NextChar();
                            mTok.Code = TOKushrass;
                        }
                        else
                            mTok.Code = TOKushr;
                    }
                    else
                        mTok.Code = TOKshr;
                }
                else
                    mTok.Code = TOKgt;
                return;

            case L'!':
                NextChar();
                if ( GetChar() == L'=' )
                {
                    NextChar();
                    if ( (GetChar() == L'=') && (mDVer < 2) )
                    {
                        NextChar();
                        mTok.Code = TOKnotidentity;
                    }
                    else
                        mTok.Code = TOKnotequal;
                }
                else if ( GetChar() == L'<' )
                {
                    NextChar();
                    if ( GetChar() == L'>' )
                    {
                        NextChar();
                        if ( GetChar() == L'=' )
                        {
                            NextChar();
                            mTok.Code = TOKunord;
                        }
                        else
                            mTok.Code = TOKue;
                    }
                    else if ( GetChar() == L'=' )
                    {
                        NextChar();
                        mTok.Code = TOKug;
                    }
                    else
                        mTok.Code = TOKuge;
                }
                else if ( GetChar() == L'>' )
                {
                    NextChar();
                    if ( GetChar() == L'=' )
                    {
                        NextChar();
                        mTok.Code = TOKul;
                    }
                    else
                        mTok.Code = TOKule;
                }
                else
                    mTok.Code = TOKnot;
                return;

            case L'(':
                NextChar();
                mTok.Code = TOKlparen;
                return;

            case L')':
                NextChar();
                mTok.Code = TOKrparen;
                return;

            case L'[':
                NextChar();
                mTok.Code = TOKlbracket;
                return;

            case L']':
                NextChar();
                mTok.Code = TOKrbracket;
                return;

            case L'{':
                NextChar();
                mTok.Code = TOKlcurly;
                return;

            case L'}':
                NextChar();
                mTok.Code = TOKrcurly;
                return;

            case L'?':
                NextChar();
                mTok.Code = TOKquestion;
                return;

            case L',':
                NextChar();
                mTok.Code = TOKcomma;
                return;

            case L';':
                NextChar();
                mTok.Code = TOKsemicolon;
                return;

            case L':':
                NextChar();
                mTok.Code = TOKcolon;
                return;

            case L'$':
                NextChar();
                mTok.Code = TOKdollar;
                return;

            case L'=':
                NextChar();
                if ( GetChar() == L'=' )
                {
                    NextChar();
                    if ( GetChar() == L'=' )
                    {
                        NextChar();
                        mTok.Code = TOKidentity;
                    }
                    else
                        mTok.Code = TOKequal;
                }
                else
                    mTok.Code = TOKassign;
                return;

            case L'*':
                NextChar();
                if ( GetChar() == L'=' )
                {
                    NextChar();
                    mTok.Code = TOKmulass;
                }
                else
                    mTok.Code = TOKmul;
                return;

            case L'%':
                NextChar();
                if ( GetChar() == L'=' )
                {
                    NextChar();
                    mTok.Code = TOKmodass;
                }
                else
                    mTok.Code = TOKmod;
                return;

            case L'^':
                NextChar();
                if ( GetChar() == L'=' )
                {
                    NextChar();
                    mTok.Code = TOKxorass;
                }
                else if ( GetChar() == L'^' )
                {
                    NextChar();
                    mTok.Code = TOKpow;
                }
                else
                    mTok.Code = TOKxor;
                return;

            case L'~':
                NextChar();
                if ( GetChar() == L'=' )
                {
                    NextChar();
                    mTok.Code = TOKcatass;
                }
                else
                    mTok.Code = TOKtilde;
                return;

            default:
                if ( !IsUniAlpha( GetChar() ) )
                    throw 2;

                ScanIdentOrKeyword();
                return;
            }
        }
    }

    void Scanner::ScanNumber()
    {
        // it might be binary, octal, or hexadecimal
        if ( GetChar() == L'0' )
        {
            wchar_t peekChar = PeekChar();
            
            if ( (peekChar == L'b') || (peekChar == L'B') )
            {
                NextChar();
                NextChar();
                ScanBinary();
            }
            else if ( (peekChar == L'x') || (peekChar == L'X') )
            {
                NextChar();
                NextChar();
                ScanHex();
            }
            else if ( iswdigit( peekChar ) || (peekChar == L'_') )
            {
                NextChar();
                ScanOctal();
            }
            else
                ScanZero();
        }
        else if ( iswdigit( GetChar() ) )
        {
            ScanDecimal();
        }
        else
            throw 3;
    }

    void Scanner::ScanZero()
    {
        NUMFLAGS        flags = (NUMFLAGS) 0;
        const wchar_t*  start = GetCharPtr();

        NextChar();

        if ( ((GetChar() == L'.') && (PeekChar() != L'.'))
            || (GetChar() == L'i') || (GetChar() == L'f') || (GetChar() == L'F')
            || ((GetChar() == L'L') && (PeekChar() == L'i')) )
        {
            Seek( start );
            ScanReal();
            return;
        }

        flags = ScanIntSuffix();

        mTok.UInt64Value = 0;

        if ( (flags & FLAGS_long) != 0 )    // [u]int64
        {
            if ( (flags & FLAGS_unsigned) != 0 )
                mTok.Code = TOKuns64v;
            else
                mTok.Code = TOKint64v;
        }
        else    // [u]int32
        {
            if ( (flags & FLAGS_unsigned) != 0 )
                mTok.Code = TOKuns32v;
            else
                mTok.Code = TOKint32v;
        }
    }

    void Scanner::ScanDecimal()
    {
        std::wstring    numStr;
        NUMFLAGS        flags = (NUMFLAGS) 0;
        const wchar_t*  start = GetCharPtr();

        for ( ; ; NextChar() )
        {
            if ( iswdigit( GetChar() ) )
            {
                numStr.append( 1, GetChar() );
            }
            else if ( GetChar() == L'_' )
            {
                // ignore it
            }
            else if ( ((GetChar() == L'.') && (PeekChar() != L'.'))
                || (GetChar() == L'i') || (GetChar() == L'f') || (GetChar() == L'F')
                || (GetChar() == L'e') || (GetChar() == L'E')
                || ((GetChar() == L'L') && (PeekChar() == L'i')) )
            {
                Seek( start );
                ScanReal();
                return;
            }
            // TODO: should we fail if there's an alpha (uni alpha) right next to the number?
            else
                break;
        }

        flags = ScanIntSuffix();

        ConvertNumberString( numStr.c_str(), 10, (NUMFLAGS) (FLAGS_decimal | flags), &mTok );
    }

    void Scanner::ScanBinary()
    {
        std::wstring    numStr;
        NUMFLAGS        flags = (NUMFLAGS) 0;

        for ( ; ; NextChar() )
        {
            if ( (GetChar() == L'0') || (GetChar() == L'1') )
            {
                numStr.append( 1, GetChar() );
            }
            else if ( GetChar() == L'_' )
            {
                // ignore it
            }
            else if ( iswdigit( GetChar() ) )
                throw 5;
            // TODO: should we fail if there's an alpha (uni alpha) right next to the number?
            else
                break;
        }

        flags = ScanIntSuffix();

        ConvertNumberString( numStr.c_str(), 2, flags, &mTok );
    }

    void Scanner::ScanHex()
    {
        std::wstring    numStr;
        NUMFLAGS        flags = (NUMFLAGS) 0;
        const wchar_t*  start = GetCharPtr();

        for ( ; ; NextChar() )
        {
            if ( IsHexDigit( GetChar() ) )
            {
                numStr.append( 1, GetChar() );
            }
            else if ( GetChar() == L'_' )
            {
                // ignore it
            }
            else if ( ((GetChar() == L'.') && (PeekChar() != L'.'))
                || (GetChar() == L'i') || (GetChar() == L'p') || (GetChar() == L'P') )
            {
                Seek( start );
                ScanHexReal();
                return;
            }
            // TODO: should we fail if there's an alpha (uni alpha) right next to the number?
            else
                break;
        }

        flags = ScanIntSuffix();

        ConvertNumberString( numStr.c_str(), 16, flags, &mTok );
    }

    void Scanner::ScanOctal()
    {
        std::wstring    numStr;
        NUMFLAGS        flags = (NUMFLAGS) 0;
        const wchar_t*  start = GetCharPtr();
        bool            sawNonOctal = false;

        for ( ; ; NextChar() )
        {
            if ( IsOctalDigit( GetChar() ) )
            {
                numStr.append( 1, GetChar() );
            }
            else if ( GetChar() == L'_' )
            {
                // ignore it
            }
            else if ( ((GetChar() == L'.') && (PeekChar() != L'.'))
                || (GetChar() == L'i') )
            {
                Seek( start );
                ScanReal();
                return;
            }
            else if ( iswdigit( GetChar() ) )
            {
                sawNonOctal = true;
                numStr.append( 1, GetChar() );
            }
            // TODO: should we fail if there's an alpha (uni alpha) right next to the number?
            else
                break;
        }

        if ( sawNonOctal )
            throw 6;        // supposed to be a real

        flags = ScanIntSuffix();

        ConvertNumberString( numStr.c_str(), 8, flags, &mTok );
    }

    Scanner::NUMFLAGS Scanner::ScanIntSuffix()
    {
        NUMFLAGS    flags = (NUMFLAGS) 0;

        for ( ; ; NextChar() )
        {
            NUMFLAGS    f = (NUMFLAGS) 0;

            if ( GetChar() == L'L' )
                f = FLAGS_long;
            else if ( (GetChar() == L'u') || (GetChar() == L'U') )
                f = FLAGS_unsigned;
            // TODO: should we fail if there's an alpha (uni alpha) or number right next to the suffix?
            else
                break;

            if ( (flags & f) != 0 )     // already got this suffix, which is not allowed
                throw 7;

            flags = (NUMFLAGS) (flags | f);
        }

        return flags;
    }

    void Scanner::ConvertNumberString( const wchar_t* numStr, int radix, NUMFLAGS flags, Token* token )
    {
        _set_errno( 0 );
        wchar_t*    p = NULL;
        uint64_t    n = _wcstoui64( numStr, &p, radix );
        errno_t     err = 0;

        if ( p == numStr )
            err = EINVAL;
        else
            _get_errno( &err );

        if ( err != 0 )
            throw 5;

        TOK     code = TOKreserved;

        switch ( (uint32_t) flags )
        {
        case 0:
            /* Octal or Hexadecimal constant.
             * First that fits: int, uint, long, ulong
             */
            if ( n & 0x8000000000000000LL )
                code = TOKuns64v;
            else if ( n & 0xFFFFFFFF00000000LL )
                code = TOKint64v;
            else if ( n & 0x80000000 )
                code = TOKuns32v;
            else
                code = TOKint32v;
            break;

        case FLAGS_decimal:
            /* First that fits: int, long, long long
             */
            if ( n & 0x8000000000000000LL )
            {
                throw 6;
                //error("signed integer overflow");
                //code = TOKuns64v;
            }
            else if ( n & 0xFFFFFFFF80000000LL )
                code = TOKint64v;
            else
                code = TOKint32v;
            break;

        case FLAGS_unsigned:
        case FLAGS_decimal | FLAGS_unsigned:
            /* First that fits: uint, ulong
             */
            if ( n & 0xFFFFFFFF00000000LL )
                code = TOKuns64v;
            else
                code = TOKuns32v;
            break;

        case FLAGS_decimal | FLAGS_long:
            if ( n & 0x8000000000000000LL )
            {
                throw 7;
                //error("signed integer overflow");
                //code = TOKuns64v;
            }
            else
                code = TOKint64v;
            break;

        case FLAGS_long:
            if ( n & 0x8000000000000000LL )
                code = TOKuns64v;
            else
                code = TOKint64v;
            break;

        case FLAGS_unsigned | FLAGS_long:
        case FLAGS_decimal | FLAGS_unsigned | FLAGS_long:
            code = TOKuns64v;
            break;

        default:
            _ASSERT( false );
        }

        token->UInt64Value = n;
        token->Code = code;
    }

    void Scanner::ScanReal()
    {
        std::wstring    numStr;
        bool            sawPoint = false;

        for ( ; ; NextChar() )
        {
            if ( GetChar() == L'_' )
                continue;

            if ( GetChar() == L'.' )
            {
                if ( sawPoint )
                    break;

                sawPoint = true;
            }
            else if ( !iswdigit( GetChar() ) )
                break;

            numStr.append( 1, GetChar() );
        }

        if ( (GetChar() == L'e') || (GetChar() == L'E') )
        {
            numStr.append( 1, GetChar() );
            NextChar();

            if ( (GetChar() == L'+') || (GetChar() == L'-') )
            {
                numStr.append( 1, GetChar() );
                NextChar();
            }

            if ( !iswdigit( GetChar() ) )
                throw 6;

            for ( ; ; NextChar() )
            {
                if ( GetChar() == L'_' )
                    continue;
                if ( !iswdigit( GetChar() ) )
                    break;

                numStr.append( 1, GetChar() );
            }
        }

        TOK         tok = ScanGeneralFloatSuffix();

        ConvertRealString( numStr.c_str(), tok, &mTok );
    }

    void Scanner::ScanHexReal()
    {
        std::wstring    numStr;
        bool            sawPoint = false;

        // our conversion function expects "0x" for hex floats
        numStr.append( L"0x" );

        for ( ; ; NextChar() )
        {
            if ( GetChar() == L'_' )
                continue;

            if ( GetChar() == L'.' )
            {
                if ( sawPoint )
                    break;

                sawPoint = true;
            }
            else if ( !IsHexDigit( GetChar() ) )
                break;

            numStr.append( 1, GetChar() );
        }

        // binary exponent required
        if ( (GetChar() != L'p') && (GetChar() != L'P') )
            throw 5;

        numStr.append( 1, GetChar() );
        NextChar();

        if ( (GetChar() == L'+') || (GetChar() == L'-') )
        {
            numStr.append( 1, GetChar() );
            NextChar();
        }

        if ( !iswdigit( GetChar() ) )
            throw 6;

        for ( ; ; NextChar() )
        {
            if ( GetChar() == L'_' )
                continue;
            if ( !iswdigit( GetChar() ) )
                break;

            numStr.append( 1, GetChar() );
        }

        TOK         tok = ScanGeneralFloatSuffix();

        ConvertRealString( numStr.c_str(), tok, &mTok );
    }

    void Scanner::ConvertRealString( const wchar_t* numStr, TOK tok, Token* token )
    {
        Real10      val;
        errno_t     err = 0;
        bool        fits = true;

        err = Real10::Parse( numStr, val );
        if ( err != 0 )
            throw 22;

        if ( (tok == TOKfloat64v) || (tok == TOKimaginary64v) )
            fits = val.FitsInDouble();
        else if ( (tok == TOKfloat32v) || (tok == TOKimaginary32v) )
            fits = val.FitsInFloat();

        if ( !fits )
            throw 23;

        token->Code = tok;
        token->Float80Value = val;
    }

    TOK Scanner::ScanGeneralFloatSuffix()
    {
        TOK tok = TOKfloat64v;

        switch ( GetChar() )
        {
        case L'f':
        case L'F':
            NextChar();
            tok = TOKfloat32v;
            break;

        case L'L':
            NextChar();
            tok = TOKfloat80v;
            break;
        }

        if ( GetChar() == L'i' )
        {
            NextChar();

            switch ( tok )
            {
            case TOKfloat32v:
                tok = TOKimaginary32v;
                break;
            case TOKfloat64v:
                tok = TOKimaginary64v;
                break;
            case TOKfloat80v:
                tok = TOKimaginary80v;
                break;
            }
        }

        return tok;
    }

    void Scanner::ScanIdentOrKeyword()
    {
        const wchar_t*  startPtr = GetCharPtr();
        size_t  len = 0;

        while ( IsIdentChar( GetChar() ) )
        {
            NextChar();
        }

        len = GetCharPtr() - startPtr;

        TOK code = MapToKeyword( startPtr, len );

        if ( code == TOKidentifier )
        {
            mTok.Utf16Str = mNameTable->AddString( startPtr, len );
        }

        mTok.Code = code;
    }

    void Scanner::ScanCharLiteral()
    {
        TOK         tok = TOKcharv;
        uint32_t    c = 0;

        NextChar();
        c = GetChar();

        switch ( c )
        {
        case L'\\':
            if ( PeekChar() == L'u' )
                tok = TOKwcharv;
            else if ( (PeekChar() == L'U') || (PeekChar() == L'&') )
                tok = TOKdcharv;
            c = ScanEscapeChar();
            break;

        case L'\n':
        case 0x2028:
        case 0x2029:
        case L'\r':
        case L'\0':
        case 0x001A:
        case L'\'':
            throw 8;

        default:
            c = ReadChar32();
            if ( c > 0xFFFF )
                tok = TOKdcharv;
            else if ( c >= 0x80 )
                tok = TOKwcharv;
            break;
        }

        if ( GetChar() != L'\'' )
            throw 10;
        NextChar();

        mTok.Code = tok;
        mTok.UInt64Value = c;
    }

    void Scanner::ScanStringLiteral()
    {
        vector<char>    buf;
        dchar_t         c = 0;

        NextChar();

        for ( ; ; )
        {
            if ( GetChar() == L'"' )
                break;
            if ( (GetChar() == L'\0') || (GetChar() == 0x001A) )
                throw 21;
            if ( GetChar() == L'\r' )
            {
                if ( PeekChar() == L'\n' )
                    continue;
                NextChar();
                c = L'\n';
            }
            else if ( (GetChar() == 0x2028) || (GetChar() == 0x2029) )
            {
                NextChar();
                c = L'\n';
            }
            else if ( GetChar() == L'\\' )
                c = ScanEscapeChar();
            else
                c = ReadChar32();

            AppendChar32( buf, c );
        }

        NextChar();
        wchar_t postfix = ScanStringPostfix();

        buf.push_back( 0 );
        AllocateStringToken( buf, postfix );
    }

    void Scanner::ScanDelimitedStringLiteral()
    {
        std::wstring    str;

        NextChar();     // skip '"'

        if ( iswalpha( GetChar() ) 
            || (GetChar() == L'_') 
            || IsUniAlpha( GetChar() ) )
            ScanDelimitedStringLiteralID( str );
        else if ( iswspace( GetChar() ) )
            throw 16;
        else
            ScanDelimitedStringLiteralSeparator( str );

        NextChar();     // skip '"'
        wchar_t postfix = ScanStringPostfix();

        AllocateStringToken( str, postfix );
    }

    void Scanner::ScanDelimitedStringLiteralSeparator( std::wstring& str )
    {
        dchar_t         left = L'\0';
        dchar_t         right = L'\0';
        int             nestCount = 1;
        bool            nesting = true;

        left = ReadChar32();

        if ( left == L'[' )
            right = L']';
        else if ( left == L'(' )
            right = L')';
        else if ( left == L'<' )
            right = L'>';
        else if ( left == L'{' )
            right = L'}';
        else
        {
            right = left;
            nesting = false;
            nestCount = 0;
        }

        for ( ; ; )
        {
            if ( (nestCount == 0) && (GetChar() == L'"') )
                break;
            if ( (GetChar() == L'\0') || (GetChar() == 0x001A) )
                throw 21;

            dchar_t c = ReadChar32();

            if ( c == L'\r' )
            {
                if ( GetChar() == L'\n' )
                    continue;
                c = L'\n';
            }
            else if ( (c == 0x2028) || (c == 0x2029) )
                c = L'\n';

            if ( c == right )
            {
                if ( nesting )
                    nestCount--;
                if ( (nestCount == 0) && (GetChar() != L'"') )
                    throw 12;
                if ( nestCount == 0 )
                    continue;
            }
            else if ( c == left )
            {
                if ( nesting )
                    nestCount++;
            }

            AppendChar32( str, c );
        }
    }

    void Scanner::ScanDelimitedStringLiteralID( std::wstring& str )
    {
        Utf16String*    id = NULL;
        const wchar_t*  start = NULL;
        bool            atLineBegin = true;

        Scan();
        
        if ( mTok.Code != TOKidentifier )
            throw 17;
        else
            id = mTok.Utf16Str;

        switch ( GetChar() )
        {
        case L'\r':
            if ( PeekChar() == L'\n' )
                NextChar();             // skip '\r'
            // fall thru
        case L'\n':
        case 0x2028:
        case 0x2029:
            NextChar();
            break;
        default:
            throw 18;
        }

        for ( ; ; )
        {
            switch ( GetChar() )
            {
            case L'\r':
                if ( PeekChar() == L'\n' )
                    NextChar();             // skip '\r'
                // fall thru
            case L'\n':
            case 0x2028:
            case 0x2029:
                atLineBegin = true;
                str.append( 1, L'\n' );
                NextChar();
                continue;
            case L'\0':
            case 0x001A:
                throw 21;
            }

            if ( atLineBegin )
            {
                start = GetCharPtr();
                Scan();
                if ( (mTok.Code == TOKidentifier) && (wcscmp( mTok.Utf16Str->Str, id->Str ) == 0) )
                {
                    if ( GetChar() != L'"' )
                        throw 14;
                    break;
                }
                Seek( start );
            }

            dchar_t c = ReadChar32();

            AppendChar32( str, c );
            atLineBegin = false;
        }
    }

    void Scanner::ScanTokenStringLiteral()
    {
        const wchar_t*  start = NULL;
        const wchar_t*  end = NULL;
        int             nestCount = 1;

        NextChar();
        start = GetCharPtr();
        Scan();

        for ( ; ; Scan() )
        {
            if ( mTok.Code == TOKlcurly )
            {
                nestCount++;
            }
            else if ( mTok.Code == TOKrcurly )
            {
                end = GetCharPtr() - 1;
                nestCount--;
                if ( nestCount == 0 )
                    break;
            }
            else if ( mTok.Code == TOKeof )
                throw 14;
        }

        wchar_t postfix = ScanStringPostfix();

        size_t  len = end - start;
        wstring str;
        str.append( start, len );
        AllocateStringToken( str, postfix );
    }

    void Scanner::ScanWysiwygString()
    {
        std::wstring    str;
        wchar_t         quoteChar = GetChar();

        NextChar();

        for ( ; ; )
        {
            switch ( GetChar() )
            {
            case L'\r':
                if ( PeekChar() == L'\n' )
                    NextChar();
                str.append( 1, L'\n' );
                NextChar();
                continue;

            case L'\0':
            case 0x001A:
                throw 7;

            case L'"':
            case L'`':
                if ( GetChar() == quoteChar )
                {
                    NextChar();
                    wchar_t post = ScanStringPostfix();

                    AllocateStringToken( str, post );
                    return;
                }
                break;
            }

            AppendChar32( str, ReadChar32() );
        }
    }

    void Scanner::ScanHexString()
    {
        std::vector<char>       buf;
        int                     digitCount = 0;
        uint8_t                 b = 0;

        NextChar();

        for ( ; GetChar() != L'"'; NextChar() )
        {
            if ( (GetChar() == L'\0') || (GetChar() == 0x001A) )
                throw 21;
            if ( iswspace( GetChar() ) )
                continue;

            wchar_t c = GetChar();
            int     digit = 0;
            
            if ( (c >= L'0') && (c <= L'9') )
                digit = c - L'0';
            else if ( (c >= L'A') && (c <= L'F') )
                digit = c - L'A';
            else if ( (c >= L'a') && (c <= L'f') )
                digit = c - L'a';
            else
                throw 11;

            digitCount++;

            if ( (digitCount & 1) == 0 )     // finished a byte
            {
                b |= digit;
                buf.push_back( b );
            }
            else
            {
                b = (uint8_t) (digit << 4);
            }
        }

        // has to be even
        if ( (digitCount & 1) == 1 )
            throw 12;

        NextChar();
        wchar_t postfix = ScanStringPostfix();

        buf.push_back( 0 );

        AllocateStringToken( buf, postfix );
    }

    wchar_t Scanner::ScanStringPostfix()
    {
        wchar_t c = 0;

        switch ( GetChar() )
        {
        case L'c':
        case L'w':
        case L'd':
            c = GetChar();
            NextChar();
            return c;

        default:
            return 0;
        }
    }

    char* Scanner::ConvertUtf16To8( const wchar_t* utf16Str )
    {
        int         len = 0;
        int         len2 = 0;
        char*       utf8Str = NULL;

        len = Utf16To8( utf16Str, -1, NULL, 0 );
        if ( (len == 0) && (GetLastError() == ERROR_NO_UNICODE_TRANSLATION) )
            throw 13;
        _ASSERT( len > 0 );

        // len includes trailing '\0'
        utf8Str = new char[ len ];
        len2 = Utf16To8( utf16Str, -1, utf8Str, len );
        _ASSERT( (len2 > 0) && (len2 == len) );

        return utf8Str;
    }

    wchar_t* Scanner::ConvertUtf8To16( const char* utf8Str )
    {
        int         len = 0;
        int         len2 = 0;
        wchar_t*    utf16Str = NULL;

        len = Utf8To16( utf8Str, -1, NULL, 0 );
        if ( (len == 0) && (GetLastError() == ERROR_NO_UNICODE_TRANSLATION) )
            throw 13;
        _ASSERT( len > 0 );

        // len includes trailing '\0'
        utf16Str = new wchar_t[ len ];
        len2 = Utf8To16( utf8Str, -1, utf16Str, len );
        _ASSERT( (len2 > 0) && (len2 == len) );

        return utf16Str;
    }

    dchar_t* Scanner::ConvertUtf8To32( const char* utf8Str, size_t& length )
    {
        int         len = 0;
        int         len2 = 0;
        dchar_t*    utf32Str = NULL;

        len = Utf8To32( utf8Str, -1, NULL, 0 );
        if ( (len == 0) && (GetLastError() == ERROR_NO_UNICODE_TRANSLATION) )
            throw 13;
        _ASSERT( len > 0 );

        // len includes trailing '\0'
        utf32Str = new dchar_t[ len ];
        len2 = Utf8To32( utf8Str, -1, utf32Str, len );
        _ASSERT( (len2 > 0) && (len2 == len) );

        length = len;
        return utf32Str;
    }

    dchar_t* Scanner::ConvertUtf16To32( const wchar_t* utf16Str, size_t& length )
    {
        int         len = 0;
        int         len2 = 0;
        dchar_t*    utf32Str = NULL;

        len = Utf16To32( utf16Str, -1, NULL, 0 );
        if ( (len == 0) && (GetLastError() == ERROR_NO_UNICODE_TRANSLATION) )
            throw 13;
        _ASSERT( len > 0 );

        // len includes trailing '\0'
        utf32Str = new dchar_t[ len ];
        len2 = Utf16To32( utf16Str, -1, utf32Str, len );
        _ASSERT( (len2 > 0) && (len2 == len) );

        length = len;
        return utf32Str;
    }

    void Scanner::AllocateStringToken( const vector<char>& buf, wchar_t postfix )
    {
        if ( (postfix == 0) || (postfix == L'c') )
        {
            // when the string is a char[], it doesn't have to be UTF-8
            mTok.ByteStr = mNameTable->AddString( &buf.front(), buf.size() - 1 );
            if ( postfix == L'c' )
                mTok.Code = TOKcstring;
            else
                mTok.Code = TOKstring;
        }
        else if ( postfix == L'w' )
        {
            wchar_t*    str = ConvertUtf8To16( &buf.front() );
            mTok.Utf16Str = mNameTable->AddString( str, wcslen( str ) );
            mTok.Code = TOKwstring;
            delete [] str;
        }
        else if ( postfix == L'd' )
        {
            size_t      len = 0;
            dchar_t*    str = ConvertUtf8To32( &buf.front(), len );
            mTok.Utf32Str = mNameTable->AddString( str, len-1 );
            mTok.Code = TOKdstring;
            delete [] str;
        }
        else
            _ASSERT( false );
    }

    void Scanner::AllocateStringToken( const std::wstring& str, wchar_t postfix )
    {
        if ( (postfix == 0) || (postfix == L'c') )
        {
            char*       astr = ConvertUtf16To8( str.c_str() );
            mTok.ByteStr = mNameTable->AddString( astr, strlen( astr ) );
            if ( postfix == L'c' )
                mTok.Code = TOKcstring;
            else
                mTok.Code = TOKstring;
            delete [] astr;
        }
        else if ( postfix == L'w' )
        {
            mTok.Utf16Str = mNameTable->AddString( str.c_str(), str.size() );
            mTok.Code = TOKwstring;
        }
        else if ( postfix == L'd' )
        {
            size_t      len = 0;
            dchar_t*    dstr = ConvertUtf16To32( str.c_str(), len );
            mTok.Utf32Str = mNameTable->AddString( dstr, len-1 );
            mTok.Code = TOKdstring;
            delete [] dstr;
        }
        else
            _ASSERT( false );
    }

    dchar_t Scanner::ScanEscapeChar()
    {
        dchar_t c = 0;
        const wchar_t*  start = NULL;

        NextChar();     // skip backslash
        c = GetChar();
        start = GetCharPtr();
        NextChar();

        switch ( c )
        {
        case L'\'': c = L'\'';  break;
        case L'"':  c = L'"';   break;
        case L'?':  c = L'\?';   break;
        case L'\\': c = L'\\';  break;
        case L'a':  c = L'\a';   break;
        case L'b':  c = L'\b';   break;
        case L'f':  c = L'\f';   break;
        case L'n':  c = L'\n';   break;
        case L'r':  c = L'\r';   break;
        case L't':  c = L'\t';   break;
        case L'v':  c = L'\v';   break;

        // TODO: EndOfFile?
        //case 

        case L'x':
            c = ScanFixedSizeHex( 2 );
            break;

        case L'u':
            c = ScanFixedSizeHex( 4 );
            break;

        case L'U':
            c = ScanFixedSizeHex( 8 );
            break;

        case L'&':
            for ( ; ; NextChar() )
            {
                if ( GetChar() == L';' )
                {
                    size_t  len = GetCharPtr() - start - 1;         // not counting '&'
                    dchar_t c = MapNamedCharacter( start + 1, len );

                    if ( c == 0 )
                        throw 20;

                    NextChar();
                    return c;
                }

                if ( !iswalpha( GetChar() )
                    && ((GetCharPtr() == (start + 1)) || !iswdigit( GetChar() )) )
                    throw 21;
            }
            break;

        default:
            Seek( start );
            if ( !IsOctalDigit( GetChar() ) )
                throw 17;
            c = 0;
            for ( int i = 0; (i < 3) && IsOctalDigit( GetChar() ); i++ )
            {
                int digit = GetChar() - L'0';
                c *= 8;
                c += digit;
                NextChar();
            }
            // D compiler doesn't care if there's an octal char after 3 in escape char
            break;
        }

        return c;
    }

    uint32_t Scanner::ScanFixedSizeHex( int length )
    {
        uint32_t    n = 0;
        uint32_t    digit = 0;

        for ( int i = 0; i < length; i++ )
        {
            if ( !IsHexDigit( GetChar() ) )
                throw 18;

            if ( iswdigit( GetChar() ) )
                digit = GetChar() - L'0';
            else if ( iswlower( GetChar() ) )
                digit = GetChar() - L'a' + 10;
            else
                digit = GetChar() - L'A' + 10;

            n *= 16;
            n += digit;
            NextChar();
        }

        if ( IsHexDigit( GetChar() ) )
            throw 19;

        return n;
    }

    wchar_t Scanner::GetChar()
    {
        if ( mCurPtr >= mEndPtr )
            return L'\0';

        wchar_t c = *mCurPtr;
        
        return c;
    }

    dchar_t Scanner::ReadChar32()
    {
        if ( mCurPtr >= mEndPtr )
            return L'\0';

        dchar_t c = *mCurPtr;
        mCurPtr++;

        if ( (c >= 0xDC00) && (c <= 0xDFFF) )   // high surrogate can't be first wchar
            throw 1;
        if ( (c >= 0xD800) && (c <= 0xDBFF) )
        {
            wchar_t c2 = *mCurPtr;
            mCurPtr++;

            if ( (c2 < 0xDC00) || (c2 > 0xDFFF) )
                throw 1;

            c &= ~0xD800;
            c2 &= ~0xDC00;
            c = (c2 | (c << 10)) + 0x10000;
        }
        // else c is the whole character

        if ( c > 0x10FFFF )
            throw 1;

        return c;
    }

    wchar_t Scanner::PeekChar()
    {
        if ( mCurPtr == L'\0' )
            return L'\0';

        return mCurPtr[1];
    }

    wchar_t Scanner::PeekChar( int index )
    {
        if ( (mCurPtr + index) >= mEndPtr )
            return L'\0';

        return mCurPtr[index];
    }

    const wchar_t* Scanner::GetCharPtr()
    {
        return mCurPtr;
    }

    void Scanner::Seek( const wchar_t* ptr )
    {
        _ASSERT( (ptr >= mInBuf) && (ptr < mEndPtr) );

        mCurPtr = ptr;
    }

    void Scanner::NextChar()
    {
        mCurPtr++;
    }

    Scanner::TokenNode*  Scanner::NewNode()
    {
        TokenNode*  node = NULL;

        if ( mFreeNodes == NULL )
        {
            node = new TokenNode();
            //node->Prev = NULL;
        }
        else
        {
            node = mFreeNodes;
            mFreeNodes = mFreeNodes->Next;
        }

        node->Next = NULL;
        return node;
    }
}
