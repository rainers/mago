/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Util.h"


// from cvinfo.h:

// variable length numeric field

typedef struct lfVarString {
    unsigned short leaf; // LF_VARSTRING
    unsigned short len; // length of value in bytes
    unsigned char value[1]; // value
} lfVarString;


uint32_t GetUIntValue( uint16_t* numericLeaf )
{
    if ( *numericLeaf < LF_NUMERIC )
        return *numericLeaf;

    switch ( *numericLeaf )
    {
    case LF_CHAR:   return *(char*) (numericLeaf + 1);

    case LF_SHORT:
    case LF_USHORT: return *(uint16_t*) (numericLeaf + 1);

    case LF_LONG:
    case LF_ULONG:  return *(uint32_t*) (numericLeaf + 1);
    }

    return 0;
}

uint16_t GetNumLeafSize( uint16_t* numLeaf )
{
    uint16_t    offset = 2;

    if ( *numLeaf < LF_NUMERIC )
        return offset;

    switch ( *numLeaf )
    {
    case LF_CHAR:       offset += 1;    break;

    case LF_SHORT:
    case LF_USHORT:     offset += 2;    break;

    case LF_LONG:
    case LF_ULONG:
    case LF_REAL32:     offset += 4;    break;

    case LF_REAL48:     offset += 6;    break;

    case LF_QUADWORD:
    case LF_UQUADWORD:
    case LF_REAL64:
    case LF_COMPLEX32:  offset += 8;    break;

    case LF_REAL80:     offset += 10;   break;

    case LF_REAL128:
    case LF_COMPLEX64:  offset += 16;   break;

    case LF_COMPLEX80:  offset += 20;   break;

    case LF_COMPLEX128: offset += 32;   break;

    case LF_VARSTRING:
        offset += 2 + ((lfVarString*) (numLeaf + 1))->len;  // +2 for the length field
        break;
    }

    return offset;
}


void GetNumLeafValue( uint16_t* numLeaf, MagoST::Variant& val )
{
    if ( *numLeaf == LF_VARSTRING )
    {
        lfVarString*    strLeaf = (lfVarString*) numLeaf;

        val.Data.VarStr.Length = strLeaf->len;
        val.Data.VarStr.Chars = (char*) strLeaf->value;
    }
    else if ( *numLeaf < LF_NUMERIC )
    {
        val.Data.I16 = *numLeaf;
        val.Tag = MagoST::VarTag_Short;
    }
    else
    {
        // includes leaf tag, so take it out
        uint32_t    len = GetNumLeafSize( numLeaf ) - 2;

        memcpy( &val.Data, numLeaf + 1, len );
        val.Tag = (MagoST::VariantTag) *numLeaf;
    }
}


DWORD GetFieldLength( CodeViewFieldType* type )
{
    _ASSERT( type != NULL );
    DWORD   len = 2;            // for the tag

    switch ( type->Generic.id )
    {
    case LF_BCLASS:
        len += 4 + GetNumLeafSize( &type->bclass.offset );
        break;

    case LF_VBCLASS:
    case LF_IVBCLASS:
        len += 6;
        len += GetNumLeafSize( &type->vbclass.vbpoff );
        len += GetNumLeafSize( (uint16_t*) ((BYTE*) type + len) );
        break;

    case LF_ENUMERATE:
        len += 2;
        len += GetNumLeafSize( &type->enumerate.value );
        len += 1 + *((BYTE*) type + len);
        break;

    case LF_FRIENDFCN:
        len += 2;
        len += 1 + *((BYTE*) type + len);
        break;

    case LF_INDEX:
        _ASSERT( false );
        break;

    case LF_MEMBER:
        len += 4;
        len += GetNumLeafSize( &type->member.offset );
        len += 1 + *((BYTE*) type + len);
        break;

    case LF_STMEMBER:
        len += 4;
        len += 1 + *((BYTE*) type + len);
        break;

    case LF_METHOD:
        len += 4;
        len += 1 + *((BYTE*) type + len);
        break;

    case LF_NESTTYPE:
        len += 2;
        len += 1 + *((BYTE*) type + len);
        break;

    case LF_VFUNCTAB:
        len += 2;
        break;

    case LF_FRIENDCLS:
        len += 2;
        break;

    case LF_ONEMETHOD:
        len += 8;
        len += 1 + *((BYTE*) type + len);
        break;

    case LF_VFUNCOFF:
        len += 6;
        break;
    }

    return len;
}


