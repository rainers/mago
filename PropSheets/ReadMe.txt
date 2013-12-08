Third-Party Libraries
=====================

Mago Debugger depends on several third-party libraries. When built with Visual Studio 2010, the 
various projects that make up Mago look up the paths to these libraries' include and library 
files using two property sheets found in the PropSheets folder. MagoDbg_3rdParty_Debug.props 
contains the information for Debug configurations and MagoDbg_3rdParty_Release.props contains the 
information for Release configurations.

A template is provided for each property sheet. To get Mago ready to build, first copy each 
template to a file with the same name, but without the "Template_" prefix. These are now the 
property sheets that Mago expects.

The templates define several macros:

Name                    Value (a path to)
----                    -----------------
STDINT_INC_PATH         stdint.h and inttypes.h
BOOST_1_40_0_PATH       Boost 1.40.0
VS_2008_SDK_PATH        Visual Studio 2008 SDK
DIA_SDK_PATH            DIA SDK
CPPTEST_1_1_0_SRC_PATH  CppTest 1.1.0 source code
CPPTEST_1_1_0_LIB_PATH  CppTest 1.1.0 library

Mago is ready to build after one of these steps:

1. Define environment variables that match the macros above. Then run msbuild or Visual Studio 
   2010 from the command line where the environment variables are defined.
2. Replace the macro references with their values in MagoDbg_3rdParty_Debug.props and 
   MagoDbg_3rdParty_Release.props.
