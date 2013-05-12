/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "SymbolInfoBase.h"


namespace MagoST
{
    class NonTypeSymbol : public SymbolInfo
    {
    public:
        NonTypeSymbol( const SymHandleIn& handle );
    };


    class RegSymbol : public NonTypeSymbol
    {
    public:
        RegSymbol( const SymHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );

        virtual bool GetLocation( LocationType& locType );
        virtual bool GetDataKind( DataKind& dataKind );
        virtual bool GetRegister( uint32_t& reg );
    };


    class ConstSymbol : public NonTypeSymbol
    {
    public:
        ConstSymbol( const SymHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );

        virtual bool GetLocation( LocationType& locType );
        virtual bool GetDataKind( DataKind& dataKind );
        virtual bool GetValue( Variant& value );
    };


    class ManyRegsSymbol : public NonTypeSymbol
    {
    public:
        ManyRegsSymbol( const SymHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );

        virtual bool GetLocation( LocationType& locType );
        virtual bool GetDataKind( DataKind& dataKind );
        virtual bool GetRegisterCount( uint8_t& count );
        virtual bool GetRegisters( uint8_t*& regs );
    };


    class BPRelSymbol : public NonTypeSymbol
    {
    public:
        BPRelSymbol( const SymHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );

        virtual bool GetLocation( LocationType& locType );
        virtual bool GetDataKind( DataKind& dataKind );
        virtual bool GetRegister( uint32_t& reg );
        virtual bool GetOffset( int32_t& offset );
    };


    class DataSymbol : public NonTypeSymbol
    {
    public:
        DataSymbol( const SymHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );

        virtual bool GetLocation( LocationType& locType );
        virtual bool GetDataKind( DataKind& dataKind );
        virtual bool GetAddressOffset( uint32_t& offset );
        virtual bool GetAddressSegment( uint16_t& segment );
    };


    class PublicSymbol : public DataSymbol
    {
    public:
        PublicSymbol( const SymHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetDataKind( DataKind& dataKind );
    };


    class ProcSymbol : public NonTypeSymbol
    {
    public:
        ProcSymbol( const SymHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );

        virtual bool GetLocation( LocationType& locType );
        virtual bool GetAddressOffset( uint32_t& offset );
        virtual bool GetAddressSegment( uint16_t& segment );
        virtual bool GetLength( uint32_t& length );

        virtual bool GetDebugStart( uint32_t& start );
        virtual bool GetDebugEnd( uint32_t& end );
        //virtual bool GetProcFlags( CV_PROCFLAGS& flags );
        virtual bool GetProcFlags( uint8_t& flags );
    };


    class ThunkSymbol : public NonTypeSymbol
    {
    public:
        ThunkSymbol( const SymHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetName( SymString& name );

        virtual bool GetLocation( LocationType& locType );
        virtual bool GetAddressOffset( uint32_t& offset );
        virtual bool GetAddressSegment( uint16_t& segment );
        virtual bool GetLength( uint32_t& length );

        virtual bool GetThunkOrdinal( uint8_t& ordinal );
    };


    class BlockSymbol : public NonTypeSymbol
    {
    public:
        BlockSymbol( const SymHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetName( SymString& name );

        virtual bool GetLocation( LocationType& locType );
        virtual bool GetAddressOffset( uint32_t& offset );
        virtual bool GetAddressSegment( uint16_t& segment );
        virtual bool GetLength( uint32_t& length );
    };


    class LabelSymbol : public NonTypeSymbol
    {
    public:
        LabelSymbol( const SymHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetName( SymString& name );

        virtual bool GetLocation( LocationType& locType );
        virtual bool GetAddressOffset( uint32_t& offset );
        virtual bool GetAddressSegment( uint16_t& segment );

        //virtual bool GetProcFlags( CV_PROCFLAGS& flags );
        virtual bool GetProcFlags( uint8_t& flags );
    };


    class RegRelSymbol : public NonTypeSymbol
    {
    public:
        RegRelSymbol( const SymHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );

        virtual bool GetLocation( LocationType& locType );
        virtual bool GetDataKind( DataKind& dataKind );
        virtual bool GetRegister( uint32_t& reg );
        virtual bool GetOffset( int32_t& offset );
    };


    // because the S_LTHREAD32 and S_GTHREAD32 records look the same as S_LDATA32 and S_GDATA32
    // base the class on the DataSymbol

    class TLSSymbol : public DataSymbol
    {
    public:
        TLSSymbol( const SymHandleIn& handle );

        virtual bool GetLocation( LocationType& locType );
        virtual bool GetDataKind( DataKind& dataKind );
    };


    class UdtSymbol : public NonTypeSymbol
    {
    public:
        UdtSymbol( const SymHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );
    };


    class EndOfArgsSymbol : public NonTypeSymbol
    {
    public:
        EndOfArgsSymbol( const SymHandleIn& handle );

        virtual SymTag GetSymTag();
    };
}
