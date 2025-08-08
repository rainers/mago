/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "ISymbolInfo.h"


namespace MagoST
{
    class SymbolInfo : public ISymbolInfo
    {
    protected:
        union
        {
            struct
            {
                SymHandleIn     Handle;
            } Symbol;

            struct
            {
                TypeHandleIn    Handle;
                uint16_t        Mod;
            } Type;
        } mData;

    public:
        SymbolInfo();

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );

        virtual bool GetLocation( LocationType& locType );
        virtual bool GetDataKind( DataKind& dataKind );
        virtual bool GetAddressOffset( uint32_t& offset );
        virtual bool GetAddressSegment( uint16_t& segment );
        virtual bool GetUdtKind( UdtKind& udtKind );
        virtual bool GetRegister( uint32_t& reg );
        virtual bool GetValue( Variant& value );
        virtual bool GetRegisterCount( uint8_t& count );
        virtual bool GetRegisters( uint8_t*& regs );
        virtual bool GetOffset( int32_t& offset );
        virtual bool GetLength( uint32_t& length );
        virtual bool GetBitfieldRange( uint32_t& position, uint32_t& length );

#if 1
        virtual bool GetDebugStart( uint32_t& start );
        virtual bool GetDebugEnd( uint32_t& end );
        //virtual bool GetProcFlags( CV_PROCFLAGS& flags );
        virtual bool GetProcFlags( uint8_t& flags );

        virtual bool GetThunkOrdinal( uint8_t& ordinal );

        virtual bool GetBasicType( DWORD& basicType );

        virtual bool GetIndexType( TypeIndex& index );
        virtual bool GetCount( uint32_t& count );

        virtual bool GetFieldCount( uint16_t& count );
        virtual bool GetFieldList( TypeIndex& index );
        virtual bool GetProperties( uint16_t& props );
        virtual bool GetDerivedList( TypeIndex& index );
        virtual bool GetVShape( TypeIndex& index );
        virtual bool GetVtblOffset( int& offset );

        virtual bool GetCallConv( uint8_t& callConv );
        virtual bool GetParamCount( uint16_t& count );
        virtual bool GetParamList( TypeIndex& index );

        virtual bool GetClass( TypeIndex& index );
        virtual bool GetThis( TypeIndex& index );
        virtual bool GetThisAdjust( int32_t& adjust );

        virtual bool GetOemId( uint32_t& oemId );
        virtual bool GetOemSymbolId( uint32_t& oemSymId );
        virtual bool GetTypes( std::vector<TypeIndex>& indexes );

        virtual bool GetAttribute( uint16_t& attr );
        virtual bool GetVBaseOffset( uint32_t& offset );

        virtual bool GetVTableDescriptor( uint32_t index, uint8_t& desc );

        virtual bool GetMod( uint16_t& mod );
#endif
    };
}
