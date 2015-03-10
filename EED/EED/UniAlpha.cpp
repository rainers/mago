/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "UniAlpha.h"


namespace MagoEE
{
    const wchar_t   ReplacementChar16 = L'\xFFFD';


    bool IsHexDigit( wchar_t c )
    {
        return iswxdigit( c ) ? true : false;
    }

    bool IsOctalDigit( wchar_t c )
    {
        return (c >= L'0') && (c <= L'7');
    }

    bool IsIdentChar( wchar_t c )
    {
        return iswalnum( c ) || (c == L'_');
    }


    // Use table from C99 Appendix D.

    bool IsUniAlpha( wchar_t ch )
    {
        static unsigned short table[][2] =
        {
            { 0x00AA, 0x00AA },
            { 0x00B5, 0x00B5 },
            { 0x00B7, 0x00B7 },
            { 0x00BA, 0x00BA },
            { 0x00C0, 0x00D6 },
            { 0x00D8, 0x00F6 },
            { 0x00F8, 0x01F5 },
            { 0x01FA, 0x0217 },
            { 0x0250, 0x02A8 },
            { 0x02B0, 0x02B8 },
            { 0x02BB, 0x02BB },
            { 0x02BD, 0x02C1 },
            { 0x02D0, 0x02D1 },
            { 0x02E0, 0x02E4 },
            { 0x037A, 0x037A },
            { 0x0386, 0x0386 },
            { 0x0388, 0x038A },
            { 0x038C, 0x038C },
            { 0x038E, 0x03A1 },
            { 0x03A3, 0x03CE },
            { 0x03D0, 0x03D6 },
            { 0x03DA, 0x03DA },
            { 0x03DC, 0x03DC },
            { 0x03DE, 0x03DE },
            { 0x03E0, 0x03E0 },
            { 0x03E2, 0x03F3 },
            { 0x0401, 0x040C },
            { 0x040E, 0x044F },
            { 0x0451, 0x045C },
            { 0x045E, 0x0481 },
            { 0x0490, 0x04C4 },
            { 0x04C7, 0x04C8 },
            { 0x04CB, 0x04CC },
            { 0x04D0, 0x04EB },
            { 0x04EE, 0x04F5 },
            { 0x04F8, 0x04F9 },
            { 0x0531, 0x0556 },
            { 0x0559, 0x0559 },
            { 0x0561, 0x0587 },
            { 0x05B0, 0x05B9 },
            { 0x05BB, 0x05BD },
            { 0x05BF, 0x05BF },
            { 0x05C1, 0x05C2 },
            { 0x05D0, 0x05EA },
            { 0x05F0, 0x05F2 },
            { 0x0621, 0x063A },
            { 0x0640, 0x0652 },
            { 0x0660, 0x0669 },
            { 0x0670, 0x06B7 },
            { 0x06BA, 0x06BE },
            { 0x06C0, 0x06CE },
            { 0x06D0, 0x06DC },
            { 0x06E5, 0x06E8 },
            { 0x06EA, 0x06ED },
            { 0x06F0, 0x06F9 },
            { 0x0901, 0x0903 },
            { 0x0905, 0x0939 },
            { 0x093D, 0x093D },
            { 0x093E, 0x094D },
            { 0x0950, 0x0952 },
            { 0x0958, 0x0963 },
            { 0x0966, 0x096F },
            { 0x0981, 0x0983 },
            { 0x0985, 0x098C },
            { 0x098F, 0x0990 },
            { 0x0993, 0x09A8 },
            { 0x09AA, 0x09B0 },
            { 0x09B2, 0x09B2 },
            { 0x09B6, 0x09B9 },
            { 0x09BE, 0x09C4 },
            { 0x09C7, 0x09C8 },
            { 0x09CB, 0x09CD },
            { 0x09DC, 0x09DD },
            { 0x09DF, 0x09E3 },
            { 0x09E6, 0x09EF },
            { 0x09F0, 0x09F1 },
            { 0x0A02, 0x0A02 },
            { 0x0A05, 0x0A0A },
            { 0x0A0F, 0x0A10 },
            { 0x0A13, 0x0A28 },
            { 0x0A2A, 0x0A30 },
            { 0x0A32, 0x0A33 },
            { 0x0A35, 0x0A36 },
            { 0x0A38, 0x0A39 },
            { 0x0A3E, 0x0A42 },
            { 0x0A47, 0x0A48 },
            { 0x0A4B, 0x0A4D },
            { 0x0A59, 0x0A5C },
            { 0x0A5E, 0x0A5E },
            { 0x0A66, 0x0A6F },
            { 0x0A74, 0x0A74 },
            { 0x0A81, 0x0A83 },
            { 0x0A85, 0x0A8B },
            { 0x0A8D, 0x0A8D },
            { 0x0A8F, 0x0A91 },
            { 0x0A93, 0x0AA8 },
            { 0x0AAA, 0x0AB0 },
            { 0x0AB2, 0x0AB3 },
            { 0x0AB5, 0x0AB9 },
            { 0x0ABD, 0x0AC5 },
            { 0x0AC7, 0x0AC9 },
            { 0x0ACB, 0x0ACD },
            { 0x0AD0, 0x0AD0 },
            { 0x0AE0, 0x0AE0 },
            { 0x0AE6, 0x0AEF },
            { 0x0B01, 0x0B03 },
            { 0x0B05, 0x0B0C },
            { 0x0B0F, 0x0B10 },
            { 0x0B13, 0x0B28 },
            { 0x0B2A, 0x0B30 },
            { 0x0B32, 0x0B33 },
            { 0x0B36, 0x0B39 },
            { 0x0B3D, 0x0B3D },
            { 0x0B3E, 0x0B43 },
            { 0x0B47, 0x0B48 },
            { 0x0B4B, 0x0B4D },
            { 0x0B5C, 0x0B5D },
            { 0x0B5F, 0x0B61 },
            { 0x0B66, 0x0B6F },
            { 0x0B82, 0x0B83 },
            { 0x0B85, 0x0B8A },
            { 0x0B8E, 0x0B90 },
            { 0x0B92, 0x0B95 },
            { 0x0B99, 0x0B9A },
            { 0x0B9C, 0x0B9C },
            { 0x0B9E, 0x0B9F },
            { 0x0BA3, 0x0BA4 },
            { 0x0BA8, 0x0BAA },
            { 0x0BAE, 0x0BB5 },
            { 0x0BB7, 0x0BB9 },
            { 0x0BBE, 0x0BC2 },
            { 0x0BC6, 0x0BC8 },
            { 0x0BCA, 0x0BCD },
            { 0x0BE7, 0x0BEF },
            { 0x0C01, 0x0C03 },
            { 0x0C05, 0x0C0C },
            { 0x0C0E, 0x0C10 },
            { 0x0C12, 0x0C28 },
            { 0x0C2A, 0x0C33 },
            { 0x0C35, 0x0C39 },
            { 0x0C3E, 0x0C44 },
            { 0x0C46, 0x0C48 },
            { 0x0C4A, 0x0C4D },
            { 0x0C60, 0x0C61 },
            { 0x0C66, 0x0C6F },
            { 0x0C82, 0x0C83 },
            { 0x0C85, 0x0C8C },
            { 0x0C8E, 0x0C90 },
            { 0x0C92, 0x0CA8 },
            { 0x0CAA, 0x0CB3 },
            { 0x0CB5, 0x0CB9 },
            { 0x0CBE, 0x0CC4 },
            { 0x0CC6, 0x0CC8 },
            { 0x0CCA, 0x0CCD },
            { 0x0CDE, 0x0CDE },
            { 0x0CE0, 0x0CE1 },
            { 0x0CE6, 0x0CEF },
            { 0x0D02, 0x0D03 },
            { 0x0D05, 0x0D0C },
            { 0x0D0E, 0x0D10 },
            { 0x0D12, 0x0D28 },
            { 0x0D2A, 0x0D39 },
            { 0x0D3E, 0x0D43 },
            { 0x0D46, 0x0D48 },
            { 0x0D4A, 0x0D4D },
            { 0x0D60, 0x0D61 },
            { 0x0D66, 0x0D6F },
            { 0x0E01, 0x0E3A },
            { 0x0E40, 0x0E5B },
            //  { 0x0E50, 0x0E59 },
            { 0x0E81, 0x0E82 },
            { 0x0E84, 0x0E84 },
            { 0x0E87, 0x0E88 },
            { 0x0E8A, 0x0E8A },
            { 0x0E8D, 0x0E8D },
            { 0x0E94, 0x0E97 },
            { 0x0E99, 0x0E9F },
            { 0x0EA1, 0x0EA3 },
            { 0x0EA5, 0x0EA5 },
            { 0x0EA7, 0x0EA7 },
            { 0x0EAA, 0x0EAB },
            { 0x0EAD, 0x0EAE },
            { 0x0EB0, 0x0EB9 },
            { 0x0EBB, 0x0EBD },
            { 0x0EC0, 0x0EC4 },
            { 0x0EC6, 0x0EC6 },
            { 0x0EC8, 0x0ECD },
            { 0x0ED0, 0x0ED9 },
            { 0x0EDC, 0x0EDD },
            { 0x0F00, 0x0F00 },
            { 0x0F18, 0x0F19 },
            { 0x0F20, 0x0F33 },
            { 0x0F35, 0x0F35 },
            { 0x0F37, 0x0F37 },
            { 0x0F39, 0x0F39 },
            { 0x0F3E, 0x0F47 },
            { 0x0F49, 0x0F69 },
            { 0x0F71, 0x0F84 },
            { 0x0F86, 0x0F8B },
            { 0x0F90, 0x0F95 },
            { 0x0F97, 0x0F97 },
            { 0x0F99, 0x0FAD },
            { 0x0FB1, 0x0FB7 },
            { 0x0FB9, 0x0FB9 },
            { 0x10A0, 0x10C5 },
            { 0x10D0, 0x10F6 },
            { 0x1E00, 0x1E9B },
            { 0x1EA0, 0x1EF9 },
            { 0x1F00, 0x1F15 },
            { 0x1F18, 0x1F1D },
            { 0x1F20, 0x1F45 },
            { 0x1F48, 0x1F4D },
            { 0x1F50, 0x1F57 },
            { 0x1F59, 0x1F59 },
            { 0x1F5B, 0x1F5B },
            { 0x1F5D, 0x1F5D },
            { 0x1F5F, 0x1F7D },
            { 0x1F80, 0x1FB4 },
            { 0x1FB6, 0x1FBC },
            { 0x1FBE, 0x1FBE },
            { 0x1FC2, 0x1FC4 },
            { 0x1FC6, 0x1FCC },
            { 0x1FD0, 0x1FD3 },
            { 0x1FD6, 0x1FDB },
            { 0x1FE0, 0x1FEC },
            { 0x1FF2, 0x1FF4 },
            { 0x1FF6, 0x1FFC },
            { 0x203F, 0x2040 },
            { 0x207F, 0x207F },
            { 0x2102, 0x2102 },
            { 0x2107, 0x2107 },
            { 0x210A, 0x2113 },
            { 0x2115, 0x2115 },
            { 0x2118, 0x211D },
            { 0x2124, 0x2124 },
            { 0x2126, 0x2126 },
            { 0x2128, 0x2128 },
            { 0x212A, 0x2131 },
            { 0x2133, 0x2138 },
            { 0x2160, 0x2182 },
            { 0x3005, 0x3007 },
            { 0x3021, 0x3029 },
            { 0x3041, 0x3093 },
            { 0x309B, 0x309C },
            { 0x30A1, 0x30F6 },
            { 0x30FB, 0x30FC },
            { 0x3105, 0x312C },
            { 0x4E00, 0x9FA5 },
            { 0xAC00, 0xD7A3 },
        };

        if ( ch > 0xD7A3 )
            return false;

        // Binary search
        int mid;
        int low;
        int high;

        low = 0;
        high = _countof( table ) - 1;
        while ( low <= high )
        {
            mid = (low + high) / 2;
            if ( ch < table[mid][0] )
                high = mid - 1;
            else if ( ch > table[mid][1] )
                low = mid + 1;
            else
                return true;
        }

        return false;
    }

