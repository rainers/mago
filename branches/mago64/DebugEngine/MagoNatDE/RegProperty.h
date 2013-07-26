/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class IRegisterSet;
    struct RegGroup;
    class Thread;


    struct Reg
    {
        const wchar_t*  Name;
        uint8_t         FullReg;
        uint8_t         Length;
        uint8_t         Shift;
        uint32_t        Mask;
    };


    struct RegGroup
    {
        uint32_t    StrId;
        const Reg*  Regs;
        uint32_t    RegCount;
        uint32_t    NeededFeature;
    };


    HRESULT EnumRegisters(
        const RegGroup* groups,
        uint32_t groupCount,
        IRegisterSet* regSet, 
        Thread* thread,
        DEBUGPROP_INFO_FLAGS fields,
        DWORD radix,
        IEnumDebugPropertyInfo2** enumerator );


    class ATL_NO_VTABLE RegGroupProperty : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugProperty2
    {
        CString                 mName;
        const RegGroup*         mGroup;
        RefPtr<IRegisterSet>    mRegSet;
        RefPtr<Thread>          mThread;

    public:
        RegGroupProperty();
        ~RegGroupProperty();

    DECLARE_NOT_AGGREGATABLE(RegGroupProperty)

    BEGIN_COM_MAP(RegGroupProperty)
        COM_INTERFACE_ENTRY(IDebugProperty2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugProperty2 

        STDMETHOD( GetPropertyInfo )( 
            DEBUGPROP_INFO_FLAGS dwFields,
            DWORD dwRadix,
            DWORD dwTimeout,
            IDebugReference2** rgpArgs,
            DWORD dwArgCount,
            DEBUG_PROPERTY_INFO* pPropertyInfo );

        STDMETHOD( SetValueAsString )( 
            LPCOLESTR pszValue,
            DWORD dwRadix,
            DWORD dwTimeout );

        STDMETHOD( SetValueAsReference )( 
            IDebugReference2** rgpArgs,
            DWORD dwArgCount,
            IDebugReference2* pValue,
            DWORD dwTimeout );

        STDMETHOD( EnumChildren )( 
            DEBUGPROP_INFO_FLAGS dwFields,
            DWORD dwRadix,
            REFGUID guidFilter,
            DBG_ATTRIB_FLAGS dwAttribFilter,
            LPCOLESTR pszNameFilter,
            DWORD dwTimeout,
            IEnumDebugPropertyInfo2** ppEnum );

        STDMETHOD( GetParent )( 
            IDebugProperty2** ppParent );

        STDMETHOD( GetDerivedMostProperty )( 
            IDebugProperty2** ppDerivedMost );

        STDMETHOD( GetMemoryBytes )( 
            IDebugMemoryBytes2** ppMemoryBytes );

        STDMETHOD( GetMemoryContext )( 
            IDebugMemoryContext2** ppMemory );

        STDMETHOD( GetSize )( 
            DWORD* pdwSize );

        STDMETHOD( GetReference )( 
            IDebugReference2** ppReference );

        STDMETHOD( GetExtendedInfo )( 
            REFGUID guidExtendedInfo,
            VARIANT* pExtendedInfo );

    public:
        HRESULT Init( const RegGroup* group, IRegisterSet* regSet, Thread* thread );
    };


    class ATL_NO_VTABLE RegisterProperty : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugProperty2
    {
        const Reg*              mReg;
        RefPtr<IRegisterSet>    mRegSet;
        RefPtr<Thread>          mThread;

    public:
        RegisterProperty();
        ~RegisterProperty();

    DECLARE_NOT_AGGREGATABLE(RegisterProperty)

    BEGIN_COM_MAP(RegisterProperty)
        COM_INTERFACE_ENTRY(IDebugProperty2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugProperty2 

        STDMETHOD( GetPropertyInfo )( 
            DEBUGPROP_INFO_FLAGS dwFields,
            DWORD dwRadix,
            DWORD dwTimeout,
            IDebugReference2** rgpArgs,
            DWORD dwArgCount,
            DEBUG_PROPERTY_INFO* pPropertyInfo );

        STDMETHOD( SetValueAsString )( 
            LPCOLESTR pszValue,
            DWORD dwRadix,
            DWORD dwTimeout );

        STDMETHOD( SetValueAsReference )( 
            IDebugReference2** rgpArgs,
            DWORD dwArgCount,
            IDebugReference2* pValue,
            DWORD dwTimeout );

        STDMETHOD( EnumChildren )( 
            DEBUGPROP_INFO_FLAGS dwFields,
            DWORD dwRadix,
            REFGUID guidFilter,
            DBG_ATTRIB_FLAGS dwAttribFilter,
            LPCOLESTR pszNameFilter,
            DWORD dwTimeout,
            IEnumDebugPropertyInfo2** ppEnum );

        STDMETHOD( GetParent )( 
            IDebugProperty2** ppParent );

        STDMETHOD( GetDerivedMostProperty )( 
            IDebugProperty2** ppDerivedMost );

        STDMETHOD( GetMemoryBytes )( 
            IDebugMemoryBytes2** ppMemoryBytes );

        STDMETHOD( GetMemoryContext )( 
            IDebugMemoryContext2** ppMemory );

        STDMETHOD( GetSize )( 
            DWORD* pdwSize );

        STDMETHOD( GetReference )( 
            IDebugReference2** ppReference );

        STDMETHOD( GetExtendedInfo )( 
            REFGUID guidExtendedInfo,
            VARIANT* pExtendedInfo );

    public:
        HRESULT Init( const Reg* reg, IRegisterSet* regSet, Thread* thread );

    private:
        BSTR GetValueStr( bool& valid );
    };
}
