/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "EnumValues.h"
#include "UniAlpha.h"


namespace MagoEE
{
    EEDEnumValues::EEDEnumValues()
        :   mRefCount( 0 )
    {
    }

    EEDEnumValues::~EEDEnumValues()
    {
    }

    void EEDEnumValues::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void EEDEnumValues::Release()
    {
        long    newRef = InterlockedDecrement( &mRefCount );
        _ASSERT( newRef >= 0 );
        if ( newRef == 0 )
        {
            delete this;
        }
    }

    HRESULT EEDEnumValues::Init( 
        IValueBinder* binder, 
        const wchar_t* parentExprText, 
        const EvalResult& parentVal,
        ITypeEnv* typeEnv,
        NameTable* strTable )
    {
        mBinder = binder;
        mParentExprText = parentExprText;
        mParentVal = parentVal;
        mTypeEnv = typeEnv;
        mStrTable = strTable;

        return S_OK;
    }

    // fallback when unable to evaluate directly from parent value
    HRESULT EEDEnumValues::EvaluateExpr( 
        const EvalOptions& options, 
        EvalResult& result, 
        std::wstring& expr )
    {
        HRESULT hr = S_OK;
        RefPtr<IEEDParsedExpr>  parsedExpr;

        hr = ParseText( expr.c_str(), mTypeEnv, mStrTable, parsedExpr.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Bind( options, mBinder );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Evaluate( options, mBinder, result );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    //------------------------------------------------------------------------
    //  EEDEnumPointer
    //------------------------------------------------------------------------

    EEDEnumPointer::EEDEnumPointer()
        :   mCountDone( 0 )
    {
    }

    uint32_t EEDEnumPointer::GetCount()
    {
        return 1;
    }

    uint32_t EEDEnumPointer::GetIndex()
    {
        return mCountDone;
    }

    void EEDEnumPointer::Reset()
    {
        mCountDone = 0;
    }

    HRESULT EEDEnumPointer::Skip( uint32_t count )
    {
        if ( count > (GetCount() - mCountDone) )
        {
            mCountDone = GetCount();
            return S_FALSE;
        }

        mCountDone += count;

        return S_OK;
    }

    HRESULT EEDEnumPointer::Clone( IEEDEnumValues*& copiedEnum )
    {
        HRESULT hr = S_OK;
        RefPtr<EEDEnumPointer>  en = new EEDEnumPointer();

        if ( en == NULL )
            return E_OUTOFMEMORY;

        hr = en->Init( mBinder, mParentExprText.c_str(), mParentVal, mTypeEnv, mStrTable );
        if ( FAILED( hr ) )
            return hr;

        en->mCountDone = mCountDone;

        copiedEnum = en.Detach();
        return S_OK;
    }

    HRESULT EEDEnumPointer::EvaluateNext( 
        const EvalOptions& options, 
        EvalResult& result,
        std::wstring& name,
        std::wstring& fullName )
    {
        if ( mCountDone >= GetCount() )
            return E_FAIL;

        RefPtr<IEEDParsedExpr>  parsedExpr;

        name.clear();
        fullName.clear();
        fullName.append( L"*(" );
        fullName.append( mParentExprText );
        fullName.append( 1, L')' );

        mCountDone++;

        if ( mParentVal.ObjVal.Value.Addr == 0 )
            return E_MAGOEE_NO_ADDRESS;

        auto tn = mParentVal.ObjVal._Type->AsTypeNext();
        if( tn == NULL )
            return E_FAIL;

        result.ObjVal._Type = tn->GetNext();
        result.ObjVal.Addr = mParentVal.ObjVal.Value.Addr;

        HRESULT hr = mBinder->GetValue( result.ObjVal.Addr, result.ObjVal._Type, result.ObjVal.Value );
        if ( FAILED( hr ) )
            return hr;

        FillValueTraits( mBinder, result, nullptr );
        return S_OK;
    }


    //------------------------------------------------------------------------
    //  EEDEnumSArray
    //------------------------------------------------------------------------

    EEDEnumSArray::EEDEnumSArray()
        : mCountDone( 0 )
        , mBaseOffset( 0 )
    {
    }

    HRESULT EEDEnumSArray::Init(
        IValueBinder* binder,
        const wchar_t* parentExprText,
        const EvalResult& parentVal,
        ITypeEnv* typeEnv,
        NameTable* strTable)
    {
        HRESULT hr = EEDEnumValues::Init( binder, parentExprText, parentVal, typeEnv, strTable );
        if ( FAILED( hr ) )
            return hr;
        if( parentVal.IsArrayContinuation && mParentExprText.length() > 2 )
        {
            int base = -1;
            if( mParentExprText.back() == ']' )
            {
                for( size_t p = mParentExprText.length() - 2; p > 0; --p )
                    if( mParentExprText[p] == '[' )
                    {
                        if( swscanf( mParentExprText.data() + p + 1, L"%d..", &base ) != 1 )
                            base = -1;
                        else
                            mParentExprText.erase( p );
                        break;
                    }
            }
            if ( base >= 0 )
                mBaseOffset = base;
        }
        return S_OK;
    }

    uint64_t EEDEnumSArray::GetUnlimitedCount()
    {
        uint32_t    count = 0;

        if ( mParentVal.ObjVal._Type->IsSArray() )
        {
            count = mParentVal.ObjVal._Type->AsTypeSArray()->GetLength();
        }
        else if ( mParentVal.ObjVal._Type->IsDArray() )
        {
            count = (uint32_t) mParentVal.ObjVal.Value.Array.Length;
        }

        return count;
    }

    uint32_t EEDEnumSArray::GetCount()
    {
        uint64_t count = GetUnlimitedCount();
        if ( gMaxArrayLength > 0 && count > gMaxArrayLength )
            count = gMaxArrayLength + 1;

        return count;
    }

    uint32_t EEDEnumSArray::GetIndex()
    {
        return mCountDone;
    }

    void EEDEnumSArray::Reset()
    {
        mCountDone = 0;
    }

    HRESULT EEDEnumSArray::Skip( uint32_t count )
    {
        if ( count > (GetCount() - mCountDone) )
        {
            mCountDone = GetCount();
            return S_FALSE;
        }

        mCountDone += count;

        return S_OK;
    }

    HRESULT EEDEnumSArray::Clone( IEEDEnumValues*& copiedEnum )
    {
        HRESULT hr = S_OK;
        RefPtr<EEDEnumSArray>  en = new EEDEnumSArray();

        if ( en == NULL )
            return E_OUTOFMEMORY;

        hr = en->Init( mBinder, mParentExprText.c_str(), mParentVal, mTypeEnv, mStrTable );
        if ( FAILED( hr ) )
            return hr;

        en->mCountDone = mCountDone;
        en->mBaseOffset = mBaseOffset;

        copiedEnum = en.Detach();
        return S_OK;
    }

    HRESULT EEDEnumSArray::EvaluateNext( 
        const EvalOptions& options, 
        EvalResult& result,
        std::wstring& name,
        std::wstring& fullName )
    {
        if ( mCountDone >= GetCount() )
            return E_FAIL;

        // 4294967295
        const int   MaxIntStrLen = 10;
        // "[indexInt]", and add some padding
        const int   MaxIndexStrLen = MaxIntStrLen + 2 + 10;

        wchar_t indexStr[ MaxIndexStrLen + 1 ] = L"";

        bool atLimit = gMaxArrayLength > 0 && mCountDone == gMaxArrayLength && GetUnlimitedCount() - 1 > gMaxArrayLength;
        if( atLimit )
            swprintf_s( indexStr, L"[%d..%I64d]", mCountDone + mBaseOffset, GetUnlimitedCount() + mBaseOffset );
        else
            swprintf_s( indexStr, L"[%d]", mCountDone + mBaseOffset );

        name.clear();
        name.append( indexStr );

        bool isIdent = IsIdentifier( mParentExprText.data() );
        fullName.clear();
        if ( !isIdent )
            fullName.append( L"(" );
        fullName.append( mParentExprText );
        if ( !isIdent )
            fullName.append( L")" );
        fullName.append( name );

        uint32_t index = mCountDone++;

        Address addr;
        if ( mParentVal.ObjVal._Type->IsSArray() )
        {
            result.ObjVal._Type = mParentVal.ObjVal._Type->AsTypeSArray()->GetElement();
            addr = mParentVal.ObjVal.Addr;
        }
        else if ( mParentVal.ObjVal._Type->IsDArray() )
        {
            result.ObjVal._Type = mParentVal.ObjVal._Type->AsTypeDArray()->GetElement();
            addr = mParentVal.ObjVal.Value.Array.Addr;
        }
        else
            return E_FAIL;

        if ( addr == 0 )
            return E_MAGOEE_NO_ADDRESS;

        result.ObjVal.Addr = addr + (uint64_t)index * result.ObjVal._Type->GetSize();
        if (atLimit)
        {
            RefPtr<Type> dtype;
            if ( mTypeEnv->NewSArray( result.ObjVal._Type, GetUnlimitedCount() - index, dtype.Ref() ) == S_OK )
                result.ObjVal._Type = dtype;
            result.IsArrayContinuation = true;
        }
        else
        {
            HRESULT hr = mBinder->GetValue( result.ObjVal.Addr, result.ObjVal._Type, result.ObjVal.Value );
            if ( FAILED( hr ) )
                return hr;
        }

        FillValueTraits( mBinder, result, nullptr );
        return S_OK;
    }

    //------------------------------------------------------------------------
    //  EEDEnumRawDArray
    //------------------------------------------------------------------------

    EEDEnumRawDArray::EEDEnumRawDArray()
        :   mCountDone( 0 )
    {
    }

    uint32_t EEDEnumRawDArray::GetCount()
    {
        return 2;
    }

    uint32_t EEDEnumRawDArray::GetIndex()
    {
        return mCountDone;
    }

    void EEDEnumRawDArray::Reset()
    {
        mCountDone = 0;
    }

    HRESULT EEDEnumRawDArray::Skip( uint32_t count )
    {
        if ( count > (GetCount() - mCountDone) )
        {
            mCountDone = GetCount();
            return S_FALSE;
        }

        mCountDone += count;

        return S_OK;
    }

    HRESULT EEDEnumRawDArray::Clone( IEEDEnumValues*& copiedEnum )
    {
        HRESULT hr = S_OK;
        RefPtr<EEDEnumRawDArray> en = new EEDEnumRawDArray();

        if ( en == NULL )
            return E_OUTOFMEMORY;

        hr = en->Init( mBinder, mParentExprText.c_str(), mParentVal, mTypeEnv, mStrTable );
        if ( FAILED( hr ) )
            return hr;

        en->mCountDone = mCountDone;

        copiedEnum = en.Detach();
        return S_OK;
    }

    HRESULT EEDEnumRawDArray::EvaluateNext( 
        const EvalOptions& options, 
        EvalResult& result,
        std::wstring& name,
        std::wstring& fullName )
    {
        if ( mCountDone >= GetCount() )
            return E_FAIL;

        const wchar_t* field = (mCountDone == 0 ? L"length" : L"ptr");

        name.clear();
        name.append( field );

        bool isIdent = IsIdentifier( mParentExprText.data() );
        fullName.clear();
        if ( !isIdent )
            fullName.append( L"(" );
        fullName.append( mParentExprText );
        if ( !isIdent )
            fullName.append( L")" );
        fullName.append( L"." );
        fullName.append( name );

        mCountDone++;
        return EvaluateExpr( options, result, fullName );
    }


    //------------------------------------------------------------------------
    //  EEDEnumAArray
    //------------------------------------------------------------------------

    EEDEnumAArray::EEDEnumAArray( int aaVersion )
        :   mCountDone( 0 )
        ,   mAAVersion ( aaVersion )
    {
        mBB.nodes = UINT64_MAX;
        mBucketIndex = 0;
        mNextNode = NULL;
    }

    HRESULT EEDEnumAArray::ReadBB( IValueBinder* binder, RefPtr<Type> type, Address address, int& AAVersion, BB64 &BB )
    {
        HRESULT hr = S_OK;
        uint32_t sizeRead;
        BB64_V1& BB_V1 = *(BB64_V1*) &BB;

        _ASSERT( type->IsAArray() );

        if( address == NULL )
        {
            memset( &BB, 0, sizeof( BB ) );
            return S_OK;
        }

        if ( type->GetSize() == 4 )
        {
            BB32    bb32;
            BB32_V1 bb32_v1;
            if ( AAVersion == 1 )
                hr = binder->ReadMemory( address, sizeof bb32_v1, sizeRead, (uint8_t*)&bb32_v1 );
            else
                hr = binder->ReadMemory( address, sizeof bb32, sizeRead, (uint8_t*)&bb32 );

            if ( FAILED( hr ) )
                return hr;

            if ( AAVersion == -1 )
            {
                if ( ( bb32.b.length <= 4 && bb32.b.ptr != address + sizeof bb32 ) || // init bucket in Impl
                     ( bb32.b.length > 4 && ( bb32.b.length & ( bb32.b.length - 1 ) ) == 0 ) )
                {
                    AAVersion = 1; // power of 2 indicates new AA
                    hr = binder->ReadMemory( address, sizeof bb32_v1, sizeRead, (uint8_t*)&bb32_v1 );
                    if ( FAILED( hr ) )
                        return hr;
                }
                else
                {
                    AAVersion = 0;
                }
            }

            if ( AAVersion == 1 )
            {
                BB_V1.buckets.length = bb32_v1.buckets.length;
                BB_V1.buckets.ptr = bb32_v1.buckets.ptr;
                BB_V1.used = bb32_v1.used;
                BB_V1.deleted = bb32_v1.deleted;
                BB_V1.entryTI = bb32_v1.entryTI;
                BB_V1.firstUsed = bb32_v1.firstUsed;
                BB_V1.keysz = bb32_v1.keysz;
                BB_V1.valsz = bb32_v1.valsz;
                BB_V1.valoff = bb32_v1.valoff;
                BB_V1.flags = bb32_v1.flags;
            }
            else
            {
                if ( bb32.firstUsedBucket > bb32.nodes )
                {
                    bb32.keyti = bb32.firstUsedBucket; // compatibility fix for dmd before 2.067
                    bb32.firstUsedBucket = 0;
                }
                BB.b.length = bb32.b.length;
                BB.b.ptr = bb32.b.ptr;
                BB.firstUsedBucket = bb32.firstUsedBucket;
                BB.keyti = bb32.keyti;
                BB.nodes = bb32.nodes;
            }
        }
        else
        {
            if ( AAVersion == 1 )
                hr = binder->ReadMemory( address, sizeof mBB_V1, sizeRead, (uint8_t*)&BB_V1 );
            else
                hr = binder->ReadMemory( address, sizeof mBB, sizeRead, (uint8_t*)&BB );
            if ( FAILED( hr ) )
                return hr;

            if ( AAVersion == -1 )
            {
                if ( ( BB.b.length <= 4 && BB.b.ptr != address + sizeof BB ) || // init bucket in Impl
                     ( BB.b.length > 4 && ( BB.b.length & ( BB.b.length - 1 ) ) == 0 ) )
                {
                    AAVersion = 1; // power of 2 indicates new AA
                    hr = binder->ReadMemory( address, sizeof BB_V1, sizeRead, (uint8_t*)&BB_V1 );
                    if ( FAILED( hr ) )
                        return hr;
                }
                else
                {
                    AAVersion = 0;
                }
            }

            if ( AAVersion == 0 && BB.firstUsedBucket > BB.nodes )
            {
                BB.keyti = BB.firstUsedBucket; // compatibility fix for dmd before 2.067
                BB.firstUsedBucket = 0;
            }
        }

        return S_OK;
    }

    HRESULT EEDEnumAArray::ReadBB()
    {
        if (mBB.nodes != UINT64_MAX)
            return S_OK;

        return ReadBB( mBinder, mParentVal.ObjVal._Type, mParentVal.ObjVal.Value.Addr, mAAVersion, mBB );
    }

    HRESULT EEDEnumAArray::ReadAddress( Address baseAddr, uint64_t index, Address& ptrValue )
    {
        HRESULT hr = S_OK;
        uint32_t ptrSize = mParentVal.ObjVal._Type->GetSize();
        uint64_t addr = baseAddr + (index * ptrSize);
        uint32_t sizeRead;

        if ( ptrSize == 4 )
        {
            uint32_t    ptrValue32;

            hr = mBinder->ReadMemory( addr, ptrSize, sizeRead, (uint8_t*)&ptrValue32 );
            if ( FAILED( hr ) )
                return hr;

            ptrValue = ptrValue32;
        }
        else
        {
            hr = mBinder->ReadMemory( addr, ptrSize, sizeRead, (uint8_t*)&ptrValue );
            if ( FAILED( hr ) )
                return hr;
        }

        return S_OK;
    }

    uint32_t EEDEnumAArray::AlignTSize( uint32_t size )
    {
        uint32_t ptrSize = mParentVal.ObjVal._Type->GetSize();
        if ( ptrSize == 4 )
            return (size + sizeof( uint32_t ) - 1) & ~(sizeof( uint32_t ) - 1);
        else
            return (size + 16 - 1) & ~(16 - 1);
    }

    uint64_t EEDEnumAArray::GetUnlimitedCount()
    {
        uint32_t    count = 0;

        HRESULT hr = ReadBB();
        if ( !FAILED( hr ) )
            count = mAAVersion == 1 ? mBB_V1.used - mBB_V1.deleted : (uint32_t)mBB.nodes;

        return count;
    }

    uint32_t EEDEnumAArray::GetCount()
    {
        uint32_t    count = GetUnlimitedCount();
        if ( gMaxArrayLength > 0 && count > gMaxArrayLength )
            count = gMaxArrayLength + 1;

        return count;
    }

    uint32_t EEDEnumAArray::GetIndex()
    {
        return (uint32_t) mCountDone;
    }

    void EEDEnumAArray::Reset()
    {
        mCountDone = 0;
        mBucketIndex = 0;
        mNextNode = NULL;
    }

    HRESULT EEDEnumAArray::FindCurrent()
    {
        if ( mAAVersion == 1 )
        {
            uint32_t ptrSize = mParentVal.ObjVal._Type->GetSize();
            uint64_t hashFilledMark = 1LL << (8 * ptrSize - 1);

            while( mNextNode == NULL && mBucketIndex < mBB.b.length )
            {
                Address hash;
                HRESULT hr = ReadAddress( mBB.b.ptr, 2 * mBucketIndex, hash );
                if ( FAILED( hr ) )
                    return hr;

                if ( hash & hashFilledMark )
                    return ReadAddress( mBB.b.ptr, 2 * mBucketIndex + 1, mNextNode );

                mBucketIndex++;
            }
        }
        else
        {
            while( mNextNode == NULL && mBucketIndex < mBB.b.length )
            {
                HRESULT hr = ReadAddress( mBB.b.ptr, mBucketIndex, mNextNode );
                if ( FAILED( hr ) )
                    return hr;

                if( mNextNode != NULL )
                    return S_OK;

                mBucketIndex++;
            }
        }
        return S_OK;
    }

    HRESULT EEDEnumAArray::FindNext()
    {
        HRESULT hr = FindCurrent();
        if ( FAILED( hr ) )
            return hr;

        if( mNextNode )
        {
            if ( mAAVersion == 1 )
            {
                mNextNode = NULL;
            }
            else
            {
                hr = ReadAddress( mNextNode, 0, mNextNode );
                if ( FAILED( hr ) )
                    return hr;

                if( mNextNode != NULL )
                    return S_OK;
            }
            mBucketIndex++;
        }
        return FindCurrent();
    }

    HRESULT EEDEnumAArray::Skip( uint32_t count )
    {
        if ( count > (GetCount() - mCountDone) )
        {
            mBucketIndex = mBB.b.length;
            mNextNode = NULL;
            mCountDone = GetCount();
            return S_FALSE;
        }

        HRESULT hr = FindCurrent();
        if ( FAILED( hr ) )
            return hr;

        for( uint32_t i = 0; i < count; i++ )
        {
            hr = FindNext();
            if ( FAILED( hr ) )
                return E_FAIL;
            
            mCountDone++;
        }

        return S_OK;
    }

    HRESULT EEDEnumAArray::Clone( IEEDEnumValues*& copiedEnum )
    {
        HRESULT hr = S_OK;
        RefPtr<EEDEnumAArray>  en = new EEDEnumAArray( mAAVersion );

        if ( en == NULL )
            return E_OUTOFMEMORY;

        hr = en->Init( mBinder, mParentExprText.c_str(), mParentVal, mTypeEnv, mStrTable );
        if ( FAILED( hr ) )
            return hr;

        en->mAAVersion = mAAVersion;
        if ( mAAVersion == 1 )
            en->mBB_V1 = mBB_V1;
        else
            en->mBB = mBB;
        en->mCountDone = mCountDone;
        en->mBucketIndex = mBucketIndex;
        en->mNextNode = mNextNode;

        copiedEnum = en.Detach();
        return S_OK;
    }

    HRESULT EEDEnumAArray::EvaluateNext( 
        const EvalOptions& options, 
        EvalResult& result,
        std::wstring& name,
        std::wstring& fullName )
    {
        if ( mCountDone >= GetCount() )
            return E_FAIL;

        HRESULT hr = FindCurrent();
        if ( FAILED( hr ) )
            return hr;

        if( !mNextNode )
            return E_FAIL;

        _ASSERT( mParentVal.ObjVal._Type->IsAArray() );
        ITypeAArray* aa = mParentVal.ObjVal._Type->AsTypeAArray();

        bool atLimit = gMaxArrayLength > 0 && mCountDone == gMaxArrayLength && GetUnlimitedCount() - 1 > gMaxArrayLength;
        if (atLimit)
        {
            wchar_t indexStr[64];
            swprintf_s( indexStr, L"%I64d more items not shown...", GetUnlimitedCount() - mCountDone );
            name = indexStr;
            fullName.clear();

            result.ObjVal.Addr = 0;
            result.ObjVal._Type = mTypeEnv->GetType( Tvoid );
        }
        else
        {
            uint32_t ptrSize = mParentVal.ObjVal._Type->GetSize();

            DataObject keyobj;
            keyobj._Type = aa->GetIndex();
            keyobj.Addr = mNextNode + ( mAAVersion == 1 ? 0 : 2 * ptrSize );

            hr = mBinder->GetValue( keyobj.Addr, keyobj._Type, keyobj.Value );
            if ( FAILED( hr ) )
                return hr;

            std::wstring keystr;
            struct FormatOptions fmt (10);
            hr = FormatValue( mBinder, keyobj, fmt, keystr, kMaxFormatValueLength );
            if ( FAILED( hr ) )
                return hr;

            name = L"[" + keystr + L"]";

            bool isIdent = IsIdentifier( mParentExprText.data() );
            fullName.clear();
            if ( !isIdent )
                fullName.append( L"(" );
            fullName.append( mParentExprText );
            if ( !isIdent )
                fullName.append( L")" );
            fullName.append( name );

            uint32_t alignKeySize = ( mAAVersion == 1 ? mBB_V1.valoff : AlignTSize( aa->GetIndex()->GetSize() ) );

            result.ObjVal.Addr = keyobj.Addr + alignKeySize;
            result.ObjVal._Type = aa->GetElement();
            hr = mBinder->GetValue( result.ObjVal.Addr, result.ObjVal._Type, result.ObjVal.Value );
            if ( FAILED( hr ) )
                return hr;
        }
        FillValueTraits( mBinder, result, nullptr );
        mCountDone++;

        return FindNext();
    }


    //------------------------------------------------------------------------
    //  EEDEnumStruct
    //------------------------------------------------------------------------

    EEDEnumStruct::EEDEnumStruct( bool skipHeadRef )
        :   mCountDone( 0 ),
            mSkipHeadRef( skipHeadRef ),
            mHasVTable( false )
    {
    }

    HRESULT EEDEnumStruct::Init( 
        IValueBinder* binder, 
        const wchar_t* parentExprText, 
        const EvalResult& parentVal,
        ITypeEnv* typeEnv,
        NameTable* strTable )
    {
        HRESULT             hr = S_OK;
        RefPtr<Declaration> decl;
        RefPtr<IEnumDeclarationMembers> members;
        EvalResult          parentValCopy = parentVal;

        if ( parentValCopy.ObjVal._Type == NULL )
            return E_INVALIDARG;

        if ( mSkipHeadRef && parentValCopy.ObjVal._Type->IsReference() )
        {
            parentValCopy.ObjVal._Type = parentValCopy.ObjVal._Type->AsTypeNext()->GetNext();
            parentValCopy.ObjVal.Addr = parentValCopy.ObjVal.Value.Addr;
        }

        ITypeStruct* typeStruct = parentValCopy.ObjVal._Type->AsTypeStruct();
        if ( typeStruct == NULL )
            return E_INVALIDARG;

        decl = parentValCopy.ObjVal._Type->GetDeclaration();
        if ( decl == NULL )
            return E_INVALIDARG;

        if ( !decl->EnumMembers( members.Ref() ) )
            return E_INVALIDARG;

        hr = EEDEnumValues::Init( binder, parentExprText, parentValCopy, typeEnv, strTable );
        if ( FAILED( hr ) )
            return hr;

        MagoEE::UdtKind kind;
        if ( decl->GetUdtKind( kind ) && kind == MagoEE::Udt_Class &&
             wcsncmp( parentExprText, L"cast(", 5 ) != 0 )  // already inside the base/derived class enumeration?
        {
            binder->GetClassName( parentVal.ObjVal.Addr, mClassName, true );

            // don't show runtime class if it is the same as the compile time type
            if( !mClassName.empty() && mClassName == decl->GetName() )
                mClassName.clear();

            if( gShowVTable )
            {
                // if the class has a virtual function table, fake a member "__vfptr" (it is skipped by normal member iteration)
                RefPtr<Declaration> vshape;
                mHasVTable = decl->GetVTableShape( vshape.Ref() ) && vshape;
            }
        }

        mMembers = members;

        return S_OK;
    }

    uint32_t EEDEnumStruct::GetCount()
    {
        return mMembers->GetCount() + ( mClassName.empty() ? 0 : 1 ) + ( mHasVTable ? 1 : 0 );
    }

    uint32_t EEDEnumStruct::GetIndex()
    {
        return mCountDone;
    }

    void EEDEnumStruct::Reset()
    {
        mCountDone = 0;
        mMembers->Reset();
    }

    uint32_t EEDEnumStruct::VShapePos() const
    {
        if ( !mHasVTable )
            return UINT32_MAX;
        return mClassName.empty() ? 0 : 1;
    }

    HRESULT EEDEnumStruct::Skip( uint32_t count )
    {
        if ( count > (GetCount() - mCountDone) )
        {
            mCountDone = GetCount();
            return S_FALSE;
        }

        if( count > 0 && mCountDone == 0 && !mClassName.empty() )
        {
            mCountDone++;
            count--;
        }
        if ( count > 0 && mCountDone == VShapePos() )
        {
            mCountDone++;
            count--;
        }
        mCountDone += count;
        mMembers->Skip( count );

        return S_OK;
    }

    HRESULT EEDEnumStruct::Clone( IEEDEnumValues*& copiedEnum )
    {
        HRESULT hr = S_OK;
        RefPtr<EEDEnumStruct>  en = new EEDEnumStruct( mSkipHeadRef );

        if ( en == NULL )
            return E_OUTOFMEMORY;

        hr = en->Init( mBinder, mParentExprText.c_str(), mParentVal, mTypeEnv, mStrTable );
        if ( FAILED( hr ) )
            return hr;

        en->Skip( mCountDone );

        copiedEnum = en.Detach();
        return S_OK;
    }

    HRESULT EEDEnumStruct::EvaluateNext( 
        const EvalOptions& options, 
        EvalResult& result,
        std::wstring& name,
        std::wstring& fullName )
    {
        if ( mCountDone >= GetCount() )
            return E_FAIL;

        HRESULT hr = S_OK;
        RefPtr<Declaration>     decl;

        name.clear();
        fullName.clear();

        if( mCountDone == 0 && !mClassName.empty() )
        {
            name = L"[" + mClassName + L"]";
            fullName = L"cast(" + mClassName + L")(" + mParentExprText + L")";
            result.IsMostDerivedClass = true;
            mCountDone++;
        }
        else if ( mCountDone == VShapePos() )
        {
            name = L"__vfptr";
            fullName = L"(" + mParentExprText + L").__vfptr";
            mCountDone++;

            Address addr = mParentVal.ObjVal._Type->IsReference() ? mParentVal.ObjVal.Value.Addr : mParentVal.ObjVal.Addr;
            if ( addr == 0 )
                return E_MAGOEE_NO_ADDRESS;

            RefPtr<Declaration> vshape;
            decl = mParentVal.ObjVal._Type->GetDeclaration();
            if( ! decl->GetVTableShape( vshape.Ref() ) )
                return E_FAIL;

            int offset = 0;
            if ( !vshape->GetOffset( offset ) )
                return E_FAIL;

            if( !vshape->GetType( result.ObjVal._Type.Ref() ) )
                return E_FAIL;

            result.ObjVal.Addr = addr + offset;

            hr = mBinder->GetValue( result.ObjVal.Addr, result.ObjVal._Type, result.ObjVal.Value );
            if( FAILED( hr ) )
                return hr;

            FillValueTraits( mBinder, result, nullptr );
            return S_OK;
        }
        else
        {
            if ( !mMembers->Next( decl.Ref() ) )
                return E_FAIL;

            mCountDone++;
            if ( decl->IsBaseClass() )
            {
                if ( !NameBaseClass( decl, name, fullName ) )
                    return E_FAIL;
                result.IsBaseClass = true;
            }
            else if( decl->IsStaticField() )
            {
                if ( !NameStaticMember( decl, name, fullName ) )
                    return E_FAIL;
                result.IsStaticField = true;
            }
            else if( decl->IsField() )
            {
                if ( !NameRegularMember( decl, name, fullName ) )
                    return E_FAIL;

                Address addr = mParentVal.ObjVal._Type->IsReference() ? mParentVal.ObjVal.Value.Addr : mParentVal.ObjVal.Addr;

                if ( addr == 0 )
                    return E_MAGOEE_NO_ADDRESS;

                int offset = 0;
                if ( !decl->GetOffset( offset ) )
                    return E_FAIL;

                if( !decl->GetType( result.ObjVal._Type.Ref() ) )
                    return E_FAIL;

                result.ObjVal.Addr = addr + offset;

                hr = mBinder->GetValue( result.ObjVal.Addr, result.ObjVal._Type, result.ObjVal.Value );
                if ( FAILED( hr ) )
                    return hr;

                FillValueTraits( mBinder, result, nullptr );
                return S_OK;
            }
        }

        return EvaluateExpr( options, result, fullName );
    }

    bool EEDEnumStruct::NameBaseClass( 
            Declaration* decl, 
            std::wstring& name,
            std::wstring& fullName )
    {
        RefPtr<Type>        baseType;
        RefPtr<Declaration> baseDecl;

        if ( !decl->GetType( baseType.Ref() ) )
            return false;

        baseDecl = baseType->GetDeclaration();
        if ( baseDecl == NULL )
            return false;

        name.append( baseDecl->GetName() );

        if ( !mSkipHeadRef )
            fullName.append( L"*" );

        fullName.append( L"cast(" );
        fullName.append( baseDecl->GetName() );

        if ( mSkipHeadRef )
            fullName.append( L")(" );
        else
            fullName.append( L"*)&(" );

        fullName.append( mParentExprText );
        fullName.append( L")" );

        return true;
    }

    bool EEDEnumStruct::NameStaticMember( 
            Declaration* decl, 
            std::wstring& name,
            std::wstring& fullName )
    {
        name.append( decl->GetName() );

        if( mParentVal.ObjVal._Type )
            mParentVal.ObjVal._Type->ToString( fullName );
        fullName.append( L"." );
        fullName.append( name );

        return true;
    }

    bool EEDEnumStruct::NameRegularMember( 
            Declaration* decl, 
            std::wstring& name,
            std::wstring& fullName )
    {
        name.append( decl->GetName() );

        bool isIdent = IsIdentifier( mParentExprText.data() );
        if( !isIdent )
            fullName.append( L"(" );
        fullName.append( mParentExprText );
        if( !isIdent )
            fullName.append( L")" );
        fullName.append( L"." );
        fullName.append( name );

        return true;
    }
}
