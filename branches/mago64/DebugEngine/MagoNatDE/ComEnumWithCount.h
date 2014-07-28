/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


template <class T>
class InterfaceArray
{
    T**     mArray;
    size_t  mLen;

public:
    InterfaceArray( size_t length )
        :   mArray( new T*[ length ] ),
            mLen( length )
    {
        if ( mArray == NULL )
            return;

        size_t  size = length * sizeof( T* );
        memset( mArray, 0, size );
    }

    ~InterfaceArray()
    {
        if ( mArray == NULL )
            return;

        for ( size_t i = 0; i < mLen; i++ )
        {
            if ( mArray[i] != NULL )
                mArray[i]->Release();
        }

        delete [] mArray;
    }

    size_t GetLength() const
    {
        return mLen;
    }

    T*& operator[]( size_t i ) const
    {
        _ASSERT( i < mLen );
        return mArray[i];
    }

    T** Get() const
    {
        return mArray;
    }

    T** Detach()
    {
        T** array = mArray;
        mArray = NULL;
        return array;
    }

private:
    InterfaceArray( const InterfaceArray& other );
    InterfaceArray& operator=( const InterfaceArray& other );
};


// based on atlcom.h, adding GetCount method

template <class Base, const IID* piid, class T, class Copy>
class CComEnumWithCountImpl : public Base
{
public:
    CComEnumWithCountImpl() {m_begin = m_end = m_iter = NULL; m_dwFlags = 0;}
    virtual ~CComEnumWithCountImpl();
    STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)(){m_iter = m_begin;return S_OK;}
    STDMETHOD(Clone)(Base** ppEnum);
    STDMETHOD(GetCount)( ULONG* count )
    {
        if ( count == NULL )
            return E_INVALIDARG;

        *count = m_end - m_begin;
        return S_OK;
    }
    HRESULT Init(T* begin, T* end, IUnknown* pUnk,
        CComEnumFlags flags = AtlFlagNoCopy);
    CComPtr<IUnknown> m_spUnk;
    T* m_begin;
    T* m_end;
    T* m_iter;
    DWORD m_dwFlags;
protected:
    enum FlagBits
    {
        BitCopy=1,
        BitOwn=2
    };
};

