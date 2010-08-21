/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.

   Purpose: structures and constants for accessing and interpreting CodeView info
            Based on cvinfo_old.h (the original cvinfo.h)
            Split up for clarity. cvinfo.h has all other definitions.
*/

#pragma once

#include <pshpack1.h>


typedef unsigned long CV_uoff32_t;
typedef long CV_off32_t;
typedef unsigned short CV_uoff16_t;
typedef short CV_off16_t;
typedef unsigned short CV_typ_t;


/** No leaf index can have a value of 0x0000. The leaf indices are
* separated into ranges depending on the use of the type record.
* The second range is for the type records that are directly referenced
* in symbols. The first range is for type records that are not
* referenced by symbols but instead are referenced by other type
* records. All type records must have a starting leaf index in these
* first two ranges. The third range of leaf indices are used to build
* up complex lists such as the field list of a class type record. No
* type record can begin with one of the leaf indices. The fourth ranges
* of type indices are used to represent numeric data in a symbol or
* type record. These leaf indices are greater than 0x8000. At the
* point that type or symbol processor is expecting a numeric field, the
* next two bytes in the type record are examined. If the value isless
* than 0x8000, then the two bytes contain the numeric value. If the
* value is greater than 0x8000, then the data follows the leaf index in
* a format specified by the leaf index. The final range of leaf indices
* are used to force alignment of subfields within a complex type record..
*/



// leaf indices starting records but referenced from symbol records

#define LF_MODIFIER 0x0001
#define LF_POINTER 0x0002
#define LF_ARRAY 0x0003
#define LF_CLASS 0x0004
#define LF_STRUCTURE 0x0005
#define LF_UNION 0x0006
#define LF_ENUM 0x0007
#define LF_PROCEDURE 0x0008
#define LF_MFUNCTION 0x0009
#define LF_VTSHAPE 0x000a
#define LF_COBOL0 0x000b
#define LF_COBOL1 0x000c
#define LF_BARRAY 0x000d
#define LF_LABEL 0x000e
#define LF_NULL 0x000f
#define LF_NOTTRAN 0x0010
#define LF_DIMARRAY 0x0011
#define LF_VFTPATH 0x0012
#define LF_PRECOMP 0x0013 // not referenced from symbol
#define LF_ENDPRECOMP 0x0014 // not referenced from symbol
#define LF_OEM 0x0015 // oem definable type string
#define LF_TYPESERVER 0x0016 // not referenced from symbol

// leaf indices starting records but referenced only from type records

#define LF_SKIP 0x0200
#define LF_ARGLIST 0x0201
#define LF_DEFARG 0x0202
#define LF_LIST 0x0203
#define LF_FIELDLIST 0x0204
#define LF_DERIVED 0x0205
#define LF_BITFIELD 0x0206
#define LF_METHODLIST 0x0207
#define LF_DIMCONU 0x0208
#define LF_DIMCONLU 0x0209
#define LF_DIMVARU 0x020a
#define LF_DIMVARLU 0x020b
#define LF_REFSYM 0x020c

#define LF_BCLASS 0x0400
#define LF_VBCLASS 0x0401
#define LF_IVBCLASS 0x0402
#define LF_ENUMERATE 0x0403
#define LF_FRIENDFCN 0x0404
#define LF_INDEX 0x0405
#define LF_MEMBER 0x0406
#define LF_STMEMBER 0x0407
#define LF_METHOD 0x0408
#define LF_NESTTYPE 0x0409
#define LF_VFUNCTAB 0x040a
#define LF_FRIENDCLS 0x040b
#define LF_ONEMETHOD 0x040c
#define LF_VFUNCOFF 0x040d

#define LF_NUMERIC 0x8000
#define LF_CHAR 0x8000
#define LF_SHORT 0x8001
#define LF_USHORT 0x8002
#define LF_LONG 0x8003
#define LF_ULONG 0x8004
#define LF_REAL32 0x8005
#define LF_REAL64 0x8006
#define LF_REAL80 0x8007
#define LF_REAL128 0x8008
#define LF_QUADWORD 0x8009
#define LF_UQUADWORD 0x800a
#define LF_REAL48 0x800b
#define LF_COMPLEX32 0x800c
#define LF_COMPLEX64 0x800d
#define LF_COMPLEX80 0x800e
#define LF_COMPLEX128 0x800f
#define LF_VARSTRING 0x8010

