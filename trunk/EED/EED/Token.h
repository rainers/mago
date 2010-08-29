/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


/* Tokens:
    (   )
    [   ]
    {   }
    <   >   <=  >=  ==  !=  === !==
    <<  >>  <<= >>= >>> >>>=
    +   -   +=  -=
    *   /   %   *=  /=  %=
    &   |   ^   &=  |=  ^=
    =   !   ~   @
    ^^  ^^=
    ++  --
    .   ->  :   ,
    ?   &&  ||
 */

enum TOK
{
    TOKreserved,

    // Other
    TOKlparen,  TOKrparen,
    TOKlbracket,    TOKrbracket,
    TOKlcurly,  TOKrcurly,
    TOKcolon,   TOKneg,
    TOKsemicolon,   TOKdotdotdot,
    TOKeof,     TOKcast,
    TOKnull,    TOKassert,
    TOKtrue,    TOKfalse,
    TOKarray,   TOKcall,
    TOKaddress,
    TOKtype,    TOKthrow,
    TOKnew,     TOKdelete,
    TOKstar,    TOKsymoff,
    TOKvar,     TOKdotvar,
    TOKdotti,   TOKdotexp,
    TOKdottype, TOKslice,
    TOKarraylength, TOKversion,
    TOKmodule,  TOKdollar,
    TOKtemplate,    TOKdottd,
    TOKdeclaration, TOKtypeof, TOKtypeof2,
    TOKpragma,  TOKdsymbol,
    TOKtypeid,  TOKuadd,
    TOKremove,
    TOKnewanonclass, TOKcomment,
    TOKarrayliteral, TOKassocarrayliteral,
    TOKstructliteral,

    // Operators
    TOKlt,      TOKgt,
    TOKle,      TOKge,
    TOKequal,   TOKnotequal,
    TOKidentity,    TOKnotidentity,
    TOKindex,   TOKis,
    TOKtobool,

// 60
    // NCEG floating point compares
    // !<>=     <>    <>=    !>     !>=   !<     !<=   !<>
    TOKunord,TOKlg,TOKleg,TOKule,TOKul,TOKuge,TOKug,TOKue,

    TOKshl,     TOKshr,
    TOKshlass,  TOKshrass,
    TOKushr,    TOKushrass,
    TOKcat,     TOKcatass,  // ~ ~=
    TOKadd,     TOKmin,     TOKaddass,  TOKminass,
    TOKmul,     TOKdiv,     TOKmod,
    TOKmulass,  TOKdivass,  TOKmodass,
    TOKand,     TOKor,      TOKxor,
    TOKandass,  TOKorass,   TOKxorass,
    TOKassign,  TOKnot,     TOKtilde,
    TOKplusplus,    TOKminusminus,  TOKconstruct,   TOKblit,
    TOKdot,     TOKarrow,   TOKcomma,
    TOKquestion,    TOKandand,  TOKoror,

// 104
    // Numeric literals
    TOKint32v, TOKuns32v,
    TOKint64v, TOKuns64v,
    TOKfloat32v, TOKfloat64v, TOKfloat80v,
    TOKimaginary32v, TOKimaginary64v, TOKimaginary80v,

    // Char constants
    TOKcharv, TOKwcharv, TOKdcharv,

    // Leaf operators
    TOKidentifier,  TOKstring,
    TOKthis,    TOKsuper,
    TOKhalt,    TOKtuple,
    TOKerror,
    TOKwstring, TOKdstring,

    // Basic types
    TOKvoid,
    TOKint8, TOKuns8,
    TOKint16, TOKuns16,
    TOKint32, TOKuns32,
    TOKint64, TOKuns64,
    TOKfloat32, TOKfloat64, TOKfloat80,
    TOKimaginary32, TOKimaginary64, TOKimaginary80,
    TOKcomplex32, TOKcomplex64, TOKcomplex80,
    TOKchar, TOKwchar, TOKdchar, TOKbit, TOKbool,
    TOKcent, TOKucent,

    // Aggregates
    TOKstruct, TOKclass, TOKinterface, TOKunion, TOKenum, TOKimport,
    TOKtypedef, TOKalias, TOKoverride, TOKdelegate, TOKfunction,
    TOKmixin,

    TOKalign, TOKextern, TOKprivate, TOKprotected, TOKpublic, TOKexport,
    TOKstatic, /*TOKvirtual,*/ TOKfinal, TOKconst, TOKabstract, TOKvolatile,
    TOKdebug, TOKdeprecated, TOKin, TOKout, TOKinout, TOKlazy,
    TOKauto, TOKpackage, TOKmanifest, TOKimmutable,

    // Statements
    TOKif, TOKelse, TOKwhile, TOKfor, TOKdo, TOKswitch,
    TOKcase, TOKdefault, TOKbreak, TOKcontinue, TOKwith,
    TOKsynchronized, TOKreturn, TOKgoto, TOKtry, TOKcatch, TOKfinally,
    TOKasm, TOKforeach, TOKforeach_reverse,
    TOKscope,
    TOKon_scope_exit, TOKon_scope_failure, TOKon_scope_success,

    // Contracts
    TOKbody, TOKinvariant,

    // Testing
    TOKunittest,

    // Added after 1.0
    TOKref,
    TOKmacro,
#if DMDV2
    TOKtraits,
    TOKoverloadset,
    TOKpure,
    TOKnothrow,
    TOKtls,
    TOKgshared,
    TOKline,
    TOKfile,
    TOKshared,
    TOKat,
    TOKpow,
    //TOKpowass,
#endif

    TOKMAX
};


namespace MagoEE
{
    struct ByteString;
    struct Utf16String;
    struct Utf32String;
}


struct Token
{
    TOK             Code;
    const wchar_t*  TextStartPtr;
    union
    {
        int64_t         Int64Value;
        uint64_t        UInt64Value;
        Real10          Float80Value;

        MagoEE::ByteString*     ByteStr;
        MagoEE::Utf16String*    Utf16Str;
        MagoEE::Utf32String*    Utf32Str;
    };
};
