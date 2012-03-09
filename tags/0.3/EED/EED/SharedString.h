/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoEE
{
    // A reference counted wide string that supports taking in-place substrings of.
    // This class is not multithread safe.

    class SharedString
    {
        long        mRefCount;
        wchar_t*    mBuf;
        uint32_t    mCapacity;
        uint32_t    mLen;
        uint32_t    mCutLen;
        wchar_t     mLastCutChar;

    public:
        ~SharedString();

        // lengths and capacities below do not include the nul-char

        void AddRef();
        void Release();

        uint32_t GetCapacity();
        uint32_t GetLength();

        bool Append( const wchar_t* str );

        const wchar_t* GetCut( uint32_t len );
        void ReleaseCut();

        static bool Make( uint32_t capacity, RefPtr<SharedString>& str );

    private:
        SharedString( wchar_t* buf, uint32_t capacity );
    };
}
