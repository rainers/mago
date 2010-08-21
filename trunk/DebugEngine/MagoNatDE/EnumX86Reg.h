/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class IRegisterSet;


    HRESULT EnumX86Registers( 
        IRegisterSet* regSet, 
        DEBUGPROP_INFO_FLAGS fields,
        DWORD radix,
        IEnumDebugPropertyInfo2** enumerator );
}
