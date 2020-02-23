/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "EED.h"

namespace MagoEE
{
    class EEDEnumValues : public IEEDEnumValues
    {
        long                mRefCount;

    protected:
        std::wstring        mParentExprText;
        EvalResult          mParentVal;
        IValueBinder*       mBinder;
        RefPtr<ITypeEnv>    mTypeEnv;
        RefPtr<NameTable>   mStrTable;

    public:
        EEDEnumValues();
        virtual ~EEDEnumValues();

        virtual void AddRef();
        virtual void Release();

        virtual HRESULT Init( 
            IValueBinder* binder, 
            const wchar_t* parentExprText, 
            const EvalResult& parentVal,
            ITypeEnv* typeEnv,
            NameTable* strTable );

        virtual HRESULT EvaluateExpr( 
            const EvalOptions& options, 
            EvalResult& result, 
            std::wstring& expr );
    };


    class EEDEnumPointer : public EEDEnumValues
    {
        uint32_t        mCountDone;

    public:
        EEDEnumPointer();

        virtual uint32_t GetCount();
        virtual uint32_t GetIndex();
        virtual void Reset();
        virtual HRESULT Skip( uint32_t count );
        virtual HRESULT Clone( IEEDEnumValues*& copiedEnum );

        virtual HRESULT EvaluateNext( 
            const EvalOptions& options, 
            EvalResult& result, 
            std::wstring& name, 
            std::wstring& fullName );
    };


    class EEDEnumSArray : public EEDEnumValues
    {
        uint32_t        mCountDone;
        uint32_t        mBaseOffset;

        uint64_t GetUnlimitedCount();

    public:
        EEDEnumSArray();

        virtual HRESULT Init(
            IValueBinder* binder,
            const wchar_t* parentExprText,
            const EvalResult& parentVal,
            ITypeEnv* typeEnv,
            NameTable* strTable);

        virtual uint32_t GetCount();
        virtual uint32_t GetIndex();
        virtual void Reset();
        virtual HRESULT Skip( uint32_t count );
        virtual HRESULT Clone( IEEDEnumValues*& copiedEnum );

        virtual HRESULT EvaluateNext( 
            const EvalOptions& options, 
            EvalResult& result, 
            std::wstring& name, 
            std::wstring& fullName );
    };

    typedef EEDEnumSArray EEDEnumDArray;

    class EEDEnumRawDArray : public EEDEnumValues
    {
        uint32_t        mCountDone;

    public:
        EEDEnumRawDArray();

        virtual uint32_t GetCount();
        virtual uint32_t GetIndex();
        virtual void Reset();
        virtual HRESULT Skip( uint32_t count );
        virtual HRESULT Clone( IEEDEnumValues*& copiedEnum );

        virtual HRESULT EvaluateNext( 
            const EvalOptions& options, 
            EvalResult& result, 
            std::wstring& name, 
            std::wstring& fullName );
    };


    class EEDEnumAArray : public EEDEnumValues
    {
        int             mAAVersion;
        uint64_t        mCountDone;
        uint64_t        mBucketIndex;
        Address         mNextNode;
        union
        {
            BB64            mBB;
            BB64_V1         mBB_V1;
        };

        HRESULT ReadBB();
        HRESULT ReadAddress( Address baseAddr, uint64_t index, Address& ptrValue );
        HRESULT FindCurrent();
        HRESULT FindNext();
        uint32_t AlignTSize( uint32_t size );
        uint64_t GetUnlimitedCount();

    public:
        EEDEnumAArray( int aaVersion );

        virtual uint32_t GetCount();
        virtual uint32_t GetIndex();
        virtual void Reset();
        virtual HRESULT Skip( uint32_t count );
        virtual HRESULT Clone( IEEDEnumValues*& copiedEnum );

        virtual HRESULT EvaluateNext( 
            const EvalOptions& options, 
            EvalResult& result, 
            std::wstring& name, 
            std::wstring& fullName );

        static HRESULT ReadBB( IValueBinder* binder, RefPtr<Type> type, Address address, int& AAVersion, BB64& BB );
    };

    class EEDEnumTuple : public EEDEnumValues
    {
        uint32_t        mCountDone;

    public:
        EEDEnumTuple();

        virtual HRESULT Init(
            IValueBinder* binder,
            const wchar_t* parentExprText,
            const EvalResult& parentVal,
            ITypeEnv* typeEnv,
            NameTable* strTable);

        virtual uint32_t GetCount();
        virtual uint32_t GetIndex();
        virtual void Reset();
        virtual HRESULT Skip( uint32_t count );
        virtual HRESULT Clone( IEEDEnumValues*& copiedEnum );

        virtual HRESULT EvaluateNext( 
            const EvalOptions& options, 
            EvalResult& result, 
            std::wstring& name, 
            std::wstring& fullName );
    };


    class EEDEnumStruct : public EEDEnumValues
    {
        uint32_t        mCountDone;
        uint32_t        mMembersPos;
        uint32_t        mHiddenFields;
        bool            mSkipHeadRef;
        bool            mHasVTable;

        RefPtr<IEnumDeclarationMembers> mMembers;
        std::wstring    mClassName;

    public:
        EEDEnumStruct( bool skipHeadRef = false );

        virtual HRESULT Init( 
            IValueBinder* binder, 
            const wchar_t* parentExprText, 
            const EvalResult& parentVal,
            ITypeEnv* typeEnv,
            NameTable* strTable );

        virtual uint32_t GetCount();
        virtual uint32_t GetIndex();
        virtual void Reset();
        virtual HRESULT Skip( uint32_t count );
        virtual HRESULT Clone( IEEDEnumValues*& copiedEnum );

        virtual HRESULT EvaluateNext( 
            const EvalOptions& options, 
            EvalResult& result, 
            std::wstring& name, 
            std::wstring& fullName );

    private:
        bool NameBaseClass( 
            Declaration* decl, 
            std::wstring& name,
            std::wstring& fullName );

        bool NameStaticMember( 
            Declaration* decl, 
            std::wstring& name,
            std::wstring& fullName );

        bool NameRegularMember( 
            Declaration* decl, 
            std::wstring& name,
            std::wstring& fullName );

        uint32_t VShapePos() const;
    };
}
