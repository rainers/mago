/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "PropTables.h"
#include "Properties.h"


namespace MagoEE
{
    struct PropPair
    {
        const wchar_t*      Name;
        StdProperty*        Property;
    };


    //------------------------------------------------------------------------

    PropPair  mBaseProps[] = 
    {
        { L"sizeof",  new PropertySize() },
    };


    PropPair  mIntProps[] = 
    {
        { L"max",  new PropertyIntegralMax() },
        { L"min",  new PropertyIntegralMin() },
    };

    PropPair  mFloatProps[] = 
    {
        { L"max",  new PropertyFloatMax() },
        { L"min_normal",  new PropertyFloatMin() },
        { L"infinity",  new PropertyFloatInfinity() },
        { L"nan",  new PropertyFloatNan() },
        { L"dig",  new PropertyFloatDigits() },
        { L"epsilon",  new PropertyFloatEpsilon() },
        { L"mant_dig",  new PropertyFloatMantissaDigits() },
        { L"max_10_exp",  new PropertyFloatMax10Exp() },
        { L"max_exp",  new PropertyFloatMaxExp() },
        { L"min_10_exp",  new PropertyFloatMin10Exp() },
        { L"min_exp",  new PropertyFloatMinExp() },
        { L"re",  new PropertyFloatReal() },
        { L"im",  new PropertyFloatImaginary() },
    };

    PropPair  mDArrayProps[] = 
    {
        { L"length",  new PropertyDArrayLength() },
        { L"ptr",  new PropertyDArrayPtr() },
    };

    PropPair  mSArrayProps[] = 
    {
        { L"length",  new PropertySArrayLength() },
        { L"ptr",  new PropertySArrayPtr() },
    };

    PropPair  mDelegateProps[] = 
    {
        { L"ptr",  new PropertyDelegatePtr() },
        { L"funcptr",  new PropertyDelegateFuncPtr() },
    };

    PropPair  mFieldProps[] = 
    {
        { L"offsetof",  new PropertyFieldOffset() },
    };

    struct PropPairPair
    {
        PropPair*   Pair;
        size_t      Len;
    };

    PropPairPair    mAllPropPairs[] = 
    {
        { mBaseProps, _countof( mBaseProps ) },
        { mIntProps, _countof( mIntProps ) },
        { mFloatProps, _countof( mFloatProps ) },
        { mDArrayProps, _countof( mDArrayProps ) },
        { mSArrayProps, _countof( mSArrayProps ) },
        { mDelegateProps, _countof( mDelegateProps ) },
        { mFieldProps, _countof( mFieldProps ) },
    };


    //------------------------------------------------------------------------

    HRESULT InitPropTables()
    {
        return S_OK;
    }

    void FreePropTables()
    {
        for ( size_t i = 0; i < _countof( mAllPropPairs ); i++ )
        {
            for ( size_t j = 0; j < mAllPropPairs[i].Len; j++ )
            {
                delete mAllPropPairs[i].Pair[j].Property;
            }
        }
    }

    StdProperty* FindPropertyInArray( const wchar_t* name, PropPair* array, size_t len )
    {
        for ( size_t i = 0; i < len; i++ )
        {
            if ( wcscmp( name, array[i].Name ) == 0 )
                return array[i].Property;
        }
        return NULL;
    }

    StdProperty* FindBaseProperty( const wchar_t* name )
    {
        return FindPropertyInArray( name, mBaseProps, _countof( mBaseProps ) );
    }

    StdProperty* FindIntProperty( const wchar_t* name )
    {
        return FindPropertyInArray( name, mIntProps, _countof( mIntProps ) );
    }

    StdProperty* FindFloatProperty( const wchar_t* name )
    {
        return FindPropertyInArray( name, mFloatProps, _countof( mFloatProps ) );
    }

    StdProperty* FindDArrayProperty( const wchar_t* name )
    {
        return FindPropertyInArray( name, mDArrayProps, _countof( mDArrayProps ) );
    }

    StdProperty* FindSArrayProperty( const wchar_t* name )
    {
        return FindPropertyInArray( name, mSArrayProps, _countof( mSArrayProps ) );
    }

    StdProperty* FindDelegateProperty( const wchar_t* name )
    {
        return FindPropertyInArray( name, mDelegateProps, _countof( mDelegateProps ) );
    }

    StdProperty* FindFieldProperty( const wchar_t* name )
    {
        return FindPropertyInArray( name, mFieldProps, _countof( mFieldProps ) );
    }
}
