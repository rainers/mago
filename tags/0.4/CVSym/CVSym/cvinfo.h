/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.

   Purpose: structures and constants for accessing and interpreting CodeView info
*/

#pragma once


// enumeration for LF_MODIFIER values


typedef struct CV_modifier_t {
    unsigned short MOD_const :1;
    unsigned short MOD_volatile :1;
    unsigned short MOD_unaligned :1;
    unsigned short MOD_unused :13;
} CV_modifier_t;


// type record for LF_POINTER

typedef struct lfPointerAttr {
    unsigned char ptrtype :5; // ordinal specifying pointer type (ptrtype-t)
    unsigned char ptrmode :3; // ordinal specifying pointer mode (ptrmode_t)
    unsigned char isflat32 :1; // true if 0:32 pointer
    unsigned char isvolatile :1; // TRUE if volatile pointer
    unsigned char isconst :1; // TRUE if const pointer
    unsigned char isunaligned :1; // TRUE if unaligned pointer
    unsigned char unused :4;
} lfPointerAttr;

        
// enumeration for method properties

typedef enum CV_methodprop_e {
    CV_MTvanilla = 0x00,
    CV_MTvirtual = 0x01,
    CV_MTstatic = 0x02,
    CV_MTfriend = 0x03,
    CV_MTintro = 0x04,
    CV_MTpurevirt = 0x05,
    CV_MTpureintro = 0x06
} CV_methodprop_e;


// class field attribute

typedef struct CV_fldattr_t {
    unsigned short access :2; // access protection CV_access_t
    unsigned short mprop :3; // method properties CV_methodprop_t
    unsigned short pseudo :1; // compiler generated fcn and does not exist
    unsigned short noinherit :1; // true if class cannot be inherited
    unsigned short noconstruct :1; // true if class cannot be constructed
    unsigned short unused :8; // unused
} CV_fldattr_t;



/** Primitive types have predefined meaning that is encoded in the
* values of the various bit fields in the value.
*
* A CodeView primitive type is defined as:
*
* 1 1
* 1 089 7654 3 210
* r mode type r sub
*
* Where
* mode is the pointer mode
* type is a type indicator
* sub is a subtype enumeration
* r is a reserved field
*
* See Microsoft Symbol and Type OMF (Version 4.0) for more
* information.
*/


#define CV_MMASK 0x700 // mode mask
#define CV_TMASK 0x0f0 // type mask

#if CC_BIGINT
// can we use the reserved bit ??
#define CV_SMASK 0x00f // subtype mask
#else
#define CV_SMASK 0x007 // subtype mask
#endif

#define CV_MSHIFT 8 // primitive mode right shift count
#define CV_TSHIFT 4 // primitive type right shift count
#define CV_SSHIFT 0 // primitive subtype right shift count

// macros to extract primitive mode, type and size

#define CV_MODE(typ) (((typ) & CV_MMASK) >> CV_MSHIFT)
#define CV_TYPE(typ) (((typ) & CV_TMASK) >> CV_TSHIFT)
#define CV_SUBT(typ) (((typ) & CV_SMASK) >> CV_SSHIFT)

// macros to insert new primitive mode, type and size

#define CV_NEWMODE(typ, nm) ((CV_typ_t)(((typ) & ~CV_MMASK) | ((nm) << CV_MSHIFT)))
#define CV_NEWTYPE(typ, nt) (((typ) & ~CV_TMASK) | ((nt) << CV_TSHIFT))
#define CV_NEWSUBT(typ, ns) (((typ) & ~CV_SMASK) | ((ns) << CV_SSHIFT))



// pointer mode enumeration values

typedef enum CV_prmode_e {
    CV_TM_DIRECT = 0, // mode is not a pointer
    CV_TM_NPTR = 1, // mode is a near pointer
    CV_TM_FPTR = 2, // mode is a far pointer
    CV_TM_HPTR = 3, // mode is a huge pointer
    CV_TM_NPTR32 = 4, // mode is a 32 bit near pointer
    CV_TM_FPTR32 = 5, // mode is a 32 bit far pointer
    CV_TM_NPTR64 = 6 // mode is a 64 bit near pointer
#if CC_BIGINT
    ,
    CV_TM_NPTR128 = 7 // mode is a 128 bit near pointer
#endif
} CV_prmode_e;




// type enumeration values


typedef enum CV_type_e {
    CV_SPECIAL = 0x00, // special type size values
    CV_SIGNED = 0x01, // signed integral size values
    CV_UNSIGNED = 0x02, // unsigned integral size values
    CV_BOOLEAN = 0x03, // Boolean size values
    CV_REAL = 0x04, // real number size values
    CV_COMPLEX = 0x05, // complex number size values
    CV_SPECIAL2 = 0x06, // second set of special types
    CV_INT = 0x07, // integral (int) values
    CV_CVRESERVED = 0x0f
} CV_type_e;




// subtype enumeration values for CV_SPECIAL


