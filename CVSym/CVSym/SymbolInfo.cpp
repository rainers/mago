/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "SymbolInfo.h"
#include "cvconst.h"
#include "Util.h"


namespace MagoST
{
    //------------------------------------------------------------------------
    //  NonTypeSymbol
    //------------------------------------------------------------------------

    NonTypeSymbol::NonTypeSymbol( const SymHandleIn& handle )
    {
        mData.Symbol.Handle = handle;
    }


    //------------------------------------------------------------------------
    //  RegSymbol
    //------------------------------------------------------------------------

    RegSymbol::RegSymbol( const SymHandleIn& handle )
        :   NonTypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag RegSymbol::GetSymTag()
    {
        return SymTagData;
    }

    bool RegSymbol::GetType( TypeIndex& index )
    {
        index = mData.Symbol.Handle.Sym->reg.type;
        return true;
    }

    bool RegSymbol::GetName( SymString& name )
    {
        assign( name, &mData.Symbol.Handle.Sym->reg.p_name );
        return true;
    }

    bool RegSymbol::GetLocation( LocationType& loc )
    {
        loc = LocIsEnregistered;
        return true;
    }

    bool RegSymbol::GetDataKind( DataKind& dataKind )
    {
        dataKind = DataIsLocal;
        return true;
    }

    bool RegSymbol::GetRegister( uint32_t& reg )
    {
        reg = mData.Symbol.Handle.Sym->reg.reg;
        return true;
    }


    //------------------------------------------------------------------------
    //  ConstSymbol
    //------------------------------------------------------------------------

    ConstSymbol::ConstSymbol( const SymHandleIn& handle )
        :   NonTypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag ConstSymbol::GetSymTag()
    {
        return SymTagData;
    }

    bool ConstSymbol::GetType( TypeIndex& index )
    {
        index = mData.Symbol.Handle.Sym->constant.type;
        return true;
    }

    bool ConstSymbol::GetName( SymString& name )
    {
        // value is variable size and it comes before name, so work it out
        uint32_t    offset = GetNumLeafSize( &mData.Symbol.Handle.Sym->constant.value );

        assign( name, (PasString*) ((char*) &mData.Symbol.Handle.Sym->constant.value + offset));
        return true;
    }

    bool ConstSymbol::GetLocation( LocationType& loc )
    {
        loc = LocIsConstant;
        return true;
    }

    bool ConstSymbol::GetDataKind( DataKind& dataKind )
    {
        dataKind = DataIsConstant;
        return true;
    }

    bool ConstSymbol::GetValue( Variant& val )
    {
        CodeViewSymbol*     sym = mData.Symbol.Handle.Sym;
        GetNumLeafValue( &sym->constant.value, val );
        return true;
    }


    //------------------------------------------------------------------------
    //  ManyRegsSymbol
    //------------------------------------------------------------------------

    ManyRegsSymbol::ManyRegsSymbol( const SymHandleIn& handle )
        :   NonTypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag ManyRegsSymbol::GetSymTag()
    {
        return SymTagData;
    }

    bool ManyRegsSymbol::GetType( TypeIndex& index )
    {
        index = mData.Symbol.Handle.Sym->manyreg.type;
        return true;
    }

    bool ManyRegsSymbol::GetName( SymString& name )
    {
        const CodeViewSymbol*   sym = mData.Symbol.Handle.Sym;
        uint32_t    offset = sizeof sym->manyreg.count;

        offset += sym->manyreg.count;

        assign( name, (PasString*) ( (char*) &sym->manyreg.count + offset ) );
        return true;
    }

    bool ManyRegsSymbol::GetLocation( LocationType& loc )
    {
        loc = LocIsEnregistered;
        return true;
    }

    bool ManyRegsSymbol::GetDataKind( DataKind& dataKind )
    {
        dataKind = DataIsLocal;
        return true;
    }

    bool ManyRegsSymbol::GetRegisterCount( uint8_t& count )
    {
        count = mData.Symbol.Handle.Sym->manyreg.count;
        return true;
    }

