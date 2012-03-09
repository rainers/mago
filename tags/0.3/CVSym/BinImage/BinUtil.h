/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


bool FindDataDirectoryData( 
    WORD dirCount,
    IMAGE_DATA_DIRECTORY* dataDirs,
    WORD secCount,
    IMAGE_SECTION_HEADER* secHeaders,
    bool mappedAsImage,
    WORD dirEntryId, 
    DataDirInfo& dirInfo );
