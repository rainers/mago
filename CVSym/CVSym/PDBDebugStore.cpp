/*
   Copyright (c) 2012 Rainer Schuetze

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "PDBDebugStore.h"
#include "ISymbolInfo.h"
#include "Util.h"

#ifndef NDEBUG
#include "../../DebugEngine/Exec/Log.h"
#endif

#include <dia2.h>
#include <assert.h>

#define UNREF_PARAM( p ) UNREFERENCED_PARAMETER( p )

namespace PDBStore
{
    struct SymbolScopeIn
    {
        DWORD id;
        IDiaEnumSymbols* pEnumSymbols;
    };

    struct TypeScopeIn
    {
        DWORD id;
        IDiaEnumSymbols* pEnumSymbols;
    };

    struct EnumNamedSymbolsDataIn
    {
        IDiaEnumSymbols* pEnumSymbols;
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
        std::unique_ptr<char> ptr( new char[nChars] );
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

#ifndef NDEBUG
        virtual void dump(DWORD id) const
        {
            IDiaSymbol* pSymbol = NULL;
            if( mStore->_symbolById( id, &pSymbol ) == S_OK )
            {
                char str[1280];
                DWORD tag = SymTagNull;
                pSymbol->get_symTag( &tag );

                BSTR bstrName = NULL;
                pSymbol->get_name( &bstrName );

                sprintf(str, "Id: 0x%x Tag: 0x%0x, Name:%S\n", id, tag, bstrName );
                Log::LogMessage( str );

                SysFreeString( bstrName );

                pSymbol->Release();
            }
        }
#endif

        virtual SymTag GetSymTag()
        {
            DWORD tag = SymTagNull;
            IDiaSymbol* pSymbol = NULL;
            if( mStore->_symbolById( mId, &pSymbol ) == S_OK )
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
            if( mStore->_symbolById( mId, &pSymbol ) == S_OK)
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
                    default:
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
            if( mStore->_symbolById( mId, &pSymbol ) != S_OK )
                return false;
            BSTR bstrName = NULL;
            pSymbol->get_name( &bstrName );
            detachBSTR( bstrName, name );
            pSymbol->Release();
            return name.GetName() != 0;
        }

        virtual bool GetAddressOffset( uint32_t& offset )
        {
            IDiaSymbol* pSymbol = NULL;
            if( mStore->_symbolById( mId, &pSymbol ) != S_OK )
                return false;

            HRESULT hr;
            // try thunk target address instead?
            hr = pSymbol->get_targetOffset( (DWORD*) &offset );
            if( hr != S_OK )
                hr = pSymbol->get_addressOffset( (DWORD*) &offset );
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetAddressSegment( uint16_t& segment ) 
        {
            IDiaSymbol* pSymbol = NULL;
            if( mStore->_symbolById( mId, &pSymbol ) != S_OK )
                return false;

            DWORD section = 0;
            HRESULT hr;
            hr = pSymbol->get_targetSection( &section );
            if( hr != S_OK )
                hr = pSymbol->get_addressSection( &section );
            pSymbol->Release();
            segment = (uint16_t) section;
            return hr == S_OK;
        }

        virtual bool GetDataKind( DataKind& dataKind ) 
        {
            IDiaSymbol* pSymbol = NULL;
            if( mStore->_symbolById( mId, &pSymbol ) != S_OK)
                return false;

            HRESULT hr = pSymbol->get_dataKind( (DWORD*) &dataKind );
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetLength( uint32_t& length )
        {
            IDiaSymbol* pSymbol = NULL;
            if( mStore->_symbolById( mId, &pSymbol ) != S_OK )
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
            if( mStore->_symbolById( mId, &pSymbol ) != S_OK )
                return false;

            HRESULT hr = pSymbol->get_locationType( (DWORD*) &locType );
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetOffset( int32_t& offset )
        {
            IDiaSymbol* pSymbol = NULL;
            if( mStore->_symbolById( mId, &pSymbol ) != S_OK )
                return false;

            HRESULT hr = pSymbol->get_offset( (LONG*) &offset );
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetRegister( uint32_t& reg )
        {
            IDiaSymbol* pSymbol = NULL;
            if( mStore->_symbolById( mId, &pSymbol ) != S_OK )
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
            if( mStore->_symbolById( mId, &pSymbol ) != S_OK )
                return false;

            HRESULT hr = pSymbol->get_udtKind( (DWORD*) &udtKind );
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetValue( Variant& value ) 
        {
            IDiaSymbol* pSymbol = NULL;
            if( mStore->_symbolById( mId, &pSymbol ) != S_OK )
                return false;

            VARIANT var;
            VariantInit( &var );
            HRESULT hr = pSymbol->get_value( &var );
            if( hr == S_OK )
                switch( var.vt )
                {
                case VT_I1:  value.Tag = VarTag_Char;      value.Data.I8 = var.bVal; break;
                case VT_UI1: value.Tag = VarTag_UChar;     value.Data.U8 = var.bVal; break;
                case VT_I2:  value.Tag = VarTag_Short;     value.Data.I16 = var.iVal; break;
                case VT_UI2: value.Tag = VarTag_UShort;    value.Data.U16 = var.iVal; break;
                case VT_I4:  value.Tag = VarTag_Long;      value.Data.I32 = var.lVal; break;
                case VT_UI4: value.Tag = VarTag_ULong;     value.Data.U32 = var.lVal; break;
                case VT_I8:  value.Tag = VarTag_Quadword;  value.Data.I64 = var.llVal; break;
                case VT_UI8: value.Tag = VarTag_UQuadword; value.Data.U64 = var.llVal; break;
                default:     value.Tag = VarTag_Long;      value.Data.I32 = 0; 
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
            if( mStore->_symbolById( mId, &pSymbol ) != S_OK )
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
            if ( mStore->_symbolById( mId, &pSymbol ) != S_OK )
                return false;

            HRESULT hr = pSymbol->get_count( (DWORD*) &count );
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetFieldCount( uint16_t& count )
        {
            IDiaSymbol* pSymbol = NULL;
            if( mStore->_symbolById( mId, &pSymbol ) != S_OK )
                return false;

            LONG cnt = 0;
            IDiaEnumSymbols* pEnumSymbols = NULL;
            HRESULT hr = pSymbol->findChildrenEx( SymTagNull, NULL, nsNone, &pEnumSymbols );
            if ( hr == S_OK )
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
        virtual bool GetVShape( TypeIndex& index )
        {
            IDiaSymbol* pSymbol = NULL;
            if ( mStore->_symbolById( mId, &pSymbol ) != S_OK )
                return false;

            HRESULT hr = pSymbol->get_virtualTableShapeId( &index );
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetVtblOffset( int& offset )
        {
            IDiaSymbol* pSymbol = NULL;
            if ( mStore->_symbolById( mId, &pSymbol ) != S_OK )
                return false;

            BOOL virt;
            HRESULT hr = pSymbol->get_virtual( &virt );

            DWORD off = 0;
            if( hr == S_OK )
                hr = virt ? pSymbol->get_virtualBaseOffset( &off ) : E_FAIL;
            offset = off;
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetCallConv( uint8_t& callConv )
        {
            IDiaSymbol* pSymbol = NULL;
            if ( mStore->_symbolById(mId, &pSymbol) != S_OK )
                return false;

            DWORD cc;
            HRESULT hr = pSymbol->get_callingConvention( &cc );
            if ( hr == S_OK )
                callConv = (uint8_t)cc;
            pSymbol->Release();
            return hr == S_OK;
        }
        virtual bool GetParamCount( uint16_t& count ) { UNREF_PARAM( count ); return false; }
        virtual bool GetParamList( TypeIndex& index )
        {
            index = mId; // type index the same as the symbol ID?
            return true;
        }

        virtual bool GetClass( TypeIndex& index )
        {
            IDiaSymbol* pSymbol = NULL;
            if ( mStore->_symbolById( mId, &pSymbol ) != S_OK )
                return false;

            IDiaSymbol* classSymbol = NULL;
            HRESULT hr = pSymbol->get_classParent( &classSymbol );
            if( hr == S_OK )
            {
                // allow indirection through pointer to class
                DWORD tag;
                if ( classSymbol->get_symTag( &tag ) == S_OK && tag == SymTagPointerType )
                    hr = classSymbol->get_typeId( (DWORD*) &index );
                else
                    hr = classSymbol->get_symIndexId( (DWORD*)&index );
                classSymbol->Release();
            }
            pSymbol->Release();
            return hr == S_OK;
        }

        virtual bool GetThis( TypeIndex& index )
        {
            IDiaSymbol* pSymbol = NULL;
            if (mStore->_symbolById(mId, &pSymbol) != S_OK)
                return false;

            IDiaSymbol* thisSymbol = NULL;
            HRESULT hr = pSymbol->get_objectPointerType(&thisSymbol);
            if( hr == S_OK)
            {
                DWORD typeID;
                hr = thisSymbol->get_symIndexId(&index);
                thisSymbol->Release();
            }
            pSymbol->Release();
            return hr == S_OK;
        }
        virtual bool GetThisAdjust( int32_t& adjust ) { UNREF_PARAM( adjust ); return false; }

        virtual bool GetOemId( uint32_t& oemId ) { UNREF_PARAM( oemId ); return false; }
        virtual bool GetOemSymbolId( uint32_t& oemSymId ) { UNREF_PARAM( oemSymId ); return false; }
        virtual bool GetTypes( std::vector<TypeIndex>& indexes )
        {
	    // supposed to work for OEM types and argument lists
            IDiaSymbol* pSymbol = NULL;
            if ( mStore->_symbolById( mId, &pSymbol ) != S_OK )
                return false;

            DWORD count;
            HRESULT hr = pSymbol->get_count( (DWORD*) &count );
            if( hr == S_OK )
            {
                indexes.resize( count );
                if( count > 0 )
                {
                    hr = pSymbol->get_typeIds( count, &count, indexes.data() );
                    if( hr != S_OK )
                    {
                        indexes.resize( 0 );
                        IDiaEnumSymbols* pEnumSymbols = NULL;
                        hr = pSymbol->findChildren( SymTagNull, NULL, nsNone, &pEnumSymbols );
                        if( hr == S_OK && pEnumSymbols )
                        {
                            for( DWORD i = 0; i < count && hr == S_OK; i++ )
                            {
                                DWORD fetched;
                                IDiaSymbol* argSymbol = nullptr;
                                if( ( hr = pEnumSymbols->Next( 1, &argSymbol, &fetched ) ) != S_OK )
                                    break;

                                // filter out btNoType arguments
                                DWORD typeID;
                                if( argSymbol->get_typeId( &typeID ) == S_OK )
                                {
                                    IDiaSymbol* typeSymbol = NULL;
                                    if( mStore->_symbolById( typeID, &typeSymbol ) == S_OK )
                                    {
                                        DWORD baseType;
                                        if( typeSymbol->get_baseType( &baseType ) != S_OK || baseType != btNoType )
                                        {
                                            indexes.push_back( typeID );
                                        }
                                        typeSymbol->Release();
                                    }
                                }
                                argSymbol->Release();
                            }
                            pEnumSymbols->Release();
                        }
                    }
                }
            }
            pSymbol->Release();
            return SUCCEEDED( hr );
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
            IDiaSymbol* pSymbol = NULL;
            if ( mStore->_symbolById( mId, &pSymbol ) != S_OK )
                return false;

            HRESULT hr;
            mod = 0;
            BOOL isConst, isStatic, isVirtual;
            hr = pSymbol->get_constType( &isConst );
            if( hr == S_OK && isConst )
                mod |= 1; // EED::MODconst

            hr = pSymbol->get_isStatic( &isStatic );
            if ( hr == S_OK && isStatic )
                mod |= 0x10; // EED::MODstatic

            hr = pSymbol->get_virtual( &isVirtual );
            if ( hr == S_OK && isVirtual )
                mod |= 0x20; // EED::MODvirtual

            pSymbol->Release();
            return true;
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
        GUID msdia140 = { 0xe6756135, 0x1e65, 0x4d17, { 0x85, 0x76, 0x61, 0x07, 0x61, 0x39, 0x8c, 0x3c } };
        GUID msdia120 = { 0x3BFCEA48, 0x620F, 0x4B6B, { 0x81, 0xF7, 0xB9, 0xAF, 0x75, 0x45, 0x4C, 0x7D } };
        GUID msdia110 = { 0x761D3BCD, 0x1304, 0x41D5, { 0x94, 0xE8, 0xEA, 0xC5, 0x4E, 0x4A, 0xC1, 0x72 } };
        GUID msdia100 = { 0xB86AE24D, 0xBF2F, 0x4AC9, { 0xB5, 0xA2, 0x34, 0xB1, 0x4E, 0x4C, 0xE1, 0x1D } }; // same as msdia80

        hr = CoCreateInstance( msdia140, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (void **) &mSource );
        if ( FAILED( hr ) )
            hr = CoCreateInstance( msdia120, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (void **) &mSource );
        if ( FAILED( hr ) )
            hr = CoCreateInstance( msdia110, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (void **) &mSource );
        if ( FAILED( hr ) )
            hr = CoCreateInstance( msdia100, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (void **) &mSource );
        if ( FAILED( hr ) )
            return hr;

#if 1

        // Load Wow64*Wow64FsRedirection functions dynamically to support Windows XP.
#define TRY_LOAD_WINAPI_FUNC(module, type, name, params) \
    typedef type (WINAPI *name ## Ptr) params; \
    name ## Ptr name = reinterpret_cast<name ## Ptr>(GetProcAddress(module, #name));

        HMODULE kernel32Module = GetModuleHandle(L"KERNEL32.dll");
        TRY_LOAD_WINAPI_FUNC(kernel32Module,
            BOOL, Wow64DisableWow64FsRedirection, (_Out_ PVOID *OldValue));
        TRY_LOAD_WINAPI_FUNC(kernel32Module,
            BOOL, Wow64RevertWow64FsRedirection, (_In_ PVOID OldValue));

#undef TRY_LOAD_WINAPI_FUNC


        PVOID OldValue = NULL;
        BOOL redir = Wow64DisableWow64FsRedirection &&
            Wow64DisableWow64FsRedirection( &OldValue );

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

        return initSession();
    }

    HRESULT PDBDebugStore::InitDebugInfo( IDiaSession* session )
    {
        if ( mInit )
            return E_ALREADY_INIT;

        HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
        if ( FAILED( hr ) )
            return hr;

        mInit = true;
        mSession = session;
        mSession->AddRef();

        return initSession();
    }

    HRESULT PDBDebugStore::initSession()
    {
        // Retrieve a reference to the global scope
        HRESULT hr = mSession->get_globalScope( &mGlobal );
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

    HRESULT PDBDebugStore::_symbolById( DWORD id, IDiaSymbol** pSymbol)
    {
        bool useMap = id >= 10'000'000;
        if ( useMap )
        {
            auto it = mSymbolCacheMap.find( id );
            if( it != mSymbolCacheMap.end() )
            {
                *pSymbol = it->second;
                (*pSymbol)->AddRef();
                return S_OK;
            }
        }
        else
        {
            if( id < mSymbolCache.size() && mSymbolCache[id] )
            {
                *pSymbol = mSymbolCache[id];
                (*pSymbol)->AddRef();
                return S_OK;
            }
        }
        HRESULT hr = mSession->symbolById( id, pSymbol );
        if( hr == S_OK && *pSymbol )
        {
            if ( useMap )
                mSymbolCacheMap[id] = *pSymbol;
            else
            {
                if ( mSymbolCache.size() <= id )
                    mSymbolCache.resize( id + 1000 );
                mSymbolCache[id] = *pSymbol;
            }
        }
        return hr;
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

        IDiaSymbol* pSymbol = NULL;
        HRESULT hr = _symbolById(symIn.id, &pSymbol);
        if (hr == S_OK && pSymbol)
            hr = pSymbol->findChildrenEx(SymTagNull, NULL, nsNone, &scopeIn.pEnumSymbols);
        scopeIn.id = symIn.id;
        if (pSymbol)
            pSymbol->Release();
        return hr;
    }

    bool PDBDebugStore::NextSymbol( SymbolScope& scope, SymHandle& handle, DWORD addr )
    {
        PDBStore::SymHandleIn& symIn = (PDBStore::SymHandleIn&) handle;
        PDBStore::SymbolScopeIn& scopeIn = (PDBStore::SymbolScopeIn&) scope;

        IDiaSymbol* pChild = NULL;
        if ( !scopeIn.pEnumSymbols )
            return S_FALSE;

        DWORD fetched = 0;
        HRESULT hr = scopeIn.pEnumSymbols->Next( 1, &pChild, &fetched );
        if ( hr == S_OK && pChild )
        {
            hr = pChild->get_symIndexId( &symIn.id );
            pChild->Release();
        }
        return hr == S_OK && pChild;
    }

    HRESULT PDBDebugStore::EndSymbolScope( SymbolScope& scope )
    {
        PDBStore::SymbolScopeIn& scopeIn = (PDBStore::SymbolScopeIn&)scope;
        if (scopeIn.pEnumSymbols)
        {
            scopeIn.pEnumSymbols->Release();
            scopeIn.pEnumSymbols = NULL;
        }
        return S_OK;
    }

    HRESULT PDBDebugStore::FindFirstSymbol( SymbolHeapId heapId, const char* nameChars, size_t nameLen, EnumNamedSymbolsData& data )
    {
        PDBStore::EnumNamedSymbolsDataIn& dataIn = (PDBStore::EnumNamedSymbolsDataIn&) data;
        if ( heapId == SymHeap_GlobalSymbols )
        {
            std::unique_ptr<wchar_t> wname;
            IDiaEnumSymbols* pEnumSymbols = NULL;
            if( nameChars )
            {
                int len = MultiByteToWideChar( CP_UTF8, 0, nameChars, nameLen, NULL, 0 );
                wname.reset (new wchar_t[len+1]);
                if ( wname.get() == NULL )
                    return E_OUTOFMEMORY;
                MultiByteToWideChar( CP_UTF8, 0, nameChars, nameLen, wname.get (), len);
                wname.get()[len] = L'\0';
            }
            HRESULT hr = mGlobal->findChildren( SymTagNull, wname.get (), nsCaseSensitive, &pEnumSymbols );
            
            if( hr == S_OK && pEnumSymbols )
            {
                IDiaSymbol* pSymbol = NULL;
                DWORD fetched = 0;
                hr = pEnumSymbols->Next( 1, &pSymbol, &fetched );
                if ( hr == S_OK && pSymbol )
                {
                    hr = pSymbol->get_symIndexId( &dataIn.id );
                    pSymbol->Release();
                    if( hr == S_OK )
                    {
                        dataIn.pEnumSymbols = pEnumSymbols;
                        return S_OK;
                    }
                }
                pEnumSymbols->Release();
            }

            return E_FAIL;
        }
        return E_NOTIMPL;
    }


    HRESULT PDBDebugStore::FindNextSymbol( EnumNamedSymbolsData& searchData )
    {
        PDBStore::EnumNamedSymbolsDataIn& dataIn = (PDBStore::EnumNamedSymbolsDataIn&) searchData;
        if( dataIn.pEnumSymbols == NULL )
            return E_INVALIDARG;

        IDiaSymbol* pSymbol = NULL;
        DWORD fetched = 0;
        HRESULT hr = dataIn.pEnumSymbols->Next( 1, &pSymbol, &fetched );
        if ( hr != S_OK || !pSymbol )
            return E_FAIL;

        hr = pSymbol->get_symIndexId( &dataIn.id );
        pSymbol->Release();
        return hr > S_OK ? E_FAIL : hr;
    }

    HRESULT PDBDebugStore::GetCurrentSymbol( const EnumNamedSymbolsData& searchData, SymHandle& handle )
    {
        const PDBStore::EnumNamedSymbolsDataIn& dataIn = (const PDBStore::EnumNamedSymbolsDataIn&) searchData;
        PDBStore::SymHandleIn& symIn = (PDBStore::SymHandleIn&) handle;
        symIn.id = dataIn.id;
        return S_OK;
    }

    HRESULT PDBDebugStore::FindSymbolDone( EnumNamedSymbolsData& searchData )
    {
        const PDBStore::EnumNamedSymbolsDataIn& dataIn = (const PDBStore::EnumNamedSymbolsDataIn&) searchData;
        if ( dataIn.pEnumSymbols )
            dataIn.pEnumSymbols->Release();
        return S_OK;
    }

    HRESULT PDBDebugStore::FindSymbol( SymbolHeapId heapId, WORD segment, DWORD offset, SymHandle& handle, DWORD& symOff )
    {
        UNREFERENCED_PARAMETER( heapId );

        PDBStore::SymHandleIn& handleIn = (PDBStore::SymHandleIn&) handle;

        IDiaSymbol* pSymbol1 = NULL;
        IDiaSymbol* pSymbol2 = NULL;
        IDiaSymbol* pSymbol3 = NULL;
        if( heapId == MagoST::SymHeap_PublicSymbols )
            mSession->findSymbolByAddr( segment, offset, SymTagPublicSymbol, &pSymbol1 );
        if ( heapId != MagoST::SymHeap_PublicSymbols )
            mSession->findSymbolByAddr( segment, offset, SymTagFunction, &pSymbol2 );
        if ( heapId != MagoST::SymHeap_PublicSymbols )
            mSession->findSymbolByAddr( segment, offset, SymTagData, &pSymbol3 );
        IDiaSymbol* pSymbol = pSymbol1;
        DWORD bestoff = 0;
        DWORD off, sec;
        if( pSymbol1 && pSymbol1->get_addressSection( &sec ) == S_OK && sec == segment )
            (pSymbol = pSymbol1)->get_addressOffset( &bestoff );

        if( pSymbol2 && pSymbol2->get_addressSection( &sec ) == S_OK && sec == segment )
            if( pSymbol2->get_addressOffset( &off ) == S_OK && off >= bestoff )
                pSymbol = pSymbol2, bestoff = off;
        if( pSymbol3 && pSymbol3->get_addressSection( &sec ) == S_OK && sec == segment )
            if( pSymbol3->get_addressOffset(&off) == S_OK && off >= bestoff )
                pSymbol = pSymbol3, bestoff = off;
        
        HRESULT hr = E_FAIL;
        if( pSymbol )
            hr = pSymbol->get_symIndexId( &handleIn.id );
        
        if ( hr == S_OK )
            symOff = bestoff - offset;

        if( pSymbol1 )
            pSymbol1->Release();
        if( pSymbol2 )
            pSymbol2->Release();
        if( pSymbol3 )
            pSymbol3->Release();
        return hr > S_OK ? E_FAIL : hr;
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

        HRESULT hr = mGlobal->get_symIndexId( &scopeIn.id );
        if( hr == S_OK )
            hr = mGlobal->findChildrenEx( SymTagNull, NULL, nsNone, &scopeIn.pEnumSymbols );
        return hr;
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

        IDiaSymbol* pSymbol = NULL;
        HRESULT hr = _symbolById( typeIn.id, &pSymbol );
        if ( hr == S_OK )
            hr = pSymbol->findChildren( SymTagNull, NULL, nsNone, &scopeIn.pEnumSymbols );
        if ( pSymbol )
            pSymbol->Release();
        return hr;
    }

    bool PDBDebugStore::NextType( TypeScope& scope, TypeHandle& handle )
    {
        PDBStore::TypeHandleIn& typeIn = (PDBStore::TypeHandleIn&) handle;
        PDBStore::TypeScopeIn& scopeIn = (PDBStore::TypeScopeIn&) scope;

        IDiaSymbol* pChild = NULL;
        DWORD fetched = 0;
        HRESULT hr = scopeIn.pEnumSymbols->Next( 1, &pChild, &fetched );
        if ( hr == S_OK && pChild )
        {
            hr = pChild->get_symIndexId( &typeIn.id );
            pChild->Release();
        }
        return hr == S_OK && pChild;
    }

    HRESULT PDBDebugStore::EndTypeScope( TypeScope& scope )
    {
        PDBStore::TypeScopeIn& scopeIn = (PDBStore::TypeScopeIn&) scope;
        if (scopeIn.pEnumSymbols)
        {
            scopeIn.pEnumSymbols->Release();
            scopeIn.pEnumSymbols = NULL;
        }
        return S_OK;
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

            if ( mGlobal->findChildren( SymTagCompiland, NULL, nsNone, &pEnumSymbols ) != S_OK )
                mCompilandCount = 0;
            else
            {
                if( pEnumSymbols->get_Count( &mCompilandCount ) != S_OK )
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
        if( hr == S_OK )
            hr = pEnumSymbols->Item( index - 1, &pCompiland );

        BSTR bstrName = NULL;
        if( hr == S_OK )
            hr = pCompiland->get_name( &bstrName );

        if( hr == S_OK )
        {
            detachBSTR( bstrName, info.Name );
            hr = (info.Name.GetName() == NULL) ? E_OUTOFMEMORY : S_OK;
        }

        IDiaEnumSourceFiles *pFiles = NULL;
        if( hr == S_OK )
            hr = mSession->findFile( pCompiland, NULL, nsNone, &pFiles );

        LONG fileCount = 0;
        if( hr == S_OK )
            hr = pFiles->get_Count( &fileCount );

        info.FileCount = (WORD) fileCount;
        info.SegmentCount = 1;

        if( pFiles )
            pFiles->Release();
        if( pCompiland )
            pCompiland->Release();
        if( pEnumSymbols )
            pEnumSymbols->Release();

        return hr > S_OK ? E_FAIL : hr;
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
        if( hr == S_OK )
            hr = pEnumSymbols->Item( compilandIndex - 1, &pCompiland );

        IDiaEnumSourceFiles *pFiles = NULL;
        if( hr == S_OK )
            hr = mSession->findFile( pCompiland, NULL, nsNone, &pFiles );

        LONG fileCount = 0;
        if( hr == S_OK )
            hr = pFiles->get_Count( &fileCount );

        if( hr == S_OK && fileIndex >= fileCount )
            hr = E_INVALIDARG;

        IDiaSourceFile *pSourceFile = NULL;
        if( hr == S_OK )
            hr = pFiles->Item( fileIndex, &pSourceFile );

        BSTR bstrName = NULL;
        if( hr == S_OK )
            hr = pSourceFile->get_fileName( &bstrName );

        if( hr == S_OK )
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

        return hr > S_OK ? E_FAIL : hr;
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
        if( hr == S_OK )
            hr = pEnumSymbols->Item( compIndex - 1, &pCompiland );

        IDiaEnumSourceFiles *pFiles = NULL;
        if( hr == S_OK )
            hr = mSession->findFile( pCompiland, NULL, nsNone, &pFiles );

        LONG fileCount = 0;
        if( hr == S_OK )
            hr = pFiles->get_Count( &fileCount );

        if( hr == S_OK && fileIndex >= fileCount )
            hr = E_INVALIDARG;

        IDiaSourceFile *pSourceFile = NULL;
        if( hr == S_OK )
            hr = pFiles->Item( fileIndex, &pSourceFile );

        IDiaEnumLineNumbers *pEnumLineNumbers = NULL;
        if( hr == S_OK )
            hr = mSession->findLines( pCompiland, pSourceFile, &pEnumLineNumbers );

        if( hr == S_OK )
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

        return hr == S_OK;
    }

    HRESULT PDBDebugStore::fillFileSegmentInfo( IDiaEnumLineNumbers *pEnumLineNumbers, FileSegmentInfo& segInfo )
    {
        HRESULT hr = S_OK;
        LONG lineNumbers = 0;
        // TODO: No samples or documentation show that Reset needs to be called before the first 
        //       call to Next. But sometimes, Next doesn't return all the elements here unless 
        //       you call Reset first. Why is that?
        hr = pEnumLineNumbers->Reset();
        if( hr == S_OK )
            hr = pEnumLineNumbers->get_Count( &lineNumbers );

        if( hr == S_OK )
        {
            mLastSegInfoLineNumbers.reset( new WORD[lineNumbers] );
            mLastSegInfoOffsets.reset( new DWORD[lineNumbers] );
            if ( mLastSegInfoLineNumbers.get() == NULL || mLastSegInfoOffsets.get() == NULL )
                hr = E_OUTOFMEMORY;
        }
        LONG lineIndex = 0;
        DWORD section = 0;
        segInfo.SegmentInstance = 0;
        while( hr == S_OK && lineIndex < lineNumbers )
        {
            IDiaLineNumber* pLineNumber = NULL;
            ULONG fetched = 0;
            hr = pEnumLineNumbers->Next( 1, &pLineNumber, &fetched );
            if ( hr != S_OK || fetched == 0 )
                break;

            DWORD off = 0, line = 0;
            if( hr == S_OK )
                hr = pLineNumber->get_addressOffset( &off );
            if( hr == S_OK )
                hr = pLineNumber->get_lineNumber( &line );
            if( hr == S_OK && lineIndex == 0 )
            {
                //segInfo.SegmentInstance = (WORD) line;
                segInfo.Start = off;
                hr = pLineNumber->get_addressSection( &section );
            }
            if( hr == S_OK && lineIndex == lineNumbers - 1 )
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

        return hr > S_OK ? E_FAIL : hr;
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
        return hr > S_OK ? E_FAIL : hr;
    }

    HRESULT PDBDebugStore::setLineNumber( IDiaLineNumber* pLineNumber, uint16_t lineIndex, LineNumber& lineNumber )
    {
        HRESULT hr = S_OK;

        IDiaSymbol *pCompiland = NULL;
        IDiaSourceFile *pSourceFile = NULL;
        if( hr == S_OK )
            hr = pLineNumber->get_compiland( &pCompiland );
        if( hr == S_OK )
            hr = pLineNumber->get_sourceFile( &pSourceFile );

        uint16_t compIndex = 0, fileIndex = 0;
        if( hr == S_OK )
            hr = findCompilandAndFile( pCompiland, pSourceFile, compIndex, fileIndex );

        if ( pCompiland )
            pCompiland->Release();
        if ( pSourceFile )
            pSourceFile->Release();

        DWORD line = 0, lineEnd = 0, offset = 0, section = 0, length = 0;
        if( hr == S_OK )
            hr = pLineNumber->get_lineNumber( &line );
        if( hr == S_OK )
            hr = pLineNumber->get_lineNumberEnd( &lineEnd );
        if( hr == S_OK )
            hr = pLineNumber->get_addressOffset( &offset );
        if( hr == S_OK )
            hr = pLineNumber->get_addressSection( &section );
        if( hr == S_OK )
            hr = pLineNumber->get_length( &length );

        if( hr == S_OK )
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
        return hr > S_OK ? E_FAIL : hr;
    }

    bool PDBDebugStore::FindLine( WORD seg, uint32_t offset, LineNumber& lineNumber )
    {
        HRESULT hr = S_OK;
        IDiaEnumLineNumbers *pEnumLineNumbers = NULL;
        if( hr == S_OK )
            hr = mSession->findLinesByAddr( seg, offset, 1, &pEnumLineNumbers );

        LONG lineNumbers = 0;
        IDiaLineNumber* pLineNumber = NULL;
        if( hr == S_OK )
            hr = pEnumLineNumbers->get_Count( &lineNumbers );
        if( hr == S_OK )
            if( lineNumbers < 1 )
                hr = E_INVALIDARG;
        if( hr == S_OK )
            hr = pEnumLineNumbers->Item( 0, &pLineNumber );

        if( hr == S_OK )
            setLineNumber( pLineNumber, 0, lineNumber );

        if( pLineNumber )
            pLineNumber->Release();
        if( pEnumLineNumbers )
            pEnumLineNumbers->Release();

        return hr == S_OK;
    }

    bool PDBDebugStore::FindLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber )
    {
        if ( (compIndex < 1) || (compIndex > getCompilandCount()) )
            return false;

        releaseFindLineEnumLineNumbers();

        IDiaEnumSymbols *pEnumSymbols = NULL;
        HRESULT hr = mGlobal->findChildren( SymTagCompiland, NULL, nsNone, &pEnumSymbols );

        IDiaSymbol *pCompiland = NULL;
        if( hr == S_OK )
            hr = pEnumSymbols->Item( compIndex - 1, &pCompiland );

        IDiaEnumSourceFiles *pFiles = NULL;
        if( hr == S_OK )
            hr = mSession->findFile( pCompiland, NULL, nsNone, &pFiles );

        LONG fileCount = 0;
        if( hr == S_OK )
            hr = pFiles->get_Count( &fileCount );

        if( hr == S_OK && fileIndex >= fileCount )
            hr = E_INVALIDARG;

        IDiaSourceFile *pSourceFile = NULL;
        if( hr == S_OK )
            hr = pFiles->Item( fileIndex, &pSourceFile );

        if( hr == S_OK )
            hr = mSession->findLinesByLinenum( pCompiland, pSourceFile, line, 0, &mFindLineEnumLineNumbers );

        if( hr == S_OK )
            hr = mFindLineEnumLineNumbers->Reset();

        if( pSourceFile )
            pSourceFile->Release();
        if( pFiles )
            pFiles->Release();
        if( pCompiland )
            pCompiland->Release();
        if( pEnumSymbols )
            pEnumSymbols->Release();

        if( hr != S_OK )
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
        if( hr == S_OK )
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
        if( hr == S_OK )
            pEnumSymbols->Reset();

        ULONG fetched;
        IDiaSymbol *pCompiland = NULL;
        while( hr == S_OK && pEnumSymbols->Next( 1, &pCompiland, &fetched ) == S_OK )
        {
            IDiaEnumSourceFiles *pFiles = NULL;
            if( hr == S_OK )
                hr = mSession->findFile( pCompiland, NULL, nsNone, &pFiles );

            IDiaSourceFile *pSourceFile = NULL;
            while( hr == S_OK && pFiles->Next( 1, &pSourceFile, &fetched ) == S_OK )
            {
                BSTR bstrName = NULL;
                if( hr == S_OK )
                    hr = pSourceFile->get_fileName( &bstrName );
                
                SymString srcFileName;
                if( hr == S_OK )
                {
                    detachBSTR( bstrName, srcFileName );
                    hr = (srcFileName.GetName() == NULL) ? E_OUTOFMEMORY : S_OK;
                }

                bool matches = false;
                if( hr == S_OK )
                {
                    if ( exactMatch )
                        matches = ExactFileNameMatch( fileName, fileNameLen, srcFileName.GetName(), srcFileName.GetLength() );
                    else
                        matches = PartialFileNameMatch( fileName, fileNameLen, srcFileName.GetName(), srcFileName.GetLength() );
                }
                if( matches )
                {
                    IDiaEnumLineNumbers *pEnumLineNumbers = 0;
                    if( hr == S_OK )
                        hr = mSession->findLinesByLinenum( pCompiland, pSourceFile, reqLineStart, 0, &pEnumLineNumbers );
                    
                    IDiaLineNumber* pLineNumber = NULL;
                    while( hr == S_OK && pEnumLineNumbers->Next( 1, &pLineNumber, &fetched ) == S_OK )
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