#if CC_BIGINT
#define LF_OCTWORD 0x8017
#define LF_UOCTWORD 0x8018
#endif

#define LF_PAD0 0xf0
#define LF_PAD1 0xf1
#define LF_PAD2 0xf2
#define LF_PAD3 0xf3
#define LF_PAD4 0xf4
#define LF_PAD5 0xf5
#define LF_PAD6 0xf6
#define LF_PAD7 0xf7
#define LF_PAD8 0xf8
#define LF_PAD9 0xf9
#define LF_PAD10 0xfa
#define LF_PAD11 0xfb
#define LF_PAD12 0xfc
#define LF_PAD13 0xfd
#define LF_PAD14 0xfe
#define LF_PAD15 0xff

// internal for this project
#define LF_MAGO_METHODOVERLOAD  0xf001


//----------------------------------------------------------------------------
//  Type records - top level, referenced from symbols or other types
//----------------------------------------------------------------------------

union CodeViewType
{
    struct
    {
        unsigned short  len;
        unsigned short  id;
    } Generic;

    // lfModifier
    struct
    {
        unsigned short  len;
        unsigned short  id;     // LF_MODIFIER
        unsigned short  attr;
        //CV_modifier_t   attr; // modifier attribute modifier_t
        CV_typ_t        type;   // modified type
    } modifier;

    struct
    {
        unsigned short  len;
        unsigned short  id;     // LF_POINTER
        unsigned short  attribute;
        unsigned short  type;   // type index of the underlying type
        // variant part, depending on pointer type
    } pointer;

    // lfArray
    struct
    {
        unsigned short  len;
        unsigned short  id;         // LF_ARRAY
        CV_typ_t        elemtype;   // type index of element type
        CV_typ_t        idxtype;    // type index of indexing type
        unsigned short  arraylen;   // numeric leaf with array length
        // PasString        p_name
    } array;

    struct
    {
        unsigned short  len;
        unsigned short  id;         // LF_CLASS, LF_STRUCT
        unsigned short  count;      // count of number of elements in class
        CV_typ_t        fieldlist;  // type index of LF_FIELD descriptor list
        unsigned short  property;
        //CV_prop_t       property; // property attribute field (prop_t)
        CV_typ_t        derived;    // type index of derived from list if not zero
        CV_typ_t        vshape;     // type index of vshape table for this class
        unsigned short  structlen;  // numeric leaf with length of struct
        // PasString        p_name;
    } _struct;

    struct
    {
        unsigned short  len;
        unsigned short  id;         // LF_UNION
        unsigned short  count;      // count of number of elements in class
        CV_typ_t        fieldlist;  // type index of LF_FIELD descriptor list
        unsigned short  property;
        //CV_prop_t       property; // property attribute field
        unsigned short  unionlen;   // numeric leaf with length of union
        // PasString        p_name
    } _union;

    struct
    {
        unsigned short  len;
        unsigned short  id;         // LF_ENUM
        unsigned short  count;      // count of number of elements in class
        CV_typ_t        type;       // underlying type of the enum
        CV_typ_t        fieldlist;  // type index of LF_FIELD descriptor list
        unsigned short  property;
        //CV_prop_t       property; // property attribute field
        PasString       p_name;     // length prefixed name of enum
    } _enum;

    struct
    {
        unsigned short  len;
        unsigned short  id;         // LF_PROCEDURE
        CV_typ_t        rvtype;     // type index of return value
        unsigned char   callconv;   // calling convention (CV_call_t)
        unsigned char   reserved;   // reserved for future use
        unsigned short  paramcount; // number of parameters
        CV_typ_t        arglist;    // type index of argument list
    } procedure;

    struct
    {
        unsigned short  len;
        unsigned short  id;         // LF_MFUNCTION
        CV_typ_t        rvtype;     // type index of return value
        CV_typ_t        class_type; // type index of containing class
        CV_typ_t        this_type;  // type index of this pointer (model specific)
        unsigned char   callconv;   // calling convention (call_t)
        unsigned char   reserved;   // reserved for future use
        unsigned short  paramcount; // number of parameters
        CV_typ_t        arglist;    // type index of argument list
        long            this_adjust; // this adjuster (long because pad required anyway)
    } mfunction;

