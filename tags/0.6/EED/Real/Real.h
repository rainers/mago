/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


struct Real10
{
    uint16_t    Words[5];

    int         GetSign() const;
    bool        IsZero() const;
    bool        IsNan() const;
    bool        FitsInDouble() const;
    bool        FitsInFloat() const;
    double      ToDouble() const;
    float       ToFloat() const;
    int16_t     ToInt16() const;
    int32_t     ToInt32() const;
    int64_t     ToInt64() const;
    uint64_t    ToUInt64() const;
    void        FromDouble( double d );
    void        FromFloat( float f );
    void        FromInt32( int32_t i );
    void        FromInt64( int64_t i );
    void        FromUInt64( uint64_t i );

    static uint16_t     Compare( const Real10& left, const Real10& right );
    static bool         IsLess( uint16_t status );
    static bool         IsGreater( uint16_t status );
    static bool         IsEqual( uint16_t status );
    static bool         IsUnordered( uint16_t status );

    void    Zero();
    void    LoadInfinity();
    void    LoadNegativeInfinity();
    void    LoadNan();
    void    LoadEpsilon();
    void    LoadMax();
    void    LoadMinNormal();

    static int     MantissaDigits();
    static int     MaxExponentBase10();
    static int     MaxExponentBase2();
    static int     MinExponentBase10();
    static int     MinExponentBase2();

    void    Add( const Real10& left, const Real10& right );
    void    Sub( const Real10& left, const Real10& right );
    void    Mul( const Real10& left, const Real10& right );
    void    Div( const Real10& left, const Real10& right );
    void    Rem( const Real10& left, const Real10& right );
    void    Negate( const Real10& orig );
    void    Abs( const Real10& orig );

    // 20 decimal digits mantissa precision
    // 4  decimal digits exponent precision
    // 1  mantissa sign
    // 1  exponent sign
    // 1  decimal point
    // 10 other characters (like 'e')
    // round to 10

    static const int    Float80DecStrLen = 40;
    static const int    Digits = 18;

    static errno_t  Parse( const wchar_t* str, Real10& val );
    void    ToString( wchar_t* str, int len ) const;
};
