/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "Enumerator.h"


template <class T, class TIt>
class IterEnumBase : public Enumerator<T>
{
protected:
    typedef TIt   It;
    It      mIt;

private:
    It      mItBegin;
    It      mItEnd;
    int     mCount;
    bool    mStarted;

public:
    IterEnumBase( It begin, It end, int count )
        :   mIt( begin ),
            mItBegin( begin ),
            mCount( count ),
            mItEnd( end ),
            mStarted( false )
    {
    }

    virtual void Release()
    {
        delete this;
    }

    virtual int GetCount()
    {
        return mCount;
    }

    //virtual T   GetCurrent() = 0;

    virtual bool MoveNext()
    {
        if ( !mStarted )
        {
            mStarted = true;
            return mIt != mItEnd;
        }

        mIt++;
        return mIt != mItEnd;
    }

    virtual void Reset()
    {
        mStarted = false;
        mIt = mItBegin;
    }
};


template <class T, class TIt>
class IterEnum : public IterEnumBase<T, TIt>
{
public:
    IterEnum( It begin, It end, int count )
        :   IterEnumBase( begin, end, count )
    {
    }

    virtual T   GetCurrent()
    {
        return *mIt;
    }
};


template <class T, class TIt>
class RefIterEnum : public IterEnumBase<T, TIt>
{
public:
    RefIterEnum( It begin, It end, int count )
        :   IterEnumBase( begin, end, count )
    {
    }

    virtual T   GetCurrent()
    {
        return mIt->Get();
    }
};


// TODO: C++0x should be able to handle templated typedefs, so I could do something like:
//      template <class T>
//      typedef IterEnum< T, std::list< T >::iterator > ListForwardIterEnum;

template <class T>
class ListForwardIterEnum : public IterEnum< T, typename std::list< T >::iterator >
{
public:
    ListForwardIterEnum( It begin, It end, int count )
        : IterEnum( begin, end, count )
    {
    }
};


template <class T>
class ListForwardRefIterEnum : 
    public RefIterEnum< T, typename std::list< RefPtr< typename boost::remove_pointer<T>::type > >::iterator >
{
public:
    ListForwardRefIterEnum( It begin, It end, int count )
        : RefIterEnum( begin, end, count )
    {
    }
};
