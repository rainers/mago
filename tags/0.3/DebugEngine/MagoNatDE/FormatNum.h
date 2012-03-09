/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include <Real.h>


void FormatFloat80( void* f80, wchar_t* str, size_t strLen );
void FormatFloat64( double f64, wchar_t* str, size_t strLen );
void FormatFloat32( float f32, wchar_t* str, size_t strLen );


enum
{
    // +X.YYY...YYYe+ZZZZ
    //  Significand sign:   1
    //  Significand digits: Real10::Digits
    //  Decimal point:      1
    //  Exponent 'e'        1
    //  Exponent sign:      1
    //  Exponent digits:    4
    Float80DecStrLen = Real10::Digits + 8,

    // Like Float80, but
    //  Exponent digits:    3
    Float64DecStrLen = std::numeric_limits<double>::digits10 + 7,

    // Like Float80, but
    //  Exponent digits:    3
    Float32DecStrLen = std::numeric_limits<float>::digits10 + 7,
};
