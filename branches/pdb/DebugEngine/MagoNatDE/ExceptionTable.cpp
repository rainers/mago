/*
   Copyright (c) 2011 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ExceptionTable.h"


const wchar_t ExceptionKeyVSSubPath[] = L"\\AD7Metrics\\Exception";
// take away one for the terminator
const int GuidLength = _countof( "{97348AC0-2B6B-4B99-A245-4C7E2C09D403}" ) - 1;
const int MaxKeyLength = 255;


static bool GetCategoryPath( const wchar_t* rootPath, const GUID& category, wchar_t* fullPath, int fullPathLen )
{
    _ASSERT( rootPath != NULL );
    _ASSERT( fullPath != NULL );

    wchar_t         guidStr[GuidLength + 1] = L"";
    int             guidStrLen = _countof( guidStr );
    errno_t         err = 0;

    guidStrLen = StringFromGUID2( category, guidStr, guidStrLen );
    _ASSERT( guidStrLen > 0 );

    err = wcscpy_s( fullPath, fullPathLen, rootPath );
    if ( err != 0 )
        return false;
    err = wcscat_s( fullPath, fullPathLen, ExceptionKeyVSSubPath );
    if ( err != 0 )
        return false;
    err = wcscat_s( fullPath, fullPathLen, L"\\" );
    if ( err != 0 )
        return false;
    err = wcscat_s( fullPath, fullPathLen, guidStr );
    if ( err != 0 )
        return false;

    return true;
}


namespace Mago
{
    //------------------------------------------------------------------------
    // ExceptionTable
    //------------------------------------------------------------------------

    HRESULT EngineExceptionTable::ExceptionTable::SetException( EXCEPTION_INFO* pException )
    {
        _ASSERT( pException != NULL );

        ExceptionInfo info;
        info.bstrExceptionName = pException->bstrExceptionName;
        info.dwCode = pException->dwCode;
        info.dwState = pException->dwState;
        info.guidType = pException->guidType;

        mExceptionInfos.push_back( info );
        return S_OK;
    }

    HRESULT EngineExceptionTable::ExceptionTable::RemoveSetException( EXCEPTION_INFO* pException )
    {
        _ASSERT( pException != NULL );

        for ( EIVector::iterator it = mExceptionInfos.begin(); it != mExceptionInfos.end(); it++ )
        {
            ExceptionInfo& info = *it;
            if ( info.bstrExceptionName == pException->bstrExceptionName &&
                info.dwCode == pException->dwCode &&
                info.guidType == pException->guidType )
            {
                mExceptionInfos.erase( it );
                return S_OK;
            }
        }
        return S_FALSE;
    }

    HRESULT EngineExceptionTable::ExceptionTable::RemoveAllSetExceptions( REFGUID guidType )
    {
        if ( guidType == GUID_NULL )
        {
            mExceptionInfos.clear();
            return S_OK;
        }

        for ( EIVector::iterator it = mExceptionInfos.begin(); it != mExceptionInfos.end(); )
        {
            if ( (*it).guidType == guidType )
                it = mExceptionInfos.erase( it );
            else
                ++it;
        }
        return S_OK;
    }

    bool EngineExceptionTable::ExceptionTable::FindExceptionInfo( 
        const GUID& guid, 
        DWORD code, 
        ExceptionInfo& excInfo )
    {
        for ( EIVector::iterator it = mExceptionInfos.begin(); it != mExceptionInfos.end(); it++ )
        {
            ExceptionInfo& info = *it;
            if ( info.guidType == guid && info.dwCode == code )
            {
                excInfo = info;
                return true;
            }
        }
        return false;
    }

    bool EngineExceptionTable::ExceptionTable::FindExceptionInfo( 
        const GUID& guid, 
        LPCOLESTR name, 
        ExceptionInfo& excInfo )
    {
        for ( EIVector::iterator it = mExceptionInfos.begin(); it != mExceptionInfos.end(); it++ )
        {
            ExceptionInfo& info = *it;
            if ( info.guidType == guid && info.bstrExceptionName == name )
            {
                excInfo = info;
                return true;
            }
        }
        return false;
    }


    //------------------------------------------------------------------------
    // EngineExceptionTable
    //------------------------------------------------------------------------

    GUID EngineExceptionTable::sSupportedCategories[] = 
    {
        GetDExceptionType(),
        GetWin32ExceptionType(),
    };


    HRESULT EngineExceptionTable::SetRegistryRoot( LPCOLESTR pszRegistryRoot )
    {
        if ( pszRegistryRoot == NULL )
            return E_INVALIDARG;

        mDefaultExceptions.RemoveAllSetExceptions( GUID_NULL );

        // load as much as possible

        for ( int i = 0; i < _countof( sSupportedCategories ); i++ )
        {
            LoadDefaultExceptionCategory( pszRegistryRoot, sSupportedCategories[i] );
        }

        return S_OK;
    }

    // Returns true if any exception records in category were loaded.
    bool EngineExceptionTable::LoadDefaultExceptionCategory( 
        LPCOLESTR pszRegistryRoot, 
        const GUID& category )
    {
        _ASSERT( pszRegistryRoot != NULL );

        wchar_t         path[MAX_PATH] = L"";
        LONG            ret = ERROR_SUCCESS;
        RegHandlePtr    rootKey;
        int             totalLoaded = 0;

        if ( !GetCategoryPath( pszRegistryRoot, category, path, _countof( path ) ) )
            return false;

        ret = RegOpenKeyEx( HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &rootKey.Ref() );
        if ( ret != ERROR_SUCCESS )
            return false;

        totalLoaded = LoadDefaultExceptionsRecursive( rootKey, category, 1 );

        return totalLoaded > 0;
    }

    // Load the default exceptions under a key. Recurse a number of levels 
    // under the immediate children of the key.
    // Returns how many exception records were loaded.
    // We don't care to keep the structure. Put everything in a single list.
    int EngineExceptionTable::LoadDefaultExceptionsRecursive( 
        HKEY hKeyRoot, 
        const GUID& category, 
        int levelsToRecurse )
    {
        if ( levelsToRecurse < 0 )
            return 0;

        int total = 0;

        for ( int i = 0; i < INT_MAX; i++ )
        {
            LONG            ret = ERROR_SUCCESS;
            wchar_t         level1Name[MaxKeyLength + 1] = L"";
            DWORD           level1NameLen = _countof( level1Name );
            RegHandlePtr    level1Key;

            ret = RegEnumKeyEx( hKeyRoot, i, level1Name, &level1NameLen, NULL, NULL, NULL, NULL );
            if ( ret == ERROR_INVALID_HANDLE )
                return 0;
            if ( ret == ERROR_NO_MORE_ITEMS )
                break;
            if ( ret != ERROR_SUCCESS )
                continue;

            ret = RegOpenKeyEx( hKeyRoot, level1Name, 0, KEY_READ, &level1Key.Ref() );
            if ( ret != ERROR_SUCCESS )
                continue;

            if ( LoadDefaultException( level1Name, level1Key, category ) )
                total++;

            if ( levelsToRecurse > 0 )
                total += LoadDefaultExceptionsRecursive( level1Key, category, levelsToRecurse - 1 );
        }

        return total;
    }

    // Returns true if exception record at key was loaded.
    // VC++ 2010 still uses obsolete state flags, but we don't worry about them.
    bool EngineExceptionTable::LoadDefaultException( const wchar_t* keyName, HKEY hKey, const GUID& category )
    {
        if ( (keyName == NULL) || (keyName[0] == L'\0') )
            return false;

        HRESULT hr = S_OK;
        EXCEPTION_INFO excInfo = { 0 };
        DWORD ret = ERROR_SUCCESS;
        DWORD type = 0;
        DWORD stateSize = sizeof excInfo.dwState;
        DWORD codeSize = sizeof excInfo.dwCode;
        CComBSTR keyNameBstr = keyName;

        ret = RegQueryValueEx( hKey, L"State", NULL, &type, (BYTE*) &excInfo.dwState, &stateSize );
        if ( ret != ERROR_SUCCESS )
            return false;
        if ( type != REG_DWORD )
            return false;

        ret = RegQueryValueEx( hKey, L"Code", NULL, &type, (BYTE*) &excInfo.dwCode, &codeSize );
        if ( ret != ERROR_SUCCESS )
            return false;
        if ( type != REG_DWORD )
            return false;

        if ( keyNameBstr == NULL )
            return false;

        excInfo.guidType = category;
        excInfo.bstrExceptionName = keyNameBstr;

        hr = mDefaultExceptions.SetException( &excInfo );
        if ( FAILED( hr ) )
            return false;

        return true;
    }

    bool EngineExceptionTable::IsSupportedCategory( const GUID& category )
    {
        for ( int i = 0; i < _countof( EngineExceptionTable::sSupportedCategories ); i++ )
        {
            if ( category == sSupportedCategories[i] )
                return true;
        }
        return false;
    }

    HRESULT EngineExceptionTable::SetException( EXCEPTION_INFO* pException )
    {
        if ( pException == NULL )
            return E_INVALIDARG;

        if ( !IsSupportedCategory( pException->guidType ) )
            return S_OK;

        return mOverrideExceptions.SetException( pException );
    }

    HRESULT EngineExceptionTable::RemoveSetException( EXCEPTION_INFO* pException )
    {
        if ( pException == NULL )
            return E_INVALIDARG;

        if ( !IsSupportedCategory( pException->guidType ) )
            return S_FALSE;

        return mOverrideExceptions.RemoveSetException( pException );
    }

    HRESULT EngineExceptionTable::RemoveAllSetExceptions( REFGUID guidType )
    {
        return mOverrideExceptions.RemoveAllSetExceptions( guidType );
    }

    bool EngineExceptionTable::FindExceptionInfo( const GUID& guid, DWORD code, ExceptionInfo& excInfo )
    {
        if ( mOverrideExceptions.FindExceptionInfo( guid, code, excInfo ) )
            return true;

        return mDefaultExceptions.FindExceptionInfo( guid, code, excInfo );
    }

    bool EngineExceptionTable::FindExceptionInfo( const GUID& guid, LPCOLESTR name, ExceptionInfo& excInfo )
    {
        if ( mOverrideExceptions.FindExceptionInfo( guid, name, excInfo ) )
            return true;

        return mDefaultExceptions.FindExceptionInfo( guid, name, excInfo );
    }
}
