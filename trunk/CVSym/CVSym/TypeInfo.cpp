/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "TypeInfo.h"
#include "cvconst.h"
#include "cvinfo.h"
#include "Util.h"


namespace MagoST
{
    //------------------------------------------------------------------------
    //  TypeSymbol
    //------------------------------------------------------------------------

    TypeSymbol::TypeSymbol( const TypeHandleIn& handle )
        :   SymbolInfo()
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );

        mData.Type.Handle = handle;
    }

    void TypeSymbol::SetMod( uint16_t mod )
    {
        mData.Type.Mod = mod;
    }

    bool TypeSymbol::GetMod( uint16_t& mod )
    {
        mod = mData.Type.Mod;
        return true;
    }


    //------------------------------------------------------------------------
    //  BaseTypeSymbol
    //------------------------------------------------------------------------

    BaseTypeSymbol::BaseTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
        _ASSERT( CV_MODE( handle.Index ) == 0 );
    }

    SymTag BaseTypeSymbol::GetSymTag()
    {
        return SymTagBaseType;
    }

    bool BaseTypeSymbol::GetLength( uint32_t& length )
    {
        DWORD basicType = 0;
        return GetBasicLengthAndType( mData.Type.Handle.Index, basicType, length );
    }

    bool BaseTypeSymbol::GetBasicType( DWORD& basicType )
    {
        uint32_t    length = 0;
        return GetBasicLengthAndType( mData.Type.Handle.Index, basicType, length );
    }

    bool BaseTypeSymbol::GetBasicLengthAndType( TypeIndex index, DWORD& basicType, uint32_t& length )
    {
        uint32_t    type = CV_TYPE( index );
        uint32_t    subType = CV_SUBT( index );

        basicType = 0;

        switch ( type )
        {
        case CV_SPECIAL:
            switch ( subType )
            {
            case CV_SP_VOID:    length = 0; basicType = btVoid; break;
            }
            break;

        case CV_SIGNED:
            switch ( subType )
            {
            case CV_IN_1BYTE:   length = 1; basicType = btInt;  break;
            case CV_IN_2BYTE:   length = 2; basicType = btInt;  break;
            case CV_IN_4BYTE:   length = 4; basicType = btInt;  break;
            case CV_IN_8BYTE:   length = 8; basicType = btInt;  break;
            case CV_IN_16BYTE:  length = 16; basicType = btInt; break;
            }
            break;

        case CV_UNSIGNED:
            switch ( subType )
            {
            case CV_IN_1BYTE:   length = 1; basicType = btUInt;  break;
            case CV_IN_2BYTE:   length = 2; basicType = btUInt;  break;
            case CV_IN_4BYTE:   length = 4; basicType = btUInt;  break;
            case CV_IN_8BYTE:   length = 8; basicType = btUInt;  break;
            case CV_IN_16BYTE:  length = 16; basicType = btUInt; break;
            }
            break;

        case CV_BOOLEAN:
            switch ( subType )
            {
            case CV_IN_1BYTE:   length = 1; basicType = btBool;  break;
            case CV_IN_2BYTE:   length = 2; basicType = btBool;  break;
            case CV_IN_4BYTE:   length = 4; basicType = btBool;  break;
            case CV_IN_8BYTE:   length = 8; basicType = btBool;  break;
            case CV_IN_16BYTE:  length = 16; basicType = btBool; break;
            }
            break;

        case CV_REAL:
            switch ( subType )
            {
            case CV_RC_REAL32:  length = 4; basicType = btFloat;    break;
            case CV_RC_REAL64:  length = 8; basicType = btFloat;    break;
            case CV_RC_REAL80:  length = 10; basicType = btFloat;   break;
            case CV_RC_REAL128: length = 16; basicType = btFloat;   break;
            case CV_RC_REAL48:  length = 6; basicType = btFloat;    break;
            }
            break;

        case CV_COMPLEX:
            switch ( subType )
            {
            case CV_RC_REAL32:  length = 4*2; basicType = btComplex;    break;
            case CV_RC_REAL64:  length = 8*2; basicType = btComplex;    break;
            case CV_RC_REAL80:  length = 10*2; basicType = btComplex;   break;
            case CV_RC_REAL128: length = 16*2; basicType = btComplex;   break;
            case CV_RC_REAL48:  length = 6*2; basicType = btComplex;    break;
            }
            break;

        case CV_SPECIAL2:
            break;

        case CV_INT:
            switch ( subType )
            {
            case CV_RI_CHAR:    length = 1; basicType = btChar;     break;
            case CV_RI_WCHAR:   length = 2; basicType = btWChar;    break;

            case CV_RI_INT2:    length = 2; basicType = btInt;      break;
            case CV_RI_UINT2:   length = 2; basicType = btUInt;     break;
            case CV_RI_INT4:    length = 4; basicType = btInt;      break;
            case CV_RI_UINT4:   length = 4; basicType = btUInt;     break;
            case CV_RI_INT8:    length = 8; basicType = btInt;      break;
            case CV_RI_UINT8:   length = 8; basicType = btUInt;     break;

                // TODO: 8 and 9 are actually reserved for 128-bit ints
                //       we should use another one like 10 instead, but that's up to the compiler
            case 8:             length = 4; basicType = btChar;     break;
            }
            break;

        default:
            break;
        }

        return basicType != 0;
    }


    //------------------------------------------------------------------------
    //  BasePointerTypeSymbol
    //------------------------------------------------------------------------

    BasePointerTypeSymbol::BasePointerTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
        _ASSERT( CV_MODE( handle.Index ) != 0 );
    }

    SymTag BasePointerTypeSymbol::GetSymTag()
    {
        return SymTagPointerType;
    }

    bool BasePointerTypeSymbol::GetType( TypeIndex& index )
    {
        index = mData.Type.Handle.Index & (CV_TMASK | CV_SMASK);
        return true;
    }


    //------------------------------------------------------------------------
    //  PointerTypeSymbol
    //------------------------------------------------------------------------

    PointerTypeSymbol::PointerTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag PointerTypeSymbol::GetSymTag()
    {
        return SymTagPointerType;
    }

    bool PointerTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->pointer.type;
        return true;
    }

    bool PointerTypeSymbol::GetMod( uint16_t& mod )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        lfPointerAttr*  ptrAttr = (lfPointerAttr*) &type->pointer.attribute;
        CV_modifier_t   extraMod = { 0 };

        extraMod.MOD_const = ptrAttr->isconst;
        extraMod.MOD_volatile = ptrAttr->isvolatile;
        extraMod.MOD_unaligned = ptrAttr->isunaligned;

        mod = mData.Type.Mod | *(uint16_t*) &extraMod;
        return true;
    }


    //------------------------------------------------------------------------
    //  ArrayTypeSymbol
    //------------------------------------------------------------------------

    ArrayTypeSymbol::ArrayTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag ArrayTypeSymbol::GetSymTag()
    {
        return SymTagArrayType;
    }

    bool ArrayTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->array.elemtype;
        return true;
    }

    bool ArrayTypeSymbol::GetName( PasString*& name )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;

        uint32_t    offset = GetNumLeafSize( &type->array.arraylen );

        name = (PasString*) ((char*) &type->array.arraylen + offset);
        return true;
    }

    bool ArrayTypeSymbol::GetIndexType( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->array.idxtype;
        return true;
    }

    bool ArrayTypeSymbol::GetCount( uint32_t& count )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        count = GetUIntValue( &type->array.arraylen );
        return true;
    }


    //------------------------------------------------------------------------
    //  StructOrClassTypeSymbol
    //------------------------------------------------------------------------

    StructOrClassTypeSymbol::StructOrClassTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag StructOrClassTypeSymbol::GetSymTag()
    {
        return SymTagUDT;
    }

    bool StructOrClassTypeSymbol::GetName( PasString*& name )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;

        uint32_t    offset = GetNumLeafSize( &type->_struct.structlen );

        name = (PasString*) ((char*) &type->_struct.structlen + offset);
        return true;
    }

    bool StructOrClassTypeSymbol::GetLength( uint32_t& length )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        length = GetUIntValue( &type->_struct.structlen );
        return true;
    }

    bool StructOrClassTypeSymbol::GetFieldCount( uint16_t& count )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        count = type->_struct.count;
        return true;
    }

    bool StructOrClassTypeSymbol::GetFieldList( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->_struct.fieldlist;
        return true;
    }

    bool StructOrClassTypeSymbol::GetProperties( uint16_t& props )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        props = type->_struct.property;
        return true;
    }

    bool StructOrClassTypeSymbol::GetDerivedList( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->_struct.derived;
        return true;
    }

    bool StructOrClassTypeSymbol::GetVShape( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->_struct.vshape;
        return true;
    }


    //------------------------------------------------------------------------
    //  StructTypeSymbol
    //------------------------------------------------------------------------

    StructTypeSymbol::StructTypeSymbol( const TypeHandleIn& handle )
        :   StructOrClassTypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    bool StructTypeSymbol::GetUdtKind( UdtKind& udtKind )
    {
        udtKind = UdtStruct;
        return true;
    }


    //------------------------------------------------------------------------
    //  ClassTypeSymbol
    //------------------------------------------------------------------------

    ClassTypeSymbol::ClassTypeSymbol( const TypeHandleIn& handle )
        :   StructOrClassTypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    bool ClassTypeSymbol::GetUdtKind( UdtKind& udtKind )
    {
        udtKind = UdtClass;
        return true;
    }


    //------------------------------------------------------------------------
    //  UnionTypeSymbol
    //------------------------------------------------------------------------

    UnionTypeSymbol::UnionTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag UnionTypeSymbol::GetSymTag()
    {
        return SymTagUDT;
    }

    bool UnionTypeSymbol::GetName( PasString*& name )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;

        uint32_t    offset = GetNumLeafSize( &type->_union.unionlen );

        name = (PasString*) ((char*) &type->_union.unionlen + offset);
        return true;
    }

    bool UnionTypeSymbol::GetLength( uint32_t& length )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        length = GetUIntValue( (uint16_t*) &type->_union.unionlen );
        return true;
    }

    bool UnionTypeSymbol::GetUdtKind( UdtKind& udtKind )
    {
        udtKind = UdtUnion;
        return true;
    }

    bool UnionTypeSymbol::GetFieldCount( uint16_t& count )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        count = type->_union.count;
        return true;
    }

    bool UnionTypeSymbol::GetFieldList( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->_union.fieldlist;
        return true;
    }

    bool UnionTypeSymbol::GetProperties( uint16_t& props )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        props = type->_union.property;
        return true;
    }


    //------------------------------------------------------------------------
    //  EnumTypeSymbol
    //------------------------------------------------------------------------

    EnumTypeSymbol::EnumTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag EnumTypeSymbol::GetSymTag()
    {
        return SymTagEnum;
    }

    bool EnumTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->_enum.type;
        return true;
    }

    bool EnumTypeSymbol::GetName( PasString*& name )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        name = &type->_enum.p_name;
        return true;
    }

    bool EnumTypeSymbol::GetFieldCount( uint16_t& count )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        count = type->_enum.count;
        return true;
    }

    bool EnumTypeSymbol::GetFieldList( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->_enum.fieldlist;
        return true;
    }

    bool EnumTypeSymbol::GetProperties( uint16_t& props )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        props = type->_enum.property;
        return true;
    }


    //------------------------------------------------------------------------
    //  ProcTypeSymbol
    //------------------------------------------------------------------------

    ProcTypeSymbol::ProcTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag ProcTypeSymbol::GetSymTag()
    {
        return SymTagFunctionType;
    }

    bool ProcTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->procedure.rvtype;
        return true;
    }

    bool ProcTypeSymbol::GetCallConv( uint8_t& callConv )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        callConv = type->procedure.callconv;
        return true;
    }

    bool ProcTypeSymbol::GetParamCount( uint16_t& count )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        count = type->procedure.paramcount;
        return true;
    }

    bool ProcTypeSymbol::GetParamList( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->procedure.arglist;
        return true;
    }


    //------------------------------------------------------------------------
    //  MProcTypeSymbol
    //------------------------------------------------------------------------

    MProcTypeSymbol::MProcTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag MProcTypeSymbol::GetSymTag()
    {
        return SymTagFunctionType;
    }

    bool MProcTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->mfunction.rvtype;
        return true;
    }

    bool MProcTypeSymbol::GetCallConv( uint8_t& callConv )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        callConv = type->mfunction.callconv;
        return true;
    }

    bool MProcTypeSymbol::GetParamCount( uint16_t& count )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        count = type->mfunction.paramcount;
        return true;
    }

    bool MProcTypeSymbol::GetParamList( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->mfunction.arglist;
        return true;
    }

    bool MProcTypeSymbol::GetClass( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->mfunction.class_type;
        return true;
    }

    bool MProcTypeSymbol::GetThis( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        index = type->mfunction.this_type;
        return true;
    }

    bool MProcTypeSymbol::GetThisAdjust( int32_t& adjust )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        adjust = type->mfunction.this_adjust;
        return true;
    }


    //------------------------------------------------------------------------
    //  VTShapeTypeSymbol
    //------------------------------------------------------------------------

    VTShapeTypeSymbol::VTShapeTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag VTShapeTypeSymbol::GetSymTag()
    {
        return SymTagVTableShape;
    }

    bool VTShapeTypeSymbol::GetCount( uint32_t& count )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        count = type->vtshape.count;
        return true;
    }

    bool VTShapeTypeSymbol::GetVTableDescriptor( uint32_t index, uint8_t& desc )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;

        if ( index >= type->vtshape.count )
            return false;

        BYTE*   doubleDescTable = (BYTE*) (&type->vtshape.count + 1);
        desc = doubleDescTable[ index / 2 ];

        if ( (index % 2) == 1 )
            desc >>= 4;

        desc &= 0x0F;
        return true;
    }


    //------------------------------------------------------------------------
    //  OEMTypeSymbol
    //------------------------------------------------------------------------

    OEMTypeSymbol::OEMTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag OEMTypeSymbol::GetSymTag()
    {
        return SymTagCustomType;
    }

    bool OEMTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;

        if ( type->oem.count == 0 )
            return false;

        index = (&type->oem.count + 1)[0];
        return true;
    }

    bool OEMTypeSymbol::GetOemId( uint32_t& oemId )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        oemId = type->oem.oemId;
        return true;
    }

    bool OEMTypeSymbol::GetOemSymbolId( uint32_t& oemSymId )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        oemSymId = type->oem.oemSymId;
        return true;
    }

    bool OEMTypeSymbol::GetCount( uint32_t& count )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        count = type->oem.count;
        return true;
    }

    bool OEMTypeSymbol::GetTypes( TypeIndex*& indexes )
    {
        CodeViewType*   type = (CodeViewType*) mData.Type.Handle.Type;
        indexes = (TypeIndex*) (&type->oem.count + 1);
        return true;
    }


    //------------------------------------------------------------------------
    //  FieldListTypeSymbol
    //------------------------------------------------------------------------

    FieldListTypeSymbol::FieldListTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag FieldListTypeSymbol::GetSymTag()
    {
        return SymTagFieldList;
    }


    //------------------------------------------------------------------------
    //  TypeListTypeSymbol
    //------------------------------------------------------------------------

    TypeListTypeSymbol::TypeListTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );

        _ASSERT( handle.Tag == LF_ARGLIST || handle.Tag == LF_DERIVED );
    }

    SymTag TypeListTypeSymbol::GetSymTag()
    {
        return SymTagTypeList;
    }

    bool TypeListTypeSymbol::GetCount( uint32_t& count )
    {
        CodeViewRefType*  reftype = (CodeViewRefType*) mData.Type.Handle.Type;

        if ( mData.Type.Handle.Tag == LF_ARGLIST )
            count = reftype->arglist.count;
        else if ( mData.Type.Handle.Tag == LF_DERIVED )
            count = reftype->derived.count;
        else
            return false;

        return true;
    }

    bool TypeListTypeSymbol::GetTypes( TypeIndex*& indexes )
    {
        CodeViewRefType*  reftype = (CodeViewRefType*) mData.Type.Handle.Type;

        if ( mData.Type.Handle.Tag == LF_ARGLIST )
            indexes = reftype->arglist.args;
        else if ( mData.Type.Handle.Tag == LF_DERIVED )
            indexes = reftype->derived.drvdcls;
        else
            return false;

        return true;
    }


    //------------------------------------------------------------------------
    //  BaseClassTypeSymbol
    //------------------------------------------------------------------------

    BaseClassTypeSymbol::BaseClassTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag BaseClassTypeSymbol::GetSymTag()
    {
        return SymTagBaseClass;
    }

    bool BaseClassTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        index = field->bclass.type;
        return true;
    }

    bool BaseClassTypeSymbol::GetOffset( int32_t& offset )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        offset = GetUIntValue( (uint16_t*) &field->bclass.offset );
        return true;
    }

    bool BaseClassTypeSymbol::GetAttribute( uint16_t& attr )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        attr = field->bclass.attr;
        return true;
    }


    //------------------------------------------------------------------------
    //  EnumMemberTypeSymbol
    //------------------------------------------------------------------------

    EnumMemberTypeSymbol::EnumMemberTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag EnumMemberTypeSymbol::GetSymTag()
    {
        return SymTagData;
    }

    bool EnumMemberTypeSymbol::GetName( PasString*& name )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;

        uint32_t            offset = GetNumLeafSize( &field->enumerate.value );

        name = (PasString*) ((char*) &field->enumerate.value + offset);
        return true;
    }

    bool EnumMemberTypeSymbol::GetLocation( LocationType& loc )
    {
        loc = LocIsConstant;
        return true;
    }

    bool EnumMemberTypeSymbol::GetDataKind( DataKind& dataKind )
    {
        dataKind = DataIsConstant;
        return true;
    }

    bool EnumMemberTypeSymbol::GetAttribute( uint16_t& attr )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        attr = field->enumerate.attr;
        return true;
    }

    bool EnumMemberTypeSymbol::GetValue( Variant& value )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        GetNumLeafValue( &field->enumerate.value, value );
        return true;
    }


    //------------------------------------------------------------------------
    //  DataMemberTypeSymbol
    //------------------------------------------------------------------------

    DataMemberTypeSymbol::DataMemberTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag DataMemberTypeSymbol::GetSymTag()
    {
        return SymTagData;
    }

    bool DataMemberTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        index = field->member.type;
        return true;
    }

    bool DataMemberTypeSymbol::GetName( PasString*& name )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;

        uint32_t            offset = GetNumLeafSize( &field->member.offset );

        name = (PasString*) ((char*) &field->member.offset + offset);
        return true;
    }

    bool DataMemberTypeSymbol::GetLocation( LocationType& loc )
    {
        loc = LocIsThisRel;
        return true;
    }

    bool DataMemberTypeSymbol::GetDataKind( DataKind& dataKind )
    {
        dataKind = DataIsMember;
        return true;
    }

    bool DataMemberTypeSymbol::GetOffset( int32_t& offset )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        offset = GetUIntValue( &field->member.offset );
        return true;
    }

    bool DataMemberTypeSymbol::GetAttribute( uint16_t& attr )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        attr = field->member.attr;
        return true;
    }


    //------------------------------------------------------------------------
    //  StaticMemberTypeSymbol
    //------------------------------------------------------------------------

    StaticMemberTypeSymbol::StaticMemberTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag StaticMemberTypeSymbol::GetSymTag()
    {
        return SymTagData;
    }

    bool StaticMemberTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        index = field->stmember.type;
        return true;
    }

    bool StaticMemberTypeSymbol::GetName( PasString*& name )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        name = &field->stmember.p_name;
        return true;
    }

    bool StaticMemberTypeSymbol::GetLocation( LocationType& loc )
    {
        loc = LocIsStatic;
        return true;
    }

    bool StaticMemberTypeSymbol::GetDataKind( DataKind& dataKind )
    {
        dataKind = DataIsStaticMember;
        return true;
    }

    bool StaticMemberTypeSymbol::GetAttribute( uint16_t& attr )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        attr = field->stmember.attr;
        return true;
    }


    //------------------------------------------------------------------------
    //  MethodOverloadsTypeSymbol
    //------------------------------------------------------------------------

    MethodOverloadsTypeSymbol::MethodOverloadsTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag MethodOverloadsTypeSymbol::GetSymTag()
    {
        return SymTagMethodOverloads;
    }

    bool MethodOverloadsTypeSymbol::GetName( PasString*& name )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        name = &field->method.p_name;
        return true;
    }

    bool MethodOverloadsTypeSymbol::GetCount( uint32_t& count )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        count = field->method.count;
        return true;
    }


    //------------------------------------------------------------------------
    //  MListMethodTypeSymbol
    //------------------------------------------------------------------------

    MListMethodTypeSymbol::MListMethodTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag MListMethodTypeSymbol::GetSymTag()
    {
        return SymTagMethod;
    }

    bool MListMethodTypeSymbol::GetType( TypeIndex& index )
    {
        MListMethod* field = (MListMethod*) mData.Type.Handle.Type;
        index = field->OneMethod.Type;
        return true;
    }

    bool MListMethodTypeSymbol::GetVBaseOffset( uint32_t& offset )
    {
        MListMethod* field = (MListMethod*) mData.Type.Handle.Type;

        CV_fldattr_t*   attr = (CV_fldattr_t*) &field->OneMethod.Attr;
        if ( (attr->mprop == CV_MTintro) || (attr->mprop == CV_MTpureintro) )
        {
            offset = field->OneMethodVirt.VTabOffset;
            return true;
        }

        return false;
    }

    bool MListMethodTypeSymbol::GetAttribute( uint16_t& attr )
    {
        MListMethod* field = (MListMethod*) mData.Type.Handle.Type;
        attr = field->OneMethod.Attr;
        return true;
    }


    //------------------------------------------------------------------------
    //  MethodTypeSymbol
    //------------------------------------------------------------------------

    MethodTypeSymbol::MethodTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag MethodTypeSymbol::GetSymTag()
    {
        return SymTagMethod;
    }

    bool MethodTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        index = field->onemethod.type;
        return true;
    }

    bool MethodTypeSymbol::GetName( PasString*& name )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;

        CV_fldattr_t*   attr = (CV_fldattr_t*) &field->onemethod.attr;
        if ( (attr->mprop == CV_MTintro) || (attr->mprop == CV_MTpureintro) )
        {
            name = &field->onemethod_virt.p_name;
        }
        else
        {
            name = &field->onemethod.p_name;
        }
        return true;
    }

    bool MethodTypeSymbol::GetVBaseOffset( uint32_t& offset )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;

        CV_fldattr_t*   attr = (CV_fldattr_t*) &field->onemethod.attr;
        if ( (attr->mprop == CV_MTintro) || (attr->mprop == CV_MTpureintro) )
        {
            offset = field->onemethod_virt.vtaboff;
            return true;
        }

        return false;
    }

    bool MethodTypeSymbol::GetAttribute( uint16_t& attr )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        attr = field->onemethod.attr;
        return true;
    }


    //------------------------------------------------------------------------
    //  VFTablePtrTypeSymbol
    //------------------------------------------------------------------------

    VFTablePtrTypeSymbol::VFTablePtrTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag VFTablePtrTypeSymbol::GetSymTag()
    {
        return SymTagVTable;
    }

    bool VFTablePtrTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        index = field->vfunctab.type;
        return true;
    }


    //------------------------------------------------------------------------
    //  VFOffsetTypeSymbol
    //------------------------------------------------------------------------

    VFOffsetTypeSymbol::VFOffsetTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag VFOffsetTypeSymbol::GetSymTag()
    {
        return SymTagVTable;
    }

    bool VFOffsetTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        index = field->vfuncoff.type;
        return true;
    }

    bool VFOffsetTypeSymbol::GetOffset( int32_t& offset )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        offset = field->vfuncoff.offset;
        return true;
    }


    //------------------------------------------------------------------------
    //  NestedTypeSymbol
    //------------------------------------------------------------------------

    NestedTypeSymbol::NestedTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag NestedTypeSymbol::GetSymTag()
    {
        return SymTagNestedType;
    }

    bool NestedTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        index = field->nesttype.type;
        return true;
    }

    bool NestedTypeSymbol::GetName( PasString*& name )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        name = &field->nesttype.p_name;
        return true;
    }


    //------------------------------------------------------------------------
    //  FriendClassTypeSymbol
    //------------------------------------------------------------------------

    FriendClassTypeSymbol::FriendClassTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag FriendClassTypeSymbol::GetSymTag()
    {
        return SymTagFriend;
    }

    bool FriendClassTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        index = field->friendcls.type;
        return true;
    }


    //------------------------------------------------------------------------
    //  FriendFunctionTypeSymbol
    //------------------------------------------------------------------------

    FriendFunctionTypeSymbol::FriendFunctionTypeSymbol( const TypeHandleIn& handle )
        :   TypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag FriendFunctionTypeSymbol::GetSymTag()
    {
        return SymTagFriend;
    }

    bool FriendFunctionTypeSymbol::GetType( TypeIndex& index )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        index = field->friendfcn.type;
        return true;
    }

    bool FriendFunctionTypeSymbol::GetName( PasString*& name )
    {
        CodeViewFieldType*  field = (CodeViewFieldType*) mData.Type.Handle.Type;
        name = &field->friendfcn.p_name;
        return true;
    }
}
