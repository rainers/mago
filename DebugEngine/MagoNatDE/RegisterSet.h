/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class DebuggerProxy;


    enum RegisterType
    {
        RegType_None,
        RegType_Int8,
        RegType_Int16,
        RegType_Int32,
        RegType_Int64,
        RegType_Float32,
        RegType_Float64,
        RegType_Float80,
        RegType_Vector128,
    };


    struct RegisterValue
    {
        union
        {
            uint8_t     I8;
            uint16_t    I16;
            uint32_t    I32;
            uint64_t    I64;
            float       F32;
            double      F64;
            uint8_t     F80Bytes[10];
            uint8_t     V8[16];
        } Value;

        RegisterType    Type;

        uint64_t    GetInt() const;
        void        SetInt( uint64_t n );
    };


    struct RegisterDesc
    {
        uint64_t        SubregMask;
        uint8_t         Type;
        uint8_t         ParentRegId;
        uint8_t         SubregLength;
        uint8_t         SubregOffset;
        uint16_t        ContextOffset;
        uint16_t        ContextSize;
    };


    class IRegisterSet
    {
    public:
        virtual void AddRef() = 0;
        virtual void Release() = 0;

        //  Returns S_FALSE if register is unavailable. 
        //      value will be zero with the right type.
        virtual HRESULT GetValue( uint32_t regId, RegisterValue& value ) = 0;
        virtual HRESULT SetValue( uint32_t regId, const RegisterValue& value ) = 0;
        virtual HRESULT IsReadOnly( uint32_t regId, bool& readOnly ) = 0;
        virtual uint64_t GetPC() = 0;
        virtual bool GetThreadContext( const void*& context, uint32_t& contextSize ) = 0;
        virtual RegisterType GetRegisterType( uint32_t regId ) = 0;
    };


    class RegisterSet : public IRegisterSet
    {
        long                    mRefCount;
        const RegisterDesc*     mRegDesc;
        uint32_t                mRegCount;
        std::unique_ptr<BYTE[]> mContextBuf;
        uint16_t                mContextSize;
        uint16_t                mPCId;

    public:
        RegisterSet( 
            const RegisterDesc* regDesc,
            uint16_t regCount,
            uint16_t pcId );
        HRESULT Init(
            const void* context,
            uint32_t contextSize );

        virtual void AddRef();
        virtual void Release();

        virtual HRESULT GetValue( uint32_t regId, RegisterValue& value );
        virtual HRESULT SetValue( uint32_t regId, const RegisterValue& value );
        virtual HRESULT IsReadOnly( uint32_t regId, bool& readOnly );
        virtual uint64_t GetPC();
        virtual bool GetThreadContext( const void*& context, uint32_t& contextSize );
        virtual RegisterType GetRegisterType( uint32_t regId );
    };


    class TinyRegisterSet : public IRegisterSet
    {
        long                    mRefCount;
        const RegisterDesc*     mRegDesc;
        uint32_t                mRegCount;
        Address                 mPC;
        Address                 mStack;
        Address                 mFrame;
        uint16_t                mPCId;
        uint16_t                mStackId;
        uint16_t                mFrameId;

    public:
        TinyRegisterSet( 
            const RegisterDesc* regDesc,
            uint32_t regCount,
            uint16_t pcId,
            uint16_t stackId,
            uint16_t frameId,
            Address pc,
            Address stack,
            Address frame );

        virtual void AddRef();
        virtual void Release();

        virtual HRESULT GetValue( uint32_t regId, RegisterValue& value );
        virtual HRESULT SetValue( uint32_t regId, const RegisterValue& value );
        virtual HRESULT IsReadOnly( uint32_t regId, bool& readOnly );
        virtual uint64_t GetPC();
        virtual bool GetThreadContext( const void*& context, uint32_t& contextSize );
        virtual RegisterType GetRegisterType( uint32_t regId );
    };
}