    bool IsIdentifier( const wchar_t* str )
    {
        if ( !str || !str[0] )
            return false;

        if( !IsIdentChar( str[0] ) && !IsUniAlpha( str[0] ) )
            return false;

        for( str++; *str; str++ )
            if( !IsIdentChar( *str ) && !IsUniAlpha( *str ) && !IsHexDigit( *str ) )
                return false;

        return true;
    }

    int Utf8To32( const char* utf8Str, int utf8Len, dchar_t* utf32Str, int utf32Len )
    {
        _ASSERT( (utf32Len == 0) || (utf32Str != NULL) );

        int         pos = 0;
        int         len = 0;
        const char* s = utf8Str;

        if ( utf8Len < 0 )
            utf8Len = (int) strlen( utf8Str ) + 1;      // include terminator in length to convert

        while ( pos < utf8Len )
        {
            uint8_t c = s[pos];
            dchar_t d = 0;

            if ( (c & 0x80) == 0 )
            {
                d = c;
                pos++;
            }
            else
            {
                int     n = 0;

                if ( (c == 0xC0) || (c == 0xC1) || (c == 0xFE) || (c == 0xFF)
                    || ((c >= 0xF5) && (c <= 0xF7)) )
                    // TODO: actually F8...FD are also restricted
                {
                    SetLastError( ERROR_NO_UNICODE_TRANSLATION );
                    return 0;
                }

                for ( n = 1; n < 6; n++ )
                {
                    c <<= 1;
                    if ( (c & 0xC0) == 0x80 )
                        break;
                }

                if ( n >= 4 )
                {
                    SetLastError( ERROR_NO_UNICODE_TRANSLATION );
                    return 0;
                }

                c >>= n;            // put it back
                c &= (0x3F >> n);   // bring out usable bits
                d = c;

                if ( (pos + n) >= utf8Len )
                {
                    SetLastError( ERROR_NO_UNICODE_TRANSLATION );
                    return 0;
                }

                for ( int j = 1; j <= n; j++ )
                {
                    c = s[pos + j];
                    if ( (c & 0xC0) != 0x80 )
                    {
                        SetLastError( ERROR_NO_UNICODE_TRANSLATION );
                        return 0;
                    }

                    d <<= 6;
                    d |= (c & 0x3F);
                }

                pos += 1 + n;
            }

            if ( len < utf32Len )
                utf32Str[len] = d;

            len++;
        }

        SetLastError( 0 );
        return len;
    }