    bool ManyRegsSymbol::GetRegisters( uint8_t*& regs )
    {
        regs = mData.Symbol.Handle.Sym->manyreg.reg;
        return true;
    }


    //------------------------------------------------------------------------
    //  BPRelSymbol
    //------------------------------------------------------------------------

    BPRelSymbol::BPRelSymbol( const SymHandleIn& handle )
        :   NonTypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag BPRelSymbol::GetSymTag()
    {
        return SymTagData;
    }

    bool BPRelSymbol::GetType( TypeIndex& index )
    {
        index = mData.Symbol.Handle.Sym->bprel.type;
        return true;
    }

    bool BPRelSymbol::GetName( SymString& name )
    {
        assign( name, &mData.Symbol.Handle.Sym->bprel.p_name );
        return true;
    }

    bool BPRelSymbol::GetLocation( LocationType& loc )
    {
        loc = LocIsRegRel;
        return true;
    }

    bool BPRelSymbol::GetDataKind( DataKind& dataKind )
    {
        dataKind = mData.Symbol.Handle.Sym->bprel.offset < 0 ? DataIsParam : DataIsLocal;
        return true;
    }

    bool BPRelSymbol::GetRegister( uint32_t& reg )
    {
        // TODO: if we support more CPUs, then we'll have to change this
        reg = CV_REG_EBP;
        return true;
    }

    bool BPRelSymbol::GetOffset( int32_t& offset )
    {
        offset = (int32_t) mData.Symbol.Handle.Sym->bprel.offset;
        return true;
    }


    //------------------------------------------------------------------------
    //  DataSymbol
    //------------------------------------------------------------------------

    DataSymbol::DataSymbol( const SymHandleIn& handle )
        :   NonTypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag DataSymbol::GetSymTag()
    {
        return SymTagData;
    }

    bool DataSymbol::GetType( TypeIndex& index )
    {
        index = mData.Symbol.Handle.Sym->data.type;
        return true;
    }

    bool DataSymbol::GetName( SymString& name )
    {
        assign( name, &mData.Symbol.Handle.Sym->data.p_name );
        return true;
    }

    bool DataSymbol::GetLocation( LocationType& loc )
    {
        loc = LocIsStatic;
        return true;
    }

    bool DataSymbol::GetDataKind( DataKind& dataKind )
    {
        // if it's in a function and is LDATA/LTHREAD, then it's DataIsStaticLocal, 
        // but we'll leave that up to the user
        if ( mData.Symbol.Handle.Sym->Generic.id == S_LDATA32 )
            dataKind = DataIsFileStatic;
        else
            dataKind = DataIsGlobal;
        return true;
    }

    bool DataSymbol::GetAddressOffset( uint32_t& offset )
    {
        offset = mData.Symbol.Handle.Sym->data.offset;
        return true;
    }

    bool DataSymbol::GetAddressSegment( uint16_t& segment )
    {
        segment = mData.Symbol.Handle.Sym->data.segment;
        return true;
    }


    //------------------------------------------------------------------------
    //  PublicSymbol
    //------------------------------------------------------------------------

    PublicSymbol::PublicSymbol( const SymHandleIn& handle )
        :   DataSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag PublicSymbol::GetSymTag()
    {
        return SymTagPublicSymbol;
    }

    bool PublicSymbol::GetDataKind( DataKind& dataKind )
    {
        // TODO: do publics have DataKind?
        dataKind = DataIsGlobal;
        return true;
    }


    //------------------------------------------------------------------------
    //  ProcSymbol
    //------------------------------------------------------------------------

    ProcSymbol::ProcSymbol( const SymHandleIn& handle )
        :   NonTypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag ProcSymbol::GetSymTag()
    {
        return SymTagFunction;
    }

    bool ProcSymbol::GetType( TypeIndex& index )
    {
        index = mData.Symbol.Handle.Sym->proc.type;
        return true;
    }

    bool ProcSymbol::GetName( SymString& name )
    {
        assign( name, &mData.Symbol.Handle.Sym->proc.p_name );
        return true;
    }

