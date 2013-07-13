/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "SymbolInfoBase.h"


namespace MagoST
{
    class TypeSymbol : public SymbolInfo
    {
    public:
        TypeSymbol( const TypeHandleIn& handle );

        void SetMod( uint16_t mod );

        virtual bool GetMod( uint16_t& mod );
    };


    class BaseTypeSymbol : public TypeSymbol
    {
    public:
        BaseTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();

        virtual bool GetLength( uint32_t& length );
        virtual bool GetBasicType( DWORD& basicType );

        static bool GetBasicLengthAndType( TypeIndex index, DWORD& basicType, uint32_t& length );
    };


    class BasePointerTypeSymbol : public TypeSymbol
    {
    public:
        BasePointerTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
    };


    class PointerTypeSymbol : public TypeSymbol
    {
    public:
        PointerTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );

        virtual bool GetMod( uint16_t& mod );
    };


    class ArrayTypeSymbol : public TypeSymbol
    {
    public:
        ArrayTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );

        virtual bool GetLength( uint32_t& length );
        virtual bool GetIndexType( TypeIndex& index );
    };


    class StructOrClassTypeSymbol : public TypeSymbol
    {
    public:
        StructOrClassTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetName( SymString& name );

        virtual bool GetLength( uint32_t& length );

        virtual bool GetFieldCount( uint16_t& count );
        virtual bool GetFieldList( TypeIndex& index );
        virtual bool GetProperties( uint16_t& props );
        virtual bool GetDerivedList( TypeIndex& index );
        virtual bool GetVShape( TypeIndex& index );
    };


    class StructTypeSymbol : public StructOrClassTypeSymbol
    {
    public:
        StructTypeSymbol( const TypeHandleIn& handle );

        virtual bool GetUdtKind( UdtKind& udtKind );
    };


    class ClassTypeSymbol : public StructOrClassTypeSymbol
    {
    public:
        ClassTypeSymbol( const TypeHandleIn& handle );

        virtual bool GetUdtKind( UdtKind& udtKind );
    };


    class UnionTypeSymbol : public TypeSymbol
    {
    public:
        UnionTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetName( SymString& name );

        virtual bool GetUdtKind( UdtKind& udtKind );
        virtual bool GetLength( uint32_t& length );
 
        virtual bool GetFieldCount( uint16_t& count );
        virtual bool GetFieldList( TypeIndex& index );
        virtual bool GetProperties( uint16_t& props );
    };


    class EnumTypeSymbol : public TypeSymbol
    {
    public:
        EnumTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );

        virtual bool GetLength( uint32_t& length );

        virtual bool GetFieldCount( uint16_t& count );
        virtual bool GetFieldList( TypeIndex& index );
        virtual bool GetProperties( uint16_t& props );
    };


    class ProcTypeSymbol : public TypeSymbol
    {
    public:
        ProcTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );

        virtual bool GetCallConv( uint8_t& callConv );
        virtual bool GetParamCount( uint16_t& count );
        virtual bool GetParamList( TypeIndex& index );
    };


    class MProcTypeSymbol : public TypeSymbol
    {
    public:
        MProcTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );

        virtual bool GetCallConv( uint8_t& callConv );
        virtual bool GetParamCount( uint16_t& count );
        virtual bool GetParamList( TypeIndex& index );

        virtual bool GetClass( TypeIndex& index );
        virtual bool GetThis( TypeIndex& index );
        virtual bool GetThisAdjust( int32_t& adjust );
    };


    class VTShapeTypeSymbol : public TypeSymbol
    {
    public:
        VTShapeTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();

        virtual bool GetCount( uint32_t& count );
        virtual bool GetVTableDescriptor( uint32_t index, uint8_t& desc );
    };


    class OEMTypeSymbol : public TypeSymbol
    {
    public:
        OEMTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );

        virtual bool GetOemId( uint32_t& oemId );
        virtual bool GetOemSymbolId( uint32_t& oemSymId );
        virtual bool GetCount( uint32_t& count );
        virtual bool GetTypes( std::vector<TypeIndex>& indexes );
    };


    class FieldListTypeSymbol : public TypeSymbol
    {
    public:
        FieldListTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
    };


    class TypeListTypeSymbol : public TypeSymbol
    {
    public:
        TypeListTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetCount( uint32_t& count );
        virtual bool GetTypes( std::vector<TypeIndex>& indexes );
    };


    class BaseClassTypeSymbol : public TypeSymbol
    {
    public:
        BaseClassTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );

        virtual bool GetOffset( int32_t& offset );

        virtual bool GetAttribute( uint16_t& attr );
    };


    class EnumMemberTypeSymbol : public TypeSymbol
    {
    public:
        EnumMemberTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetName( SymString& name );

        virtual bool GetLocation( LocationType& loc );
        virtual bool GetDataKind( DataKind& dataKind );
        virtual bool GetValue( Variant& value );

        virtual bool GetAttribute( uint16_t& attr );
    };


    class DataMemberTypeSymbol : public TypeSymbol
    {
    public:
        DataMemberTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );

        virtual bool GetLocation( LocationType& loc );
        virtual bool GetDataKind( DataKind& dataKind );
        virtual bool GetOffset( int32_t& offset );

        virtual bool GetAttribute( uint16_t& attr );
    };


    class StaticMemberTypeSymbol : public TypeSymbol
    {
    public:
        StaticMemberTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );

        virtual bool GetLocation( LocationType& loc );
        virtual bool GetDataKind( DataKind& dataKind );

        virtual bool GetAttribute( uint16_t& attr );
    };


    class MethodOverloadsTypeSymbol : public TypeSymbol
    {
    public:
        MethodOverloadsTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetName( SymString& name );

        virtual bool GetCount( uint32_t& count );
    };


    class MethodTypeSymbol : public TypeSymbol
    {
    public:
        MethodTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );

        virtual bool GetAttribute( uint16_t& attr );
        virtual bool GetVBaseOffset( uint32_t& offset );
    };


    class MListMethodTypeSymbol : public TypeSymbol
    {
        union MListMethod
        {
            struct
            {
                uint16_t    Attr;
                uint16_t    Type;
            } OneMethod;

            struct
            {
                uint16_t    Attr;
                uint16_t    Type;
                uint32_t    VTabOffset;
            } OneMethodVirt;
        };

    public:
        MListMethodTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );

        virtual bool GetAttribute( uint16_t& attr );
        virtual bool GetVBaseOffset( uint32_t& offset );
    };


    class VFTablePtrTypeSymbol : public TypeSymbol
    {
    public:
        VFTablePtrTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
    };


    class VFOffsetTypeSymbol : public TypeSymbol
    {
    public:
        VFOffsetTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );

        virtual bool GetOffset( int32_t& offset );
    };


    class NestedTypeSymbol : public TypeSymbol
    {
    public:
        NestedTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );
    };


    class FriendClassTypeSymbol : public TypeSymbol
    {
    public:
        FriendClassTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
    };


    class FriendFunctionTypeSymbol : public TypeSymbol
    {
    public:
        FriendFunctionTypeSymbol( const TypeHandleIn& handle );

        virtual SymTag GetSymTag();
        virtual bool GetType( TypeIndex& index );
        virtual bool GetName( SymString& name );
    };
}
