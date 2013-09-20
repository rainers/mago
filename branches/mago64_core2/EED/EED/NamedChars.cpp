/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "NamedChars.h"


struct NamedChar
{
    const wchar_t*  Name;
    MagoEE::dchar_t         Code;
};


static NamedChar gChars[] = 
{
    // Name Value Symbol
    { L"quot", 0x22 },    // 
    { L"amp", 0x26 },    // &
    { L"lt", 0x3C },    // <
    { L"gt", 0x3E },    // >
    { L"OElig", 0x152 },    // Œ
    { L"oelig", 0x153 },    // œ
    { L"Scaron", 0x160 },    // Š
    { L"scaron", 0x161 },    // š
    { L"Yuml", 0x178 },    // Ÿ
    { L"circ", 0x2C6 },    // ˆ
    { L"tilde", 0x2DC },    // ˜
    { L"ensp", 0x2002 },    //  
    { L"emsp", 0x2003 },    //  
    { L"thinsp", 0x2009 },    //  
    { L"zwnj", 0x200C },    // ‌
    { L"zwj", 0x200D },    // ‍
    { L"lrm", 0x200E },    // ‎
    { L"rlm", 0x200F },    // ‏
    { L"ndash", 0x2013 },    // –
    { L"mdash", 0x2014 },    // —
    { L"lsquo", 0x2018 },    // ‘
    { L"rsquo", 0x2019 },    // ’
    { L"sbquo", 0x201A },    // ‚
    { L"ldquo", 0x201C },    // “
    { L"rdquo", 0x201D },    // ”
    { L"bdquo", 0x201E },    // „
    { L"dagger", 0x2020 },    // †
    { L"Dagger", 0x2021 },    // ‡
    { L"permil", 0x2030 },    // ‰
    { L"lsaquo", 0x2039 },    // ‹
    { L"rsaquo", 0x203A },    // ›
    { L"euro", 0x20AC },    // €
// Latin-1 (ISO-8859-1) Entities
    { L"nbsp", 0xA0 },    // 
    { L"iexcl", 0xA1 },    // ¡
    { L"cent", 0xA2 },    // ¢
    { L"pound", 0xA3 },    // £
    { L"curren", 0xA4 },    // ¤
    { L"yen", 0xA5 },    // ¥
    { L"brvbar", 0xA6 },    // ¦
    { L"sect", 0xA7 },    // §
    { L"uml", 0xA8 },    // ¨
    { L"copy", 0xA9 },    // ©
    { L"ordf", 0xAA },    // ª
    { L"laquo", 0xAB },    // «
    { L"not", 0xAC },    // ¬
    { L"shy", 0xAD },    // ­
    { L"reg", 0xAE },    // ®
    { L"macr", 0xAF },    // ¯
    { L"deg", 0xB0 },    // °
    { L"plusmn", 0xB1 },    // ±
    { L"sup2", 0xB2 },    // ²
    { L"sup3", 0xB3 },    // ³
    { L"acute", 0xB4 },    // ´
    { L"micro", 0xB5 },    // µ
    { L"para", 0xB6 },    // ¶
    { L"middot", 0xB7 },    // ·
    { L"cedil", 0xB8 },    // ¸
    { L"sup1", 0xB9 },    // ¹
    { L"ordm", 0xBA },    // º
    { L"raquo", 0xBB },    // »
    { L"frac14", 0xBC },    // ¼
    { L"frac12", 0xBD },    // ½
    { L"frac34", 0xBE },    // ¾
    { L"iquest", 0xBF },    // ¿
    { L"Agrave", 0xC0 },    // À
    { L"Aacute", 0xC1 },    // Á
    { L"Acirc", 0xC2 },    // Â
    { L"Atilde", 0xC3 },    // Ã
    { L"Auml", 0xC4 },    // Ä
    { L"Aring", 0xC5 },    // Å
    { L"AElig", 0xC6 },    // Æ
    { L"Ccedil", 0xC7 },    // Ç
    { L"Egrave", 0xC8 },    // È
    { L"Eacute", 0xC9 },    // É
    { L"Ecirc", 0xCA },    // Ê
    { L"Euml", 0xCB },    // Ë
    { L"Igrave", 0xCC },    // Ì
    { L"Iacute", 0xCD },    // Í
    { L"Icirc", 0xCE },    // Î
    { L"Iuml", 0xCF },    // Ï
    { L"ETH", 0xD0 },    // Ð
    { L"Ntilde", 0xD1 },    // Ñ
    { L"Ograve", 0xD2 },    // Ò
    { L"Oacute", 0xD3 },    // Ó
    { L"Ocirc", 0xD4 },    // Ô
    { L"Otilde", 0xD5 },    // Õ
    { L"Ouml", 0xD6 },    // Ö
    { L"times", 0xD7 },    // ×
    { L"Oslash", 0xD8 },    // Ø
    { L"Ugrave", 0xD9 },    // Ù
    { L"Uacute", 0xDA },    // Ú
    { L"Ucirc", 0xDB },    // Û
    { L"Uuml", 0xDC },    // Ü
    { L"Yacute", 0xDD },    // Ý
    { L"THORN", 0xDE },    // Þ
    { L"szlig", 0xDF },    // ß
    { L"agrave", 0xE0 },    // à
    { L"aacute", 0xE1 },    // á
    { L"acirc", 0xE2 },    // â
    { L"atilde", 0xE3 },    // ã
    { L"auml", 0xE4 },    // ä
    { L"aring", 0xE5 },    // å
    { L"aelig", 0xE6 },    // æ
    { L"ccedil", 0xE7 },    // ç
    { L"egrave", 0xE8 },    // è
    { L"eacute", 0xE9 },    // é
    { L"ecirc", 0xEA },    // ê
    { L"euml", 0xEB },    // ë
    { L"igrave", 0xEC },    // ì
    { L"iacute", 0xED },    // í
    { L"icirc", 0xEE },    // î
    { L"iuml", 0xEF },    // ï
    { L"eth", 0xF0 },    // ð
    { L"ntilde", 0xF1 },    // ñ
    { L"ograve", 0xF2 },    // ò
    { L"oacute", 0xF3 },    // ó
    { L"ocirc", 0xF4 },    // ô
    { L"otilde", 0xF5 },    // õ
    { L"ouml", 0xF6 },    // ö
    { L"divide", 0xF7 },    // ÷
    { L"oslash", 0xF8 },    // ø
    { L"ugrave", 0xF9 },    // ù
    { L"uacute", 0xFA },    // ú
    { L"ucirc", 0xFB },    // û
    { L"uuml", 0xFC },    // ü
    { L"yacute", 0xFD },    // ý
    { L"thorn", 0xFE },    // þ
    { L"yuml", 0xFF },    // ÿ
// Symbols and Greek letter entities
    { L"fnof", 0x192 },    // ƒ
    { L"Alpha", 0x391 },    // Α
    { L"Beta", 0x392 },    // Β
    { L"Gamma", 0x393 },    // Γ
    { L"Delta", 0x394 },    // Δ
    { L"Epsilon", 0x395 },    // Ε
    { L"Zeta", 0x396 },    // Ζ
    { L"Eta", 0x397 },    // Η
    { L"Theta", 0x398 },    // Θ
    { L"Iota", 0x399 },    // Ι
    { L"Kappa", 0x39A },    // Κ
    { L"Lambda", 0x39B },    // Λ
    { L"Mu", 0x39C },    // Μ
    { L"Nu", 0x39D },    // Ν
    { L"Xi", 0x39E },    // Ξ
    { L"Omicron", 0x39F },    // Ο
    { L"Pi", 0x3A0 },    // Π
    { L"Rho", 0x3A1 },    // Ρ
    { L"Sigma", 0x3A3 },    // Σ
    { L"Tau", 0x3A4 },    // Τ
    { L"Upsilon", 0x3A5 },    // Υ
    { L"Phi", 0x3A6 },    // Φ
    { L"Chi", 0x3A7 },    // Χ
    { L"Psi", 0x3A8 },    // Ψ
    { L"Omega", 0x3A9 },    // Ω
    { L"alpha", 0x3B1 },    // α
    { L"beta", 0x3B2 },    // β
    { L"gamma", 0x3B3 },    // γ
    { L"delta", 0x3B4 },    // δ
    { L"epsilon", 0x3B5 },    // ε
    { L"zeta", 0x3B6 },    // ζ
    { L"eta", 0x3B7 },    // η
    { L"theta", 0x3B8 },    // θ
    { L"iota", 0x3B9 },    // ι
    { L"kappa", 0x3BA },    // κ
    { L"lambda", 0x3BB },    // λ
    { L"mu", 0x3BC },    // μ
    { L"nu", 0x3BD },    // ν
    { L"xi", 0x3BE },    // ξ
    { L"omicron", 0x3BF },    // ο
    { L"pi", 0x3C0 },    // π
    { L"rho", 0x3C1 },    // ρ
    { L"sigmaf", 0x3C2 },    // ς
    { L"sigma", 0x3C3 },    // σ
    { L"tau", 0x3C4 },    // τ
    { L"upsilon", 0x3C5 },    // υ
    { L"phi", 0x3C6 },    // φ
    { L"chi", 0x3C7 },    // χ
    { L"psi", 0x3C8 },    // ψ
    { L"omega", 0x3C9 },    // ω
    { L"thetasym", 0x3D1 },    // ϑ
    { L"upsih", 0x3D2 },    // ϒ
    { L"piv", 0x3D6 },    // ϖ
    { L"bull", 0x2022 },    // •
    { L"hellip", 0x2026 },    // …
    { L"prime", 0x2032 },    // ′
    { L"Prime", 0x2033 },    // ″
    { L"oline", 0x203E },    // ‾
    { L"frasl", 0x2044 },    // ⁄
    { L"weierp", 0x2118 },    // ℘
    { L"image", 0x2111 },    // ℑ
    { L"real", 0x211C },    // ℜ
    { L"trade", 0x2122 },    // ™
    { L"alefsym", 0x2135 },    // ℵ
    { L"larr", 0x2190 },    // ←
    { L"uarr", 0x2191 },    // ↑
    { L"rarr", 0x2192 },    // →
    { L"darr", 0x2193 },    // ↓
    { L"harr", 0x2194 },    // ↔
    { L"crarr", 0x21B5 },    // ↵
    { L"lArr", 0x21D0 },    // ⇐
    { L"uArr", 0x21D1 },    // ⇑
    { L"rArr", 0x21D2 },    // ⇒
    { L"dArr", 0x21D3 },    // ⇓
    { L"hArr", 0x21D4 },    // ⇔
    { L"forall", 0x2200 },    // ∀
    { L"part", 0x2202 },    // ∂
    { L"exist", 0x2203 },    // ∃
    { L"empty", 0x2205 },    // ∅
    { L"nabla", 0x2207 },    // ∇
    { L"isin", 0x2208 },    // ∈
    { L"notin", 0x2209 },    // ∉
    { L"ni", 0x220B },    // ∋
    { L"prod", 0x220F },    // ∏
    { L"sum", 0x2211 },    // ∑
    { L"minus", 0x2212 },    // −
    { L"lowast", 0x2217 },    // ∗
    { L"radic", 0x221A },    // √
    { L"prop", 0x221D },    // ∝
    { L"infin", 0x221E },    // ∞
    { L"ang", 0x2220 },    // ∠
    { L"and", 0x2227 },    // ∧
    { L"or", 0x2228 },    // ∨
    { L"cap", 0x2229 },    // ∩
    { L"cup", 0x222A },    // ∪
    { L"int", 0x222B },    // ∫
    { L"there4", 0x2234 },    // ∴
    { L"sim", 0x223C },    // ∼
    { L"cong", 0x2245 },    // ≅
    { L"asymp", 0x2248 },    // ≈
    { L"ne", 0x2260 },    // ≠
    { L"equiv", 0x2261 },    // ≡
    { L"le", 0x2264 },    // ≤
    { L"ge", 0x2265 },    // ≥
    { L"sub", 0x2282 },    // ⊂
    { L"sup", 0x2283 },    // ⊃
    { L"nsub", 0x2284 },    // ⊄
    { L"sube", 0x2286 },    // ⊆
    { L"supe", 0x2287 },    // ⊇
    { L"oplus", 0x2295 },    // ⊕
    { L"otimes", 0x2297 },    // ⊗
    { L"perp", 0x22A5 },    // ⊥
    { L"sdot", 0x22C5 },    // ⋅
    { L"lceil", 0x2308 },    // ⌈
    { L"rceil", 0x2309 },    // ⌉
    { L"lfloor", 0x230A },    // ⌊
    { L"rfloor", 0x230B },    // ⌋
    { L"lang", 0x2329 },    // 〈
    { L"rang", 0x232A },    // 〉
    { L"loz", 0x25CA },    // ◊
    { L"spades", 0x2660 },    // ♠
    { L"clubs", 0x2663 },    // ♣
    { L"hearts", 0x2665 },    // ♥
    { L"diams", 0x2666 },    // ♦
};


MagoEE::dchar_t MapNamedCharacter( const wchar_t* name )
{
    return MapNamedCharacter( name, wcslen( name ) );
}

MagoEE::dchar_t MapNamedCharacter( const wchar_t* name, size_t len )
{
    for ( int i = 0; i < _countof( gChars ); i++ )
    {
        if ( wcsncmp( gChars[i].Name, name, len ) == 0 )
        {
            // they're the same up to our passed in length, 
            // now see if the name ends there or is longer
            if ( gChars[i].Name[len] == L'\0' )
                return gChars[i].Code;
        }
    }

    return 0;
}
