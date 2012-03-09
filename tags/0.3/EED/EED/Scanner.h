/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "Token.h"


namespace MagoEE
{
    class NameTable;


    class Scanner
    {
        struct TokenNode : public Token
        {
            TokenNode*  Next;

            TokenNode* operator=( const Token& tok )
            {
                memcpy( this, &tok, sizeof( Token ) );
                return this;
            }
        };

        static const int    TokenQueueMaxSize = 2;

        enum NUMFLAGS
        {
            FLAGS_decimal  = 1,     // decimal
            FLAGS_unsigned = 2,     // u or U suffix
            FLAGS_long     = 4,     // l or L suffix
        };

        uint32_t        mDVer;
        const wchar_t*  mInBuf;
        size_t          mBufLen;
        const wchar_t*  mCurPtr;
        const wchar_t*  mEndPtr;
        Token           mTok;
        TokenNode*      mCurNodes;
        TokenNode*      mFreeNodes;
        NameTable*      mNameTable;

    public:
        Scanner( const wchar_t* inBuffer, size_t inBufferLen, NameTable* nameTable );
        ~Scanner();

        TOK NextToken();
        const Token& PeekToken( int index );
        const Token* PeekToken( const Token* token );
        const Token& GetToken();

        NameTable* GetNameTable();

    private:
        wchar_t GetChar();
        dchar_t ReadChar32();
        const wchar_t* GetCharPtr();
        void Seek( const wchar_t* ptr );
        wchar_t PeekChar();
        wchar_t PeekChar( int index );
        void NextChar();
        TokenNode*  NewNode();

        void Scan();
        void ScanNumber();
        void ScanZero();
        void ScanDecimal();
        void ScanBinary();
        void ScanHex();
        void ScanOctal();
        NUMFLAGS ScanIntSuffix();
        void ScanReal();
        void ScanHexReal();
        TOK ScanGeneralFloatSuffix();
        void ScanIdentOrKeyword();
        void ScanCharLiteral();
        void ScanStringLiteral();
        void ScanDelimitedStringLiteral();
        void ScanDelimitedStringLiteralID( std::wstring& str );
        void ScanDelimitedStringLiteralSeparator( std::wstring& str );
        void ScanTokenStringLiteral();
        void ScanWysiwygString();
        void ScanHexString();
        wchar_t ScanStringPostfix();
        dchar_t ScanEscapeChar();
        uint32_t ScanFixedSizeHex( int length );

        void ConvertNumberString( const wchar_t* numStr, int radix, NUMFLAGS flags, Token* token );
        void ConvertRealString( const wchar_t* numStr, TOK tok, Token* token );
        void AllocateStringToken( const std::wstring& str, wchar_t postfix );
        void AllocateStringToken( const std::vector<char>& buf, wchar_t postfix );

        // TODO: put these somewhere else
        static char* ConvertUtf16To8( const wchar_t* utf16Str );
        static wchar_t* ConvertUtf8To16( const char* utf8Str );
        static dchar_t* ConvertUtf8To32( const char* utf8Str, size_t& length );
        static dchar_t* ConvertUtf16To32( const wchar_t* utf16Str, size_t& length );
    };
}
