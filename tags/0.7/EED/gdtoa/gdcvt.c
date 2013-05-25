/****************************************************************

Copyright (C) 1998 by Lucent Technologies
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of Lucent or any of its entities
not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.

    Written by Aldo J. Nunez
    Purposed: The double-extended version of fcvt/ecvt
    Based on g_xfmt
****************************************************************/

#include "gdtoaimp.h"

#undef _0
#undef _1

/* one or the other of IEEE_MC68k or IEEE_8087 should be #defined */

#ifdef IEEE_MC68k
#define _0 0
#define _1 1
#define _2 2
#define _3 3
#define _4 4
#endif
#ifdef IEEE_8087
#define _0 4
#define _1 3
#define _2 2
#define _3 1
#define _4 0
#endif


char* gdcvt( void* V, int ndig, int* kind, int* decpt, int* sign )
{
    static FPI     fpi = { 64, 1-16383-64+1, 32766 - 16383 - 64 + 1, 1, 0 };
    char*   se = NULL;
    int     ex = 0;
    ULong   bits[2];
    UShort* L = NULL;

    if ( (kind == NULL) || (decpt == NULL) || (sign == NULL) )
        return NULL;

    L = (UShort*) V;
	*sign = L[_0] & 0x8000;
    bits[1] = (L[_1] << 16) | L[_2];
    bits[0] = (L[_3] << 16) | L[_4];
    ex = L[_0] & 0x7fff;

    if ( ex == 0x7fff )
    {
        if ( bits[0] | (bits[1] & 0x7fffffff) )
        {
            *kind = STRTOG_NaN;
            return NULL;
        }
        else
        {
            *kind = STRTOG_Infinite;
            return NULL;
        }
    }
    else if ( (bits[1] & 0x80000000) != 0 )
    {
        *kind = STRTOG_Normal;
    }
    else if ( bits[0] | bits[1] )
    {
        *kind = STRTOG_Denormal;
        ex = 1;
    }
    else    /* is zero */
    {
        *kind = STRTOG_Zero;
        return NULL;
    }

    ex -= 0x3fff + 63;

    return gdtoa( &fpi, ex, bits, kind, 2, ndig, decpt, &se );
}
