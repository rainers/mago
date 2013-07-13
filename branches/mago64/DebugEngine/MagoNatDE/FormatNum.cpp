/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "FormatNum.h"
#include <Real.h>
#include <gdtoa.h>


struct FloatToDecParams
{
    const char* DigitsStr;
    int         Digits;
    int         ExpDigits;
    int         Kind;
    int         DecPt;
    int         Sign;
};


// canonical string is "+X.YYY...YYYe+ZZZZ"

void FormatFloat( const FloatToDecParams& convParams, wchar_t* str, size_t strLen )
{
    _ASSERT( (str != NULL) && (strLen > 0) );

    // we don't want any extra information
    int     kind = convParams.Kind & STRTOG_Retmask;
    int     nChars = 0;

    nChars = swprintf_s( str, strLen, L"%lc", (convParams.Sign == 0) ? L'+' : L'-' );
    str += nChars;
    strLen -= nChars;

    switch ( kind )
    {
    case STRTOG_NaN:
        swprintf_s( str, strLen, L"%ls", L"1#QNAN" );
        break;

    case STRTOG_Infinite:
        swprintf_s( str, strLen, L"%ls", L"1#INF" );
        break;

    case STRTOG_Zero:
        swprintf_s( str, strLen, L"0.%0.*de+%0.*d", 
            convParams.Digits - 1, 0, 
            convParams.ExpDigits, 0 );
        break;

    default:
        {
            _ASSERT( convParams.DigitsStr != NULL );
            _ASSERT( convParams.DigitsStr[0] != '\0' );

            int     digitsWanted = convParams.Digits - 1;

            if ( kind == STRTOG_Denormal )
                digitsWanted -= 4;          // to make room for "#DEN"
            _ASSERT( digitsWanted >= 0 );

            nChars = swprintf_s( str, strLen, L"%hc.%.*hs", 
                convParams.DigitsStr[0], 
                digitsWanted, 
                &convParams.DigitsStr[1] );
            str += nChars;
            strLen -= nChars;

            if ( nChars < (digitsWanted + 2) )
            {
                // fill in least significant zeros
                nChars = swprintf_s( str, strLen, L"%0.*d", 
                    digitsWanted + 2 - nChars, 
                    0 );
                str += nChars;
                strLen -= nChars;
            }

            swprintf_s( str, strLen, L"e%+0.*d%ls", 
                convParams.ExpDigits,
                convParams.DecPt - 1, 
                (kind == STRTOG_Normal) ? L"" : L"#DEN" );
        }
        break;
    }
}


void FormatFloat80( void* f80, wchar_t* str, size_t strLen )
{
    _ASSERT( (f80 != NULL) && (str != NULL) && (strLen > 0) );

    char*   s = NULL;
    int     kind = 0;
    int     decpt = 0;
    int     sign = 0;
    FloatToDecParams    params = { 0 };

    s = gdcvt( f80, Real10::Digits, &kind, &decpt, &sign );

    params.Digits = Real10::Digits;
    params.ExpDigits = 4;
    params.DigitsStr = s;
    params.Kind = kind;
    params.DecPt = decpt;
    params.Sign = sign;

    FormatFloat( params, str, strLen );

    if ( s != NULL )
        freedtoa( s );
}


template <typename T>
int GetFloatKindBasic( T f );

template <>
int GetFloatKindBasic<double>( double f )
{
    uint32_t*   words = (uint32_t*) &f;

    if ( (words[1] & 0x7ff00000) == 0x7ff00000 )
    {
        if ( (words[1] & 0xfffff) || words[0] )
            return STRTOG_NaN;
        else
            return STRTOG_Infinite;
    }
    else if ( (words[1] & 0x7ff00000) == 0 )
    {
        if ( (words[1] & 0xfffff) || words[0] )
            return STRTOG_Denormal;
        else
            return STRTOG_Zero;
    }
    else
        return STRTOG_Normal;
}

template <>
int GetFloatKindBasic<float>( float f )
{
    uint32_t*   words = (uint32_t*) &f;

    if ( (words[0] & 0x7f800000) == 0x7f800000 )
    {
        if ( words[0] & 0x7fffff )
            return STRTOG_NaN;
        else
            return STRTOG_Infinite;
    }
    else if ( (words[0] & 0x7f800000) == 0 )
    {
        if ( words[0] & 0x7fffff )
            return STRTOG_Denormal;
        else
            return STRTOG_Zero;
    }
    else
        return STRTOG_Normal;
}


template <typename T>
void FormatFloatBasic( T f, wchar_t* str, size_t strLen )
{
    C_ASSERT( !std::numeric_limits<T>::is_integer );
    _ASSERT( (str != NULL) && (strLen > 0) );

    // _ecvt and _fcvt say they need an extra char for "overflow". What does it mean?
    char    s[ std::numeric_limits<T>::digits10 + 1 + 1 ] = "";
    int     decpt = 0;
    int     sign = 0;
    errno_t err = 0;
    FloatToDecParams    params = { 0 };

    err = _ecvt_s( s, (double) f, std::numeric_limits<T>::digits10, &decpt, &sign );
    _ASSERT( err == 0 );

    params.Digits = std::numeric_limits<T>::digits10;
    params.ExpDigits = 3;
    params.DigitsStr = s;
    params.Kind = GetFloatKindBasic<T>( f );
    params.DecPt = decpt;
    params.Sign = sign;

    FormatFloat( params, str, strLen );
}


void FormatFloat64( double f64, wchar_t* str, size_t strLen )
{
    FormatFloatBasic<double>( f64, str, strLen );
}

void FormatFloat32( float f32, wchar_t* str, size_t strLen )
{
    FormatFloatBasic<float>( f32, str, strLen );
}
