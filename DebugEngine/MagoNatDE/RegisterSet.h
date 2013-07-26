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


    enum RegX86
    {
        RegX86_NONE,
        RegX86_AL,
        RegX86_CL,
        RegX86_DL,
        RegX86_BL,
        RegX86_AH,
        RegX86_CH,
        RegX86_DH,
        RegX86_BH,
        RegX86_AX,
        RegX86_CX,
        RegX86_DX,
        RegX86_BX,
        RegX86_SP,
        RegX86_BP,
        RegX86_SI,
        RegX86_DI,
        RegX86_EAX,
        RegX86_ECX,
        RegX86_EDX,
        RegX86_EBX,
        RegX86_ESP,
        RegX86_EBP,
        RegX86_ESI,
        RegX86_EDI,
        RegX86_ES,
        RegX86_CS,
        RegX86_SS,
        RegX86_DS,
        RegX86_FS,
        RegX86_GS,
        RegX86_IP,
        RegX86_FLAGS,
        RegX86_EIP,
        RegX86_EFLAGS,

        RegX86_DR0,
        RegX86_DR1,
        RegX86_DR2,
        RegX86_DR3,
        RegX86_DR6,
        RegX86_DR7,

        RegX86_ST0,
        RegX86_ST1,
        RegX86_ST2,
        RegX86_ST3,
        RegX86_ST4,
        RegX86_ST5,
        RegX86_ST6,
        RegX86_ST7,
        RegX86_CTRL,
        RegX86_STAT,
        RegX86_TAG,
        RegX86_FPIP,
        RegX86_FPCS,
        RegX86_FPDO,
        RegX86_FPDS,
        RegX86_FPEIP,
        RegX86_FPEDO,

        RegX86_MM0,
        RegX86_MM1,
        RegX86_MM2,
        RegX86_MM3,
        RegX86_MM4,
        RegX86_MM5,
        RegX86_MM6,
        RegX86_MM7,

        RegX86_XMM0,
        RegX86_XMM1,
        RegX86_XMM2,
        RegX86_XMM3,
        RegX86_XMM4,
        RegX86_XMM5,
        RegX86_XMM6,
        RegX86_XMM7,

        RegX86_XMM00,
        RegX86_XMM01,
        RegX86_XMM02,
        RegX86_XMM03,
        RegX86_XMM10,
        RegX86_XMM11,
        RegX86_XMM12,
        RegX86_XMM13,
        RegX86_XMM20,
        RegX86_XMM21,
        RegX86_XMM22,
        RegX86_XMM23,
        RegX86_XMM30,
        RegX86_XMM31,
        RegX86_XMM32,
        RegX86_XMM33,
        RegX86_XMM40,
        RegX86_XMM41,
        RegX86_XMM42,
        RegX86_XMM43,
        RegX86_XMM50,
        RegX86_XMM51,
        RegX86_XMM52,
        RegX86_XMM53,
        RegX86_XMM60,
        RegX86_XMM61,
        RegX86_XMM62,
        RegX86_XMM63,
        RegX86_XMM70,
        RegX86_XMM71,
        RegX86_XMM72,
        RegX86_XMM73,

        RegX86_XMM0L,
        RegX86_XMM1L,
        RegX86_XMM2L,
        RegX86_XMM3L,
        RegX86_XMM4L,
        RegX86_XMM5L,
        RegX86_XMM6L,
        RegX86_XMM7L,

        RegX86_XMM0H,
        RegX86_XMM1H,
        RegX86_XMM2H,
        RegX86_XMM3H,
        RegX86_XMM4H,
        RegX86_XMM5H,
        RegX86_XMM6H,
        RegX86_XMM7H,

        RegX86_MXCSR,

        RegX86_EMM0L,
        RegX86_EMM1L,
        RegX86_EMM2L,
        RegX86_EMM3L,
        RegX86_EMM4L,
        RegX86_EMM5L,
        RegX86_EMM6L,
        RegX86_EMM7L,

        RegX86_EMM0H,
        RegX86_EMM1H,
        RegX86_EMM2H,
        RegX86_EMM3H,
        RegX86_EMM4H,
        RegX86_EMM5H,
        RegX86_EMM6H,
        RegX86_EMM7H,

        RegX86_MM00,
        RegX86_MM01,
        RegX86_MM10,
        RegX86_MM11,
        RegX86_MM20,
        RegX86_MM21,
        RegX86_MM30,
        RegX86_MM31,
        RegX86_MM40,
        RegX86_MM41,
        RegX86_MM50,
        RegX86_MM51,
        RegX86_MM60,
        RegX86_MM61,
        RegX86_MM70,
        RegX86_MM71,
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
        virtual bool GetThreadContext( const void*& context, uint32_t& contextSize ) = 0;
    };


    class RegisterSet : public IRegisterSet
    {
        long    mRefCount;
        CONTEXT mContext;

    public:
        RegisterSet( 
            const CONTEXT& context );

        virtual void AddRef();
        virtual void Release();

        virtual HRESULT GetValue( uint32_t regId, RegisterValue& value );
        virtual HRESULT SetValue( uint32_t regId, const RegisterValue& value );
        virtual HRESULT IsReadOnly( uint32_t regId, bool& readOnly );
        virtual bool GetThreadContext( const void*& context, uint32_t& contextSize );
    };


    class TinyRegisterSet : public IRegisterSet
    {
        long    mRefCount;
        Address mEip;
        Address mEsp;
        Address mEbp;

    public:
        TinyRegisterSet( 
            Address eip,
            Address esp,
            Address ebp );

        virtual void AddRef();
        virtual void Release();

        virtual HRESULT GetValue( uint32_t regId, RegisterValue& value );
        virtual HRESULT SetValue( uint32_t regId, const RegisterValue& value );
        virtual HRESULT IsReadOnly( uint32_t regId, bool& readOnly );
        virtual bool GetThreadContext( const void*& context, uint32_t& contextSize );
    };


    RegisterType GetRegisterType( uint32_t regId );
}
