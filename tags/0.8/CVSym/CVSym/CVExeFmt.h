/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.

   Purpose: format of CodeView information in exe files
*/

#pragma once

// All of the structures described below must start on a long word boundary
// to maintain natural alignment. Pad space can be inserted during the
// write operation and the addresses adjusted without affecting the contents
// of the structures.


// Type of subsection entry.

#define sstModule       0x120
#define sstTypes        0x121
#define sstPublic       0x122
#define sstPublicSym    0x123 // publics as symbol (waiting for link)
#define sstSymbols      0x124
#define sstAlignSym     0x125
#define sstSrcLnSeg     0x126 // because link doesn't emit SrcModule
#define sstSrcModule    0x127
#define sstLibraries    0x128
#define sstGlobalSym    0x129
#define sstGlobalPub    0x12a
#define sstGlobalTypes  0x12b
#define sstMPC          0x12c
#define sstSegMap       0x12d
#define sstSegName      0x12e
#define sstPreComp      0x12f // precompiled types
#define sstPreCompMap   0x130 // map precompiled types in global types
#define sstOffsetMap16  0x131
#define sstOffsetMap32  0x132
#define sstFileIndex    0x133 // Index of file names
#define sstStaticSym    0x134


typedef enum OMFHash
{
    OMFHASH_NONE, // no hashing
    OMFHASH_SUMUC16, // upper case sum of chars in 16 bit table
    OMFHASH_SUMUC32, // upper case sum of chars in 32 bit table
    OMFHASH_ADDR16, // sorted by increasing address in 16 bit table
    OMFHASH_ADDR32 // sorted by increasing address in 32 bit table
} OMFHASH;


// CodeView Debug OMF signature. The signature at the end of the file is
// a negative offset from the end of the file to another signature. At
// the negative offset (base address) is another signature whose filepos
// field points to the first OMFDirHeader in a chain of directories.
// The NB05 signature is used by the link utility to indicated a completely
// unpacked file. The NB06 signature is used by ilink to indicate that the
// executable has had CodeView information from an incremental link appended
// to the executable. The NB08 signature is used by cvpack to indicate that
// the CodeView Debug OMF has been packed. CodeView will only process
// executables with the NB08 signature.

typedef struct OMFSignature
{
    char            Signature[4];  // "NBxx"
    long            filepos;       // offset in file
} OMFSignature;


// directory information structure
// This structure contains the information describing the directory.
// It is pointed to by the signature at the base address or the directory
// link field of a preceeding directory. The directory entries immediately
// follow this structure.

typedef struct OMFDirHeader
{
    unsigned short  cbDirHeader;    // length of this structure
    unsigned short  cbDirEntry;     // number of bytes in each directory entry
    unsigned long   cDir;           // number of directorie entries
    long            lfoNextDir;     // offset from base of next directory
    unsigned long flags;            // status flags
} OMFDirHeader;


// directory structure
// The data in this structure is used to reference the data for each
// subsection of the CodeView Debug OMF information. Tables that are
// not associated with a specific module will have a module index of
// oxffff. These tables are the global types table, the global symbol
// table, the global public table and the library table.

typedef struct OMFDirEntry
{
    unsigned short  SubSection; // subsection type (sst...)
    unsigned short  iMod;       // module index
    long            lfo;        // large file offset of subsection
    unsigned long   cb;         // number of bytes in subsection
} OMFDirEntry;


// Structures contained in the sstModule subsection


// information decribing each segment in a module

typedef struct OMFSegDesc
{
    unsigned short Seg; // segment index
    unsigned short pad; // pad to maintain alignment
    unsigned long Off; // offset of code in segment
    unsigned long cbSeg; // number of bytes in segment
} OMFSegDesc;


// per module information
// There is one of these subsection entries for each module
// in the executable. The entry is generated by link/ilink.
// This table will probably require padding because of the
// variable length module name.

typedef struct OMFModule
{
    unsigned short  ovlNumber; // overlay number
    unsigned short  iLib; // library that the module was linked from
    unsigned short  cSeg; // count of number of segments in module
    char            Style[2]; // debugging style "CV"
    //OMFSegDesc      SegInfo[1]; // describes segments in module
    //char            Name[1]; // length prefixed module name padded to
    // long word boundary
} OMFModule;


// Structures contained in the sstGlobalSym and sstGlobalPub subsections


// Symbol hash table format
// This structure immediately preceeds the global publics table
// and global symbol tables.

typedef struct OMFSymHash
{
    unsigned short  symhash;    // symbol hash function index
    unsigned short  addrhash;   // address hash function index
    unsigned long   cbSymbol;   // length of symbol information
    unsigned long   cbHSym;     // length of symbol hash data
    unsigned long   cbHAddr;    // length of address hashdata
} OMFSymHash;


// Structures contained in the sstGlobalTypes subsection


// Global types subsection format
// This structure immediately preceeds the global types table.
// The offsets in the typeOffset array are relative to the address
// of ctypes. Each type entry following the typeOffset array must
// begin on a long word boundary.

typedef struct OMFGlobalTypes
{
    unsigned long   flags;
    //OMFTypeFlags flags;
    unsigned long   cTypes; // number of types
    //unsigned long   typeOffset[1]; // array of offsets to types
} OMFGlobalTypes;


// Structures contained in the sstSrcModule subsection


// Source line to address mapping table.
// This table is generated by the link/ilink utility from line number
// information contained in the object file OMF data. This table contains
// only the code contribution for one segment from one source file.

typedef struct OMFSourceLine
{
    unsigned short  Seg;            // linker segment index
    unsigned short  cLnOff;         // count of line/offset pairs
    unsigned long   offset[1];      // array of offsets in segment
    unsigned short  lineNbr[1];     // array of line lumber in source
} OMFSourceLine;


// Source file description
// This table is generated by the linker

typedef struct OMFSourceFile
{
    unsigned short  cSeg;           // number of segments from source file
    unsigned short  reserved;
    unsigned long   baseSrcLn[1];   // base of OMFSourceLine tables
    // this array is followed by array
    // of segment start/end pairs followed by
    // an array of linker indices
    // for each segment in the file
    unsigned short  cFName;         // length of source file name
    char            Name[1];        // name of file padded to long boundary
} OMFSourceFile;


// Source line to address mapping header structure
// This structure describes the number and location of the
// OMFAddrLine tables for a module. The offSrcLine entries are
// relative to the beginning of this structure.

typedef struct OMFSourceModule
{
    unsigned short  cFile;          // number of OMFSourceTables
    unsigned short  cSeg;           // number of segments in module
    unsigned long   baseSrcFile[1]; // base of OMFSourceFile table
    // this array is followed by array
    // of segment start/end pairs followed
    // by an array of linker indices
    // for each segment in the module
} OMFSourceModule;
