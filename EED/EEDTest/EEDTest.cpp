/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.

   Purpose: Defines the entry point for the EED test console app
*/

#include "Common.h"
#include "PrintingContentHandler.h"
#include "BuildingContentHandler.h"
#include "SAXErrorHandler.h"
#include "DataElement.h"
#include "TestElement.h"
#include "DataValue.h"
#include "DataEnv.h"
#include "ProgValueEnv.h"
#include "AppSettings.h"
#include "SymUtil.h"

using namespace std;
using MagoEE::ITypeEnv;

bool TestReal10();

AppSettings gAppSettings = { 0 };


class TopScope : public IScope
{
    typedef std::map< std::wstring, RefPtr<MagoEE::Type> >  TypeMap;

    RefPtr<DeclDataElement> mDecl;
    ITypeEnv*               mTypeEnv;

public:
    TopScope( DeclDataElement* decl, ITypeEnv* typeEnv )
        :   mDecl( decl ),
            mTypeEnv( typeEnv )
    {
    }

    virtual HRESULT FindObject( const wchar_t* name, MagoEE::Declaration*& decl )
    {
        HRESULT hr = S_OK;

        hr = FindBasicType( name, mTypeEnv, decl );
        if ( hr == S_OK )
            return S_OK;

        return mDecl->FindObject( name, decl );
    }
};


void InitDebug()
{
    int f = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
    f |= _CRTDBG_LEAK_CHECK_DF;     // should always use in debug build
    f |= _CRTDBG_CHECK_ALWAYS_DF;   // check on free AND alloc
    _CrtSetDbgFlag( f );

    //_CrtSetAllocHook( LocalMemAllocHook );
    //SetLocalMemWorkingSetLimit( 550 );
}


class Static
{
public:
    int y;

    static int x;
};

class Static2
{
public:
    int z;
};

class Static3 : public Static
{
public:
    int y2;
};

int Static::x = 10;


struct Options
{
    const wchar_t*  TestFile;
    const wchar_t*  DataFile;
    const wchar_t*  ProgFile;
    uint32_t        Rva;
    bool            PrintDText;
    bool            Verbose;
    bool            SelfTest;
    bool            DisableAssignment;
    bool            TempAssignment;

    static bool ParseOptions( int argc, wchar_t* argv[], Options& options )
    {
        for ( int i = 1; i < argc; i++ )
        {
            if ( _wcsicmp( argv[i], L"-data" ) == 0 )
            {
                i++;
                if ( i >= argc )
                    return false;

                options.DataFile = argv[i];
            }
            else if ( _wcsicmp( argv[i], L"-test" ) == 0 )
            {
                i++;
                if ( i >= argc )
                    return false;

                options.TestFile = argv[i];
            }
            else if ( _wcsicmp( argv[i], L"-prog" ) == 0 )
            {
                i++;
                if ( i >= argc )
                    return false;

                options.ProgFile = argv[i];
                i++;
                if ( i >= argc )
                    return false;

                if ( (argv[i][0] == L'0') && (argv[i][1] == L'x') )
                    options.Rva = wcstoul( argv[i] + 2, NULL, 16 );
                else
                    options.Rva = wcstoul( argv[i], NULL, 10 );
            }
            else if ( _wcsicmp( argv[i], L"-dtext" ) == 0 )
            {
                options.PrintDText = true;
            }
            else if ( _wcsicmp( argv[i], L"-verbose" ) == 0 )
            {
                options.Verbose = true;
            }
            else if ( _wcsicmp( argv[i], L"-self" ) == 0 )
            {
                options.SelfTest = true;
            }
            else if ( _wcsicmp( argv[i], L"-noassign" ) == 0 )
            {
                options.DisableAssignment = true;
            }
            else if ( _wcsicmp( argv[i], L"-tempassign" ) == 0 )
            {
                options.TempAssignment = true;
            }
        }

        if ( (options.DataFile == NULL) && (options.TestFile == NULL) && (options.ProgFile == NULL) )
            return false;

        return true;
    }
};

//#include <limits>
//#include <complex>


