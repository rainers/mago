/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    struct RegGroupInternal;


    void GetX86RegisterGroups( const RegGroupInternal*& groups, uint32_t& count );
}
