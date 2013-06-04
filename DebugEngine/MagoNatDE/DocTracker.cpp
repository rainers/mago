/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DocTracker.h"
#include "Program.h"
#include "Module.h"
#include <algorithm>


namespace Mago
{
    DocTracker::DocData::DocData()
        :   mHasLineInfo( false ),
            mCompilandIndex( 0 ),
            mFileIndex( 0 )
    {
    }

    bool DocTracker::DocData::IsValid()
    {
        if ( mHasLineInfo )
        {
            if ( (mMod == NULL) || (mCompilandIndex == 0) )
                return false;
        }

        return true;
    }

    DocTracker::DocTracker()
        :   mLineIndex( 0 ),
            mByteOffset( 0 )
    {
        memset( &mFileSeg, 0, sizeof mFileSeg );
    }

    void DocTracker::Init( Program* program )
    {
        _ASSERT( program != NULL );
        mProg = program;
    }

    bool DocTracker::HasLineInfo()
    {
        _ASSERT( IsValid() );
        return mCurData.mHasLineInfo;
    }

    uint32_t DocTracker::GetByteOffset()
    {
        _ASSERT( IsValid() );
        return mByteOffset;
    }

    uint16_t DocTracker::GetLine()
    {
        _ASSERT( IsValid() );

        if ( !mCurData.mHasLineInfo )
            return 0;

        uint16_t    lineEnd = mFileSeg.LineNumbers[ mLineIndex ];
        LineEntry   targetEntry = { lineEnd };

        // do a binary search to find the end line, which we know ...
        std::vector<LineEntry>::iterator it = std::lower_bound( mLines.begin(), mLines.end(), targetEntry );

        if ( it == mLines.end() )
            return 0;
        if ( it == mLines.begin() )
            return 1;

        // ... then, take it back one entry, so we can get the start line in the range
        it--;
        return it->LineNumber + 1;
    }

    uint16_t DocTracker::GetLineEnd()
    {
        _ASSERT( IsValid() );

        if ( !mCurData.mHasLineInfo )
            return 0;

        return mFileSeg.LineNumbers[ mLineIndex ];
    }

    BSTR DocTracker::GetFilename()
    {
        _ASSERT( IsValid() );

        HRESULT                     hr = S_OK;
        RefPtr<MagoST::ISession>    session;
        MagoST::FileInfo            info = { 0 };
        CComBSTR                    filename;

        if ( !mCurData.mHasLineInfo )
            return NULL;

        if ( !mCurData.mMod->GetSymbolSession( session ) )
            return NULL;

        hr = session->GetFileInfo( mCurData.mCompilandIndex, mCurData.mFileIndex, info );
        if ( FAILED( hr ) )
            return NULL;

        hr = Utf8To16( info.Name.ptr, info.Name.length, filename.m_str );
        if ( FAILED( hr ) )
            return NULL;

        return filename.Detach();
    }

    bool DocTracker::DocChanged()
    {
        if ( mCurData.mHasLineInfo != mOldData.mHasLineInfo )
            return true;

        if ( mCurData.mHasLineInfo )
        {
            // there might be more than one compiland or module that has a file, 
            // but that's OK, as long as inside a compiland files are unique

            // valid across break/run mode changes
            if ( mOldData.mMod->GetId() != mCurData.mMod->GetId() )
                return true;
            // only valid for 1 module
            if ( mOldData.mCompilandIndex != mCurData.mCompilandIndex )
                return true;
            if ( mOldData.mFileIndex != mCurData.mFileIndex )
                return true;
        }
        // else, doesn't and didn't have line info, so nothing changed

        return false;
    }

    void DocTracker::Update( Address address )
    {
        RefPtr<Module>              mod;
        RefPtr<MagoST::ISession>    session;
        uint16_t                    section = 0;
        uint32_t                    offset = 0;
        MagoST::LineNumber          lineNum = { 0 };

        // set the old info, so we can tell if the doc changed
        mOldData = mCurData;

        // see if we can get line info from the mod we already have

        if ( mCurData.mHasLineInfo )
        {
            _ASSERT( mCurData.mMod != NULL );

            if ( mCurData.mMod->GetSymbolSession( session ) )
            {
                section = session->GetSecOffsetFromVA( address, offset );
            }

            if ( section != 0 )
            {
                uint16_t                lineIndex = 0;

                if ( FindLineInSegment( section, offset, lineIndex ) )
                {
                    UpdateLine( offset, lineIndex );
                    return;
                }

                if ( session->FindLine( section, offset, lineNum ) )
                {
                    UpdateLine( session, offset, lineNum );
                    return;
                }
            }

            mCurData.mHasLineInfo = false;
        }

        // look for the module, then the line inside it

        mod = mCurData.mMod;

        if ( (mod == NULL) || !mod->Contains( address ) )
        {
            mod.Release();
            if ( !mProg->FindModuleContainingAddress( address, mod ) )
                return;
        }

        if ( !mod->GetSymbolSession( session ) )
            return;

        section = session->GetSecOffsetFromVA( address, offset );

        if ( !session->FindLine( section, offset, lineNum ) )
            return;

        UpdateLine( session, offset, lineNum, mod );
    }

