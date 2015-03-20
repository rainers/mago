/*
   Copyright (c) 2012 Rainer Schuetze

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "PDBDebugStore.h"
#include "ISymbolInfo.h"
#include "Util.h"

#include <dia2.h>
#include <assert.h>

#define UNREF_PARAM( p ) UNREFERENCED_PARAMETER( p )

namespace PDBStore
{
    struct SymbolScopeIn
    {
        DWORD id;
        DWORD current;
    };

    struct TypeScopeIn
    {
        DWORD id;
        DWORD current;
    };

    struct EnumNamedSymbolsDataIn
    {
        DWORD id;
    };

    struct TypeHandleIn
    {
        DWORD id;
    };

    struct SymHandleIn
    {
        DWORD id;
    };

}

class CCallback : public IDiaLoadCallback2
{
    int m_nRefCount;
public:
    CCallback() { m_nRefCount = 0; }

    //IUnknown
    ULONG STDMETHODCALLTYPE AddRef() {
        m_nRefCount++;
        return m_nRefCount;
    }
    ULONG STDMETHODCALLTYPE Release() {
        if ( (--m_nRefCount) == 0 )
            delete this;
        return m_nRefCount;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID rid, void **ppUnk ) {
        if ( ppUnk == NULL ) {
            return E_INVALIDARG;
        }
        if (rid == __uuidof( IDiaLoadCallback2 ) )
            *ppUnk = (IDiaLoadCallback2 *)this;
        else if (rid == __uuidof( IDiaLoadCallback ) )
            *ppUnk = (IDiaLoadCallback *)this;
        else if (rid == __uuidof( IUnknown ) )
            *ppUnk = (IUnknown *)this;
        else
            *ppUnk = NULL;
        if ( *ppUnk != NULL ) {
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE NotifyDebugDir(
                BOOL fExecutable, 
                DWORD cbData,
                BYTE data[]) // really a const struct _IMAGE_DEBUG_DIRECTORY *
    {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE NotifyOpenDBG(
                LPCOLESTR dbgPath, 
                HRESULT resultCode)
    {
        // wprintf(L"opening %s...\n", dbgPath);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE NotifyOpenPDB(
                LPCOLESTR pdbPath, 
                HRESULT resultCode)
    {
        // wprintf(L"opening %s...\n", pdbPath);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictRegistryAccess()         
    {
        // return hr != S_OK to prevent querying the registry for symbol search paths
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictSymbolServerAccess()
    {
      // return hr != S_OK to prevent accessing a symbol server
      return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictOriginalPathAccess()     
    {
        // return hr != S_OK to prevent querying the registry for symbol search paths
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictReferencePathAccess()
    {
        // return hr != S_OK to prevent accessing a symbol server
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictDBGAccess()
    {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictSystemRootAccess()
    {
        return S_OK;
    }
};

namespace MagoST
{
    void detachBSTR( BSTR bstr, SymString &sym )
    {
        DWORD   flags = 0;
        int nChars = WideCharToMultiByte( CP_UTF8, flags, bstr, -1, NULL, 0, NULL, NULL );
        std::auto_ptr<char> ptr( new char[nChars] );
        nChars = WideCharToMultiByte( CP_UTF8, flags, bstr, -1, ptr.get(), nChars, NULL, NULL );
        SysFreeString( bstr );
        if( nChars > 0 )
            sym.set( nChars - 1, ptr.release(), true );
    }

    class PDBSymbolInfo : public ISymbolInfo
    {
    public:
        PDBDebugStore* mStore;
        DWORD mId;

        PDBSymbolInfo( PDBDebugStore* store, DWORD id )
        {
            mStore = store;
            mId = id;
        }

        virtual SymTag GetSymTag()
        {
            DWORD tag = SymTagNull;
            IDiaSymbol* pSymbol = NULL;
            if( !FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
            {
                pSymbol->get_symTag( &tag );
                pSymbol->Release();
            }
            return (SymTag) tag;
        }

        virtual bool GetType( TypeIndex& index )
        {
            DWORD type = ~0U;
            IDiaSymbol* pSymbol = NULL;
            if( !FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
            {
                pSymbol->get_typeId( &type );
                if( type == ~0 )
                {
                    DWORD tag = SymTagNull;
                    pSymbol->get_symTag( &tag );
                    switch( tag )
                    {
                    case SymTagUDT:
                    case SymTagEnum:
                    case SymTagFunctionType:
                    case SymTagPointerType:
                    case SymTagArrayType:
                    case SymTagBaseType:
                    case SymTagTypedef:
                    case SymTagBaseClass:
                        type = mId;
                        break;
                    }
                }
                pSymbol->Release();
            }
            index = type;
            return type != ~0;
        }

        virtual bool GetName( SymString& name )
        {
            IDiaSymbol* pSymbol = NULL;
            if( !FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
            {
                BSTR bstrName = NULL;
                pSymbol->get_name( &bstrName );
                detachBSTR( bstrName, name );
                pSymbol->Release();
            }
            return name.GetName() != 0;
        }

        virtual bool GetAddressOffset( uint32_t& offset )
        {
            IDiaSymbol* pSymbol = NULL;
            if( FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
                return false;

            HRESULT hr = pSymbol->get_addressOffset( (DWORD*) &offset );
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetAddressSegment( uint16_t& segment ) 
        {
            IDiaSymbol* pSymbol = NULL;
            if( FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
                return false;

            DWORD section = 0;
            HRESULT hr = pSymbol->get_addressSection( &section );
            pSymbol->Release();
            segment = (uint16_t) section;
            return hr == S_OK;
        }

        virtual bool GetDataKind( DataKind& dataKind ) 
        {
            IDiaSymbol* pSymbol = NULL;
            if( FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
                return false;

            HRESULT hr = pSymbol->get_dataKind( (DWORD*) &dataKind );
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetLength( uint32_t& length )
        {
            IDiaSymbol* pSymbol = NULL;
            if( FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
                return false;

            ULONGLONG llength = 0;
            HRESULT hr = pSymbol->get_length( &llength );
            pSymbol->Release();
            length = (uint32_t) llength;
            return hr == S_OK;
        }

        virtual bool GetLocation( LocationType& locType ) 
        {
            IDiaSymbol* pSymbol = NULL;
            if( FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
                return false;

            HRESULT hr = pSymbol->get_locationType( (DWORD*) &locType );
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetOffset( int32_t& offset )
        {
            IDiaSymbol* pSymbol = NULL;
            if( FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
                return false;

            HRESULT hr = pSymbol->get_offset( (LONG*) &offset );
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetRegister( uint32_t& reg )
        {
            IDiaSymbol* pSymbol = NULL;
            if( FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
                return false;

            HRESULT hr = pSymbol->get_registerId( (DWORD*) &reg );
            pSymbol->Release();
            return hr == S_OK;
        }

        // unused
        virtual bool GetRegisterCount( uint8_t& count ) { UNREF_PARAM( count ); return false; }
        virtual bool GetRegisters( uint8_t*& regs ) { UNREF_PARAM( regs ); return false; }

        virtual bool GetUdtKind( UdtKind& udtKind )
        {
            IDiaSymbol* pSymbol = NULL;
            if( FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
                return false;

            HRESULT hr = pSymbol->get_udtKind( (DWORD*) &udtKind );
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetValue( Variant& value ) 
        {
            IDiaSymbol* pSymbol = NULL;
            if( FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
                return false;

            VARIANT var;
            VariantInit( &var );
            HRESULT hr = pSymbol->get_value( &var );
            if( hr == S_OK )
                switch( var.vt )
                {
                case VT_I1: value.Tag = VarTag_Char;  value.Data.I8 = var.bVal; break;
                case VT_I2: value.Tag = VarTag_Short; value.Data.I16 = var.iVal; break;
                case VT_I4: value.Tag = VarTag_Long;  value.Data.I32 = var.lVal; break;
                case VT_I8: value.Tag = VarTag_Quadword; value.Data.I64 = var.llVal; break;
                default:    value.Tag = VarTag_Long;  value.Data.I32 = 0; 
                    assert( false );
                    break;
                }

            VariantClear( &var );
            pSymbol->Release();
            return hr == S_OK;
        }

#if 1
        // unused
        virtual bool GetDebugStart( uint32_t& start ) { UNREF_PARAM( start ); return false; }
        virtual bool GetDebugEnd( uint32_t& end ) { UNREF_PARAM( end ); return false; }
        //virtual bool GetProcFlags( CV_PROCFLAGS& flags ) { return false; }
        virtual bool GetProcFlags( uint8_t& flags ) { UNREF_PARAM( flags ); return false; }
        virtual bool GetThunkOrdinal( uint8_t& ordinal ) { UNREF_PARAM( ordinal ); return false; }

        virtual bool GetBasicType( DWORD& basicType )
        {
            IDiaSymbol* pSymbol = NULL;
            if( FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
                return false;

            HRESULT hr = pSymbol->get_baseType( &basicType );
            pSymbol->Release();
            return hr == S_OK;
        }

        // unused
        virtual bool GetIndexType( TypeIndex& index ) { UNREF_PARAM( index ); return false; }
        virtual bool GetCount( uint32_t& count ) 
        {
            IDiaSymbol* pSymbol = NULL;
            if ( FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
                return false;

            HRESULT hr = pSymbol->get_count( (DWORD*) &count );
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetFieldCount( uint16_t& count )
        {
            IDiaSymbol* pSymbol = NULL;
            if( FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
                return false;

            LONG cnt = 0;
            IDiaEnumSymbols* pEnumSymbols = NULL;
            HRESULT hr = pSymbol->findChildren( SymTagNull, NULL, nsNone, &pEnumSymbols );
            if ( !FAILED( hr ) )
                hr = pEnumSymbols->get_Count( &cnt );
            if ( pEnumSymbols )
                pEnumSymbols->Release();
            pSymbol->Release();
            count = (uint16_t) cnt;
            return hr == S_OK;
        }

        virtual bool GetFieldList( TypeIndex& index )
        {
            index = mId; // type index the same as the symbol ID, and the fieldlist is the children list
            return true;
        }
        virtual bool GetProperties( uint16_t& props ) { UNREF_PARAM( props ); return false; }
        virtual bool GetDerivedList( TypeIndex& index ) { UNREF_PARAM( index ); return false; }
        virtual bool GetVShape( TypeIndex& index ) { UNREF_PARAM( index ); return false; }

        virtual bool GetCallConv( uint8_t& callConv ) { UNREF_PARAM( callConv ); return false; }
        virtual bool GetParamCount( uint16_t& count ) { UNREF_PARAM( count ); return false; }
        virtual bool GetParamList( TypeIndex& index )
        {
            index = mId; // type index the same as the symbol ID?
            return true;
        }

        virtual bool GetClass( TypeIndex& index ) { UNREF_PARAM( index ); return false; }
        virtual bool GetThis( TypeIndex& index ) { UNREF_PARAM( index ); return false; }
        virtual bool GetThisAdjust( int32_t& adjust ) { UNREF_PARAM( adjust ); return false; }

        virtual bool GetOemId( uint32_t& oemId ) { UNREF_PARAM( oemId ); return false; }
        virtual bool GetOemSymbolId( uint32_t& oemSymId ) { UNREF_PARAM( oemSymId ); return false; }
        virtual bool GetTypes( std::vector<TypeIndex>& indexes )
        {
            IDiaSymbol* pSymbol = NULL;
            if ( FAILED( mStore->getSession()->symbolById( mId, &pSymbol ) ) )
                return false;

            DWORD count;
            HRESULT hr = pSymbol->get_count( (DWORD*) &count );
            if( !FAILED( hr ) )
            {
                indexes.resize( count );
                if( count > 0 )
                    hr = pSymbol->get_typeIds( count, &count, indexes.data() );
            }
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetAttribute( uint16_t& attr ) { UNREF_PARAM( attr ); return false; }
        virtual bool GetVBaseOffset( uint32_t& offset ) { UNREF_PARAM( offset ); return false; }

        virtual bool GetVTableDescriptor( uint32_t index, uint8_t& desc )
        {
            UNREFERENCED_PARAMETER( index );
            UNREFERENCED_PARAMETER( desc );
            return false;
        }

        virtual bool GetMod( uint16_t& mod )
        {
            UNREFERENCED_PARAMETER( mod );
            return false;
        }
#endif
    };

    PDBDebugStore::PDBDebugStore()
        :   mSource( NULL ),
            mSession( NULL ),
            mGlobal( NULL ),
            mFindLineEnumLineNumbers( NULL ),
            mInit( false ),
            mCompilandCount( -1 )
    {
        mMachineType = CV_CFL_80386;

        C_ASSERT( sizeof( PDBStore::SymbolScopeIn ) <= sizeof( SymbolScope ) );
        C_ASSERT( sizeof( PDBStore::TypeScopeIn ) <= sizeof( TypeScope ) );
        C_ASSERT( sizeof( PDBStore::EnumNamedSymbolsDataIn ) <= sizeof( EnumNamedSymbolsData ) );
        C_ASSERT( sizeof( PDBStore::TypeHandleIn ) <= sizeof( TypeHandle ) );
        C_ASSERT( sizeof( PDBStore::SymHandleIn ) <= sizeof( SymHandle ) );
	}

    PDBDebugStore::~PDBDebugStore()
    {
        CloseDebugInfo();
    }

    HRESULT PDBDebugStore::InitDebugInfo( BYTE* buffer, DWORD size, const wchar_t* filename, const wchar_t* searchPath )
    {
        if ( (buffer == NULL) || (size == 0) )
            return E_INVALIDARG;
        if ( mInit )
            return E_ALREADY_INIT;

        HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
        if ( FAILED( hr ) )
            return hr;

        // set it after successfully calling CoInit, 
        // so later CoUninit is only called once for each successful call to CoInit
        mInit = true;

        // Obtain access to the provider
        GUID msdia120 = { 0x3BFCEA48, 0x620F, 0x4B6B, { 0x81, 0xF7, 0xB9, 0xAF, 0x75, 0x45, 0x4C, 0x7D } };
        GUID msdia110 = { 0x761D3BCD, 0x1304, 0x41D5, { 0x94, 0xE8, 0xEA, 0xC5, 0x4E, 0x4A, 0xC1, 0x72 } };
        GUID msdia100 = { 0xB86AE24D, 0xBF2F, 0x4AC9, { 0xB5, 0xA2, 0x34, 0xB1, 0x4E, 0x4C, 0xE1, 0x1D } }; // same as msdia80

        hr = CoCreateInstance( msdia120, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (void **) &mSource );
        if ( FAILED( hr ) )
            hr = CoCreateInstance( msdia110, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (void **) &mSource );
        if ( FAILED( hr ) )
            hr = CoCreateInstance( msdia100, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (void **) &mSource );
        if ( FAILED( hr ) )
            return hr;

#if 1
        PVOID OldValue = NULL;
        BOOL redir = Wow64DisableWow64FsRedirection( &OldValue );

        RefPtr<CCallback> callback = new CCallback;
        hr = mSource->loadDataForExe( filename, searchPath, callback );

        if( redir )
            Wow64RevertWow64FsRedirection( OldValue );

        if ( FAILED( hr ) )
            return hr;
#else
        if ( memcmp( buffer, "RSDS", 4 ) != 0 )
            return E_BAD_FORMAT;

        const char* pdbname = (const char*) buffer + 24;
        wchar_t wszFilename[MAX_PATH];
        int ret = MultiByteToWideChar( CP_UTF8, 0, pdbname, -1, wszFilename, MAX_PATH );
        if ( ret == 0 )
            return HRESULT_FROM_WIN32( GetLastError() );

        // Open and prepare a program database (.pdb) file as a debug data source
        hr = mSource->loadDataFromPdb( wszFilename );
        if ( FAILED( hr ) )
            return hr;
#endif

        // Open a session for querying symbols
        hr = mSource->openSession( &mSession );
        if ( FAILED( hr ) )
            return hr;

        // Retrieve a reference to the global scope
        hr = mSession->get_globalScope( &mGlobal );
        if ( hr != S_OK )
            return hr;

        // Set Machine type for getting correct register names
        DWORD dwMachType = 0;
        if ( mGlobal->get_machineType( &dwMachType ) == S_OK ) 
        {
            switch ( dwMachType ) 
            {
            case IMAGE_FILE_MACHINE_I386 : mMachineType = CV_CFL_80386; break;
            case IMAGE_FILE_MACHINE_IA64 : mMachineType = CV_CFL_IA64; break;
            case IMAGE_FILE_MACHINE_AMD64 : mMachineType = CV_CFL_AMD64; break;
            case IMAGE_FILE_MACHINE_CEE : mMachineType = CV_CFL_CEE; break;
            default:
#if defined _DEBUG
                if( IsDebuggerPresent() )
                    __debugbreak();
#endif
                return E_PDB_NOT_IMPLEMENTED;
            }
        }

        return S_OK;
    }

    void PDBDebugStore::CloseDebugInfo()
    {
        if( !mInit )
            return;

        releaseFindLineEnumLineNumbers();

        if ( mGlobal ) 
        {
            mGlobal->Release();
            mGlobal = NULL;
        }

        if ( mSession )
        {
            mSession->Release();
            mSession = NULL;
        }

        if ( mSource )
        {
            mSource->Release();
            mSource = NULL;
        }

        CoUninitialize();
        mInit = false;
    }

    void PDBDebugStore::releaseFindLineEnumLineNumbers()
    {
        if( mFindLineEnumLineNumbers )
        {
            mFindLineEnumLineNumbers->Release();
            mFindLineEnumLineNumbers = NULL;
        }
        
    }

    HRESULT PDBDebugStore::SetCompilandSymbolScope( DWORD compilandIndex, SymbolScope& scope )
    {
        UNREFERENCED_PARAMETER( compilandIndex );
        UNREFERENCED_PARAMETER( scope );
        // not used
        assert(false);
        return E_NOTIMPL;
    }

    HRESULT PDBDebugStore::SetSymbolScope( SymbolHeapId heapId, SymbolScope& scope )
    {
        UNREFERENCED_PARAMETER( heapId );
        UNREFERENCED_PARAMETER( scope );
        // not used
        assert(false);
        return E_NOTIMPL;
    }

    HRESULT PDBDebugStore::SetChildSymbolScope( SymHandle handle, SymbolScope& scope )
    {
        PDBStore::SymHandleIn& symIn = (PDBStore::SymHandleIn&) handle;
        PDBStore::SymbolScopeIn& scopeIn = (PDBStore::SymbolScopeIn&) scope;

        scopeIn.id = symIn.id;
        scopeIn.current = 0;
        return S_OK;
    }

    bool PDBDebugStore::NextSymbol( SymbolScope& scope, SymHandle& handle )
    {
        PDBStore::SymHandleIn& symIn = (PDBStore::SymHandleIn&) handle;
        PDBStore::SymbolScopeIn& scopeIn = (PDBStore::SymbolScopeIn&) scope;

        IDiaSymbol* pSymbol = NULL;
        HRESULT hr = mSession->symbolById( scopeIn.id, &pSymbol );
        IDiaEnumSymbols* pEnumSymbols = NULL;
        if ( !FAILED( hr ) && pSymbol )
            hr = pSymbol->findChildren( SymTagNull, NULL, nsNone, &pEnumSymbols );
        IDiaSymbol* pChild = NULL;
        if ( !FAILED( hr ) && pEnumSymbols )
            hr = pEnumSymbols->Item( scopeIn.current, &pChild );
        if ( !FAILED( hr ) && pChild )
            scopeIn.current++;
        if ( !FAILED( hr ) && pChild )
            hr = pChild->get_symIndexId( &symIn.id );

        if ( pChild )
            pChild->Release();
        if ( pEnumSymbols )
            pEnumSymbols->Release();
        if ( pSymbol )
            pSymbol->Release();
        return !FAILED( hr ) && pChild;
    }

    HRESULT PDBDebugStore::FindFirstSymbol( SymbolHeapId heapId, const char* nameChars, size_t nameLen, EnumNamedSymbolsData& data )
    {
        PDBStore::EnumNamedSymbolsDataIn& dataIn = (PDBStore::EnumNamedSymbolsDataIn&) data;
        if ( heapId == SymHeap_GlobalSymbols )
        {
            IDiaEnumSymbols* pEnumSymbols = NULL;
            int len = MultiByteToWideChar( CP_UTF8, 0, nameChars, nameLen, NULL, 0 );
            std::auto_ptr<wchar_t> wname (new wchar_t[len+1]);
            if ( wname.get() == NULL )
                return E_OUTOFMEMORY;
            MultiByteToWideChar( CP_UTF8, 0, nameChars, nameLen, wname.get (), len);
            wname.get()[len] = L'\0';

            HRESULT hr = mGlobal->findChildren( SymTagNull, wname.get (), nsCaseSensitive, &pEnumSymbols );
            
            IDiaSymbol* pSymbol = NULL;
            if ( !FAILED( hr ) && pEnumSymbols )
                hr = pEnumSymbols->Item( 0, &pSymbol );

            if ( !FAILED( hr ) && pSymbol )
                hr = pSymbol->get_symIndexId( &dataIn.id );

            if ( pSymbol )
                pSymbol->Release();
            if ( pEnumSymbols )
                pEnumSymbols->Release();

            return hr;
        }
        return E_NOTIMPL;
    }


    HRESULT PDBDebugStore::FindNextSymbol( EnumNamedSymbolsData& handle )
    {
        UNREFERENCED_PARAMETER( handle );
        // not used
        assert(false);
        return E_NOTIMPL;
    }

    HRESULT PDBDebugStore::GetCurrentSymbol( const EnumNamedSymbolsData& searchData, SymHandle& handle )
    {
        const PDBStore::EnumNamedSymbolsDataIn& dataIn = (const PDBStore::EnumNamedSymbolsDataIn&) searchData;
        PDBStore::SymHandleIn& symIn = (PDBStore::SymHandleIn&) handle;
        symIn.id = dataIn.id;
        return S_OK;
    }

    HRESULT PDBDebugStore::FindSymbol( SymbolHeapId heapId, WORD segment, DWORD offset, SymHandle& handle )
    {
        UNREFERENCED_PARAMETER( heapId );

        PDBStore::SymHandleIn& handleIn = (PDBStore::SymHandleIn&) handle;

        IDiaSymbol* pSymbol1 = NULL;
        IDiaSymbol* pSymbol2 = NULL;
        IDiaSymbol* pSymbol3 = NULL;
        mSession->findSymbolByAddr( segment, offset, SymTagPublicSymbol, &pSymbol1 );
        mSession->findSymbolByAddr( segment, offset, SymTagFunction, &pSymbol2 );
        mSession->findSymbolByAddr( segment, offset, SymTagData, &pSymbol3 );
        IDiaSymbol* pSymbol = pSymbol1;
        DWORD symoff = 0;
        DWORD off, sec;
        if( pSymbol1 && pSymbol1->get_addressSection( &sec ) == S_OK && sec == segment )
            (pSymbol = pSymbol1)->get_addressOffset(&symoff);

        if( pSymbol2 && pSymbol2->get_addressSection( &sec ) == S_OK && sec == segment )
            if( pSymbol2->get_addressOffset(&off) == S_OK && off >= symoff )
                pSymbol = pSymbol2, symoff = off;
        if( pSymbol3 && pSymbol3->get_addressSection( &sec ) == S_OK && sec == segment )
            if( pSymbol3->get_addressOffset(&off) == S_OK && off >= symoff )
                pSymbol = pSymbol3, symoff = off;
        
        HRESULT hr = E_FAIL;
        if( pSymbol )
            hr = pSymbol->get_symIndexId( &handleIn.id );

        if( pSymbol1 )
            pSymbol1->Release();
        if( pSymbol2 )
            pSymbol2->Release();
        if( pSymbol3 )
            pSymbol3->Release();
        return !FAILED( hr ) ? S_OK : E_FAIL;
    }


    HRESULT PDBDebugStore::GetSymbolInfo( SymHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo )
    {
        PDBStore::SymHandleIn& handleIn = (PDBStore::SymHandleIn&) handle;

        symInfo = new (&privateData) PDBSymbolInfo( this, handleIn.id );
        return S_OK;
    }

    HRESULT PDBDebugStore::SetGlobalTypeScope( TypeScope& scope )
    {
        PDBStore::TypeScopeIn& scopeIn = (PDBStore::TypeScopeIn&) scope;

        scopeIn.current = 0;
        return mGlobal->get_symIndexId( &scopeIn.id );
    }

    bool PDBDebugStore::GetTypeFromTypeIndex( TypeIndex typeIndex, TypeHandle& handle )
    {
        PDBStore::TypeHandleIn& handleIn = (PDBStore::TypeHandleIn&) handle;
        handleIn.id = typeIndex;
        return true;
    }

    HRESULT PDBDebugStore::SetChildTypeScope( TypeHandle handle, TypeScope& scope )
    {
        PDBStore::TypeHandleIn& typeIn = (PDBStore::TypeHandleIn&) handle;
        PDBStore::TypeScopeIn& scopeIn = (PDBStore::TypeScopeIn&) scope;

        scopeIn.id = typeIn.id;
        scopeIn.current = 0;

        return S_OK;
    }

    bool PDBDebugStore::NextType( TypeScope& scope, TypeHandle& handle )
    {
        PDBStore::TypeHandleIn& typeIn = (PDBStore::TypeHandleIn&) handle;
        PDBStore::TypeScopeIn& scopeIn = (PDBStore::TypeScopeIn&) scope;

        IDiaSymbol* pSymbol = NULL;
        HRESULT hr = mSession->symbolById( scopeIn.id, &pSymbol );
        IDiaEnumSymbols* pEnumSymbols = NULL;
        if ( !FAILED( hr ) )
            hr = pSymbol->findChildren( SymTagNull, NULL, nsNone, &pEnumSymbols );
        IDiaSymbol* pChild = NULL;
        if ( !FAILED( hr ) )
            hr = pEnumSymbols->Item( scopeIn.current, &pChild );
        if ( !FAILED( hr ) )
            scopeIn.current++;
        if ( !FAILED( hr ) )
            hr = pChild->get_symIndexId( &typeIn.id );

        if ( pChild )
            pChild->Release();
        if ( pEnumSymbols )
            pEnumSymbols->Release();
        if ( pSymbol )
            pSymbol->Release();
        return !FAILED( hr );
    }


    HRESULT PDBDebugStore::GetTypeInfo( TypeHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo )
    {
        PDBStore::TypeHandleIn& handleIn = (PDBStore::TypeHandleIn&) handle;

        symInfo = new (&privateData) PDBSymbolInfo( this, handleIn.id );
        return S_OK;
    }

    uint32_t PDBDebugStore::getCompilandCount()
    {
        if( mCompilandCount < 0 )
        {
            IDiaEnumSymbols *pEnumSymbols;

            if ( FAILED( mGlobal->findChildren( SymTagCompiland, NULL, nsNone, &pEnumSymbols ) ) )
                mCompilandCount = 0;
            else
            {
                if( FAILED( pEnumSymbols->get_Count( &mCompilandCount ) ) )
                    mCompilandCount = 0;
                pEnumSymbols->Release();
            }
        }
        return mCompilandCount;
    }

    HRESULT PDBDebugStore::GetCompilandCount( uint32_t& count )
    {
        count = getCompilandCount();
        return S_OK;
    }

    HRESULT PDBDebugStore::GetCompilandInfo( uint16_t index, CompilandInfo& info )
    {
        if ( (index < 1) || (index > getCompilandCount()) )
            return E_INVALIDARG;

        IDiaEnumSymbols *pEnumSymbols = NULL;
        HRESULT hr = mGlobal->findChildren( SymTagCompiland, NULL, nsNone, &pEnumSymbols );

        IDiaSymbol *pCompiland = NULL;
        if( !FAILED( hr ) )
            hr = pEnumSymbols->Item( index - 1, &pCompiland );

        BSTR bstrName = NULL;
        if( !FAILED( hr ) )
            hr = pCompiland->get_name( &bstrName );

        if( !FAILED( hr ) )
        {
            detachBSTR( bstrName, info.Name );
            hr = (info.Name.GetName() == NULL) ? E_OUTOFMEMORY : S_OK;
        }

        IDiaEnumSourceFiles *pFiles = NULL;
        if( !FAILED( hr ) )
            hr = mSession->findFile( pCompiland, NULL, nsNone, &pFiles );

        LONG fileCount = 0;
        if( !FAILED( hr ) )
            hr = pFiles->get_Count( &fileCount );

        info.FileCount = (WORD) fileCount;
        info.SegmentCount = 1;

        if( pFiles )
            pFiles->Release();
        if( pCompiland )
            pCompiland->Release();
        if( pEnumSymbols )
            pEnumSymbols->Release();

        return hr;
    }

    HRESULT PDBDebugStore::GetCompilandSegmentInfo( uint16_t index, uint16_t count, SegmentInfo* infos )
    {
        UNREFERENCED_PARAMETER( index );
        UNREFERENCED_PARAMETER( count );
        UNREFERENCED_PARAMETER( infos );
        // not used?
        assert( false );
        return E_NOTIMPL;
    }

    HRESULT PDBDebugStore::GetFileInfo( uint16_t compilandIndex, uint16_t fileIndex, FileInfo& info )
    {
        if ( (compilandIndex < 1) || (compilandIndex > getCompilandCount()) )
            return E_INVALIDARG;

        IDiaEnumSymbols *pEnumSymbols = NULL;
        HRESULT hr = mGlobal->findChildren( SymTagCompiland, NULL, nsNone, &pEnumSymbols );

        IDiaSymbol *pCompiland = NULL;
        if( !FAILED( hr ) )
            hr = pEnumSymbols->Item( compilandIndex - 1, &pCompiland );

        IDiaEnumSourceFiles *pFiles = NULL;
        if( !FAILED( hr ) )
            hr = mSession->findFile( pCompiland, NULL, nsNone, &pFiles );

        LONG fileCount = 0;
        if( !FAILED( hr ) )
            hr = pFiles->get_Count( &fileCount );

        if( !FAILED( hr ) && fileIndex >= fileCount )
            hr = E_INVALIDARG;

        IDiaSourceFile *pSourceFile = NULL;
        if( !FAILED( hr ) )
            hr = pFiles->Item( fileIndex, &pSourceFile );

        BSTR bstrName = NULL;
        if( !FAILED( hr ) )
            hr = pSourceFile->get_fileName( &bstrName );

        if( !FAILED( hr ) )
        {
            detachBSTR( bstrName, info.Name );
            hr = (info.Name.GetName() == NULL) ? E_OUTOFMEMORY : S_OK;
        }

        info.SegmentCount = 1;

        if( pSourceFile )
            pSourceFile->Release();
        if( pFiles )
            pFiles->Release();
        if( pCompiland )
            pCompiland->Release();
        if( pEnumSymbols )
            pEnumSymbols->Release();

        return hr;
    }

    HRESULT PDBDebugStore::GetFileSegmentInfo( uint16_t compilandIndex, uint16_t fileIndex, uint16_t count, SegmentInfo* infos )
    {
        UNREFERENCED_PARAMETER( compilandIndex );
        UNREFERENCED_PARAMETER( fileIndex );
        UNREFERENCED_PARAMETER( count );
        UNREFERENCED_PARAMETER( infos );
        // not used?
        assert( false );
        return E_NOTIMPL;
    }

    HRESULT PDBDebugStore::GetLineInfo( uint16_t compilandIndex, uint16_t fileIndex, uint16_t segInstanceIndex, uint16_t count, LineInfo* infos )
    {
        UNREFERENCED_PARAMETER( compilandIndex );
        UNREFERENCED_PARAMETER( fileIndex );
        UNREFERENCED_PARAMETER( segInstanceIndex );
        UNREFERENCED_PARAMETER( count );
        UNREFERENCED_PARAMETER( infos );
        assert( false );
        return E_NOTIMPL;
    }

    bool PDBDebugStore::GetFileSegment( uint16_t compIndex, uint16_t fileIndex, uint16_t segInstanceIndex, FileSegmentInfo& segInfo )
    {
        if ( (compIndex < 1) || (compIndex > getCompilandCount()) )
            return false;
        if( segInstanceIndex > 0 )
            return false;


        IDiaEnumSymbols *pEnumSymbols = NULL;
        HRESULT hr = mGlobal->findChildren( SymTagCompiland, NULL, nsNone, &pEnumSymbols );

        IDiaSymbol *pCompiland = NULL;
        if( !FAILED( hr ) )
            hr = pEnumSymbols->Item( compIndex - 1, &pCompiland );

        IDiaEnumSourceFiles *pFiles = NULL;
        if( !FAILED( hr ) )
            hr = mSession->findFile( pCompiland, NULL, nsNone, &pFiles );

        LONG fileCount = 0;
        if( !FAILED( hr ) )
            hr = pFiles->get_Count( &fileCount );

        if( !FAILED( hr ) && fileIndex >= fileCount )
            hr = E_INVALIDARG;

        IDiaSourceFile *pSourceFile = NULL;
        if( !FAILED( hr ) )
            hr = pFiles->Item( fileIndex, &pSourceFile );

        IDiaEnumLineNumbers *pEnumLineNumbers = NULL;
        if( !FAILED( hr ) )
            hr = mSession->findLines( pCompiland, pSourceFile, &pEnumLineNumbers );

        if( !FAILED( hr ) )
            hr = fillFileSegmentInfo( pEnumLineNumbers, segInfo );

        if( pEnumLineNumbers )
            pEnumLineNumbers->Release();
        if( pSourceFile )
            pSourceFile->Release();
        if( pFiles )
            pFiles->Release();
        if( pCompiland )
            pCompiland->Release();
        if( pEnumSymbols )
            pEnumSymbols->Release();

        return !FAILED( hr );
    }

    HRESULT PDBDebugStore::fillFileSegmentInfo( IDiaEnumLineNumbers *pEnumLineNumbers, FileSegmentInfo& segInfo )
    {
        HRESULT hr = S_OK;
        LONG lineNumbers = 0;
        // TODO: No samples or documentation show that Reset needs to be called before the first 
        //       call to Next. But sometimes, Next doesn't return all the elements here unless 
        //       you call Reset first. Why is that?
        hr = pEnumLineNumbers->Reset();
        if( !FAILED( hr ) )
            hr = pEnumLineNumbers->get_Count( &lineNumbers );

        if( !FAILED( hr ) )
        {
            mLastSegInfoLineNumbers.reset( new WORD[lineNumbers] );
            mLastSegInfoOffsets.reset( new DWORD[lineNumbers] );
            if ( mLastSegInfoLineNumbers.get() == NULL || mLastSegInfoOffsets.get() == NULL )
                hr = E_OUTOFMEMORY;
        }
        LONG lineIndex = 0;
        DWORD section = 0;
        segInfo.SegmentInstance = 0;
        while( !FAILED( hr ) && lineIndex < lineNumbers )
        {
            IDiaLineNumber* pLineNumber = NULL;
            ULONG fetched = 0;
            hr = pEnumLineNumbers->Next( 1, &pLineNumber, &fetched );
            if ( hr != S_OK || fetched == 0 )
                break;

            DWORD off = 0, line = 0;
            if( !FAILED( hr ) )
                hr = pLineNumber->get_addressOffset( &off );
            if( !FAILED( hr ) )
                hr = pLineNumber->get_lineNumber( &line );
            if( !FAILED( hr ) && lineIndex == 0 )
            {
                //segInfo.SegmentInstance = (WORD) line;
                segInfo.Start = off;
                hr = pLineNumber->get_addressSection( &section );
            }
            if( !FAILED( hr ) && lineIndex == lineNumbers - 1 )
            {
                DWORD length = 1;
                hr = pLineNumber->get_length( &length );
                hr = pLineNumber->get_addressOffset( &segInfo.End );
                if ( length > 0 )
                    segInfo.End += length - 1;
            }

            mLastSegInfoOffsets.get()[lineIndex] = off;
            mLastSegInfoLineNumbers.get()[lineIndex] = (WORD) line;

            lineIndex++;
            pLineNumber->Release();
        }
        segInfo.SegmentIndex = (WORD) section;
        segInfo.LineCount = (WORD) lineIndex;
        segInfo.Offsets = mLastSegInfoOffsets.get ();
        segInfo.LineNumbers = mLastSegInfoLineNumbers.get ();
        return hr;
    }

    HRESULT PDBDebugStore::findCompilandAndFile( IDiaSymbol *pCompiland, IDiaSourceFile *pSourceFile, uint16_t& compIndex, uint16_t& fileIndex )
    {
        if( !pCompiland || !pSourceFile )
            return E_INVALIDARG;

        IDiaEnumSymbols *pEnumSymbols = NULL;
        HRESULT hr = mGlobal->findChildren( SymTagCompiland, NULL, nsNone, &pEnumSymbols );

        DWORD cid;
        pCompiland->get_symIndexId( &cid );
        compIndex = 1;
        fileIndex = 0;
        IDiaSymbol *pComp = NULL;
        while( hr == S_OK )
        {
            ULONG fetched = 0;
            hr = pEnumSymbols->Next( 1, &pComp, &fetched );
            if( hr == S_OK )
            {
                DWORD id;
                pComp->get_symIndexId( &id );
                pComp->Release();
                if( cid == id )
                    break;
            }
            compIndex++;
        }
        if( pEnumSymbols )
            pEnumSymbols->Release();

        DWORD sid;
        pSourceFile->get_uniqueId( &sid );

        IDiaEnumSourceFiles *pEnumFiles = NULL;
        if( hr == S_OK )
            hr = mSession->findFile( pCompiland, NULL, nsNone, &pEnumFiles );

        IDiaSourceFile *pFile = NULL;
        while( hr == S_OK )
        {
            ULONG fetched = 0;
            hr = pEnumFiles->Next( 1, &pFile, &fetched );
            if( hr == S_OK )
            {
                DWORD id;
                pFile->get_uniqueId( &id );
                pFile->Release();
                if( sid == id )
                    break;
            }
            fileIndex++;
        }
        if( pEnumFiles )
            pEnumFiles->Release();
        return hr;
    }

    HRESULT PDBDebugStore::setLineNumber( IDiaLineNumber* pLineNumber, uint16_t lineIndex, LineNumber& lineNumber )
    {
        HRESULT hr = S_OK;

        IDiaSymbol *pCompiland = NULL;
        IDiaSourceFile *pSourceFile = NULL;
        if( !FAILED( hr ) )
            hr = pLineNumber->get_compiland( &pCompiland );
        if( !FAILED( hr ) )
            hr = pLineNumber->get_sourceFile( &pSourceFile );

        uint16_t compIndex = 0, fileIndex = 0;
        if( !FAILED( hr ) )
            hr = findCompilandAndFile( pCompiland, pSourceFile, compIndex, fileIndex );

        if ( pCompiland )
            pCompiland->Release();
        if ( pSourceFile )
            pSourceFile->Release();

        DWORD line = 0, lineEnd = 0, offset = 0, section = 0, length = 0;
        if( !FAILED( hr ) )
            hr = pLineNumber->get_lineNumber( &line );
        if( !FAILED( hr ) )
            hr = pLineNumber->get_lineNumberEnd( &lineEnd );
        if( !FAILED( hr ) )
            hr = pLineNumber->get_addressOffset( &offset );
        if( !FAILED( hr ) )
            hr = pLineNumber->get_addressSection( &section );
        if( !FAILED( hr ) )
            hr = pLineNumber->get_length( &length );

        if( !FAILED( hr ) )
        {
            lineNumber.CompilandIndex = compIndex;
            lineNumber.FileIndex = fileIndex;
            lineNumber.LineIndex = lineIndex;

            lineNumber.Number = (uint16_t) line;
            lineNumber.NumberEnd = (uint16_t) lineEnd;
            lineNumber.Offset = (uint32_t) offset;
            lineNumber.Section = (uint16_t) section;
            lineNumber.Length = (uint32_t) length;
        }
        return hr;
    }

    bool PDBDebugStore::FindLine( WORD seg, uint32_t offset, LineNumber& lineNumber )
    {
        HRESULT hr = S_OK;
        IDiaEnumLineNumbers *pEnumLineNumbers = NULL;
        if( !FAILED( hr ) )
            hr = mSession->findLinesByAddr( seg, offset, 1, &pEnumLineNumbers );

        LONG lineNumbers = 0;
        IDiaLineNumber* pLineNumber = NULL;
        if( !FAILED( hr ) )
            hr = pEnumLineNumbers->get_Count( &lineNumbers );
        if( !FAILED( hr ) )
            if( lineNumbers < 1 )
                hr = E_INVALIDARG;
        if( !FAILED( hr ) )
            hr = pEnumLineNumbers->Item( 0, &pLineNumber );

        if( !FAILED( hr ) )
            setLineNumber( pLineNumber, 0, lineNumber );

        if( pLineNumber )
            pLineNumber->Release();
        if( pEnumLineNumbers )
            pEnumLineNumbers->Release();

        return !FAILED( hr );
    }

    bool PDBDebugStore::FindLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber )
    {
        if ( (compIndex < 1) || (compIndex > getCompilandCount()) )
            return false;

        releaseFindLineEnumLineNumbers();

        IDiaEnumSymbols *pEnumSymbols = NULL;
        HRESULT hr = mGlobal->findChildren( SymTagCompiland, NULL, nsNone, &pEnumSymbols );

        IDiaSymbol *pCompiland = NULL;
        if( !FAILED( hr ) )
            hr = pEnumSymbols->Item( compIndex - 1, &pCompiland );

        IDiaEnumSourceFiles *pFiles = NULL;
        if( !FAILED( hr ) )
            hr = mSession->findFile( pCompiland, NULL, nsNone, &pFiles );

        LONG fileCount = 0;
        if( !FAILED( hr ) )
            hr = pFiles->get_Count( &fileCount );

        if( !FAILED( hr ) && fileIndex >= fileCount )
            hr = E_INVALIDARG;

        IDiaSourceFile *pSourceFile = NULL;
        if( !FAILED( hr ) )
            hr = pFiles->Item( fileIndex, &pSourceFile );

        if( !FAILED( hr ) )
            hr = mSession->findLinesByLinenum( pCompiland, pSourceFile, line, 0, &mFindLineEnumLineNumbers );

        if( !FAILED( hr ) )
            hr = mFindLineEnumLineNumbers->Reset();

        if( pSourceFile )
            pSourceFile->Release();
        if( pFiles )
            pFiles->Release();
        if( pCompiland )
            pCompiland->Release();
        if( pEnumSymbols )
            pEnumSymbols->Release();

        if( FAILED( hr ) )
            return false;

        return FindNextLineByNum( compIndex, fileIndex, line, lineNumber );
    }

    bool PDBDebugStore::FindNextLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber )
    {
        // assume arguments are the same as last call to FindLineByNum
        UNREFERENCED_PARAMETER( compIndex );
        UNREFERENCED_PARAMETER( fileIndex );
        UNREFERENCED_PARAMETER( line );

        if( !mFindLineEnumLineNumbers )
            return false;

        HRESULT hr = S_OK;
        IDiaLineNumber* pLineNumber = NULL;
        ULONG fetched = 0;
        if( !FAILED( hr ) )
            hr = mFindLineEnumLineNumbers->Next( 1, &pLineNumber, &fetched );
        if( hr == S_OK )
            hr = setLineNumber( pLineNumber, 0, lineNumber );
        else
            releaseFindLineEnumLineNumbers();

        if( pLineNumber )
            pLineNumber->Release();

        return hr == S_OK;
    }

    bool PDBDebugStore::FindLines( bool exactMatch, const char* fileName, size_t fileNameLen, uint16_t reqLineStart, uint16_t reqLineEnd, 
                                   std::list<LineNumber>& lines )
    {
        IDiaEnumSymbols *pEnumSymbols = NULL;
        HRESULT hr = mGlobal->findChildren( SymTagCompiland, NULL, nsNone, &pEnumSymbols );
        if( !FAILED( hr ) )
            pEnumSymbols->Reset();

        ULONG fetched;
        IDiaSymbol *pCompiland = NULL;
        while( !FAILED( hr ) && pEnumSymbols->Next( 1, &pCompiland, &fetched ) == S_OK )
        {
            IDiaEnumSourceFiles *pFiles = NULL;
            if( !FAILED( hr ) )
                hr = mSession->findFile( pCompiland, NULL, nsNone, &pFiles );

            IDiaSourceFile *pSourceFile = NULL;
            while( !FAILED( hr ) && pFiles->Next( 1, &pSourceFile, &fetched ) == S_OK )
            {
                BSTR bstrName = NULL;
                if( !FAILED( hr ) )
                    hr = pSourceFile->get_fileName( &bstrName );
                
                SymString srcFileName;
                if( !FAILED( hr ) )
                {
                    detachBSTR( bstrName, srcFileName );
                    hr = (srcFileName.GetName() == NULL) ? E_OUTOFMEMORY : S_OK;
                }

                bool matches = false;
                if( !FAILED( hr ) )
                {
                    if ( exactMatch )
                        matches = ExactFileNameMatch( fileName, fileNameLen, srcFileName.GetName(), srcFileName.GetLength() );
                    else
                        matches = PartialFileNameMatch( fileName, fileNameLen, srcFileName.GetName(), srcFileName.GetLength() );
                }
                if( matches )
                {
                    IDiaEnumLineNumbers *pEnumLineNumbers = 0;
                    if( !FAILED( hr ) )
                        hr = mSession->findLinesByLinenum( pCompiland, pSourceFile, reqLineStart, 0, &pEnumLineNumbers );
                    
                    IDiaLineNumber* pLineNumber = NULL;
                    while( !FAILED( hr ) && pEnumLineNumbers->Next( 1, &pLineNumber, &fetched ) == S_OK )
                    {
                        LineNumber line;
                        setLineNumber( pLineNumber, (uint16_t) lines.size(), line );
                        if( line.Number <= reqLineEnd && line.NumberEnd >= reqLineStart )
                            lines.push_back( line );
                        pLineNumber->Release();
                    }

                    if( pEnumLineNumbers )
                        pEnumLineNumbers->Release();
                }
                pSourceFile->Release();
            }
            if( pFiles )
                pFiles->Release();
            pCompiland->Release();
        }
        if( pEnumSymbols )
            pEnumSymbols->Release();
        return lines.size() > 0;
    }
}

