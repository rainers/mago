#include "Common.h"

#include "../Real/Real.h"
#include <assert.h>

bool TestReal10()
{
    Real10 r0, rnan, r1, r2, r3, r4, r5;
    r0.Zero();
    assert(r0.IsZero());
    assert(!r0.IsNan());
    rnan.LoadNan();
    assert(rnan.IsNan());

    r1.FromDouble(1);
    r2.FromFloat(2);
    r3.FromInt32(3);
    r4.FromInt64(4);
    r5.FromUInt64((1LLU << 63) + 1);

    assert(!r1.IsZero());
    assert(!r5.IsZero());

    Real10 r6;  r6.Mul(r2, r3);
    Real10 r7;  r7.Add(r6, r1);
    Real10 rm2; rm2.Negate(r2);
    Real10 r8;  r8.Sub(r6, rm2);
    Real10 rm3; rm3.Div(r6, rm2);
    Real10 rr1; rr1.Rem(r7, r2);
    Real10 ra2; ra2.Abs(rm2);

    assert(r1.ToInt16() == 1);
    assert(r2.ToInt32() == 2);
    assert(r3.ToInt64() == 3);
    assert(r4.ToUInt64() == 4);
    assert(r5.ToUInt64() == (1LLU << 63) + 1);
    assert(r6.ToDouble() == 6);
    assert(r7.ToDouble() == 7);
    assert(r8.ToFloat()  == 8);
    assert(rm2.ToFloat() == -2);
    assert(rm3.ToFloat() == -3);
    assert(rr1.ToFloat() == 1);
    assert(ra2.ToFloat() == 2);

    assert(Real10::IsLess(Real10::Compare(r1, r2)));
    assert(Real10::IsEqual(Real10::Compare(r1, rr1)));
    assert(Real10::IsGreater(Real10::Compare(r3, r2)));

    assert(r1.FitsInDouble());
    r0.LoadEpsilon();
    r1.Add(r1, r0);
    assert(!Real10::IsEqual(Real10::Compare(r1, rr1)));

    return true;
}