    bool DocTracker::IsValid()
    {
        if ( mCurData.mHasLineInfo )
        {
            if ( !mCurData.IsValid() )
                return false;
            if ( mFileSeg.SegmentIndex == 0 )
                return false;
            if ( mFileSeg.LineCount == 0 )
                return false;
            if ( mFileSeg.LineNumbers == NULL )
                return false;
            if ( mFileSeg.Offsets == NULL )
                return false;
            if ( mLines.size() != mFileSeg.LineCount )
                return false;
            if ( mLineIndex >= mFileSeg.LineCount )
                return false;
        }

        return true;
    }

    void DocTracker::UpdateLine( uint32_t curOffset, uint16_t lineIndex )
    {
        _ASSERT( mCurData.mHasLineInfo );
        _ASSERT( IsValid() );
        _ASSERT( mFileSeg.Offsets != NULL );
        _ASSERT( lineIndex < mFileSeg.LineCount );

        // has to have line info already
        mLineIndex = lineIndex;
        mByteOffset = curOffset - mFileSeg.Offsets[ lineIndex ];
    }

    void DocTracker::UpdateLine( MagoST::ISession* session, uint32_t curOffset, const MagoST::LineNumber& lineNum )
    {
        _ASSERT( session != NULL );
        _ASSERT( mCurData.mHasLineInfo );
        _ASSERT( mCurData.mMod != NULL );

        bool found = session->GetFileSegment( 
            lineNum.CompilandIndex, 
            lineNum.FileIndex, 
            lineNum.SegmentInstanceIndex, 
            mFileSeg );
        // we have a segment instance index, so we have to be able to find it
        _ASSERT( found );
        UNREFERENCED_PARAMETER( found );

        mLines.reserve( mFileSeg.LineCount );
        mLines.resize( mFileSeg.LineCount );

        for ( int i = 0; i < mFileSeg.LineCount; i++ )
        {
            mLines[i].LineNumber = mFileSeg.LineNumbers[i];
        }

        // When returning line numbers, we need to return the two lines that 
        // span the target offset. The lines come sorted by offset, not line 
        // number. So, in order to quickly find the lines when needed, we sort
        // them by line number here.

        std::sort( mLines.begin(), mLines.end() );

        mCurData.mCompilandIndex = lineNum.CompilandIndex;
        mCurData.mFileIndex = lineNum.FileIndex;
        mLineIndex = lineNum.LineIndex;

        mByteOffset = curOffset - mFileSeg.Offsets[ mLineIndex ];
    }

    void DocTracker::UpdateLine( MagoST::ISession* session, uint32_t curOffset, const MagoST::LineNumber& lineNum, Module* mod )
    {
        _ASSERT( session != NULL );
        _ASSERT( mod != NULL );

        mCurData.mMod = mod;
        mCurData.mHasLineInfo = true;

        UpdateLine( session, curOffset, lineNum );

        _ASSERT( IsValid() );
    }

    // As an optimization, try to find the line in the lines left in the segment.
    // It's OK if we can't find it. We'll do a full search in that case.

    bool DocTracker::FindLineInSegment( uint16_t section, uint32_t offset, uint16_t& lineIndex )
    {
        _ASSERT( mCurData.mHasLineInfo );
        _ASSERT( IsValid() );

        uint16_t    lineCount = mFileSeg.LineCount;
        uint16_t    i = 0;

        if ( section != mFileSeg.SegmentIndex )
            return false;

        if ( (offset < mFileSeg.Start) || (offset > mFileSeg.End) )
            return false;

        if ( lineCount == std::numeric_limits<uint16_t>::max() )
            lineCount--;

        // starting at the line entry we last used, find the closest line that this offset is in
        for ( i = mLineIndex; i < lineCount; i++ )
        {
            // stop when we pass the target offset, or reach the end
            if ( offset < mFileSeg.Offsets[i] )
                break;
        }

        // offset is before any entry, so none found
        if ( i == 0 )
            return false;

        // we passed the line, so take it back one; this is the closest
        lineIndex = i - 1;

        return true;
    }
}
