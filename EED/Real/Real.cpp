/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Real.h"
#include "gdtoa.h"


// instead of relying on ecx holding the this pointer
// use:         mov edx, dword ptr [this]
// because ecx might have garbage if the method was inlined

void Real10::Zero()
{
    memset( Words, 0, sizeof Words );
}

int     Real10::GetSign() const
{
    return ((Words[4] & 0x8000) == 0) ? 1 : -1;
}

#ifndef _WIN64
/*  FTST
Condition               C3  C2  C0
ST(0) > 0.0             0   0   0
ST(0) < 0.0             0   0   1
ST(0) = 0.0             1   0   0
Unordered*              1   1   1
*/

// condition code bits in status word: (C0, C1, C2, C3) (8, 9, 10, 14)

bool    Real10::IsZero() const
{
    const uint16_t  Mask = 0x4500;
    uint16_t        status = 0;
    const uint16_t* words = Words;

    _asm
    {
        mov edx, dword ptr [words]
        fld tbyte ptr [edx]
        ftst
        fnstsw word ptr status
        ffree ST(0)
        fincstp
    }

    return (status & Mask) == 0x4000;
}

bool    Real10::IsNan() const
{
    const uint16_t  Mask = 0x4500;
    uint16_t        status = 0;
    const uint16_t* words = Words;

    _asm
    {
        mov edx, dword ptr [words]
        fld tbyte ptr [edx]
        fxam
        fnstsw word ptr status
        ffree ST(0)
        fincstp
    }

    return (status & Mask) == 0x0100;
}

/*  FCOM
Condition               C3  C2  C0
ST(0) > SRC             0   0   0
ST(0) < SRC             0   0   1
ST(0) = SRC             1   0   0
Unordered*              1   1   1
*/

// condition code bits in status word: (C0, C1, C2, C3) (8, 9, 10, 14)

uint16_t    Real10::Compare( const Real10& left, const Real10& right )
{
    uint16_t        status = 0;

    _asm
    {
        mov eax, right
        fld tbyte ptr [eax]
        mov eax, left
        fld tbyte ptr [eax]
        fcompp                  ; compare ST(0) with ST(1) then pop twice
        fnstsw word ptr status
    }

    return status;
}
#endif // _WIN64

bool         Real10::IsLess( uint16_t status )
{
    const uint16_t  Mask = 0x4500;
    return (status & Mask) == 0x0100;
}

bool         Real10::IsGreater( uint16_t status )
{
    const uint16_t  Mask = 0x4500;
    return (status & Mask) == 0x0;
}

bool         Real10::IsEqual( uint16_t status )
{
    const uint16_t  Mask = 0x4500;
    return (status & Mask) == 0x4000;
}

bool         Real10::IsUnordered( uint16_t status )
{
    const uint16_t  Mask = 0x4500;
    return (status & Mask) == 0x4500;
}

#ifndef _WIN64

/*  FXAM
Class                   C3  C2  C0
Unsupported             0   0   0
NaN                     0   0   1
Normal finite number    0   1   0
Infinity                0   1   1
Zero                    1   0   0
Empty                   1   0   1
Denormal number         1   1   0
*/

// condition code bits in status word: (C0, C1, C2, C3) (8, 9, 10, 14)

bool    Real10::FitsInDouble() const
{
    const uint16_t  Mask = 0x4500;
    uint16_t        status1 = 0;
    uint16_t        status2 = 0;
    double          d = 0.0;
    const uint16_t* words = Words;

    _asm
    {
        mov edx, dword ptr [words]
        fld tbyte ptr [edx]
        fxam
        fnstsw word ptr status1
        fstp qword ptr d
        fld qword ptr d
        fxam
        fnstsw word ptr status2
        ffree ST(0)
        fincstp
    }

    return (status1 & Mask) == (status2 & Mask);
}

