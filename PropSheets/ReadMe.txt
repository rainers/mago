Third-Party Libraries
=====================

Mago Debugger depends on several third-party libraries. When built with Visual Studio 2010 or
later, the various projects that make up Mago look up the paths to these libraries' include 
and library files using a property sheet MagoDbg_properties.props in the PropSheets folder. 

A template is provided for this property sheet. To get Mago ready to build, first copy the 
template to a file with the same name, but without the "Template_" prefix. This is now the 
property sheet that Mago expects.

The template use several environment variables:

Name                    Value (a path to)
----                    -----------------
STDINT_INC_PATH         stdint.h and inttypes.h (not needed for VS 2013+)
VS_SDK_PATH             Visual Studio 2008 SDK (or later SDK)
BOOST_1_40_0_PATH       Boost 1.40.0 (only needed for some tests)
CPPTEST_1_1_0_SRC_PATH  CppTest 1.1.0 source code (only needed for some tests)
CPPTEST_1_1_0_LIB_PATH  CppTest 1.1.0 library

You should also have the DIA SDK installed with Visual Studio.

Mago is ready to build after one of these steps:

1. Define environment variables that match the macros above. Then run msbuild or Visual Studio 
   from the command line where the environment variables are defined.
2. Replace the macro references with their values in MagoDbg_properties.props.
