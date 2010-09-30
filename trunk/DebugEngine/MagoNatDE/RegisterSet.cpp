/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "RegisterSet.h"


namespace Mago
{
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


    static const RegisterDesc gRegDesc[] = 
    {
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // NONE
        { 0xFF, RegType_Int8, RegX86_EAX, 8, 0, 0, 0 },    // AL
        { 0xFF, RegType_Int8, RegX86_ECX, 8, 0, 0, 0 },    // CL
        { 0xFF, RegType_Int8, RegX86_EDX, 8, 0, 0, 0 },    // DL
        { 0xFF, RegType_Int8, RegX86_EBX, 8, 0, 0, 0 },    // BL
        { 0xFF, RegType_Int8, RegX86_EAX, 8, 8, 0, 0 },    // AH
        { 0xFF, RegType_Int8, RegX86_ECX, 8, 8, 0, 0 },    // CH
        { 0xFF, RegType_Int8, RegX86_EDX, 8, 8, 0, 0 },    // DH
        { 0xFF, RegType_Int8, RegX86_EBX, 8, 8, 0, 0 },    // BH
        { 0xFFFF, RegType_Int16, RegX86_EAX, 16, 0, 0, 0 },    // AX
        { 0xFFFF, RegType_Int16, RegX86_ECX, 16, 0, 0, 0 },    // CX
        { 0xFFFF, RegType_Int16, RegX86_EDX, 16, 0, 0, 0 },    // DX
        { 0xFFFF, RegType_Int16, RegX86_EBX, 16, 0, 0, 0 },    // BX
        { 0xFFFF, RegType_Int16, RegX86_ESP, 16, 0, 0, 0 },    // SP
        { 0xFFFF, RegType_Int16, RegX86_EBP, 16, 0, 0, 0 },    // BP
        { 0xFFFF, RegType_Int16, RegX86_ESI, 16, 0, 0, 0 },    // SI
        { 0xFFFF, RegType_Int16, RegX86_EDI, 16, 0, 0, 0 },    // DI
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Eax ), 4 },    // EAX
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Ecx ), 4 },    // ECX
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Edx ), 4 },    // EDX
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Ebx ), 4 },    // EBX
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Esp ), 4 },    // ESP
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Ebp ), 4 },    // EBP
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Esi ), 4 },    // ESI
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Edi ), 4 },    // EDI
        { 0xFFFF, RegType_Int16, RegX86_ES, 16, 0, offsetof( CONTEXT, SegEs ), 4 },    // ES
        { 0xFFFF, RegType_Int16, RegX86_CS, 16, 0, offsetof( CONTEXT, SegCs ), 4 },    // CS
        { 0xFFFF, RegType_Int16, RegX86_SS, 16, 0, offsetof( CONTEXT, SegSs ), 4 },    // SS
        { 0xFFFF, RegType_Int16, RegX86_DS, 16, 0, offsetof( CONTEXT, SegDs ), 4 },    // DS
        { 0xFFFF, RegType_Int16, RegX86_FS, 16, 0, offsetof( CONTEXT, SegFs ), 4 },    // FS
        { 0xFFFF, RegType_Int16, RegX86_GS, 16, 0, offsetof( CONTEXT, SegGs ), 4 },    // GS
        { 0xFFFF, RegType_Int16, RegX86_EIP, 16, 0, 0, 0 },    // IP
        { 0xFFFF, RegType_Int16, RegX86_EFLAGS, 16, 0, 0, 0 },    // FLAGS
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Eip ), 4 },    // EIP
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, EFlags ), 4 },    // EFLAGS

        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Dr0 ), 4 },    // DR0
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Dr1 ), 4 },    // DR1
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Dr2 ), 4 },    // DR2
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Dr3 ), 4 },    // DR3
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Dr6 ), 4 },    // DR6
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, Dr7 ), 4 },    // DR7

        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT, FloatSave.RegisterArea[ 0 * 10 ] ), 10 },  // ST0
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT, FloatSave.RegisterArea[ 1 * 10 ] ), 10 },  // ST1
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT, FloatSave.RegisterArea[ 2 * 10 ] ), 10 },  // ST2
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT, FloatSave.RegisterArea[ 3 * 10 ] ), 10 },  // ST3
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT, FloatSave.RegisterArea[ 4 * 10 ] ), 10 },  // ST4
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT, FloatSave.RegisterArea[ 5 * 10 ] ), 10 },  // ST5
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT, FloatSave.RegisterArea[ 6 * 10 ] ), 10 },  // ST6
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT, FloatSave.RegisterArea[ 7 * 10 ] ), 10 },  // ST7
        { 0xFFFF, RegType_Int16, RegX86_CTRL, 16, 0, offsetof( CONTEXT, FloatSave.ControlWord ), 4 },   // CTRL
        { 0xFFFF, RegType_Int16, RegX86_STAT, 16, 0, offsetof( CONTEXT, FloatSave.StatusWord ), 4 },    // STAT
        { 0xFFFF, RegType_Int16, RegX86_TAG, 16, 0, offsetof( CONTEXT, FloatSave.TagWord ), 4 },        // TAG
        { 0xFFFF, RegType_Int16, RegX86_FPEIP, 16, 0, offsetof( CONTEXT, FloatSave.ErrorOffset ), 4 }, // FPIP
        { 0xFFFF, RegType_Int16, RegX86_FPCS, 16, 0, offsetof( CONTEXT, FloatSave.ErrorSelector ), 4 },   // FPCS
        { 0xFFFF, RegType_Int16, RegX86_FPEDO, 16, 0, offsetof( CONTEXT, FloatSave.DataOffset ), 4 },    // FPDO
        { 0xFFFF, RegType_Int16, RegX86_FPDS, 16, 0, offsetof( CONTEXT, FloatSave.DataSelector ), 4 },  // FPDS
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, FloatSave.ErrorOffset ), 4 }, // FPEIP
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT, FloatSave.DataOffset ), 4 },   // FPEDO

        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM0
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM1
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM2
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM3
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM4
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM5
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM6
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM7

        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM0
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM1
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM2
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM3
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM4
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM5
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM6
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM7

        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM00
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM01
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM02
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM03
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM10
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM11
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM12
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM13
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM20
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM21
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM22
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM23
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM30
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM31
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM32
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM33
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM40
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM41
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM42
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM43
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM50
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM51
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM52
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM53
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM60
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM61
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM62
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM63
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM70
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM71
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM72
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM73

        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM0L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM1L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM2L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM3L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM4L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM5L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM6L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM7L

        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM0H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM1H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM2H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM3H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM4H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM5H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM6H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // XMM7H

        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MXCSR

        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM0L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM1L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM2L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM3L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM4L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM5L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM6L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM7L

        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM0H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM1H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM2H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM3H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM4H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM5H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM6H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM7H

        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM00
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM01
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM10
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM11
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM20
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM21
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM30
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM31
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM40
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM41
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM50
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM51
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM60
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM61
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM70
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM71
    };

    C_ASSERT( _countof( gRegDesc ) == 155 );


    static bool IsInteger( RegisterType type )
    {
        if ( (type == RegType_Int8)
            || (type == RegType_Int16)
            || (type == RegType_Int32)
            || (type == RegType_Int64) )
            return true;

        return false;
    }

    static bool IsFloat( RegisterType type )
    {
        if ( (type == RegType_Float32) 
            || (type == RegType_Float64) 
            || (type == RegType_Float80) )
            return true;

        return false;
    }

    static uint64_t ReadInteger( const CONTEXT& context, uint32_t offset, uint32_t size )
    {
        _ASSERT( (offset + size) <= sizeof context );
        if ( (offset + size) > sizeof context )
            return 0;

        BYTE*       bytes = (BYTE*) &context;

        switch ( size )
        {
        case 1: return *(uint8_t*) (bytes + offset);
        case 2: return *(uint16_t*) (bytes + offset);
        case 4: return *(uint32_t*) (bytes + offset);
        case 8: return *(uint64_t*) (bytes + offset);
        }

        _ASSERT( false );
        return 0;
    }

    static void WriteInteger( uint64_t val, CONTEXT& context, uint32_t offset, uint32_t size )
    {
        _ASSERT( (offset + size) <= sizeof context );
        if ( (offset + size) > sizeof context )
            return;

        BYTE*       bytes = (BYTE*) &context;

        switch ( size )
        {
        case 1: *(uint8_t*) (bytes + offset) = (uint8_t) val; break;
        case 2: *(uint16_t*) (bytes + offset) = (uint16_t) val;    break;
        case 4: *(uint32_t*) (bytes + offset) = (uint32_t) val;    break;
        case 8: *(uint64_t*) (bytes + offset) = val;    break;
        default:    _ASSERT( false );   break;
        }
    }


    uint64_t    RegisterValue::GetInt() const
    {
        uint64_t    n = 0;

        switch ( Type )
        {
        case RegType_Int8:  n = this->Value.I8;  break;
        case RegType_Int16: n = this->Value.I16; break;
        case RegType_Int32: n = this->Value.I32; break;
        case RegType_Int64: n = this->Value.I64; break;
        default:
            _ASSERT( false );
            break;
        }

        return n;
    }

    void        RegisterValue::SetInt( uint64_t n )
    {
        switch ( Type )
        {
        case RegType_Int8:  this->Value.I8 = (uint8_t) n;   break;
        case RegType_Int16: this->Value.I16 = (uint16_t) n; break;
        case RegType_Int32: this->Value.I32 = (uint32_t) n; break;
        case RegType_Int64: this->Value.I64 = (uint64_t) n; break;
        default:
            _ASSERT( false );
            break;
        }
    }

    RegisterType GetRegisterType( uint32_t regId )
    {
        if ( regId >= _countof( gRegDesc ) )
            return RegType_None;

        return (RegisterType) gRegDesc[regId].Type;
    }


    //------------------------------------------------------------------------
    //  RegisterSet
    //------------------------------------------------------------------------

    RegisterSet::RegisterSet( 
        const CONTEXT& context, 
        ::Thread* coreThread )
        :   mRefCount( 0 ),
            mContext( context ),
            mCoreThread( coreThread )
    {
        _ASSERT( (context.ContextFlags & CONTEXT_FULL) == CONTEXT_FULL );
        _ASSERT( coreThread != NULL );
    }

    void RegisterSet::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void RegisterSet::Release()
    {
        long    newRef = InterlockedDecrement( &mRefCount );
        _ASSERT( newRef >= 0 );
        if ( newRef == 0 )
            delete this;
    }

    HRESULT RegisterSet::GetValue( uint32_t regId, RegisterValue& value )
    {
        if ( regId >= _countof( gRegDesc ) )
            return E_INVALIDARG;

        const RegisterDesc& regDesc = gRegDesc[regId];
        if ( regDesc.Type == RegType_None )
            return E_FAIL;

        if ( IsInteger( (RegisterType) regDesc.Type ) && (regDesc.ParentRegId != 0) )
        {
            const RegisterDesc& parentRegDesc = gRegDesc[regDesc.ParentRegId];
            uint64_t    n = 0;

            n = ReadInt( (uint8_t*) &mContext, parentRegDesc.ContextOffset, parentRegDesc.ContextSize, false );

            n = (n >> regDesc.SubregOffset) & regDesc.SubregMask;

            switch ( regDesc.Type )
            {
            case RegType_Int8:  value.Value.I8 = (uint8_t) n; break;
            case RegType_Int16: value.Value.I16 = (uint16_t) n; break;
            case RegType_Int32: value.Value.I32 = (uint32_t) n; break;
            case RegType_Int64: value.Value.I64 = n; break;
            default:    _ASSERT( false );   break;
            }
        }
        else
        {
            _ASSERT( (regDesc.ContextOffset + regDesc.ContextSize) <= sizeof mContext );
            BYTE*   bytes = (BYTE*) &mContext;
            memcpy( &value.Value, bytes + regDesc.ContextOffset, regDesc.ContextSize );
        }

        value.Type = (RegisterType) regDesc.Type;

        return S_OK;
    }

    HRESULT RegisterSet::SetValue( uint32_t regId, const RegisterValue& value )
    {
        BOOL        bRet = FALSE;
        BYTE        backup[ sizeof( RegisterValue ) ] = { 0 };
        uint32_t    backupOffset = 0;
        uint32_t    backupSize = 0;

        if ( regId >= _countof( gRegDesc ) )
            return E_INVALIDARG;

        const RegisterDesc& regDesc = gRegDesc[regId];
        if ( regDesc.Type == RegType_None )
            return E_FAIL;

        if ( value.Type != regDesc.Type )
            return E_INVALIDARG;

        if ( IsInteger( (RegisterType) regDesc.Type ) && (regDesc.ParentRegId != 0) )
        {
            const RegisterDesc& parentRegDesc = gRegDesc[regDesc.ParentRegId];
            uint64_t    shiftedMask = regDesc.SubregMask << regDesc.SubregOffset;
            uint64_t    oldN = 0;
            uint64_t    newN = 0;

            newN = value.GetInt();

            oldN = ReadInt( 
                (uint8_t*) &mContext, 
                parentRegDesc.ContextOffset, 
                parentRegDesc.ContextSize, 
                false );

            newN = (oldN & ~shiftedMask) | ((newN << regDesc.SubregOffset) & shiftedMask);

            backupOffset = parentRegDesc.ContextOffset;
            backupSize = parentRegDesc.ContextSize;

            memcpy( backup, (BYTE*) &mContext + backupOffset, backupSize );
            WriteInteger( newN, mContext, parentRegDesc.ContextOffset, parentRegDesc.ContextSize );
        }
        else
        {
            _ASSERT( (regDesc.ContextOffset + regDesc.ContextSize) <= sizeof mContext );
            BYTE*   bytes = (BYTE*) &mContext;

            backupOffset = regDesc.ContextOffset;
            backupSize = regDesc.ContextSize;

            memcpy( backup, bytes + regDesc.ContextOffset, regDesc.ContextSize );
            memcpy( bytes + regDesc.ContextOffset, &value.Value, regDesc.ContextSize );
        }

        bRet = SetThreadContext( mCoreThread->GetHandle(), &mContext );
        if ( !bRet )
        {
            memcpy( (BYTE*) &mContext + backupOffset, backup, backupSize );
            return GetLastHr();
        }

        return S_OK;
    }

    HRESULT RegisterSet::IsReadOnly( uint32_t regId, bool& readOnly )
    {
        if ( regId >= _countof( gRegDesc ) )
            return E_INVALIDARG;

        readOnly = false;
        return S_OK;
    }


    //------------------------------------------------------------------------
    //  TinyRegisterSet
    //------------------------------------------------------------------------

    TinyRegisterSet::TinyRegisterSet( 
        Address eip,
        Address esp,
        Address ebp )
        :   mRefCount( 0 ),
            mEip( eip ),
            mEsp( esp ),
            mEbp( ebp )
    {
    }

    void TinyRegisterSet::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void TinyRegisterSet::Release()
    {
        long    newRef = InterlockedDecrement( &mRefCount );
        _ASSERT( newRef >= 0 );
        if ( newRef == 0 )
            delete this;
    }

    HRESULT TinyRegisterSet::GetValue( uint32_t regId, RegisterValue& value )
    {
        if ( regId >= _countof( gRegDesc ) )
            return E_INVALIDARG;

        switch ( regId )
        {
        case RegX86_EIP:
            value.Value.I32 = mEip;
            value.Type = RegType_Int32;
            break;

        case RegX86_IP:
            value.Value.I16 = (uint16_t) mEip;
            value.Type = RegType_Int16;
            break;

        case RegX86_ESP:
            value.Value.I32 = mEsp;
            value.Type = RegType_Int32;
            break;

        case RegX86_SP:
            value.Value.I16 = (uint16_t) mEsp;
            value.Type = RegType_Int16;
            break;

        case RegX86_EBP:
            value.Value.I32 = mEbp;
            value.Type = RegType_Int32;
            break;

        case RegX86_BP:
            value.Value.I16 = (uint16_t) mEbp;
            value.Type = RegType_Int16;
            break;

        default:
            if ( gRegDesc[regId].Type == RegType_None )
                return E_FAIL;

            memset( &value.Value, 0, sizeof value.Value );
            value.Type = (RegisterType) gRegDesc[regId].Type;
            return S_FALSE;
        }

        return S_OK;
    }

    HRESULT TinyRegisterSet::SetValue( uint32_t regId, const RegisterValue& value )
    {
        return E_NOTIMPL;
    }

    HRESULT TinyRegisterSet::IsReadOnly( uint32_t regId, bool& readOnly )
    {
        if ( regId >= _countof( gRegDesc ) )
            return E_INVALIDARG;

        readOnly = true;
        return S_OK;
    }
}
