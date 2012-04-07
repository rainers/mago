/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "SharedString.h"
#include <atlbase.h>


namespace MagoEE
{
    SharedString::SharedString( wchar_t* str, uint32_t capacity )
        :   mRefCount( 0 ),
            mBuf( str ),
            mCapacity( capacity ),
            mLen( 0 ),
            mCutLen( 0 ),
            mLastCutChar( L'\0' )
    {
        _ASSERT( str != NULL );
        _ASSERT( capacity != 0 );
    }

    SharedString::~SharedString()
    {
        delete [] mBuf;
    }

    void SharedString::AddRef()
    {
        mRefCount++;
    }

    void SharedString::Release()
    {
        mRefCount--;
        _ASSERT( mRefCount >= 0 );
        if ( mRefCount == 0 )
        {
            delete this;
        }
    }

    uint32_t SharedString::GetCapacity()
    {
        return mCapacity;
    }

    uint32_t SharedString::GetLength()
    {
        return mLen;
    }

    bool SharedString::Append( const wchar_t* str )
    {
        _ASSERT( str != NULL );
        if ( str == NULL )
            return false;

        _ASSERT( mCutLen == 0 );
        if ( mCutLen != 0 )
            return false;

        size_t  len = wcslen( str );

        _ASSERT( ((mLen + len) <= mCapacity) && ((mLen + len) >= mLen) );
        if ( ((mLen + len) > mCapacity) || ((mLen + len) < mLen) )
            return false;

        // add one for space for nul-char
        // wcscpy_s puts a nul-char at the end
        wcscpy_s( mBuf + mLen, (mCapacity - mLen) + 1, str );
        mLen += len;

        return true;
    }

    const wchar_t* SharedString::GetCut( uint32_t len )
    {
        _ASSERT( (len <= mLen) && (len != 0)  && (mCutLen == 0) );

        if ( (len > mLen) || (len == 0)  || (mCutLen != 0) )
            return NULL;

        mCutLen = len;
        mLastCutChar = mBuf[ len ];
        mBuf[ len ] = L'\0';

        return mBuf;
    }

    void SharedString::ReleaseCut()
    {
        if ( mCutLen == 0 )
            return;

        mBuf[ mCutLen ] = mLastCutChar;
        mCutLen = 0;
    }

    bool SharedString::Make( uint32_t capacity, RefPtr<SharedString>& sharedStr )
    {
        CAutoVectorPtr<wchar_t>     str;

        if ( !str.Allocate( capacity + 1 ) )
            return false;

        sharedStr = new SharedString( str, capacity );
        if ( sharedStr == NULL )
            return false;

        str.Detach();

        return true;
    }
}
