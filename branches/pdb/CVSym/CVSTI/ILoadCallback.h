/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoST
{
    class ILoadCallback
    {
    public:
        virtual void AddRef() = 0;
        virtual void Release() = 0;

        virtual HRESULT NotifyDebugDir ( 
           bool  fExecutable,
           DWORD cbData,
           BYTE  data[]
        ) = 0;

        virtual HRESULT NotifyOpenDBG ( 
           const wchar_t* dbgPath,
           HRESULT        resultCode
        ) = 0;
    };
}
