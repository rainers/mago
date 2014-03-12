/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    //  Document Tracker
    //
    //  Keeps track of document and line information for a changing code 
    //  address. This is used for disassembly.

    class DocTracker
    {
        struct LineEntry
        {
            uint16_t    LineNumber;

            bool operator<( const LineEntry& other ) const
            {
                return LineNumber < other.LineNumber;
            }
        };

        struct DocData
        {
            bool            mHasLineInfo;
            RefPtr<Module>  mMod;
            uint16_t        mCompilandIndex;
            uint16_t        mFileIndex;

            DocData();
            bool IsValid();
        };

        RefPtr<Program>         mProg;

        DocData                 mCurData;
        DocData                 mOldData;

        MagoST::FileSegmentInfo mFileSeg;
        uint16_t                mLineIndex;
        uint32_t                mByteOffset;

        // the sorted lines for the current segment
        // the lines are stored in the debug info sorted by offset, but we need
        // to quickly find a line by its line number, so we have to sort them

        std::vector<LineEntry>  mLines;

    public:
        DocTracker();

        void Init( Program* program );

        void Update( Address64 address );

        // Returns true if the last update changed the document. This includes 
        // changes to and from no document. Documents with the same name but in
        // different compilands or modules are treated as different documents.

        bool DocChanged();

        // Current information for the address from the last Update call
        // These pieces are only valid if HasLineInfo is true

        bool HasLineInfo();
        uint32_t GetByteOffset();
        uint16_t GetLine();
        uint16_t GetLineEnd();          // line = lineEnd if only 1 line
        BSTR GetFilename();

    private:
        bool IsValid();

        void UpdateLine( uint32_t curOffset, uint16_t lineIndex );
        void UpdateLine( MagoST::ISession* session, uint32_t curOffset, const MagoST::LineNumber& lineNum );
        void UpdateLine( MagoST::ISession* session, uint32_t curOffset, const MagoST::LineNumber& lineNum, Module* mod );
        bool FindLineInSegment( uint16_t section, uint32_t offset, uint16_t& lineIndex );
    };
}
