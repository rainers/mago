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

    // used this before adding operator T*
    //bool operator ==( const RefPtr& other ) const
    //{
    //    return p == other.p;
    //}

    //bool operator ==( const void* pOther ) const
    //{
    //    return p == pOther;
    //}

    //bool operator !=( const void* pOther ) const
    //{
    //    return p != pOther;
    //}

    operator T*() const
    {
        return p;
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


#ifdef _WINNT_

template <HANDLE EmptyHandle>
class HandlePtrBase
{
    HANDLE  h;

public:
    HandlePtrBase()
        :   h( EmptyHandle )
    {
    }

    HandlePtrBase( HANDLE handle )
        :   h( handle )
    {
    }

    ~HandlePtrBase()
    {
        if ( h != EmptyHandle )
        {
            CloseHandle( h );
        }
    }

    HANDLE Get() const
    {
        return h;
    }

    operator HANDLE() const
    {
        return h;
    }

    HANDLE& Ref()
    {
        _ASSERT( h == EmptyHandle );
        return h;
    }

    bool IsEmpty() const
    {
        return h == EmptyHandle;
    }

    // TODO: doesn't seem right to return HandlePtrBase instead of derived class
    //       one way to get around it is by declaring HandlePtrBase as:
    // template <class TDerived, HANDLE EmptyHandle> class HandlePtrBase
    //       and deriving it as:
    // class HandlePtr : public HandlePtrBase<HandlePtr, NULL>
    //       remember to add the right constructors that forward to HandlePtrBase's

    HandlePtrBase& operator =( HANDLE handle )
    {
        if ( h != EmptyHandle )
            CloseHandle( h );

        h = handle;

        return *this;
    }

    void Attach( HANDLE handle )
    {
        if ( h != EmptyHandle )
            CloseHandle( h );

        h = handle;
    }

    HANDLE Detach()
    {
        HANDLE  outH = h;
        h = EmptyHandle;
        return outH;
    }

    void Close()
    {
        if ( h != EmptyHandle )
        {
            CloseHandle( h );
            h = EmptyHandle;
        }
    }
};


typedef HandlePtrBase<NULL> HandlePtr;
typedef HandlePtrBase<INVALID_HANDLE_VALUE> FileHandlePtr;

#endif


template <class T, T EmptyHandle, class TCloser>
class HandlePtrBase2
{
    T   h;

public:
    HandlePtrBase2()
        :   h( EmptyHandle )
    {
    }

    HandlePtrBase2( T handle )
        :   h( handle )
    {
    }

    ~HandlePtrBase2()
    {
        if ( h != EmptyHandle )
        {
            ForceClose( h );
        }
    }

    T Get() const
    {
        return h;
    }

    operator T() const
    {
        return h;
    }

    T& Ref()
    {
        _ASSERT( h == EmptyHandle );
        return h;
    }

    bool IsEmpty() const
    {
        return h == EmptyHandle;
    }

    // TODO: doesn't seem right to return HandlePtrBase2 instead of derived class
    //       one way to get around it is by declaring HandlePtrBase2 as:
    // template <class TDerived, HANDLE EmptyHandle> class HandlePtrBase2
    //       and deriving it as:
    // class HandlePtr : public HandlePtrBase2<HandlePtr, NULL>
    //       remember to add the right constructors that forward to HandlePtrBase2's

    HandlePtrBase2& operator =( T handle )
    {
        if ( h != EmptyHandle )
        {
            ForceClose( h );
        }

        h = handle;

        return *this;
    }

    void Attach( T handle )
    {
        if ( h != EmptyHandle )
        {
            ForceClose( h );
        }

        h = handle;
    }

    T Detach()
    {
        T  outH = h;
        h = EmptyHandle;
        return outH;
    }

    void Close()
    {
        if ( h != EmptyHandle )
        {
            ForceClose( h );
            h = EmptyHandle;
        }
    }

private:
    void ForceClose( T h )
    {
        TCloser()( h );
    }
};