    // type record for virtual function table shape

    struct
    {
        unsigned short  len;
        unsigned short  id;     // LF_VTSHAPE
        unsigned short  count;  // number of entries in vfunctable
        // 4 bit (CV_VTS_desc) descriptors
    } vtshape;

    // type record describing path to virtual function table

    struct
    {
        unsigned short  len;
        unsigned short  id;         // LF_VFTPATH
        unsigned short  count;      // count of number of bases in path
        CV_typ_t        base[1];    // bases from root to leaf
    } vftpath;

    // type record for OEM definable type strings

    struct
    {
        unsigned short  len;
        unsigned short  id;         // LF_OEM
        unsigned short  oemId;      // MS assigned OEM identified
        unsigned short  oemSymId;   // OEM assigned type identifier
        unsigned short  count;      // count of type indices to follow
        CV_typ_t        index[1];   // array of type indices followed
        // by OEM defined data
    } oem;
};


//----------------------------------------------------------------------------
//  Type records - used to build complex lists
//      description of type records that can be referenced from
//      type records referenced by symbols
//----------------------------------------------------------------------------

union CodeViewRefType
{
    struct
    {
        unsigned short  len;
        unsigned short  id;
    } Generic;

    struct
    {
        unsigned short  len;
        unsigned short  id;         // LF_ARGLIST
        unsigned short  count;      // number of arguments
        CV_typ_t        args[1];    // number of arguments
    } arglist;

    struct
    {
        unsigned short  len;
        unsigned short  id;         // LF_DERIVED
        unsigned short  count;      // number of arguments
        CV_typ_t        drvdcls[1]; // type indices of derived classes
    } derived;

    // field list leaf
    // This is the header leaf for a complex list of class and structure
    // subfields.

    struct
    {
        unsigned short  len;
        unsigned short  id;         // LF_FIELDLIST
        char            data[1];    // field list sub lists
    } fieldlist;

    struct
    {
        unsigned short  len;
        unsigned short  id;
        unsigned char   mlist[1];   // really a mlMethod type
    } methodlist;

    // type record for LF_BITFIELD

    struct
    {
        unsigned short  len;
        unsigned short  id;         // LF_BITFIELD
        unsigned char   length;
        unsigned char   position;   // starting position (from bit 0) of object
        CV_typ_t        type;       // type of bitfield
    } bitfield;
};


//----------------------------------------------------------------------------
//  Type records - used for fields of complex lists
//----------------------------------------------------------------------------

union CodeViewFieldType
{
    struct
    {
        unsigned short  id;
    } Generic;

    // index leaf - contains type index of another leaf
    // a major use of this leaf is to allow the compilers to emit a
    // long complex list (LF_FIELD) in smaller pieces.

    struct
    {
        unsigned short  id;     // LF_INDEX
        CV_typ_t        type;   // type index of referenced leaf
    } index;

    // subfield record for base class field

    struct
    {
        unsigned short  id;     // LF_BCLASS
        CV_typ_t        type;   // type index of base class
        unsigned short  attr;
        //CV_fldattr_t    attr; // attribute
        unsigned short  offset; // numeric leaf with offset of base within class
    } bclass;

    // subfield record for direct and indirect virtual base class field

    struct
    {
        unsigned short  id;     // LF_VBCLASS | LV_IVBCLASS
        CV_typ_t        vbtype; // type index of direct virtual base class
        CV_typ_t        vbptr;  // type index of virtual base pointer
        unsigned short  attr;
        //CV_fldattr_t    attr; // attribute
        unsigned short  vbpoff; // numeric leaf with virtual base pointer offset from address point
        // followed by virtual base offset from vbtable
        // unsigned short vboff    // numeric leaf with virtual base offset from vbtable
    } vbclass;

    // subfield record for friend class

    struct
    {
        unsigned short  id;     // LF_FRIENDCLS
        CV_typ_t        type;   // index to type record of friend class
    } friendcls;

    // subfield record for friend function

    struct
    {
        unsigned short  id;     // LF_FRIENDFCN
        CV_typ_t        type;   // index to type record of friend function
        PasString       p_name; // name of friend function
    } friendfcn;

