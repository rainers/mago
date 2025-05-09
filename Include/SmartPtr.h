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
        if( p != other )
        {
            if ( p != NULL )
                p->Release();

            p = other;

            if ( p != NULL )
                p->AddRef();
        }

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
};


template <class T>
struct DefaultDeleter
{
    static void Delete( T* value )
    {
        delete value;
    }
};

template <class T>
struct DefaultDeleter<T[]>
{
    static void Delete( T* value )
    {
        delete [] value;
    }
};

struct HandleDeleter
{
    static void Delete( HANDLE handle )
    {
        CloseHandle( handle );
    }
};

template <class T>
struct ReleaseDeleter
{
    static void Delete( T* value )
    {
        value->Release();
    }
};

struct RegistryDeleter
{
    static void Delete( HKEY hKey )
    {
        RegCloseKey( hKey );
    }
};


template <class T, T EmptyValue, class TDeleter>
class UniquePtrBase
{
protected:
    T p;

public:
    UniquePtrBase()
        : p( EmptyValue )
    {
    }

    explicit UniquePtrBase( T value )
        : p( value )
    {
    }

    // no destructor

    T Get() const
    {
        return p;
    }

    operator T() const
    {
        return p;
    }

    T& Ref()
    {
        _ASSERT( p == EmptyValue );
        return p;
    }

    bool IsEmpty() const
    {
        return p == EmptyValue;
    }

    UniquePtrBase& operator=( T value )
    {
        Attach( value );
        return *this;
    }

    void Attach( T value )
    {
        if ( value != p )
        {
            Delete();
            p = value;
        }
    }

    T Detach()
    {
        T outP = p;
        p = EmptyValue;
        return outP;
    }

protected:
    void Delete()
    {
        if ( p != EmptyValue )
        {
            TDeleter::Delete( p );
        }
    }

private:
    UniquePtrBase( const UniquePtrBase& );
    UniquePtrBase& operator=( const UniquePtrBase& );
};


template <HANDLE EmptyHandle>
class HandlePtrBase : public UniquePtrBase<HANDLE, EmptyHandle, HandleDeleter>
{
public:
    HandlePtrBase()
        : UniquePtrBase()
    {
    }

    explicit HandlePtrBase( HANDLE value )
        : UniquePtrBase( value )
    {
    }

    ~HandlePtrBase()
    {
        Delete();
    }

    HandlePtrBase& operator=( HANDLE value )
    {
        Attach( value );
        return *this;
    }

private:
    HandlePtrBase( const HandlePtrBase& other );
    HandlePtrBase& operator=( const HandlePtrBase& other );
};


template <class T, class TDeleter = DefaultDeleter<T> >
class UniquePtr : public UniquePtrBase<T*, NULL, TDeleter>
{
public:
    UniquePtr()
        : UniquePtrBase()
    {
    }

    explicit UniquePtr( T* value )
        : UniquePtrBase( value )
    {
    }

    ~UniquePtr()
    {
        Delete();
    }

    T* operator->() const
    {
        _ASSERT( Get() != NULL );
        return Get();
    }

    void Swap( UniquePtr& other )
    {
        T* otherP = other.p;
        other.p = p;
        p = otherP;
    }

private:
    UniquePtr( const UniquePtr& other );
    UniquePtr& operator=( const UniquePtr& other );
};


template <class T, class TDeleter>
class UniquePtr<T[], TDeleter> : public UniquePtrBase<T*, NULL, TDeleter>
{
public:
    UniquePtr()
        : UniquePtrBase()
    {
    }

    explicit UniquePtr( T* value )
        : UniquePtrBase( value )
    {
    }

    ~UniquePtr()
    {
        Delete();
    }

    T* operator->() const
    {
        _ASSERT( Get() != NULL );
        return Get();
    }

    void Swap( UniquePtr& other )
    {
        T* otherP = other.p;
        other.p = p;
        p = otherP;
    }

private:
    UniquePtr( const UniquePtr& other );
    UniquePtr& operator=( const UniquePtr& other );
};


template <class T>
struct _RefReleasePtr
{
    typedef UniquePtr<T, ReleaseDeleter<T> > type;
};


typedef HandlePtrBase<NULL> HandlePtr;
typedef HandlePtrBase<INVALID_HANDLE_VALUE> FileHandlePtr;
typedef UniquePtrBase<HKEY, NULL, RegistryDeleter> RegHandlePtr;