bool    Real10::FitsInFloat() const
{
    const uint16_t  Mask = 0x4500;
    uint16_t        status1 = 0;
    uint16_t        status2 = 0;
    float           f = 0.0;
    const uint16_t* words = Words;

    _asm
    {
        mov edx, dword ptr [words]
        fld tbyte ptr [edx]
        fxam
        fnstsw word ptr status1
        fstp dword ptr f
        fld dword ptr f
        fxam
        fnstsw word ptr status2
        ffree ST(0)
        fincstp
    }

    return (status1 & Mask) == (status2 & Mask);
}

double  Real10::ToDouble() const
{
    double          d = 0.0;
    const uint16_t* words = Words;

    _asm
    {
        mov edx, dword ptr [words]
        fld tbyte ptr [edx]
        fstp qword ptr d
    }

    return d;
}

float   Real10::ToFloat() const
{
    float           f = 0.0;
    const uint16_t* words = Words;

    _asm
    {
        mov edx, dword ptr [words]
        fld tbyte ptr [edx]
        fstp dword ptr f
    }

    return f;
}

int16_t Real10::ToInt16() const
{
    int16_t         d = 0;
    const uint16_t* words = Words;
    uint16_t        control = 0;
    const uint16_t  rcControl = 0x0F7F;

    _asm
    {
        fnstcw word ptr [control]
        fldcw word ptr [rcControl]          ; set rounding to truncate and precision to double-extended

        mov edx, dword ptr [words]
        fld tbyte ptr [edx]
        fistp word ptr d        ; or should we use fisttp to truncate instead of rounding?

        fldcw word ptr [control]
    }

    return d;
}

int32_t Real10::ToInt32() const
{
    int32_t         d = 0;
    const uint16_t* words = Words;
    uint16_t        control = 0;
    const uint16_t  rcControl = 0x0F7F;

    _asm
    {
        fnstcw word ptr [control]
        fldcw word ptr [rcControl]          ; set rounding to truncate and precision to double-extended

        mov edx, dword ptr [words]
        fld tbyte ptr [edx]
        fistp dword ptr d       ; or should we use fisttp to truncate instead of rounding?

        fldcw word ptr [control]
    }

    return d;
}

int64_t Real10::ToInt64() const
{
    int64_t         d = 0;
    const uint16_t* words = Words;
    uint16_t        control = 0;
    const uint16_t  rcControl = 0x0F7F;

    _asm
    {
        fnstcw word ptr [control]
        fldcw word ptr [rcControl]          ; set rounding to truncate and precision to double-extended

        mov edx, dword ptr [words]
        fld tbyte ptr [edx]
        fistp qword ptr d       ; or should we use fisttp to truncate instead of rounding?

        fldcw word ptr [control]
    }

    return d;
}

uint64_t Real10::ToUInt64() const
{
    uint64_t        d = 0;
    const uint16_t* words = Words;
    uint16_t        control = 0;
    const uint16_t  rcControl = 0x0F7F;
    // represents 2^63 = 0x8000000000000000 in float80
    static uint16_t unsignedMSB64[5] = { 0x0000, 0x0000, 0x0000, 0x8000, 0x403e };

    // ___LDBLULLNG
    _asm
    {
        mov edx, dword ptr [words]
        fld tbyte ptr [edx]
        fld tbyte ptr [unsignedMSB64]
        fcomp st(1)
        fnstsw ax
        fnstcw word ptr [control]
        fldcw word ptr [rcControl]
        sahf
        jae NoMore
        fld tbyte ptr [unsignedMSB64]
        fsubp st(1), st(0)
        fistp qword ptr [d]
        add dword ptr [d+4], 80000000h
        jmp Done
NoMore:
        fistp qword ptr [d]
Done:
        fldcw word ptr [control]
    }
    return d;
}

void    Real10::FromDouble( double d )
{
    uint16_t*   words = Words;

    _asm
    {
        fld qword ptr d
        mov edx, dword ptr [words]
        fstp tbyte ptr [edx]
    }
}

