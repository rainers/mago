/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoEE
{
    enum ObjectKind
    {
        ObjectKind_None,
        ObjectKind_ObjectList,
        ObjectKind_Expression,
        ObjectKind_ExpressionList,
        ObjectKind_Type,
        ObjectKind_NamePart,
        ObjectKind_Parameter,
        ObjectKind_ParameterList,
    };

    class Object
    {
        long    mRefCount;

    public:
        Object();
        virtual ~Object();

        virtual void AddRef();
        virtual void Release();

        virtual ObjectKind GetObjectKind() = 0;
    };


    class ObjectList : public Object
    {
    public:
        std::list< RefPtr<Object> > List;

        virtual ObjectKind GetObjectKind();
    };
}
