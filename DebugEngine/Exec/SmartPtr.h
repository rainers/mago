/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


template <class T>
class RefPtr
{
    T*  p;

public:
    RefPtr()
        :   p( NULL )
    {
    }

    RefPtr( T* other )
        :   p( other )
    {
        if ( p != NULL )
            p->AddRef();
    }

    RefPtr( const RefPtr& other )
        :   p( other.p )
    {
        if ( p != NULL )
            p->AddRef();
    }

    ~RefPtr()
    {
        if ( p != NULL )
            p->Release();
    }

    T* Get() const
    {
        return p;
    }

    T*& Ref()
    {
        _ASSERT( p == NULL );
        return p;
    }

    void Release()
    {
        if ( p != NULL )
        {
            p->Release();
            p = NULL;
        }
    }

    void Attach( T* other )
    {
        if ( p != NULL )
            p->Release();

        p = other;
    }

    T* Detach()
    {
        T*  outP = p;
        p = NULL;
        return outP;
    }

    RefPtr<T>& operator =( T* other )
    {
        if ( p != NULL )
            p->Release();

        p = other;

        if ( p != NULL )
            p->AddRef();

        return *this;
    }

    RefPtr<T>& operator =( const RefPtr& other )
    {
        // delegate to operator =( T* other )
        return *this = other.p;
    }

    T* operator ->() const
    {
        return p;
    }

    T& operator *() const
    {
        _ASSERT( p != NULL );
        return *p;
    }

    bool operator ==( const RefPtr& other ) const
    {
        return p == other.p;
    }

    //T*& Ptr() const
    //{
    //    return p;
    //}
};


template <class T>
class RefReleasePtr
{
    T*  p;

public:
    RefReleasePtr()
        :   p( NULL )
    {
    }

    RefReleasePtr( T* other )
        :   p( other )
    {
    }

    RefReleasePtr( const RefReleasePtr& other )
        :   p( other.p )
    {
    }

    ~RefReleasePtr()
    {
        if ( p != NULL )
            p->Release();
    }

    T* Get() const
    {
        return p;
    }

    T*& Ref()
    {
        _ASSERT( p == NULL );
        return p;
    }

    void Release()
    {
        if ( p != NULL )
        {
            p->Release();
            p = NULL;
        }
    }

    void Attach( T* other )
    {
        if ( p != NULL )
            p->Release();

        p = other;
    }

    T* Detach()
    {
        T*  outP = p;
        p = NULL;
        return outP;
    }

    RefReleasePtr<T>& operator =( T* other )
    {
        if ( p != NULL )
            p->Release();

        p = other;

        return *this;
    }

    RefReleasePtr<T>& operator =( const RefReleasePtr& other )
    {
        // delegate to operator =( T* other )
        return *this = other.p;
    }

    T* operator ->() const
    {
        return p;
    }

    T& operator *() const
    {
        _ASSERT( p != NULL );
        return *p;
    }

    bool operator ==( const RefReleasePtr& other ) const
    {
        return p == other.p;
    }

    //T*& Ptr() const
    //{
    //    return p;
    //}
};


class HandlePtr
{
    HANDLE  h;

public:
    HandlePtr()
        :   h( NULL )
    {
    }

    HandlePtr( HANDLE handle )
        :   h( handle )
    {
    }

    ~HandlePtr()
    {
        if ( h != NULL )
        {
            CloseHandle( h );
        }
    }

    HANDLE Get() const
    {
        return h;
    }

    HANDLE& Ref()
    {
        _ASSERT( h == NULL );
        return h;
    }

    bool IsEmpty() const
    {
        return h == NULL;
    }

    HandlePtr& operator =( HANDLE handle )
    {
        if ( h != NULL )
            CloseHandle( h );

        h = handle;

        return *this;
    }

    void Attach( HANDLE handle )
    {
        if ( h != NULL )
            CloseHandle( h );

        h = handle;
    }

    HANDLE Detach()
    {
        HANDLE  outH = h;
        h = NULL;
        return outH;
    }

    void Close()
    {
        if ( h != NULL )
        {
            CloseHandle( h );
            h = NULL;
        }
    }
};