    // subfield record for non-static data members

    struct
    {
        unsigned short  id;     // LF_MEMBER
        CV_typ_t        type;   // index of type record for field
        unsigned short  attr;
        //CV_fldattr_t    attr; // attribute mask
        unsigned short  offset; // numeric leaf with offset of field
        // PasString        p_name
    } member;

    // type record for static data members

    struct
    {
        unsigned short  id;     // LF_STMEMBER
        CV_typ_t        type;   // index of type record for field
        unsigned short  attr;
        //CV_fldattr_t    attr; // attribute mask
        PasString       p_name; // length prefixed name of field
    } stmember;

    // subfield record for virtual function table pointer

    struct
    {
        unsigned short  id;     // LF_VFUNCTAB
        CV_typ_t        type;   // type index of pointer
    } vfunctab;

    // subfield record for virtual function table pointer with offset

    struct
    {
        unsigned short  id;     // LF_VFUNCTAB
        CV_typ_t        type;   // type index of pointer
        CV_off32_t      offset; // offset of virtual function table pointer
    } vfuncoff;

    // subfield record for overloaded method list

    struct
    {
        unsigned short  id;     // LF_METHOD
        unsigned short  count;  // number of occurances of function
        CV_typ_t        mlist;  // index to LF_METHODLIST record
        PasString       p_name; // length prefixed name of method
    } method;

    // subfield record for nonoverloaded method

    struct
    {
        unsigned short  id;     // LF_ONEMETHOD
        unsigned short  attr;
        //CV_fldattr_t    attr; // method attribute
        CV_typ_t        type;   // index to type record for procedure
        PasString       p_name;
    } onemethod;

    struct
    {
        unsigned short  id;         // LF_ONEMETHOD
        unsigned short  attr;
        //CV_fldattr_t    attr;     // method attribute
        CV_typ_t        type;       // index to type record for procedure
        unsigned long   vtaboff;    // offset in vfunctable if
                                    // intro virtual followed by
                                    // length prefixed name of method
        PasString       p_name;
    } onemethod_virt;

    // subfield record for enumerate

    struct
    {
        unsigned short  id;     // LF_ENUMERATE
        unsigned short  attr;
        //CV_fldattr_t    attr; // access
        unsigned short  value;  // numeric leaf with value
        // PasString        p_name
    } enumerate;

    // type record for nested (scoped) type definition

    struct
    {
        unsigned short  id;     // LF_NESTTYPE
        CV_typ_t        type;   // index of nested type definition
        PasString       p_name; // length prefixed type name
    } nesttype;

    // type record for pad leaf

    struct
    {
        unsigned char leaf;
    } SYM_PAD;
};



//----------------------------------------------------------------------------
// Symbol definitions
//----------------------------------------------------------------------------