bool     QuickGetAddrOffset( CodeViewSymbol* sym, uint32_t& offset )
{
    _ASSERT( sym != NULL );

    switch ( sym->Generic.id )
    {
    case S_LDATA32:
    case S_GDATA32:
    case S_PUB32:
    case S_LTHREAD32:
    case S_GTHREAD32:
        offset = sym->data.offset;
        break;

    case S_LPROC32:
    case S_GPROC32:
        offset = sym->proc.offset;
        break;

    case S_THUNK32:
        offset = sym->thunk.offset;
        break;

    case S_BLOCK32:
        offset = sym->block.offset;
        break;

    case S_LABEL32:
        offset = sym->label.offset;
        break;

    default:
        return false;
    }

    return true;
}

bool     QuickGetAddrSegment( CodeViewSymbol* sym, uint16_t& segment )
{
    _ASSERT( sym != NULL );

    switch ( sym->Generic.id )
    {
    case S_LDATA32:
    case S_GDATA32:
    case S_PUB32:
    case S_LTHREAD32:
    case S_GTHREAD32:
        segment = sym->data.segment;
        break;

    case S_LPROC32:
    case S_GPROC32:
        segment = sym->proc.segment;
        break;

    case S_THUNK32:
        segment = sym->thunk.segment;
        break;

    case S_BLOCK32:
        segment = sym->block.segment;
        break;

    case S_LABEL32:
        segment = sym->label.segment;
        break;

    default:
        return false;
    }

    return true;
}


bool     QuickGetName( CodeViewSymbol* sym, SymString& name )
{
    _ASSERT( sym != NULL );
    DWORD   offset = 0;

    switch ( sym->Generic.id )
    {
    case S_REGISTER:
        assign( name, &sym->reg.p_name );
        break;

    case S_CONSTANT:
        offset = GetNumLeafSize( &sym->constant.value );
        assign( name, (PasString*) ((BYTE*) &sym->constant.value + offset) );
        break;

    case S_UDT:
        assign( name, &sym->udt.p_name );
        break;

    case S_OBJNAME:
        assign( name, &sym->objname.p_name );
        break;

    //case S_COBOLUDT:

    case S_MANYREG:
        assign( name, (PasString*) &sym->manyreg.reg[ sym->manyreg.count ] );
        break;

    case S_BPREL32:
        assign( name, &sym->bprel.p_name );
        break;

    case S_LDATA32:
    case S_GDATA32:
    case S_PUB32:
    case S_LTHREAD32:
    case S_GTHREAD32:
        assign( name, &sym->data.p_name );
        break;

    case S_LPROC32:
    case S_GPROC32:
        assign( name, &sym->proc.p_name );
        break;

    case S_THUNK32:
        assign( name, &sym->thunk.p_name );
        break;

    case S_BLOCK32:
        assign( name, &sym->block.p_name );
        break;

    case S_LABEL32:
        assign( name, &sym->label.p_name );
        break;

    case S_REGREL32:
        assign( name, &sym->regrel.p_name );
        break;

    default:
        return false;
    }

    return true;
}


bool     QuickGetLength( CodeViewSymbol* sym, uint32_t& length )
{
    _ASSERT( sym != NULL );

    switch ( sym->Generic.id )
    {
    case S_LPROC32:
    case S_GPROC32:
        length = sym->proc.length;
        break;

    case S_THUNK32:
        length = sym->thunk.length;
        break;

    case S_BLOCK32:
        length = sym->block.length;
        break;

    default:
        return false;
    }

    return true;
}
