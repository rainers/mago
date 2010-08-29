/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Keywords.h"
#include "Token.h"


struct Keyword
{
    const wchar_t*  Name;
    TOK             Code;
};


static Keyword gKeywords[] = 
{
    { L"", TOKreserved },
    { L"abstract", TOKabstract },
    { L"alias", TOKalias },
    { L"align", TOKalign },
    { L"asm", TOKasm },
    { L"assert", TOKassert },
    { L"auto", TOKauto },

    { L"body", TOKbody },
    { L"bool", TOKbool },
    { L"break", TOKbreak },
    { L"byte", TOKint8 },

    { L"case", TOKcase },
    { L"cast", TOKcast },
    { L"catch", TOKcatch },
    { L"cdouble", TOKcomplex64 },
    { L"cent", TOKcent },
    { L"cfloat", TOKcomplex32 },
    { L"char", TOKchar },
    { L"class", TOKclass },
    { L"const", TOKconst },
    { L"continue", TOKcontinue },
    { L"creal", TOKcomplex80 },

    { L"dchar", TOKdchar },
    { L"debug", TOKdebug },
    { L"default", TOKdefault },
    { L"delegate", TOKdelegate },
    { L"delete", TOKdelete },
    { L"deprecated", TOKdeprecated },
    { L"do", TOKdo },
    { L"double", TOKfloat64 },

    { L"else", TOKelse },
    { L"enum", TOKenum },
    { L"export", TOKexport },
    { L"extern", TOKextern },

    { L"false", TOKfalse },
    { L"final", TOKfinal },
    { L"finally", TOKfinally },
    { L"float", TOKfloat32 },
    { L"for", TOKfor },
    { L"foreach", TOKforeach },
    { L"foreach_reverse", TOKforeach_reverse },
    { L"function", TOKfunction },

    { L"goto", TOKgoto },

    { L"idouble", TOKimaginary64 },
    { L"if", TOKif },
    { L"ifloat", TOKimaginary32 },
    { L"immutable", TOKimmutable },
    { L"import", TOKimport },
    { L"in", TOKin },
    { L"inout", TOKinout },
    { L"int", TOKint32 },
    { L"interface", TOKinterface },
    { L"invariant", TOKinvariant },
    { L"ireal", TOKimaginary80 },
    { L"is", TOKis },

    { L"lazy", TOKlazy },
    { L"long", TOKint64 },

    { L"macro", TOKmacro },
    { L"mixin", TOKmixin },
    { L"module", TOKmodule },

    { L"new", TOKnew },
    { L"nothrow", TOKnothrow },
    { L"null", TOKnull },

    { L"out", TOKout },
    { L"override", TOKoverride },

    { L"package", TOKpackage },
    { L"pragma", TOKpragma },
    { L"private", TOKprivate },
    { L"protected", TOKprotected },
    { L"public", TOKpublic },
    { L"pure", TOKpure },

    { L"real", TOKfloat80 },
    { L"ref", TOKref },
    { L"return", TOKreturn },

    { L"scope", TOKscope },
    { L"shared", TOKshared },
    { L"short", TOKint16 },
    { L"static", TOKstatic },
    { L"struct", TOKstruct },
    { L"super", TOKsuper },
    { L"switch", TOKswitch },
    { L"synchronized", TOKsynchronized },

    { L"template", TOKtemplate },
    { L"this", TOKthis },
    { L"throw", TOKthrow },
    { L"true", TOKtrue },
    { L"try", TOKtry },
    { L"typedef", TOKtypedef },
    { L"typeid", TOKtypeid },
    { L"typeof", TOKtypeof },
    { L"typeof2", TOKtypeof2 },

    { L"ubyte", TOKuns8 },
    { L"ucent", TOKucent },
    { L"uint", TOKuns32 },
    { L"ulong", TOKuns64 },
    { L"union", TOKunion },
    { L"unittest", TOKunittest },
    { L"ushort", TOKuns16 },

    { L"version", TOKversion },
    { L"void", TOKvoid },
    { L"volatile", TOKvolatile },

    { L"wchar", TOKwchar },
    { L"while", TOKwhile },
    { L"with", TOKwith },

    { L"__FILE__", TOKfile },
    { L"__LINE__", TOKline },
    { L"__gshared", TOKgshared },
    { L"__thread", TOKtls },
    { L"__traits", TOKtraits },
};


TOK MapToKeyword( const wchar_t* id )
{
    return MapToKeyword( id, wcslen( id ) );
}

TOK MapToKeyword( const wchar_t* id, size_t len )
{
    for ( int i = 0; i < _countof( gKeywords ); i++ )
    {
        if ( wcsncmp( gKeywords[i].Name, id, len ) == 0 )
        {
            // they're the same up to our passed in length, 
            // now see if the keyword ends there or is longer
            if ( gKeywords[i].Name[len] == L'\0' )
                return gKeywords[i].Code;
        }
    }

    return TOKidentifier;
}