int _tmain(int argc, _TCHAR* argv[])
{
    Static::x = 3;
    int a = (1 < 2) ? 99 : 100;

    int*    pa = &a;
    void*   pva = &a;


    InitDebug();

    Options options = { 0 };

    MagoEE::Init();
    atexit( &MagoEE::Uninit );

    TestReal10();

    if ( !Options::ParseOptions( argc, argv, options ) )
        return 1;

    gAppSettings.SelfTest = options.SelfTest;
    gAppSettings.PromoteTypedValue = true;
    gAppSettings.AllowAssignment = !options.DisableAssignment;
    gAppSettings.TempAssignment = options.TempAssignment;

    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    {
        HRESULT hr = S_OK;
        CComPtr<ISAXXMLReader>  reader;
        //CComPtr<ISAXContentHandler> handler;
        CComPtr<BuildingContentHandler> handler;
        CComPtr<ISAXErrorHandler> errorHandler;
        RefPtr<ITypeEnv>    typeEnv;
        IScope*             scope = NULL;
        IValueEnv*          valueEnv = NULL;

        unique_ptr<TopScope>          topScope;
        unique_ptr<DataEnv>           dataEnv;
        unique_ptr<ProgramValueEnv>   progValEnv;


        errorHandler = new SAXErrorHandler();

        hr = reader.CoCreateInstance( CLSID_SAXXMLReader60 );
        if ( FAILED( hr ) )
            return 1;

        hr = MagoEE::MakeTypeEnv( 4, typeEnv.Ref() );
        if ( FAILED( hr ) )
            return 1;

        if ( options.DataFile != NULL )
        {
            DataFactory factory;

            handler = new BuildingContentHandler( &factory );
        
            hr = reader->putContentHandler( handler );
            hr = reader->putErrorHandler( errorHandler );
            hr = reader->parseURL( options.DataFile );

            if ( FAILED( hr ) )
                return 1;

            // 1. Bind Types
            // 2. Layout Fields - at this point types are closed and all their sizes are known
            // 3. Layout Vars
            // 4. Write Data

            RefPtr<DeclDataElement> declRoot = dynamic_cast<DeclDataElement*>( handler->GetRoot() );

            if ( options.Verbose )
                declRoot->PrintElement();

            topScope.reset( new TopScope( declRoot.Get(), typeEnv ) );
            scope = topScope.get();

            dataEnv.reset( new DataEnv( 1024 ) );
            valueEnv = dataEnv.get();

            declRoot->BindTypes( typeEnv, scope );
            declRoot->LayoutFields( NULL );
            declRoot->LayoutVars( dataEnv.get() );
            declRoot->WriteData( dataEnv->GetBuffer(), dataEnv->GetBufferLimit() );

            if ( options.Verbose )
            {
                printf( "\n-----------------\n" );
                declRoot->PrintElement();
                printf( "\n-----------------\n" );

                uint8_t*    buf = dataEnv->GetBuffer();
                for ( int i = 0; i < 176; i++ )
                {
                    printf( "%02x ", buf[i] );
                }
                printf( "\n" );
            }
        }
        else if ( options.ProgFile != NULL )
        {
            progValEnv.reset( new ProgramValueEnv( options.ProgFile, options.Rva, typeEnv ) );
            valueEnv = progValEnv.get();
            scope = progValEnv.get();

            hr = progValEnv->StartProgram();
            if ( FAILED( hr ) )
                return 1;
        }
        else
        {
            printf( "No data to work with. Specify a data file or program file and RVA.\n" );
            return 1;
        }


        // changing the content handler for the reader (we're keeping the reader)
        // deletes the root element, so make sure we keep the root element here in a RefPtr
        // so it stays alive between the two content handlers

        TestFactory testFactory;
        handler = new BuildingContentHandler( &testFactory );
        hr = reader->putContentHandler( handler );
        hr = reader->putErrorHandler( errorHandler );
        hr = reader->parseURL( options.TestFile );
        if ( FAILED( hr ) )
            return 1;

        TestElement*    testRoot = (TestElement*) handler->GetRoot();
        std::wostringstream dtext;
        std::shared_ptr<DataObj>  v;

        if ( options.Verbose )
            testRoot->PrintElement();

        gAppSettings.TestEvalDataEnv = dataEnv.get();       // can be NULL
        gAppSettings.TestElemFactory = &testFactory;

        try
        {
            testRoot->Bind( typeEnv, scope, valueEnv );
            v = testRoot->Evaluate( typeEnv, scope, valueEnv );
            testRoot->ToDText( dtext );
        }
        catch ( const wchar_t* msg )
        {
            printf( "%ls\n", msg );
            return 1;
        }
        catch ( const std::wstring& msg )
        {
            printf( "%ls\n", msg.c_str() );
            return 1;
        }

        if ( options.PrintDText )
            std::wcout << dtext.str() << std::endl;

        if ( v.get() != NULL )
            std::wcout << v->ToString() << std::endl;
    }

    CoUninitialize();
    return 0;
}
