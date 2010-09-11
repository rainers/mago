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
        DataObject          mParentVal;
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
            const DataObject& parentVal,
            ITypeEnv* typeEnv,
            NameTable* strTable );
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

    public:
        EEDEnumSArray();

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


    class EEDEnumStruct : public EEDEnumValues
    {
        uint32_t        mCountDone;
        bool            mSkipHeadRef;

        RefPtr<IEnumDeclarationMembers> mMembers;

    public:
        EEDEnumStruct( bool skipHeadRef = false );

        virtual HRESULT Init( 
            IValueBinder* binder, 
            const wchar_t* parentExprText, 
            const DataObject& parentVal,
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

        bool NameRegularMember( 
            Declaration* decl, 
            std::wstring& name,
            std::wstring& fullName );
    };
}