void    Real10::FromFloat( float f )
{
    uint16_t*   words = Words;

    _asm
    {
        fld dword ptr f
        mov edx, dword ptr [words]
        fstp tbyte ptr [edx]
    }
}

void    Real10::FromInt32( int32_t i )
{
    uint16_t*   words = Words;

    _asm
    {
        fild dword ptr i
        mov edx, dword ptr [words]
        fstp tbyte ptr [edx]
    }
}

void    Real10::FromInt64( int64_t i )
{
    uint16_t*   words = Words;

    _asm
    {
        fild qword ptr i
        mov edx, dword ptr [words]
        fstp tbyte ptr [edx]
    }
}

void    Real10::FromUInt64( uint64_t i )
{
    uint16_t*       words = Words;
    uint64_t        iNoMSB = i & 0x7fffffffffffffff;
    // represents 2^63 = 0x8000000000000000 in float80
    static uint16_t unsignedMSB64[5] = { 0x0000, 0x0000, 0x0000, 0x8000, 0x403e };
    uint16_t        control = 0;

    _asm
    {
        fnstcw word ptr [control]
        or word ptr [control], 0300h    ; set precision to double-extended
        fldcw word ptr [control]

        ; load the integer without the most significant bit
        fild qword ptr iNoMSB
    }

    // was the MSB set? if so, add it as an already converted float80
    if ( i != iNoMSB )
    {
        _asm
        {
            fld tbyte ptr [unsignedMSB64]
            fadd
        }
    }

    _asm
    {
        mov edx, dword ptr [words]
        fstp tbyte ptr [edx]
    }
}

#endif // _WIN64

void    Real10::LoadInfinity()
{
    strtopx( "infinity", NULL, Words );
}

void    Real10::LoadNegativeInfinity()
{
    strtopx( "-infinity", NULL, Words );
}

void    Real10::LoadNan()
{
    strtopx( "nan", NULL, Words );
}

void    Real10::LoadEpsilon()
{
    Words[0] = 0x0000;
    Words[1] = 0x0000;
    Words[2] = 0x0000;
    Words[3] = 0x8000;
    Words[4] = 0x3fc0;
}

void    Real10::LoadMax()
{
    Words[0] = 0xffff;
    Words[1] = 0xffff;
    Words[2] = 0xffff;
    Words[3] = 0xffff;
    Words[4] = 0x7ffe;
}

void    Real10::LoadMinNormal()
{
    Words[0] = 0x0000;
    Words[1] = 0x0000;
    Words[2] = 0x0000;
    Words[3] = 0x8000;
    Words[4] = 0x0001;
}

int     Real10::MantissaDigits()
{
    return 64;
}

int     Real10::MaxExponentBase10()
{
    return 4932;
}

int     Real10::MaxExponentBase2()
{
    return 16384;
}

int     Real10::MinExponentBase10()
{
    return -4932;
}

int     Real10::MinExponentBase2()
{
    return -16381;
}

#ifndef _WIN64

void    Real10::Add( const Real10& left, const Real10& right )
{
    uint16_t*   words = Words;
    uint16_t    control = 0;

    _asm
    {
        // TODO: modify the control word like this for all operations, 
        //      maybe it should be global?
        //      do we need to set it back after the operation?

        fnstcw word ptr [control]
        or word ptr [control], 0300h    ; set precision to double-extended
        fldcw word ptr [control]

        mov eax, left
        fld tbyte ptr [eax]
        mov eax, right
        fld tbyte ptr [eax]
        fadd                    ; ST(1) := ST(1) + ST(0) then pop
        mov edx, dword ptr [words]
        fstp tbyte ptr [edx]
    }
}

