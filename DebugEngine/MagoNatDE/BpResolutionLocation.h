/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    struct BpResolutionLocation : public BP_RESOLUTION_LOCATION
    {
        BpResolutionLocation()
        {
            bpType = BPT_NONE;
            memset( &bpResLocation, 0, sizeof bpResLocation );
        }

        ~BpResolutionLocation()
        {
            if ( bpType == BPT_CODE )
            {
                if ( bpResLocation.bpresCode.pCodeContext != NULL )
                    bpResLocation.bpresCode.pCodeContext->Release();
            }
            else if ( bpType == BPT_DATA )
            {
                SysFreeString( bpResLocation.bpresData.bstrDataExpr );
                SysFreeString( bpResLocation.bpresData.bstrFunc );
                SysFreeString( bpResLocation.bpresData.bstrImage );
            }
        }

        static HRESULT InitCode( BP_RESOLUTION_LOCATION& loc, IDebugCodeContext2* codeContext )
        {
            loc.bpType = BPT_CODE;

            _ASSERT( codeContext != NULL );
            if ( codeContext != NULL )
            {
                loc.bpResLocation.bpresCode.pCodeContext = codeContext;
                loc.bpResLocation.bpresCode.pCodeContext->AddRef();
            }

            return S_OK;
        }

        static HRESULT InitData( 
            BP_RESOLUTION_LOCATION& loc, 
            const wchar_t* dataExpr, 
            const wchar_t* func, 
            const wchar_t* image, 
            BP_RES_DATA_FLAGS flags )
        {
            CComBSTR bstrDataExpr = dataExpr;
            CComBSTR bstrFunc = func;
            CComBSTR bstrImage = image;

            if ( (bstrDataExpr == NULL) || (bstrFunc == NULL) || (bstrImage == NULL) )
                return E_OUTOFMEMORY;

            loc.bpType = BPT_DATA;

            loc.bpResLocation.bpresData.bstrDataExpr = bstrDataExpr.Detach();
            loc.bpResLocation.bpresData.bstrFunc = bstrFunc.Detach();
            loc.bpResLocation.bpresData.bstrImage = bstrImage.Detach();
            loc.bpResLocation.bpresData.dwFlags = flags;

            return S_OK;
        }

        HRESULT CopyTo( BP_RESOLUTION_LOCATION& other )
        {
            HRESULT hr = S_OK;

            if ( bpType == BPT_CODE )
                hr = InitCode( other, bpResLocation.bpresCode.pCodeContext );
            else if ( bpType == BPT_DATA )
                hr = InitData( 
                    other,
                    bpResLocation.bpresData.bstrDataExpr, 
                    bpResLocation.bpresData.bstrFunc, 
                    bpResLocation.bpresData.bstrImage, 
                    bpResLocation.bpresData.dwFlags );

            return hr;
        }
    };


    struct BpRequestInfo : public BP_REQUEST_INFO
    {
        BpRequestInfo()
        {
            memset( this, 0, sizeof *this );
        }

        ~BpRequestInfo()
        {
            if ( (dwFields & BPREQI_BPLOCATION) == BPREQI_BPLOCATION )
            {
                switch ( bpLocation.bpLocationType )
                {
                case BPLT_CODE_FILE_LINE:
                    SysFreeString( bpLocation.bpLocation.bplocCodeFileLine.bstrContext );
                    if ( bpLocation.bpLocation.bplocCodeFileLine.pDocPos != NULL )
                        bpLocation.bpLocation.bplocCodeFileLine.pDocPos->Release();
                    break;

                case BPLT_CODE_FUNC_OFFSET:
                    SysFreeString( bpLocation.bpLocation.bplocCodeFuncOffset.bstrContext );
                    if ( bpLocation.bpLocation.bplocCodeFuncOffset.pFuncPos != NULL )
                        bpLocation.bpLocation.bplocCodeFuncOffset.pFuncPos->Release();
                    break;

                case BPLT_CODE_CONTEXT:
                    if ( bpLocation.bpLocation.bplocCodeContext.pCodeContext != NULL )
                        bpLocation.bpLocation.bplocCodeContext.pCodeContext->Release();
                    break;

                case BPLT_CODE_STRING:
                    SysFreeString( bpLocation.bpLocation.bplocCodeString.bstrContext );
                    SysFreeString( bpLocation.bpLocation.bplocCodeString.bstrCodeExpr );
                    break;

                case BPLT_CODE_ADDRESS:
                    SysFreeString( bpLocation.bpLocation.bplocCodeAddress.bstrContext );
                    SysFreeString( bpLocation.bpLocation.bplocCodeAddress.bstrModuleUrl );
                    SysFreeString( bpLocation.bpLocation.bplocCodeAddress.bstrFunction );
                    SysFreeString( bpLocation.bpLocation.bplocCodeAddress.bstrAddress );
                    break;

                case BPLT_DATA_STRING:
                    SysFreeString( bpLocation.bpLocation.bplocDataString.bstrContext );
                    SysFreeString( bpLocation.bpLocation.bplocDataString.bstrDataExpr );
                    if ( bpLocation.bpLocation.bplocDataString.pThread != NULL )
                        bpLocation.bpLocation.bplocDataString.pThread->Release();
                    break;

                case BPLT_RESOLUTION:
                    if ( bpLocation.bpLocation.bplocResolution.pResolution != NULL )
                        bpLocation.bpLocation.bplocResolution.pResolution->Release();
                    break;
                }
            }

            if ( (dwFields & BPREQI_PROGRAM) == BPREQI_PROGRAM )
            {
                if ( pProgram != NULL )
                    pProgram->Release();
            }

            if ( (dwFields & BPREQI_PROGRAMNAME) == BPREQI_PROGRAMNAME )
            {
                SysFreeString( bstrProgramName );
            }

            if ( (dwFields & BPREQI_THREAD) == BPREQI_THREAD )
            {
                if ( pThread != NULL )
                    pThread->Release();
            }

            if ( (dwFields & BPREQI_THREADNAME) == BPREQI_THREADNAME )
            {
                SysFreeString( bstrThreadName );
            }

            if ( (dwFields & BPREQI_CONDITION) == BPREQI_CONDITION )
            {
                if ( bpCondition.pThread != NULL )
                    bpCondition.pThread->Release();
                SysFreeString( bpCondition.bstrContext );
                SysFreeString( bpCondition.bstrCondition );
            }
        }
    };
}