    int Utf8To16( const char* utf8Str, int utf8Len, wchar_t* utf16Str, int utf16Len )
    {
        const DWORD     Flags = MB_ERR_INVALID_CHARS;

        return MultiByteToWideChar( 
            CP_UTF8,
            Flags,
            utf8Str,
            utf8Len,
            utf16Str,
            utf16Len );
    }

    int Utf16To8( const wchar_t* utf16Str, int utf16Len, char* utf8Str, int utf8Len )
    {
        const DWORD     Flags = 0;

        return WideCharToMultiByte( 
            CP_UTF8,
            Flags,
            utf16Str,
            utf16Len,
            utf8Str,
            utf8Len,
            NULL,
            NULL );
    }

    int Utf16To32( const wchar_t* utf16Str, int utf16Len, dchar_t* utf32Str, int utf32Len )
    {
        _ASSERT( (utf32Len == 0) || (utf32Str != NULL) );

        int             pos = 0;
        int             len = 0;
        const wchar_t*  s = utf16Str;

        if ( utf16Len < 0 )
            utf16Len = (int) wcslen( utf16Str ) + 1;      // include terminator in length to convert

        while ( pos < utf16Len )
        {
            wchar_t c = s[pos];
            dchar_t d = 0;

            if ( (c < 0xD800) || (c > 0xDFFF) )
            {
                d = c;
                pos++;
            }
            else
            {
                // high surrogate can't be first wchar
                if ( c >= 0xDC00 )
                {
                    SetLastError( ERROR_NO_UNICODE_TRANSLATION );
                    return 0;
                }
                
                if ( (pos + 1) >= utf16Len )
                {
                    SetLastError( ERROR_NO_UNICODE_TRANSLATION );
                    return 0;
                }

                wchar_t c2 = s[pos + 1];

                if ( (c2 < 0xDC00) || (c2 > 0xDFFF) )
                {
                    SetLastError( ERROR_NO_UNICODE_TRANSLATION );
                    return 0;
                }

                c &= ~0xD800;
                c2 &= ~0xDC00;
                d = (c2 | (c << 10)) + 0x10000;

                pos += 2;
            }

            if ( len < utf32Len )
                utf32Str[len] = d;

            len++;
        }

        SetLastError( 0 );
        return len;
    }

