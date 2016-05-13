MagoMI fork:

This is fork of Mago debugger which adds support of GDM MI compatible interface.

See MagoMI/mago-mi/README.md for details.

Original project: https://github.com/rainers/mago


Versions:

I've built the project with Visual Studio 2008 and 2010 and the VS 2008 SDK; 
and tested it with 2005 and 2008. I believe it'll build and run with any 
version of VS and its SDK from 2005 to 2010.


Dependencies:

- inttypes.h and stdint.h
- CPPTest 1.0
- VS 2008 SDK
- Boost 1.40


Build:

You can build the solutions and projects in Visual Studio. To build from the 
Visual Studio 2008 Command Prompt instead, you can follow the following steps:

1. Add to the environment variables INCLUDE and LIB the dependencies listed above.
    a. You also need to add the IDL folder for the VS 2008 SDK to INCLUDE.
2. vcbuild /u /platform:win32
    a. This also works for the C# project MagoDELauncher. It ends up running msbuild


Installation:

- Register the COM DLL MagoNatDE.dll with regsvr32.
- Import the right EngineMetric reg file for your system and VS combo.
    - At this point you can use the debug engine from a project system, but if you 
      want to use it without one, you'll need the launcher add-in for VS.

- Put MagoDELauncher.addin and its DLL into one of the following
    - All Users\Application Data\Microsoft\VisualStudio\9.0\Addins
    - Users\You\Documents\Visual Studio 2008\Addins