typedef enum CV_special_e {
    CV_SP_NOTYPE = 0x00,
    CV_SP_ABS = 0x01,
    CV_SP_SEGMENT = 0x02,
    CV_SP_VOID = 0x03,
    CV_SP_CURRENCY = 0x04,
    CV_SP_NBASICSTR = 0x05,
    CV_SP_FBASICSTR = 0x06,
    CV_SP_NOTTRANS = 0x07
} CV_special_e;




// subtype enumeration values for CV_SPECIAL2


typedef enum CV_special2_e {
    CV_S2_BIT = 0x00,
    CV_S2_PASCHAR = 0x01 // Pascal CHAR
} CV_special2_e;





// subtype enumeration values for CV_SIGNED, CV_UNSIGNED and CV_BOOLEAN


typedef enum CV_integral_e {
    CV_IN_1BYTE = 0x00,
    CV_IN_2BYTE = 0x01,
    CV_IN_4BYTE = 0x02,
    CV_IN_8BYTE = 0x03
#if CC_BIGINT
    ,
    CV_IN_16BYTE = 0x04
#endif
} CV_integral_e;





// subtype enumeration values for CV_REAL and CV_COMPLEX


typedef enum CV_real_e {
    CV_RC_REAL32 = 0x00,
    CV_RC_REAL64 = 0x01,
    CV_RC_REAL80 = 0x02,
    CV_RC_REAL128 = 0x03,
    CV_RC_REAL48 = 0x04
} CV_real_e;




// subtype enumeration values for CV_INT (really int)


typedef enum CV_int_e {
    CV_RI_CHAR = 0x00,
    CV_RI_INT1 = 0x00,
    CV_RI_WCHAR = 0x01,
    CV_RI_UINT1 = 0x01,
    CV_RI_INT2 = 0x02,
    CV_RI_UINT2 = 0x03,
    CV_RI_INT4 = 0x04,
    CV_RI_UINT4 = 0x05,
    CV_RI_INT8 = 0x06,
    CV_RI_UINT8 = 0x07
#if CC_BIGINT
    ,
    CV_RI_INT16 = 0x08,
    CV_RI_UINT16 = 0x09
#endif
} CV_int_e;




// macros to check the type of a primitive

#define CV_TYP_IS_DIRECT(typ) (CV_MODE(typ) == CV_TM_DIRECT)
#define CV_TYP_IS_PTR(typ) (CV_MODE(typ) != CV_TM_DIRECT)
#define CV_TYP_IS_NPTR(typ) (CV_MODE(typ) == CV_TM_NPTR)
#define CV_TYP_IS_FPTR(typ) (CV_MODE(typ) == CV_TM_FPTR)
#define CV_TYP_IS_HPTR(typ) (CV_MODE(typ) == CV_TM_HPTR)
#define CV_TYP_IS_NPTR32(typ) (CV_MODE(typ) == CV_TM_NPTR32)
#define CV_TYP_IS_FPTR32(typ) (CV_MODE(typ) == CV_TM_FPTR32)

#if CC_BIGINT
#define CV_TYP_IS_SIGNED(typ) (((CV_TYPE(typ) == CV_SIGNED) && CV_TYP_IS_DIRECT(typ)) || \
    (typ == T_INT1) || \
    (typ == T_INT2) || \
    (typ == T_INT4) || \
    (typ == T_INT8) || \
    (typ == T_INT16) || \
    (typ == T_RCHAR))
#define CV_TYP_IS_UNSIGNED(typ) (((CV_TYPE(typ) == CV_UNSIGNED) && CV_TYP_IS_DIRECT(typ)) || \
    (typ == T_UINT1) || \
    (typ == T_UINT2) || \
    (typ == T_UINT4) || \
    (typ == T_UINT8) || \
    (typ == T_UINT16))
#else
#define CV_TYP_IS_SIGNED(typ) (((CV_TYPE(typ) == CV_SIGNED) && CV_TYP_IS_DIRECT(typ)) || \
    (typ == T_INT1) || \
    (typ == T_INT2) || \
    (typ == T_INT4) || \
    (typ == T_RCHAR))

#define CV_TYP_IS_UNSIGNED(typ) (((CV_TYPE(typ) == CV_UNSIGNED) && CV_TYP_IS_DIRECT(typ)) || \
    (typ == T_UINT1) || \
    (typ == T_UINT2) || \
    (typ == T_UINT4))
#endif
#define CV_TYP_IS_REAL(typ) ((CV_TYPE(typ) == CV_REAL) && CV_TYP_IS_DIRECT(typ))

#define CV_FIRST_NONPRIM 0x1000
#define CV_IS_PRIMITIVE(typ) ((typ) < CV_FIRST_NONPRIM)
#define CV_TYP_IS_COMPLEX(typ) ((CV_TYPE(typ) == CV_COMPLEX) && CV_TYP_IS_DIRECT(typ))