    int Utf32To16( bool ignoreErrors, const dchar_t* utf32Str, int utf32Len, wchar_t* utf16Str, int utf16Len )
    {
        _ASSERT( (utf16Len == 0) || (utf16Str != NULL) );

        int             pos = 0;
        int             len = 0;
        const dchar_t*  s = utf32Str;

        if ( utf32Len < 0 )
            utf32Len = (int) dcslen( utf32Str ) + 1;      // include terminator in length to convert

        while ( pos < utf32Len )
        {
            dchar_t c = s[pos];
            pos++;

            if ( c > 0xFFFF )
            {
                dchar_t c2 = c - 0x10000;
                wchar_t w1 = (wchar_t) (c2 >> 10);
                wchar_t w2 = (wchar_t) (c2 & 0x3FF);
                w1 |= 0xD800;
                w2 |= 0xDC00;

                if ( utf16Len == 0 )
                {
                    len += 2;                   // we're only counting, so count away
                }
                else if ( (len + 1) < utf16Len )
                {
                    utf16Str[len] = w1;         // we need to add two, and they fit
                    utf16Str[len+1] = w2;
                    len += 2;
                }
                else                            // they don't fit, so do the right thing, then get out
                {
                    if ( len < utf16Len )
                    {
                        utf16Str[len] = ReplacementChar16;
                        len++;
                    }
                    break;
                }
            }
            else if ( (c >= 0xD800) && (c <= 0xDFFF) )
            {
                if ( ignoreErrors )
                {
                    if ( len < utf16Len )
                        utf16Str[len] = ReplacementChar16;
                    else if ( utf16Len > 0 )
                        break;
                    len++;
                }
                else
                {
                    SetLastError( ERROR_NO_UNICODE_TRANSLATION );
                    return 0;
                }
            }
            else
            {
                if ( len < utf16Len )
                    utf16Str[len] = (wchar_t) c;
                // the user wanted to translate, but we can't fit any more
                else if ( utf16Len > 0 )
                    break;
                len++;
            }
        }

        SetLastError( 0 );
        return len;
    }


