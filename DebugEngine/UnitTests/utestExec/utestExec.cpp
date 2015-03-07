/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

// testExec.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "StartStopSuite.h"
#include "EventSuite.h"
#include "StepOneThreadSuite.h"

using namespace std;
using namespace boost;


enum OutputType
{
    Out_None,
    Out_Text,
    Out_Compiler,
    Out_Html,
};

struct Options
{
    OutputType                      OutType;
    std::shared_ptr<Test::Output> Out;
    wstring                         Filename;
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

bool ParseCommandLine( int argc, wchar_t* argv[], Options& options )
{
    options.OutType = Out_None;

    for ( int i = 1; i < argc; i++ )
    {
        if ( _wcsicmp( argv[i], L"-textOut" ) == 0 )
        {
            if ( (i + 1) >= argc )
                return false;

            Test::TextOutput::Mode  mode;

            i++;
            if ( _wcsicmp( argv[i], L"terse" ) == 0 )
                mode = Test::TextOutput::Terse;
            else if ( _wcsicmp( argv[i], L"verbose" ) == 0 )
                mode = Test::TextOutput::Verbose;
            else
                return false;

            options.OutType = Out_Text;
            options.Out.reset( new Test::TextOutput( mode ) );
        }
        else if ( _wcsicmp( argv[i], L"-compilerOut" ) == 0 )
        {
            if ( (i + 1) >= argc )
                return false;

            Test::CompilerOutput::Format    format;

            i++;
            if ( _wcsicmp( argv[i], L"generic" ) == 0 )
                format = Test::CompilerOutput::Generic;
            else if ( _wcsicmp( argv[i], L"bcc" ) == 0 )
                format = Test::CompilerOutput::BCC;
            else if ( _wcsicmp( argv[i], L"gcc" ) == 0 )
                format = Test::CompilerOutput::GCC;
            else if ( _wcsicmp( argv[i], L"msvc" ) == 0 )
                format = Test::CompilerOutput::MSVC;
            else
                return false;

            options.OutType = Out_Compiler;
            options.Out.reset( new Test::CompilerOutput( format ) );
        }
        else if ( _wcsicmp( argv[i], L"-htmlOut" ) == 0 )
        {
            options.OutType = Out_Html;
            options.Out.reset( new Test::HtmlOutput() );
        }
        else if ( _wcsicmp( argv[i], L"-filename" ) == 0 )
        {
            if ( (i + 1) >= argc )
                return false;

            i++;
            options.Filename = argv[i];
        }
        else
            return false;
    }

    if ( options.OutType == Out_None )
    {
        options.Out.reset( new Test::TextOutput( Test::TextOutput::Verbose ) );
    }

    _ASSERT( options.Out.get() != NULL );

    return true;
}

void GenerateHtml( Options& options )
{
    if ( options.Filename.empty() )
    {
        ((Test::HtmlOutput*) options.Out.get())->generate( cout );
    }
    else
    {
        ofstream    file( options.Filename.c_str() );
        ((Test::HtmlOutput*) options.Out.get())->generate( file );
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    Options options;

    InitDebug();

    if ( !ParseCommandLine( argc, argv, options ) )
        return EXIT_FAILURE;

    Test::Suite         comboSuite;

    comboSuite.add( auto_ptr<Test::Suite>( new StartStopSuite() ) );
    comboSuite.add( auto_ptr<Test::Suite>( new EventSuite() ) );
    comboSuite.add( auto_ptr<Test::Suite>( new StepOneThreadSuite() ) );

    bool    passed = comboSuite.run( *options.Out.get() );

    if ( options.OutType == Out_Html )
        GenerateHtml( options );

    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
