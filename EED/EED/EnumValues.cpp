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
    const uint32_t  MaxArrayLength = 1024*1024*1024;


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
        const DataObject& parentVal,
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

        HRESULT hr = S_OK;
        RefPtr<IEEDParsedExpr>  parsedExpr;

        name.clear();
        fullName.clear();
        fullName.append( L"*(" );
        fullName.append( mParentExprText );
        fullName.append( 1, L')' );

        hr = ParseText( fullName.c_str(), mTypeEnv, mStrTable, parsedExpr.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Bind( options, mBinder );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Evaluate( options, mBinder, result );
        if ( FAILED( hr ) )
            return hr;

        mCountDone++;

        return S_OK;
    }


    //------------------------------------------------------------------------
    //  EEDEnumSArray
    //------------------------------------------------------------------------

    EEDEnumSArray::EEDEnumSArray()
        :   mCountDone( 0 )
    {
    }

    uint32_t EEDEnumSArray::GetCount()
    {
        uint32_t    count = 0;

        if ( mParentVal._Type->IsSArray() )
        {
            count = mParentVal._Type->AsTypeSArray()->GetLength();
        }
        else if ( mParentVal._Type->IsDArray() )
        {
            if ( mParentVal.Value.Array.Length > MaxArrayLength )
                count = MaxArrayLength;
            else
                count = (uint32_t) mParentVal.Value.Array.Length;
        }

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

        HRESULT hr = S_OK;
        RefPtr<IEEDParsedExpr>  parsedExpr;
        wchar_t indexStr[ MaxIndexStrLen + 1 ] = L"";

        swprintf_s( indexStr, L"[%d]", mCountDone );

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

        hr = ParseText( fullName.c_str(), mTypeEnv, mStrTable, parsedExpr.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Bind( options, mBinder );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Evaluate( options, mBinder, result );
        if ( FAILED( hr ) )
            return hr;

        mCountDone++;

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

        HRESULT hr = S_OK;
        RefPtr<IEEDParsedExpr>  parsedExpr;
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

        hr = ParseText( fullName.c_str(), mTypeEnv, mStrTable, parsedExpr.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Bind( options, mBinder );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Evaluate( options, mBinder, result );
        if ( FAILED( hr ) )
            return hr;

        mCountDone++;

        return S_OK;
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

    HRESULT EEDEnumAArray::ReadBB()
    {
        HRESULT hr = S_OK;
        uint32_t sizeRead;

        if ( mBB.nodes != UINT64_MAX )
            return S_OK;

        _ASSERT( mParentVal._Type->IsAArray() );
        Address address = mParentVal.Value.Addr;

        if( address == NULL )
        {
            memset( &mBB, 0, sizeof mBB );
            return S_OK;
        }

        if ( mParentVal._Type->GetSize() == 4 )
        {
            BB32    bb32;
            BB32_V1 bb32_v1;
            if ( mAAVersion == 1 )
                hr = mBinder->ReadMemory( address, sizeof bb32_v1, sizeRead, (uint8_t*)&bb32_v1 );
            else
                hr = mBinder->ReadMemory( address, sizeof bb32, sizeRead, (uint8_t*)&bb32 );

            if ( FAILED( hr ) )
                return hr;

            if ( mAAVersion == -1 )
            {
                if ( bb32.b.length > 4 && ( bb32.b.length & ( bb32.b.length - 1 ) ) == 0 )
                {
                    mAAVersion = 1; // power of 2 indicates new AA
                    hr = mBinder->ReadMemory( address, sizeof bb32_v1, sizeRead, (uint8_t*)&bb32_v1 );
                    if ( FAILED( hr ) )
                        return hr;
                }
                else
                {
                    mAAVersion = 0;
                }
            }

            if ( mAAVersion == 1 )
            {
                mBB_V1.buckets.length = bb32_v1.buckets.length;
                mBB_V1.buckets.ptr = bb32_v1.buckets.ptr;
                mBB_V1.used = bb32_v1.used;
                mBB_V1.deleted = bb32_v1.deleted;
                mBB_V1.entryTI = bb32_v1.entryTI;
                mBB_V1.firstUsed = bb32_v1.firstUsed;
                mBB_V1.keysz = bb32_v1.keysz;
                mBB_V1.valsz = bb32_v1.valsz;
                mBB_V1.valoff = bb32_v1.valoff;
                mBB_V1.flags = bb32_v1.flags;
            }
            else
            {
                if ( bb32.firstUsedBucket > bb32.nodes )
                {
                    bb32.keyti = bb32.firstUsedBucket; // compatibility fix for dmd before 2.067
                    bb32.firstUsedBucket = 0;
                }
                mBB.b.length = bb32.b.length;
                mBB.b.ptr = bb32.b.ptr;
                mBB.firstUsedBucket = bb32.firstUsedBucket;
                mBB.keyti = bb32.keyti;
                mBB.nodes = bb32.nodes;
            }
        }
        else
        {
            if ( mAAVersion == 1 )
                hr = mBinder->ReadMemory( address, sizeof mBB_V1, sizeRead, (uint8_t*)&mBB_V1 );
            else
                hr = mBinder->ReadMemory( address, sizeof mBB, sizeRead, (uint8_t*)&mBB );
            if ( FAILED( hr ) )
                return hr;

            if ( mAAVersion == -1 )
            {
                if ( mBB.b.length > 4 && ( mBB.b.length & ( mBB.b.length - 1 ) ) == 0 )
                {
                    mAAVersion = 1; // power of 2 indicates new AA
                    hr = mBinder->ReadMemory( address, sizeof mBB_V1, sizeRead, (uint8_t*)&mBB_V1 );
                    if ( FAILED( hr ) )
                        return hr;
                }
                else
                {
                    mAAVersion = 0;
                }
            }

            if ( mAAVersion == 0 && mBB.firstUsedBucket > mBB.nodes )
            {
                mBB.keyti = mBB.firstUsedBucket; // compatibility fix for dmd before 2.067
                mBB.firstUsedBucket = 0;
            }
        }

        return S_OK;
    }

    HRESULT EEDEnumAArray::ReadAddress( Address baseAddr, uint64_t index, Address& ptrValue )
    {
        HRESULT hr = S_OK;
        uint32_t ptrSize = mParentVal._Type->GetSize();
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
        uint32_t ptrSize = mParentVal._Type->GetSize();
        if ( ptrSize == 4 )
            return (size + sizeof( uint32_t ) - 1) & ~(sizeof( uint32_t ) - 1);
        else
            return (size + 16 - 1) & ~(16 - 1);
    }

    uint32_t EEDEnumAArray::GetCount()
    {
        uint32_t    count = 0;

        HRESULT hr = ReadBB();
        if ( !FAILED( hr ) )
            count = mAAVersion == 1 ? mBB_V1.used - mBB_V1.deleted : (uint32_t) mBB.nodes;

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
            uint32_t ptrSize = mParentVal._Type->GetSize();
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

        _ASSERT( mParentVal._Type->IsAArray() );
        ITypeAArray* aa = mParentVal._Type->AsTypeAArray();

        uint32_t ptrSize = mParentVal._Type->GetSize();

        DataObject keyobj;
        keyobj._Type = aa->GetIndex();
        keyobj.Addr = mNextNode + ( mAAVersion == 1 ? 0 : 2 * ptrSize );

        hr = mBinder->GetValue( keyobj.Addr, keyobj._Type, keyobj.Value );
        if ( FAILED( hr ) )
            return hr;

        std::wstring keystr;
        struct FormatOptions fmt (10);
        hr = FormatValue( mBinder, keyobj, fmt, keystr );
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

        FillValueTraits( result, nullptr );
        mCountDone++;

        return FindNext();
    }


    //------------------------------------------------------------------------
    //  EEDEnumStruct
    //------------------------------------------------------------------------

    EEDEnumStruct::EEDEnumStruct( bool skipHeadRef )
        :   mCountDone( 0 ),
            mSkipHeadRef( skipHeadRef )
    {
    }

    HRESULT EEDEnumStruct::Init( 
        IValueBinder* binder, 
        const wchar_t* parentExprText, 
        const DataObject& parentVal,
        ITypeEnv* typeEnv,
        NameTable* strTable )
    {
        HRESULT             hr = S_OK;
        RefPtr<Declaration> decl;
        RefPtr<IEnumDeclarationMembers> members;
        DataObject          parentValCopy = parentVal;

        if ( parentValCopy._Type == NULL )
            return E_INVALIDARG;

        if ( mSkipHeadRef && parentValCopy._Type->IsReference() )
        {
            parentValCopy._Type = parentValCopy._Type->AsTypeNext()->GetNext();
        }

        if ( parentValCopy._Type->AsTypeStruct() == NULL )
            return E_INVALIDARG;

        decl = parentValCopy._Type->GetDeclaration();
        if ( decl == NULL )
            return E_INVALIDARG;

        if ( !decl->EnumMembers( members.Ref() ) )
            return E_INVALIDARG;

        hr = EEDEnumValues::Init(
            binder,
            parentExprText,
            parentValCopy,
            typeEnv,
            strTable );
        if ( FAILED( hr ) )
            return hr;

        mMembers = members;

        return S_OK;
    }

    uint32_t EEDEnumStruct::GetCount()
    {
        return mMembers->GetCount();
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

    HRESULT EEDEnumStruct::Skip( uint32_t count )
    {
        if ( count > (GetCount() - mCountDone) )
        {
            mCountDone = GetCount();
            return S_FALSE;
        }

        mCountDone += count;
        mMembers->Skip( count );

        return S_OK;
    }

    HRESULT EEDEnumStruct::Clone( IEEDEnumValues*& copiedEnum )
    {
        HRESULT hr = S_OK;
        RefPtr<EEDEnumStruct>  en = new EEDEnumStruct();

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
        RefPtr<IEEDParsedExpr>  parsedExpr;

        if ( !mMembers->Next( decl.Ref() ) )
            return E_FAIL;

        mCountDone++;

        name.clear();
        fullName.clear();

        if ( decl->IsBaseClass() )
        {
            if ( !NameBaseClass( decl, name, fullName ) )
                return E_FAIL;
        }
        else if( decl->IsStaticField() )
        {
            if ( !NameStaticMember( decl, name, fullName ) )
                return E_FAIL;
        }
        else
        {
            if ( !NameRegularMember( decl, name, fullName ) )
                return E_FAIL;
        }

        hr = ParseText( fullName.c_str(), mTypeEnv, mStrTable, parsedExpr.Ref() );
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
        fullName.append( name );

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

        if( mParentVal._Type )
            mParentVal._Type->ToString( fullName );
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