void    Real10::Sub( const Real10& left, const Real10& right )
{
    uint16_t*   words = Words;
    uint16_t    control = 0;

    _asm
    {
        fnstcw word ptr [control]
        or word ptr [control], 0300h    ; set precision to double-extended
        fldcw word ptr [control]

        mov eax, left
        fld tbyte ptr [eax]
        mov eax, right
        fld tbyte ptr [eax]
        fsub                    ; ST(1) := ST(1) - ST(0) then pop
        mov edx, dword ptr [words]
        fstp tbyte ptr [edx]
    }
}

void    Real10::Mul( const Real10& left, const Real10& right )
{
    uint16_t*   words = Words;
    uint16_t    control = 0;

    _asm
    {
        fnstcw word ptr [control]
        or word ptr [control], 0300h    ; set precision to double-extended
        fldcw word ptr [control]

        mov eax, left
        fld tbyte ptr [eax]
        mov eax, right
        fld tbyte ptr [eax]
        fmul                    ; ST(1) := ST(1) * ST(0) then pop
        mov edx, dword ptr [words]
        fstp tbyte ptr [edx]
    }
}

void    Real10::Div( const Real10& left, const Real10& right )
{
    uint16_t*   words = Words;
    uint16_t    control = 0;

    _asm
    {
        fnstcw word ptr [control]
        or word ptr [control], 0300h    ; set precision to double-extended
        fldcw word ptr [control]

        mov eax, left
        fld tbyte ptr [eax]
        mov eax, right
        fld tbyte ptr [eax]
        fdiv                    ; ST(1) := ST(1) / ST(0) then pop
        mov edx, dword ptr [words]
        fstp tbyte ptr [edx]
    }
}

void    Real10::Rem( const Real10& left, const Real10& right )
{
    uint16_t*   words = Words;
    uint16_t    control = 0;

    _asm
    {
        fnstcw word ptr [control]
        or word ptr [control], 0300h    ; set precision to double-extended
        fldcw word ptr [control]

        mov eax, right
        fld tbyte ptr [eax]
        mov eax, left
        fld tbyte ptr [eax]
Retry:
        fprem                   ; ST(0) := ST(0) % ST(1)    doesn't pop!
        fnstsw ax
        sahf
        jp Retry

        mov edx, dword ptr [words]
        fstp tbyte ptr [edx]
        ffree ST(0)
        fincstp
    }
}

void    Real10::Negate( const Real10& orig )
{
    uint16_t*   words = Words;

    _asm
    {
#if 0
        fldz
        fld tbyte ptr [orig]
        fsub
#else
        mov eax, orig
        fld tbyte ptr [eax]
        fchs
#endif
        mov edx, dword ptr [words]
        fstp tbyte ptr [edx]
    }
}

void    Real10::Abs( const Real10& orig )
{
    uint16_t*   words = Words;

    _asm
    {
        mov eax, orig
        fld tbyte ptr [eax]
        fabs
        mov edx, dword ptr [words]
        fstp tbyte ptr [edx]
    }
}

#endif // _WIN64

errno_t Real10::Parse( const wchar_t* str, Real10& val )
{
    size_t  len = wcslen( str );
    char*   astr = new char[ len + 1 ];

    for ( size_t i = 0; i < len + 1; i++ )
        astr[i] = (char) str[i];

    errno_t err = 0;
    char*   p = NULL;

    _set_errno( 0 );
    strtopx( astr, &p, val.Words );

    if ( p == astr )
        err = EINVAL;
    else
        _get_errno( &err );

    delete [] astr;

    return err;
}


void Real10::ToString( wchar_t* str, int len, int digits ) const
{
    char    s[Float80DecStrLen+1] = "";
    int     bufLen = (len > _countof( s )) ? _countof( s ) : len;

    // TODO: let the user change digit count
    g_xfmt( s, (void*) Words, digits, bufLen );

    for ( char* p = s; *p != '\0'; p++, str++ )
        *str = (wchar_t) *p;

    *str = L'\0';
}
