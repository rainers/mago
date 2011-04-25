/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoEE
{
    class Type;
    enum ENUMTY;
    class IEnumDeclarationMembers;
    enum UdtKind;


    class Declaration
    {
    public:
        virtual ~Declaration()
        {
        }

        virtual void AddRef() = 0;
        virtual void Release() = 0;

        virtual const wchar_t* GetName() = 0;

        virtual bool GetType( Type*& type ) = 0;
        virtual bool GetAddress( Address& addr ) = 0;
        virtual bool GetOffset( int& offset ) = 0;
        virtual bool GetSize( uint32_t& size ) = 0;
        virtual bool GetBackingTy( ENUMTY& ty ) = 0;
        virtual bool GetUdtKind( UdtKind& kind ) = 0;
        virtual bool GetBaseClassOffset( Declaration* baseClass, int& offset ) = 0;

        virtual bool IsField() = 0;
        virtual bool IsVar() = 0;
        virtual bool IsConstant() = 0;
        virtual bool IsType() = 0;
        virtual bool IsBaseClass() = 0;

        virtual HRESULT FindObject( const wchar_t* name, Declaration*& decl ) = 0;
        virtual bool EnumMembers( IEnumDeclarationMembers*& members ) = 0;
        virtual HRESULT FindObjectByValue( uint64_t intVal, Declaration*& decl ) = 0;
    };


    class IEnumDeclarationMembers
    {
    public:
        virtual void AddRef() = 0;
        virtual void Release() = 0;

        virtual uint32_t GetCount() = 0;
        virtual bool Next( MagoEE::Declaration*& decl ) = 0;
        virtual bool Skip( uint32_t count ) = 0;
        virtual bool Reset() = 0;
    };
}
