/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


struct DataDirInfo
{
    uint32_t                FileOffset;
    uint32_t                Size;
    IMAGE_SECTION_HEADER*   SectionHeader;
};
