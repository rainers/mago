/*
   Copyright (c) 2011 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    struct ExceptionInfo
    {
        // leaving out unimportant info from EXCEPTION_INFO
        CComBSTR bstrExceptionName;
        DWORD dwCode;
        EXCEPTION_STATE dwState;
        GUID guidType;
    };

    // Keeps track of exception settings.
    // Defaults come from the registry and are changed all at once by 
    // SetRegistryRoot. Overrides can be added and removed individually.

    class EngineExceptionTable
    {
        class ExceptionTable
        {
            typedef std::vector<ExceptionInfo> EIVector;

            EIVector            mExceptionInfos;

        public:
            HRESULT SetException( EXCEPTION_INFO* pException );
            HRESULT RemoveSetException( EXCEPTION_INFO* pException );
            HRESULT RemoveAllSetExceptions( REFGUID guidType );
            bool FindExceptionInfo( const GUID& guid, DWORD code, ExceptionInfo& excInfo );
            bool FindExceptionInfo( const GUID& guid, LPCOLESTR name, ExceptionInfo& excInfo );
        };

        ExceptionTable  mDefaultExceptions;
        ExceptionTable  mOverrideExceptions;

        static GUID sSupportedCategories[];

    public:
        HRESULT SetRegistryRoot( LPCOLESTR pszRegistryRoot );
        HRESULT SetException( EXCEPTION_INFO* pException );
        HRESULT RemoveSetException( EXCEPTION_INFO* pException );
        HRESULT RemoveAllSetExceptions( REFGUID guidType );

        // Returns true if exception info was found.
        // If found, then returns info in output param. A copy is returned instead 
        // of a pointer to the entry, because another thread can change the list 
        // before the user gets around to using the info.
        bool FindExceptionInfo( const GUID& guid, DWORD code, ExceptionInfo& excInfo );
        bool FindExceptionInfo( const GUID& guid, LPCOLESTR name, ExceptionInfo& excInfo );

    private:
        bool LoadDefaultExceptionCategory( LPCOLESTR pszRegistryRoot, const GUID& category );
        bool LoadDefaultException( const wchar_t* keyName, HKEY hKey, const GUID& category );
        int LoadDefaultExceptionsRecursive( HKEY hKeyRoot, const GUID& category, int levelsToRecurse );

        bool IsSupportedCategory( const GUID& category );
    };
}