    bool ProcSymbol::GetLocation( LocationType& loc )
    {
        loc = LocIsStatic;
        return true;
    }

    bool ProcSymbol::GetAddressOffset( uint32_t& offset )
    {
        offset = mData.Symbol.Handle.Sym->proc.offset;
        return true;
    }

    bool ProcSymbol::GetAddressSegment( uint16_t& segment )
    {
        segment = mData.Symbol.Handle.Sym->proc.segment;
        return true;
    }

    bool ProcSymbol::GetLength( uint32_t& length )
    {
        length = mData.Symbol.Handle.Sym->proc.length;
        return true;
    }

    bool ProcSymbol::GetDebugStart( uint32_t& start )
    {
        start = mData.Symbol.Handle.Sym->proc.debug_start;
        return true;
    }

    bool ProcSymbol::GetDebugEnd( uint32_t& end )
    {
        end = mData.Symbol.Handle.Sym->proc.debug_end;
        return true;
    }

#if 0
    bool ProcSymbol::GetProcFlags( CV_PROCFLAGS& flags )
    {
        PROCSYM32* sym = (PROCSYM32*) mSymRec;
        flags = sym->flags;
        return true;
    }
#else
    bool ProcSymbol::GetProcFlags( uint8_t& flags )
    {
        flags = mData.Symbol.Handle.Sym->proc.flags;
        return true;
    }
#endif


    //------------------------------------------------------------------------
    //  ThunkSymbol
    //------------------------------------------------------------------------

    ThunkSymbol::ThunkSymbol( const SymHandleIn& handle )
        :   NonTypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag ThunkSymbol::GetSymTag()
    {
        return SymTagThunk;
    }

    bool ThunkSymbol::GetName( SymString& name )
    {
        assign( name, &mData.Symbol.Handle.Sym->thunk.p_name );
        return true;
    }

    bool ThunkSymbol::GetLocation( LocationType& loc )
    {
        loc = LocIsStatic;
        return true;
    }

    bool ThunkSymbol::GetAddressOffset( uint32_t& offset )
    {
        offset = mData.Symbol.Handle.Sym->thunk.offset;
        return true;
    }

    bool ThunkSymbol::GetAddressSegment( uint16_t& segment )
    {
        segment = mData.Symbol.Handle.Sym->thunk.segment;
        return true;
    }

    bool ThunkSymbol::GetLength( uint32_t& length )
    {
        length = mData.Symbol.Handle.Sym->thunk.length;
        return true;
    }

    bool ThunkSymbol::GetThunkOrdinal( uint8_t& ordinal )
    {
        ordinal = mData.Symbol.Handle.Sym->thunk.ord;
        return true;
    }


    //------------------------------------------------------------------------
    //  BlockSymbol
    //------------------------------------------------------------------------

    BlockSymbol::BlockSymbol( const SymHandleIn& handle )
        :   NonTypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag BlockSymbol::GetSymTag()
    {
        return SymTagBlock;
    }

    bool BlockSymbol::GetName( SymString& name )
    {
        assign( name, &mData.Symbol.Handle.Sym->block.p_name );
        return true;
    }

    bool BlockSymbol::GetLocation( LocationType& loc )
    {
        loc = LocIsStatic;
        return true;
    }

    bool BlockSymbol::GetAddressOffset( uint32_t& offset )
    {
        offset = mData.Symbol.Handle.Sym->block.offset;
        return true;
    }

    bool BlockSymbol::GetAddressSegment( uint16_t& segment )
    {
        segment = mData.Symbol.Handle.Sym->block.segment;
        return true;
    }

    bool BlockSymbol::GetLength( uint32_t& length )
    {
        length = mData.Symbol.Handle.Sym->block.length;
        return true;
    }


    //------------------------------------------------------------------------
    //  LabelSymbol
    //------------------------------------------------------------------------

    LabelSymbol::LabelSymbol( const SymHandleIn& handle )
        :   NonTypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag LabelSymbol::GetSymTag()
    {
        return SymTagLabel;
    }