typedef enum SYM_ENUM_e {
    S_COMPILE = 0x0001, // Compile flags symbol
    S_REGISTER = 0x0002, // Register variable
    S_CONSTANT = 0x0003, // constant symbol
    S_UDT = 0x0004, // User defined type
    S_SSEARCH = 0x0005, // Start Search
    S_END = 0x0006, // Block, procedure, "with" or thunk end
    S_SKIP = 0x0007, // Reserve symbol space in $$Symbols table
    S_CVRESERVE = 0x0008, // Reserved symbol for CV internal use
    S_OBJNAME = 0x0009, // path to object file name
    S_ENDARG = 0x000a, // end of argument/return list
    S_COBOLUDT = 0x000b, // special UDT for cobol that does not symbol pack
    S_MANYREG = 0x000c, // multiple register variable
    S_RETURN = 0x000d, // return description symbol
    S_ENTRYTHIS = 0x000e, // description of this pointer on entry

    S_BPREL16 = 0x0100, // BP-relative
    S_LDATA16 = 0x0101, // Module-local symbol
    S_GDATA16 = 0x0102, // Global data symbol
    S_PUB16 = 0x0103, // a public symbol
    S_LPROC16 = 0x0104, // Local procedure start
    S_GPROC16 = 0x0105, // Global procedure start
    S_THUNK16 = 0x0106, // Thunk Start
    S_BLOCK16 = 0x0107, // block start
    S_WITH16 = 0x0108, // with start
    S_LABEL16 = 0x0109, // code label
    S_CEXMODEL16 = 0x010a, // change execution model
    S_VFTABLE16 = 0x010b, // address of virtual function table
    S_REGREL16 = 0x010c, // register relative address

    S_BPREL32 = 0x0200, // BP-relative
    S_LDATA32 = 0x0201, // Module-local symbol
    S_GDATA32 = 0x0202, // Global data symbol
    S_PUB32 = 0x0203, // a public symbol (CV internal reserved)
    S_LPROC32 = 0x0204, // Local procedure start
    S_GPROC32 = 0x0205, // Global procedure start
    S_THUNK32 = 0x0206, // Thunk Start
    S_BLOCK32 = 0x0207, // block start
    S_WITH32 = 0x0208, // with start
    S_LABEL32 = 0x0209, // code label
    S_CEXMODEL32 = 0x020a, // change execution model
    S_VFTABLE32 = 0x020b, // address of virtual function table
    S_REGREL32 = 0x020c, // register relative address
    S_LTHREAD32 = 0x020d, // local thread storage
    S_GTHREAD32 = 0x020e, // global thread storage
    S_SLINK32 = 0x020f, // static link for MIPS EH implementation

    S_LPROCMIPS = 0x0300, // Local procedure start
    S_GPROCMIPS = 0x0301, // Global procedure start

    S_PROCREF = 0x0400, // Reference to a procedure
    S_DATAREF = 0x0401, // Reference to data
    S_ALIGN = 0x0402, // Used for page alignment of symbols
    S_LPROCREF = 0x0403 // Local Reference to a procedure
} SYM_ENUM_e;


//----------------------------------------------------------------------------
//  Symbol records
//----------------------------------------------------------------------------

union CodeViewSymbol
{
    struct
    {
        unsigned short  len;    // Record length
        unsigned short  id;     // Record type
    } Generic;

    // non-model specific symbol types

    struct
    {
        unsigned short  len;    // Record length
        unsigned short  id;     // S_REGISTER
        CV_typ_t        type;   // Type index
        unsigned short  reg;    // register enumerate
        PasString       p_name; // Length-prefixed name
        // the spec says that register tracking info can be put at the end of
        // this struct, but the format is still undefined
    } reg;

    struct
    {
        unsigned short  len;    // Record length
        unsigned short  id;     // S_MANYREG
        CV_typ_t        type;   // Type index
        unsigned char   count;  // count of number of registers
        unsigned char   reg[1]; // count register enumerates followed by
        // length-prefixed name. Registers are
        // most significant first.
    } manyreg;

    struct
    {
        unsigned short  len;    // Record length
        unsigned short  id;     // S_CONSTANT
        CV_typ_t        type;   // Type index (containing enum if enumerate)
        unsigned short  value;  // numeric leaf containing value
        // PasString        p_name; // Length-prefixed name
    } constant;

    struct
    {
        unsigned short  len;    // Record length
        unsigned short  id;     // S_UDT | S_COBOLUDT
        CV_typ_t        type;   // Type index
        PasString       p_name; // Length-prefixed name
    } udt;

    struct
    {
        unsigned short  len;        // Record length
        unsigned short  id;         // S_SSEARCH
        unsigned long   startsym;   // offset of the procedure
        unsigned short  segment;    // segment of symbol
    } search;

    struct
    {
        unsigned short  len;        // Record length
        unsigned short  id;         // S_COMPILE
        unsigned char   machine;    // target processor
        struct
        {
            unsigned char language :8; // language index
            unsigned char pcode :1; // true if pcode present
            unsigned char floatprec :2; // floating precision
            unsigned char floatpkg :2; // float package
            unsigned char ambdata :3; // ambiant data model
            unsigned char ambcode :3; // ambiant code model
            unsigned char mode32 :1; // true if compiled 32 bit mode
            unsigned char pad :4; // reserved
        } flags;
        PasString       p_version;  // Length-prefixed compiler version string
    } compile;

    struct
    {
        unsigned short  len;        // Record length
        unsigned short  id;         // S_OBJNAME
        unsigned long   signature;  // signature
        PasString       p_name;     // Length-prefixed name
    } objname;

