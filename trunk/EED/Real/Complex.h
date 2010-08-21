/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "Real.h"


struct Complex10
{
    Real10  RealPart;
    Real10  ImaginaryPart;

    static uint16_t     Compare( const Complex10& left, const Complex10& right );

    void    Zero();

    void    Add( const Complex10& left, const Complex10& right );
    void    Sub( const Complex10& left, const Complex10& right );
    void    Mul( const Complex10& left, const Complex10& right );
    void    Div( const Complex10& left, const Complex10& right );
    void    Rem( const Complex10& left, const Real10& right );
    void    Negate( const Complex10& orig );
};
