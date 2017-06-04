/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "RegisterSet.h"


namespace Mago
{
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

    static void WriteInteger( uint64_t val, void* context, uint32_t offset, uint32_t size )
    {
        BYTE*       bytes = (BYTE*) context;

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


    //------------------------------------------------------------------------
    //  RegisterSet
    //------------------------------------------------------------------------

    RegisterSet::RegisterSet( 
        const RegisterDesc* regDesc,
        uint16_t regCount,
        uint16_t pcId )
        :   mRefCount( 0 ),
            mRegDesc( regDesc ),
            mRegCount( regCount ),
            mContextSize( 0 ),
            mPCId( pcId )
    {
        _ASSERT( regDesc != NULL );
        _ASSERT( regCount > 0 );
        _ASSERT( pcId < regCount );
    }

    HRESULT RegisterSet::Init( 
        const void* context,
        uint32_t contextSize )
    {
        _ASSERT( context != NULL );
        _ASSERT( contextSize > 0 );
        if ( context == NULL || contextSize == 0 )
            return E_INVALIDARG;

        mContextBuf.Attach( new BYTE[contextSize] );
        if ( mContextBuf.Get() == NULL )
            return E_OUTOFMEMORY;

        mContextSize = (uint16_t) contextSize;
        memcpy( mContextBuf.Get(), context, contextSize );
        return S_OK;
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
        if ( regId >= mRegCount )
            return E_INVALIDARG;

        const RegisterDesc& regDesc = mRegDesc[regId];
        if ( regDesc.Type == RegType_None )
            return E_FAIL;

        if ( IsInteger( (RegisterType) regDesc.Type ) && (regDesc.ParentRegId != 0) )
        {
            const RegisterDesc& parentRegDesc = mRegDesc[regDesc.ParentRegId];
            uint64_t    n = 0;

            n = ReadInt( mContextBuf.Get(), parentRegDesc.ContextOffset, parentRegDesc.ContextSize, false );

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
            _ASSERT( (uint32_t) (regDesc.ContextOffset + regDesc.ContextSize) <= mContextSize );
            BYTE*   bytes = mContextBuf.Get();
            memcpy( &value.Value, bytes + regDesc.ContextOffset, regDesc.ContextSize );
        }

        value.Type = (RegisterType) regDesc.Type;

        return S_OK;
    }

    HRESULT RegisterSet::SetValue( uint32_t regId, const RegisterValue& value )
    {
        if ( regId >= mRegCount )
            return E_INVALIDARG;

        const RegisterDesc& regDesc = mRegDesc[regId];
        if ( regDesc.Type == RegType_None )
            return E_FAIL;

        if ( value.Type != regDesc.Type )
            return E_INVALIDARG;

        if ( IsInteger( (RegisterType) regDesc.Type ) && (regDesc.ParentRegId != 0) )
        {
            const RegisterDesc& parentRegDesc = mRegDesc[regDesc.ParentRegId];
            uint64_t    shiftedMask = regDesc.SubregMask << regDesc.SubregOffset;
            uint64_t    oldN = 0;
            uint64_t    newN = 0;

            newN = value.GetInt();

            oldN = ReadInt( 
                mContextBuf.Get(), 
                parentRegDesc.ContextOffset, 
                parentRegDesc.ContextSize, 
                false );

            newN = (oldN & ~shiftedMask) | ((newN << regDesc.SubregOffset) & shiftedMask);

            _ASSERT( (uint32_t) (parentRegDesc.ContextOffset + parentRegDesc.ContextSize) <= mContextSize );
            WriteInteger( newN, mContextBuf.Get(), parentRegDesc.ContextOffset, parentRegDesc.ContextSize );
        }
        else
        {
            _ASSERT( (uint32_t) (regDesc.ContextOffset + regDesc.ContextSize) <= mContextSize );
            BYTE*   bytes = mContextBuf.Get();

            memcpy( bytes + regDesc.ContextOffset, &value.Value, regDesc.ContextSize );
        }

        return S_OK;
    }

    HRESULT RegisterSet::IsReadOnly( uint32_t regId, bool& readOnly )
    {
        if ( regId >= mRegCount )
            return E_INVALIDARG;

        readOnly = false;
        return S_OK;
    }

    bool RegisterSet::GetThreadContext( const void*& context, uint32_t& contextSize )
    {
        context = mContextBuf.Get();
        contextSize = mContextSize;
        return true;
    }

    RegisterType RegisterSet::GetRegisterType( uint32_t regId )
    {
        if ( regId >= mRegCount )
            return RegType_None;

        return (RegisterType) mRegDesc[regId].Type;
    }

    bool RegisterSet::is64Bit()
    {
        return mRegDesc[mPCId].Type == RegType_Int64;
    }

    uint64_t RegisterSet::GetPC()
    {
        RegisterValue regVal = { 0 };
        GetValue( mPCId, regVal );
        return regVal.GetInt();
    }

    HRESULT RegisterSet::SetPC( uint64_t addr )
    {
        RegisterValue regVal = { 0 };
        GetValue( mPCId, regVal ); // to grab type
        regVal.SetInt( addr );
        return SetValue( mPCId, regVal );
    }


    //------------------------------------------------------------------------
    //  TinyRegisterSet
    //------------------------------------------------------------------------

    TinyRegisterSet::TinyRegisterSet( 
        const RegisterDesc* regDesc,
        uint32_t regCount,
        uint16_t pcId,
        uint16_t stackId,
        uint16_t frameId,
        Address64 pc,
        Address64 stack,
        Address64 frame )
        :   mRefCount( 0 ),
            mRegDesc( regDesc ),
            mRegCount( regCount ),
            mPC( pc ),
            mStack( stack ),
            mFrame( frame ),
            mPCId( pcId ),
            mStackId( stackId ),
            mFrameId( frameId )
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
        if ( regId >= mRegCount )
            return E_INVALIDARG;

        value.Type = (RegisterType) mRegDesc[regId].Type;

        if ( regId == mPCId )
        {
            value.SetInt( mPC );
        }
        else if ( regId == mStackId )
        {
            value.SetInt( mStack );
        }
        else if ( regId == mFrameId )
        {
            value.SetInt( mFrame );
        }
        else
        {
            if ( mRegDesc[regId].Type == RegType_None )
                return E_FAIL;

            memset( &value.Value, 0, sizeof value.Value );
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
        if ( regId >= mRegCount )
            return E_INVALIDARG;

        readOnly = true;
        return S_OK;
    }

    uint64_t TinyRegisterSet::GetPC()
    {
        return mPC;
    }

    HRESULT TinyRegisterSet::SetPC(uint64_t addr)
    {
        return E_NOTIMPL;
    }

    bool TinyRegisterSet::GetThreadContext( const void*& context, uint32_t& contextSize )
    {
        return false;
    }

    RegisterType TinyRegisterSet::GetRegisterType( uint32_t regId )
    {
        if ( regId >= mRegCount )
            return RegType_None;

        return (RegisterType) mRegDesc[regId].Type;
    }

    bool TinyRegisterSet::is64Bit()
    {
        return mRegDesc[mPCId].Type == RegType_Int64;
    }

}
