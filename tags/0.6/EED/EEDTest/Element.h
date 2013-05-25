/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


class Element
{
    long    mRefCount;

public:
    Element()
        :   mRefCount( 0 )
    {
    }

    virtual ~Element()
    {
    }

    virtual void AddRef()
    {
        mRefCount++;
    }

    virtual void Release()
    {
        mRefCount--;
        if ( mRefCount == 0 )
            delete this;
    }

    virtual const wchar_t* GetName()
    {
        return NULL;
    }

    virtual void AddChild( Element* elem ) = 0;
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value ) = 0;
    virtual void SetAttribute( const wchar_t* name, Element* elemValue ) = 0;
    virtual void PrintElement() = 0;
};


class ElementFactory
{
public:
    virtual Element* NewElement( const wchar_t* name ) = 0;
};