template <class Base, const IID* piid, class T, class Copy>
CComEnumWithCountImpl<Base, piid, T, Copy>::~CComEnumWithCountImpl()
{
    if (m_dwFlags & BitOwn)
    {
        for (T* p = m_begin; p != m_end; p++)
            Copy::destroy(p);
        delete [] m_begin;
    }
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumWithCountImpl<Base, piid, T, Copy>::Next(ULONG celt, T* rgelt,
    ULONG* pceltFetched)
{
    if (pceltFetched != NULL)
        *pceltFetched = 0;
    if (celt == 0)
        return E_INVALIDARG;
    if (rgelt == NULL || (celt != 1 && pceltFetched == NULL))
        return E_POINTER;
    if (m_begin == NULL || m_end == NULL || m_iter == NULL)
        return E_FAIL;
    ULONG nRem = (ULONG)(m_end - m_iter);
    HRESULT hRes = S_OK;
    if (nRem < celt)
        hRes = S_FALSE;
    ULONG nMin = celt < nRem ? celt : nRem ;
    if (pceltFetched != NULL)
        *pceltFetched = nMin;
    T* pelt = rgelt;
    while(nMin--)
    {
        HRESULT hr = Copy::copy(pelt, m_iter);
        if (FAILED(hr))
        {
            while (rgelt < pelt)
                Copy::destroy(rgelt++);
            if (pceltFetched != NULL)
                *pceltFetched = 0;
            return hr;
        }
        pelt++;
        m_iter++;
    }
    return hRes;
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumWithCountImpl<Base, piid, T, Copy>::Skip(ULONG celt)
{
    if (celt == 0)
        return S_OK;

    ULONG nRem = ULONG(m_end - m_iter);
    ULONG nSkip = (celt > nRem) ? nRem : celt;
    m_iter += nSkip;
    return (celt == nSkip) ? S_OK : S_FALSE;
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumWithCountImpl<Base, piid, T, Copy>::Clone(Base** ppEnum)
{
    typedef CComObject<CComEnumWithCount<Base, piid, T, Copy> > _class;
    HRESULT hRes = E_POINTER;
    if (ppEnum != NULL)
    {
        *ppEnum = NULL;
        _class* p;
        hRes = _class::CreateInstance(&p);
        if (SUCCEEDED(hRes))
        {
            // If this object has ownership of the data then we need to keep it around
            hRes = p->Init(m_begin, m_end, (m_dwFlags & BitOwn) ? this : m_spUnk);
            if (SUCCEEDED(hRes))
            {
                p->m_iter = m_iter;
                hRes = p->_InternalQueryInterface(*piid, (void**)ppEnum);
            }
            if (FAILED(hRes))
                delete p;
        }
    }
    return hRes;
}

template <class Base, const IID* piid, class T, class Copy>
HRESULT CComEnumWithCountImpl<Base, piid, T, Copy>::Init(T* begin, T* end, IUnknown* pUnk,
    CComEnumFlags flags)
{
    if (flags == AtlFlagCopy)
    {
        ATLASSUME(m_begin == NULL); //Init called twice?
        ATLTRY(m_begin = new T[end-begin])
        m_iter = m_begin;
        if (m_begin == NULL)
            return E_OUTOFMEMORY;
        for (T* i=begin; i != end; i++)
        {
            Copy::init(m_iter);
            HRESULT hr = Copy::copy(m_iter, i);
            if (FAILED(hr))
            {
                T* p = m_begin;
                while (p < m_iter)
                    Copy::destroy(p++);
                delete [] m_begin;
                m_begin = m_end = m_iter = NULL;
                return hr;
            }
            m_iter++;
        }
        m_end = m_begin + (end-begin);
    }
    else
    {
        m_begin = begin;
        m_end = end;
    }
    m_spUnk = pUnk;
    m_iter = m_begin;
    m_dwFlags = flags;
    return S_OK;
}


template <class Base, const IID* piid, class T, class Copy, class ThreadModel = CComObjectThreadModel>
class CComEnumWithCount :
    public CComEnumWithCountImpl<Base, piid, T, Copy>,
    public CComObjectRootEx< ThreadModel >
{
public:
    typedef CComEnumWithCount<Base, piid, T, Copy > _CComEnum;
    typedef CComEnumWithCountImpl<Base, piid, T, Copy > _CComEnumBase;
    BEGIN_COM_MAP(_CComEnum)
        COM_INTERFACE_ENTRY_IID(*piid, _CComEnumBase)
    END_COM_MAP()
};


template <class TCEnum, class TIEnum, class TMap, class T>
HRESULT MakeEnumWithCount( TMap& map, TIEnum** ppEnum )
{
    if ( ppEnum == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    size_t              i = 0;
    InterfaceArray<T>   array( map.size() );
    RefPtr<TCEnum>      tEnum;

    for ( TMap::iterator it = map.begin();
        it != map.end();
        it++, i++ )
    {
        it->second->QueryInterface( __uuidof( T ), (void**) &array[i] );
    }

    hr = MakeCComObject( tEnum );
    if ( FAILED( hr ) )
        return hr;

    hr = tEnum->Init( 
        array.Get(), 
        array.Get() + array.GetLength(), 
        NULL, 
        AtlFlagTakeOwnership );
    if ( FAILED( hr ) )
        return hr;

    array.Detach();
    *ppEnum = tEnum.Detach();

    return hr;
}

template <class TCEnum, class TIEnum, class T>
HRESULT MakeEnumWithCount( InterfaceArray<T>& array, TIEnum** ppEnum )
{
    if ( ppEnum == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    RefPtr<TCEnum>      tEnum;

    hr = MakeCComObject( tEnum );
    if ( FAILED( hr ) )
        return hr;

    hr = tEnum->Init( 
        array.Get(), 
        array.Get() + array.GetLength(), 
        NULL, 
        AtlFlagTakeOwnership );
    if ( FAILED( hr ) )
        return hr;

    array.Detach();
    *ppEnum = tEnum.Detach();

    return hr;
}

template <class TCEnum, class TIEnum, class T, class TCopy>
HRESULT MakeEnumWithCount( ScopedArray<T, TCopy>& array, TIEnum** ppEnum )
{
    if ( ppEnum == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    RefPtr<TCEnum>      tEnum;

    hr = MakeCComObject( tEnum );
    if ( FAILED( hr ) )
        return hr;

    hr = tEnum->Init( 
        array.Get(), 
        array.Get() + array.GetLength(), 
        NULL, 
        AtlFlagTakeOwnership );
    if ( FAILED( hr ) )
        return hr;

    array.Detach();
    *ppEnum = tEnum.Detach();

    return hr;
}
