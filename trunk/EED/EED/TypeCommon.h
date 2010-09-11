/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoEE
{
    enum ENUMTY
    {
        Tarray,     // slice array, aka T[]
        Tsarray,        // static array, aka T[dimension]
        Tnarray,        // resizable array, aka T[new]
        Taarray,        // associative array, aka T[type]
        Tpointer,
        Treference,
        Tfunction,
        Tident,
        Tclass,
        Tstruct,
        Tenum,
        Ttypedef,
        Tdelegate,

        Tnone,
        Tvoid,
        Tint8,
        Tuns8,
        Tint16,
        Tuns16,
        Tint32,
        Tuns32,
        Tint64,
        Tuns64,
        Tfloat32,
        Tfloat64,
        Tfloat80,

        Timaginary32,
        Timaginary64,
        Timaginary80,

        Tcomplex32,
        Tcomplex64,
        Tcomplex80,

        Tbit,
        Tbool,
        Tchar,
        Twchar,
        Tdchar,

        Terror,
        Tinstance,
        Ttypeof,
        Ttuple,
        Tslice,
        Treturn,
        TMAX
    };

    enum ALIASTY
    {
        Tsize_t,
        Tptrdiff_t,

        ALIASTMAX
    };

    enum UdtKind
    {
        Udt_Struct,
        Udt_Class,
        Udt_Union
    };
}