    void AppendChar32( std::wstring& str, dchar_t c )
    {
        if ( c > 0xFFFF )
        {
            dchar_t c2 = c - 0x10000;
            wchar_t w1 = (wchar_t) (c2 >> 10);
            wchar_t w2 = (wchar_t) (c2 & 0x3FF);
            w1 |= 0xD800;
            w2 |= 0xDC00;

            str.append( 1, w1 );
            str.append( 1, w2 );
        }
        else
        {
            str.append( 1, (wchar_t) c );
        }
    }

    void AppendChar32( std::vector<char>& buf, dchar_t c )
    {
        int     n = 0;
        uint8_t b = 0;

        if ( c > 0x10FFFF )
            throw 1;

        if ( c >= 0x10000 )
            n = 3;
        else if ( c >= 0x800 )
            n = 2;
        else if ( c >= 0x80 )
            n = 1;
        else
        {
            buf.push_back( (char) c );
            return;
        }

        char    byteStack[3];

        for ( int i = 0; i < n; i++ )
        {
            b = (uint8_t) (c & 0x3F);
            b |= 0x80;
            c >>= 6;

            byteStack[i] = b;
        }

        // take advantage of sign extension to fill in bits emptied by right shift
        uint8_t decor = (uint8_t) (((int) 0xFFFFFFC0) >> (n - 1));
        b = (uint8_t) ((c & 0x1F) >> (n - 1));
        b |= decor;

        buf.push_back( b );

        for ( int i = n - 1; i >= 0; i-- )
            buf.push_back( byteStack[i] );
    }

    size_t dcslen( const dchar_t* s )
    {
        const dchar_t*  start = s;

        for ( ; *s != 0; s++ )
        {
        }

        return s - start;
    }

    dchar_t* dmemchr( dchar_t* s, dchar_t c, size_t n )
    {
        for ( ; n > 0; s++, n-- )
        {
            if ( *s == c )
                return s;
        }

        return NULL;
    }
}
