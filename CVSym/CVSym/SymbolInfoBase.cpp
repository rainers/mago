/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "SymbolInfoBase.h"
#include "cvconst.h"


C_ASSERT( sizeof( MagoST::SymbolInfo ) == sizeof( MagoST::SymInfoData ) );


namespace MagoST
{
    //------------------------------------------------------------------------
    //  SymbolInfo
    //------------------------------------------------------------------------

    SymbolInfo::SymbolInfo()
    {
        memset( &mData, 0, sizeof mData );
    }

    SymTag SymbolInfo::GetSymTag()
    {
        return SymTagNull;
    }

    bool SymbolInfo::GetType( TypeIndex& index )
    {
        UNREFERENCED_PARAMETER( index );
        return false;
    }

    bool SymbolInfo::GetName( SymString& name )
    {
        UNREFERENCED_PARAMETER( name );
        return false;
    }

    bool SymbolInfo::GetLocation( LocationType& loc )
    {
        UNREFERENCED_PARAMETER( loc );
        return false;
    }

    bool SymbolInfo::GetDataKind( DataKind& dataKind )
    {
        UNREFERENCED_PARAMETER( dataKind );
        return false;
    }

    bool SymbolInfo::GetAddressOffset( uint32_t& offset )
    {
        UNREFERENCED_PARAMETER( offset );
        return false;
    }

    bool SymbolInfo::GetAddressSegment( uint16_t& segment )
    {
        UNREFERENCED_PARAMETER( segment );
        return false;
    }

    bool SymbolInfo::GetUdtKind( UdtKind& udtKind )
    {
        UNREFERENCED_PARAMETER( udtKind );
        return false;
    }

    bool SymbolInfo::GetRegister( uint32_t& reg )
    {
        UNREFERENCED_PARAMETER( reg );
        return false;
    }

    bool SymbolInfo::GetValue( Variant& value )
    {
        UNREFERENCED_PARAMETER( value );
        return false;
    }

    bool SymbolInfo::GetRegisterCount( uint8_t& count )
    {
        UNREFERENCED_PARAMETER( count );
        return false;
    }

    bool SymbolInfo::GetRegisters( uint8_t*& regs )
    {
        UNREFERENCED_PARAMETER( regs );
        return false;
    }

    bool SymbolInfo::GetOffset( int32_t& offset )
    {
        UNREFERENCED_PARAMETER( offset );
        return false;
    }

    bool SymbolInfo::GetLength( uint32_t& length )
    {
        UNREFERENCED_PARAMETER( length );
        return false;
    }

    bool SymbolInfo::GetBitfieldRange( uint32_t& position, uint32_t& length )
    {
        UNREFERENCED_PARAMETER( position );
        UNREFERENCED_PARAMETER( length );
        return false;
    }

#if 1
    bool SymbolInfo::GetDebugStart( uint32_t& start )
    {
        UNREFERENCED_PARAMETER( start );
        return false;
    }

    bool SymbolInfo::GetDebugEnd( uint32_t& end )
    {
        UNREFERENCED_PARAMETER( end );
        return false;
    }

    bool SymbolInfo::GetProcFlags( uint8_t& flags )
    {
        UNREFERENCED_PARAMETER( flags );
        return false;
    }

    bool SymbolInfo::GetThunkOrdinal( uint8_t& ordinal )
    {
        UNREFERENCED_PARAMETER( ordinal );
        return false;
    }

    bool SymbolInfo::GetBasicType( DWORD& basicType )
    {
        UNREFERENCED_PARAMETER( basicType );
        return false;
    }

    bool SymbolInfo::GetIndexType( TypeIndex& index )
    {
        UNREFERENCED_PARAMETER( index );
        return false;
    }

    bool SymbolInfo::GetCount( uint32_t& count )
    {
        UNREFERENCED_PARAMETER( count );
        return false;
    }

    bool SymbolInfo::GetFieldCount( uint16_t& count )
    {
        UNREFERENCED_PARAMETER( count );
        return false;
    }

    bool SymbolInfo::GetFieldList( TypeIndex& index )
    {
        UNREFERENCED_PARAMETER( index );
        return false;
    }

    bool SymbolInfo::GetProperties( uint16_t& props )
    {
        UNREFERENCED_PARAMETER( props );
        return false;
    }

    bool SymbolInfo::GetDerivedList( TypeIndex& index )
    {
        UNREFERENCED_PARAMETER( index );
        return false;
    }

    bool SymbolInfo::GetVShape( TypeIndex& index )
    {
        UNREFERENCED_PARAMETER( index );
        return false;
    }

    bool SymbolInfo::GetVtblOffset( int& offset )
    {
        UNREFERENCED_PARAMETER( offset );
        return false;
    }

    bool SymbolInfo::GetCallConv( uint8_t& callConv )
    {
        UNREFERENCED_PARAMETER( callConv );
        return false;
    }

    bool SymbolInfo::GetParamCount( uint16_t& count )
    {
        UNREFERENCED_PARAMETER( count );
        return false;
    }

    bool SymbolInfo::GetParamList( TypeIndex& index )
    {
        UNREFERENCED_PARAMETER( index );
        return false;
    }

    bool SymbolInfo::GetClass( TypeIndex& index )
    {
        UNREFERENCED_PARAMETER( index );
        return false;
    }

    bool SymbolInfo::GetThis( TypeIndex& index )
    {
        UNREFERENCED_PARAMETER( index );
        return false;
    }

    bool SymbolInfo::GetThisAdjust( int32_t& adjust )
    {
        UNREFERENCED_PARAMETER( adjust );
        return false;
    }

    bool SymbolInfo::GetOemId( uint32_t& oemId )
    {
        UNREFERENCED_PARAMETER( oemId );
        return false;
    }

    bool SymbolInfo::GetOemSymbolId( uint32_t& oemSymId )
    {
        UNREFERENCED_PARAMETER( oemSymId );
        return false;
    }

    bool SymbolInfo::GetTypes( std::vector<TypeIndex>& indexes )
    {
        UNREFERENCED_PARAMETER( indexes );
        return false;
    }

    bool SymbolInfo::GetAttribute( uint16_t& attr )
    {
        UNREFERENCED_PARAMETER( attr );
        return false;
    }

    bool SymbolInfo::GetVBaseOffset( uint32_t& offset )
    {
        UNREFERENCED_PARAMETER( offset );
        return false;
    }

    bool SymbolInfo::GetVTableDescriptor( uint32_t index, uint8_t& desc )
    {
        UNREFERENCED_PARAMETER( index );
        UNREFERENCED_PARAMETER( desc );
        return false;
    }

    bool SymbolInfo::GetMod( uint16_t& mod )
    {
        UNREFERENCED_PARAMETER( mod );
        return false;
    }
#endif
}