    // symbol types for 16:32 memory model

    struct
    {
        unsigned short  len;    // Record length
        unsigned short  id;     // S_BPREL32
        CV_off32_t      offset; // BP-relative offset
        CV_typ_t        type;   // Type index
        PasString       p_name; // Length-prefixed name
    } bprel;

    // also includes S_LTHREAD32 and S_GTHREAD32

    struct
    {
        unsigned short  len;        // Record length
        unsigned short  id;         // S_LDATA32, S_GDATA32 or S_PUB32
        CV_uoff32_t     offset;
        unsigned short  segment;
        CV_typ_t        type;       // Type index
        PasString       p_name;     // Length-prefixed name
    } data;

    struct
    {
        unsigned short  len;            // Record length
        unsigned short  id;             // S_GPROC32 or S_LPROC32
        unsigned long   parent;         // pointer to the parent
        unsigned long   end;            // pointer to this blocks end
        unsigned long   next;           // pointer to next symbol
        unsigned long   length;         // Proc length
        unsigned long   debug_start;    // Debug start offset
        unsigned long   debug_end;      // Debug end offset
        CV_uoff32_t     offset;
        unsigned short  segment;
        CV_typ_t        type;           // Type index
        //CV_PROCFLAGS flags;       // Proc flags
        unsigned char   flags;
        PasString       p_name;         // Length-prefixed name
    } proc;

    struct
    {
        unsigned short  len;        // Record length
        unsigned short  id;         // S_THUNK32
        unsigned long   parent;     // pointer to the parent
        unsigned long   end;        // pointer to this blocks end
        unsigned long   next;       // pointer to next symbol
        CV_uoff32_t     offset;
        unsigned short  segment;
        unsigned short  length;     // length of thunk
        unsigned char   ord;        // ordinal specifying type of thunk
        PasString       p_name;     // Length-prefixed name
        //unsigned char variant[CV_ZEROLEN]; // variant portion of thunk
    } thunk;

    struct
    {
        unsigned short  len;        // Record length
        unsigned short  id;         // S_LABEL32
        CV_uoff32_t     offset;
        unsigned short  segment;
        //CV_PROCFLAGS    flags;    // flags
        unsigned char   flags;
        PasString       p_name;     // Length-prefixed name
    } label;

    struct
    {
        unsigned short  len;        // Record length
        unsigned short  id;         // S_BLOCK32
        unsigned long   parent;     // pointer to the parent
        unsigned long   end;        // pointer to this blocks end
        unsigned long   length;     // Block length
        CV_uoff32_t     offset;     // Offset in code segment
        unsigned short  segment;    // segment of label
        PasString       p_name;     // Length-prefixed name
    } block;

    struct
    {
        unsigned short  len;        // Record length
        unsigned short  id;         // S_WITH32
        unsigned long   parent;     // pointer to the parent
        unsigned long   end;        // pointer to this blocks end
        unsigned long   length;     // Block length
        CV_uoff32_t     offset;     // Offset in code segment
        unsigned short  segment;    // segment of label
        PasString       p_expr;     // Length-prefixed expression string
    } with;

    struct
    {
        unsigned short  len;        // record length
        unsigned short  id;         // S_VFTPATH32
        CV_uoff32_t     offset;     // offset of virtual function table
        unsigned short  segment;    // segment of virtual function table
        CV_typ_t        root;       // type index of the root of path
        CV_typ_t        path;       // type index of the path record
    } vpath;

    struct
    {
        unsigned short  len;    // Record length
        unsigned short  id;     // S_REGREL32
        CV_off32_t      offset; // offset of symbol
                                // [was originally CV_uoff32_t]
        unsigned short  reg;    // register index for symbol
        CV_typ_t        type;   // Type index
        PasString       p_name; // Length-prefixed name
    } regrel;

    struct
    {
        unsigned short  len;        // Record length
        unsigned short  id;         // S_PROCREF or S_DATAREF
        unsigned long   sumName;    // SUC of the name
        unsigned long   ibSym;      // Offset of actual symbol in $$Symbols
        unsigned short  imod;       // Module containing the actual symbol
        unsigned short  usFill;     // align this record
    } symref;
};


#include <poppack.h>
