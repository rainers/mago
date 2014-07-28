/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "BinUtil.h"
#include <limits.h>


IMAGE_SECTION_HEADER* FindSection( DWORD virtAddr, WORD secCount, IMAGE_SECTION_HEADER* secHeaders )
{
    _ASSERT( secHeaders != NULL );

    if ( secCount == USHRT_MAX )
        return NULL;

    // we have an RVA, do we need to convert it to a file offset/address?
    // look for the section the RVA is in, to get the section's file addr

    for ( WORD i = 0; i < secCount; i++ )
    {
        DWORD   secSize = secHeaders[i].SizeOfRawData;

        if ( secHeaders[i].Misc.VirtualSize > 0 )
            secSize = secHeaders[i].Misc.VirtualSize;

        DWORD   secLimit = secHeaders[i].VirtualAddress + secSize;

        if ( (virtAddr >= secHeaders[i].VirtualAddress)
            && (virtAddr < secLimit) )
        {
            return &secHeaders[i];
        }
    }

    return NULL;
}


bool FindDataDirectoryData( 
    WORD dirCount,
    IMAGE_DATA_DIRECTORY* dataDirs,
    WORD secCount,
    IMAGE_SECTION_HEADER* secHeaders,
    bool mappedAsImage,
    WORD dirEntryId, 
    DataDirInfo& dirInfo )
{
    if ( dirEntryId >= dirCount )
        return false;

    IMAGE_DATA_DIRECTORY*   dataDir = &dataDirs[dirEntryId];
    uint32_t                offset = 0;
    IMAGE_SECTION_HEADER*   targetSec = NULL;

    if ( (dataDir->Size == 0) || (dataDir->VirtualAddress == 0) )
        return false;

    targetSec = FindSection( dataDir->VirtualAddress, secCount, secHeaders );
    if ( targetSec == NULL )
        return false;

    if ( mappedAsImage )
    {
        offset = dataDir->VirtualAddress;
    }
    else
    {
        int32_t rvaFileDiff = targetSec->VirtualAddress - targetSec->PointerToRawData;
        offset = dataDir->VirtualAddress - rvaFileDiff;
    }

    dirInfo.Size = dataDir->Size;
    dirInfo.SectionHeader = targetSec;
    dirInfo.FileOffset = offset;
    return true;
}
