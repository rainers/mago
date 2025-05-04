/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "ComEnumWithCount.h"


namespace Mago
{
    class FrameInfoArray
    {
        FRAMEINFO*  mArray;
        size_t      mLen;

    public:
        FrameInfoArray( size_t length );
        ~FrameInfoArray();

        size_t GetLength() const;
        FRAMEINFO& operator[]( size_t i ) const;
        FRAMEINFO* Get() const;
        FRAMEINFO* Detach();

    private:
        FrameInfoArray( const FrameInfoArray& );
        FrameInfoArray& operator=( const FrameInfoArray& );
    };

    class _CopyFrameInfo
    {
    public:
        static HRESULT copy( FRAMEINFO* dest, const FRAMEINFO* source );
        static void init( FRAMEINFO* p );
        static void destroy( FRAMEINFO* p );
    };

    typedef CComEnumWithCount< 
        IEnumDebugFrameInfo2, 
        &IID_IEnumDebugFrameInfo2, 
        FRAMEINFO, 
        _CopyFrameInfo, 
        CComMultiThreadModel
    > EnumDebugFrameInfo;

    typedef ScopedStruct<FRAMEINFO, _CopyFrameInfo> FrameInfo;

}