    bool LabelSymbol::GetName( SymString& name )
    {
        assign( name, &mData.Symbol.Handle.Sym->label.p_name );
        return true;
    }

    bool LabelSymbol::GetLocation( LocationType& loc )
    {
        loc = LocIsStatic;
        return true;
    }

    bool LabelSymbol::GetAddressOffset( uint32_t& offset )
    {
        offset = mData.Symbol.Handle.Sym->label.offset;
        return true;
    }

    bool LabelSymbol::GetAddressSegment( uint16_t& segment )
    {
        segment = mData.Symbol.Handle.Sym->label.segment;
        return true;
    }

#if 0
    bool LabelSymbol::GetProcFlags( CV_PROCFLAGS& flags )
    {
        LABELSYM32* sym = (LABELSYM32*) mSymRec;
        flags = sym->flags;
        return true;
    }
#else
    bool LabelSymbol::GetProcFlags( uint8_t& flags )
    {
        flags = mData.Symbol.Handle.Sym->label.flags;
        return true;
    }
#endif


    //------------------------------------------------------------------------
    //  RegRelSymbol
    //------------------------------------------------------------------------

    RegRelSymbol::RegRelSymbol( const SymHandleIn& handle )
        :   NonTypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag RegRelSymbol::GetSymTag()
    {
        return SymTagData;
    }

    bool RegRelSymbol::GetType( TypeIndex& index )
    {
        index = mData.Symbol.Handle.Sym->regrel.type;
        return true;
    }

    bool RegRelSymbol::GetName( SymString& name )
    {
        assign( name, &mData.Symbol.Handle.Sym->regrel.p_name );
        return true;
    }

    bool RegRelSymbol::GetLocation( LocationType& loc )
    {
        loc = LocIsRegRel;
        return true;
    }

    bool RegRelSymbol::GetDataKind( DataKind& dataKind )
    {
        dataKind = DataIsLocal;
        return true;
    }

    bool RegRelSymbol::GetRegister( uint32_t& reg )
    {
        reg = mData.Symbol.Handle.Sym->regrel.reg;
        return true;
    }

    bool RegRelSymbol::GetOffset( int32_t& offset )
    {
        offset = mData.Symbol.Handle.Sym->regrel.offset;
        return true;
    }


    //------------------------------------------------------------------------
    //  TLSSymbol
    //------------------------------------------------------------------------

    TLSSymbol::TLSSymbol( const SymHandleIn& handle )
        :   DataSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    bool TLSSymbol::GetLocation( LocationType& loc )
    {
        loc = LocIsTLS;
        return true;
    }

    bool TLSSymbol::GetDataKind( DataKind& dataKind )
    {
        // if it's in a function and is LDATA/LTHREAD, then it's DataIsStaticLocal
        // but we'll leave that up to the user
        if ( mData.Symbol.Handle.Sym->Generic.id == S_LTHREAD32 )
            dataKind = DataIsFileStatic;
        else
            dataKind = DataIsGlobal;
        return true;
    }


    //------------------------------------------------------------------------
    //  UdtSymbol
    //------------------------------------------------------------------------

    UdtSymbol::UdtSymbol( const SymHandleIn& handle )
        :   NonTypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag UdtSymbol::GetSymTag()
    {
        return SymTagTypedef;
    }

    bool UdtSymbol::GetType( TypeIndex& index )
    {
        index = mData.Symbol.Handle.Sym->udt.type;
        return true;
    }

    bool UdtSymbol::GetName( SymString& name )
    {
        assign( name, &mData.Symbol.Handle.Sym->udt.p_name );
        return true;
    }


    //------------------------------------------------------------------------
    //  EndOfArgsSymbol
    //------------------------------------------------------------------------

    EndOfArgsSymbol::EndOfArgsSymbol( const SymHandleIn& handle )
        :   NonTypeSymbol( handle )
    {
        C_ASSERT( sizeof( *this ) == sizeof( SymbolInfo ) );
    }

    SymTag EndOfArgsSymbol::GetSymTag()
    {
        return SymTagEndOfArgs;
    }
}
