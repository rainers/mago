/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "BPBinders.h"
#include "BpResolutionLocation.h"
#include "Module.h"
#include "CodeContext.h"


namespace Mago
{
    //----------------------------------------------------------------------------
    //  BPCodeFileLineBinder
    //----------------------------------------------------------------------------

    BPCodeFileLineBinder::BPCodeFileLineBinder(
        IDebugBreakpointRequest2* request )
        :   mReqLineStart( 0 ),
            mReqLineEnd( 0 )
    {
        _ASSERT( request != NULL );

        HRESULT         hr = S_OK;
        BpRequestInfo   reqInfo;
        TEXT_POSITION   posBegin = { 0 };
        TEXT_POSITION   posEnd = { 0 };

        hr = request->GetRequestInfo( BPREQI_BPLOCATION, &reqInfo );

        _ASSERT( reqInfo.bpLocation.bpLocationType == BPLT_CODE_FILE_LINE );

        reqInfo.bpLocation.bpLocation.bplocCodeFileLine.pDocPos->GetFileName( &mFilename );
        reqInfo.bpLocation.bpLocation.bplocCodeFileLine.pDocPos->GetRange( &posBegin, &posEnd );

        // AD7 lines are 0-based, DIA ones are 1-based
        mReqLineStart = posBegin.dwLine + 1;
        mReqLineEnd = posEnd.dwLine + 1;
    }

    void BPCodeFileLineBinder::Bind( Module* mod, ModuleBinding* binding, BPBoundBPMaker* maker, Error& err )
    {
        HRESULT                 hr = S_OK;
        CAutoVectorPtr<char>    u8FileName;
        size_t                  u8FileNameLen = 0;

        PutDocError( err );

        hr = Utf16To8( mFilename, wcslen( mFilename ), u8FileName.m_p, u8FileNameLen );
        if ( FAILED( hr ) )
            return;

        bool    foundExact = false;
        
        foundExact = BindToFile( true, u8FileName, u8FileNameLen, mod, binding, maker, err );

        if ( !foundExact )
            BindToFile( false, u8FileName, u8FileNameLen, mod, binding, maker, err );
    }

    bool BPCodeFileLineBinder::BindToFile( 
        bool exactMatch, 
        const char* fileName, 
        size_t fileNameLen, 
        Module* mod, 
        ModuleBinding* binding, 
        BPBoundBPMaker* maker, 
        Error& err )
    {
        HRESULT                     hr = S_OK;
        RefPtr<MagoST::ISession>    session;
        uint32_t                    compCount = 0;
        bool                        foundMatch = false;

        if ( !mod->GetSymbolSession( session ) )
            return false;

        hr = session->GetCompilandCount( compCount );
        if ( FAILED( hr ) )
            return false;

        for ( uint16_t compIx = 1; compIx <= compCount; compIx++ )
        {
            MagoST::CompilandInfo   compInfo = { 0 };

            hr = session->GetCompilandInfo( compIx, compInfo );
            if ( FAILED( hr ) )
                continue;

            for ( uint16_t fileIx = 0; fileIx < compInfo.FileCount; fileIx++ )
            {
                MagoST::FileInfo    fileInfo = { 0 };
                bool                matches = false;

                hr = session->GetFileInfo( compIx, fileIx, fileInfo );
                if ( FAILED( hr ) )
                    continue;

                if ( exactMatch )
                    matches = ExactFileNameMatch( fileName, fileNameLen, fileInfo.Name.ptr, fileInfo.Name.length );
                else
                    matches = PartialFileNameMatch( fileName, fileNameLen, fileInfo.Name.ptr, fileInfo.Name.length );

                if ( !matches )
                    continue;

                foundMatch = true;

                PutLineError( err );

                MagoST::LineNumber  line = { 0 };
                if ( !session->FindLineByNum( compIx, fileIx, (uint16_t) mReqLineStart, line ) )
                    continue;

                // do the line ranges overlap?
                if ( ((line.Number <= mReqLineEnd) && (line.Number >= mReqLineStart)) )
                {
                    line.NumberEnd = line.Number;

                    hr = maker->MakeDocContext( session, compIx, fileIx, line );
                    if ( FAILED( hr ) )
                        continue;

                    do
                    {
                        UINT64  addr = 0;
                        addr = session->GetVAFromSecOffset( line.Section, line.Offset );
                        maker->AddBoundBP( addr, mod, binding );
                    }
                    while( session->FindNextLineByNum( compIx, fileIx, (uint16_t) mReqLineStart, line ) );
                }
            }
        }

        return foundMatch;
    }

    void BPCodeFileLineBinder::PutDocError( Error& err )
    {
        err.PutError( BPET_TYPE_WARNING, BPET_SEV_LOW, IDS_NO_SYMS_FOR_DOC );
    }

    void BPCodeFileLineBinder::PutLineError( Error& err )
    {
        err.PutError( BPET_TYPE_WARNING, BPET_SEV_GENERAL, IDS_NO_CODE_FOR_LINE );
    }


    //----------------------------------------------------------------------------
    //  BPCodeAddressBinder
    //----------------------------------------------------------------------------

    BPCodeAddressBinder::BPCodeAddressBinder( IDebugBreakpointRequest2* request )
        :   mAddress( 0 )
    {
        _ASSERT( request != NULL );

        HRESULT             hr = S_OK;
        BpRequestInfo       reqInfo;

        hr = request->GetRequestInfo( BPREQI_BPLOCATION, &reqInfo );

        _ASSERT( reqInfo.bpLocation.bpLocationType == BPLT_CODE_ADDRESS 
            || reqInfo.bpLocation.bpLocationType == BPLT_CODE_CONTEXT );

        if ( reqInfo.bpLocation.bpLocationType == BPLT_CODE_ADDRESS )
        {
            // will return 0 on error, which is a good invalid address
            uint64_t addr = _wcstoui64( reqInfo.bpLocation.bpLocation.bplocCodeAddress.bstrAddress, NULL, 0 );

            if ( addr <= std::numeric_limits<Address>::max() )
                mAddress = (Address) addr;
        }
        else
        {
            CComPtr<IMagoMemoryContext> memContext;

            hr = reqInfo.bpLocation.bpLocation.bplocCodeContext.pCodeContext->QueryInterface( &memContext );
            if ( SUCCEEDED( hr ) )
                memContext->GetAddress( mAddress );
        }
    }

    void BPCodeAddressBinder::Bind( Module* mod, ModuleBinding* binding, BPBoundBPMaker* maker, Error& err )
    {
        if ( (mAddress != 0) && mod->Contains( mAddress ) )
        {
            maker->AddBoundBP( mAddress, mod, binding );
        }
        else
        {
            err.PutError( BPET_TYPE_ERROR, BPET_SEV_GENERAL, IDS_INVALID_ADDRESS );
        }
    }
}
