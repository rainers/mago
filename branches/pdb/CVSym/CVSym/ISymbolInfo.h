/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


struct PasString;


namespace MagoST
{
    enum SymTag;
    enum LocationType;
    enum DataKind;
    enum UdtKind;


    class ISymbolInfo
    {
    public:
        virtual SymTag GetSymTag() = 0;
        virtual bool GetType( TypeIndex& index ) = 0;
        virtual bool GetName( PasString*& name ) = 0;

        virtual bool GetAddressOffset( uint32_t& offset ) = 0;
        virtual bool GetAddressSegment( uint16_t& segment ) = 0;
        virtual bool GetDataKind( DataKind& dataKind ) = 0;
        virtual bool GetLength( uint32_t& length ) = 0;
        virtual bool GetLocation( LocationType& locType ) = 0;
        virtual bool GetOffset( int32_t& offset ) = 0;
        virtual bool GetRegister( uint32_t& reg ) = 0;
        virtual bool GetRegisterCount( uint8_t& count ) = 0;
        virtual bool GetRegisters( uint8_t*& regs ) = 0;
        virtual bool GetUdtKind( UdtKind& udtKind ) = 0;
        virtual bool GetValue( Variant& value ) = 0;

#if 1
        virtual bool GetDebugStart( uint32_t& start ) = 0;
        virtual bool GetDebugEnd( uint32_t& end ) = 0;
        //virtual bool GetProcFlags( CV_PROCFLAGS& flags ) = 0;
        virtual bool GetProcFlags( uint8_t& flags ) = 0;

        virtual bool GetThunkOrdinal( uint8_t& ordinal ) = 0;

        virtual bool GetBasicType( DWORD& basicType ) = 0;

        virtual bool GetIndexType( TypeIndex& index ) = 0;
        virtual bool GetCount( uint32_t& count ) = 0;

        virtual bool GetFieldCount( uint16_t& count ) = 0;
        virtual bool GetFieldList( TypeIndex& index ) = 0;
        virtual bool GetProperties( uint16_t& props ) = 0;
        virtual bool GetDerivedList( TypeIndex& index ) = 0;
        virtual bool GetVShape( TypeIndex& index ) = 0;

        virtual bool GetCallConv( uint8_t& callConv ) = 0;
        virtual bool GetParamCount( uint16_t& count ) = 0;
        virtual bool GetParamList( TypeIndex& index ) = 0;

        virtual bool GetClass( TypeIndex& index ) = 0;
        virtual bool GetThis( TypeIndex& index ) = 0;
        virtual bool GetThisAdjust( int32_t& adjust ) = 0;

        virtual bool GetOemId( uint32_t& oemId ) = 0;
        virtual bool GetOemSymbolId( uint32_t& oemSymId ) = 0;
        virtual bool GetTypes( TypeIndex*& indexes ) = 0;

        virtual bool GetAttribute( uint16_t& attr ) = 0;
        virtual bool GetVBaseOffset( uint32_t& offset ) = 0;

        virtual bool GetVTableDescriptor( uint32_t index, uint8_t& desc ) = 0;

        virtual bool GetMod( uint16_t& mod ) = 0;
#endif
    };
}
