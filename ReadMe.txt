Build with VS 2010 or later:

Use MagoDbg_2010.sln to build with any Visual Studio 2010 or later.

Build with VS 2008 or earlier:

I've built the project MagoDbg.sln with Visual Studio 2008 and 2010 and the VS 2008 SDK; 
and tested it with 2005 and 2008. I believe it'll build and run with any 
version of VS and its SDK from 2005 to 2010.

You can build the solutions and projects in Visual Studio. To build from the 
Visual Studio 2008 Command Prompt instead, you can follow the following steps:

1. Add to the environment variables INCLUDE and LIB the dependencies listed above.
    a. You also need to add the IDL folder for the VS 2008 SDK to INCLUDE.
2. vcbuild /u /platform:win32
    a. This also works for the C# project MagoDELauncher. It ends up running msbuild


Dependencies:

- CPPTest 1.0 (needed for tests only)
- Visual Studio SDK


Installation:

Mago is automatically installed as part of Visual D,
see https://rainers.github.io/visuald/visuald/StartPage.html

Manual Installation of the concord extension in VS:

- Copy MagoNatCC.dll and MagoNatCC.vsdconfig into
  <VS-Installation-Path>\Common7\Packages\Debugger


Manual Installation of the debug engine:

- Register the COM DLL MagoNatDE.dll with regsvr32.

- Import the right EngineMetric reg file from the Install folder for your system and VS combo.
    - At this point you can use the debug engine from a project system, but if you 
      want to use it without one, you'll need the launcher add-in for VS.

- VS2008: Put MagoDELauncher.addin and its DLL into one of the following
    - All Users\Application Data\Microsoft\VisualStudio\9.0\Addins
    - Users\You\Documents\Visual Studio 2008\Addins


Manual Installation of the Concord extension in VS Code (cpptools-1.4.0 or
later needed):

- Copy MagoNatCC.dll, MagoNatCC.vsdconfig and .vsdbg-config.json into
  a common directory

- add a file %USERPROFILE%\.cppvsdbg\extensions\mago.link just containing
  the full path to the directory with MagoNatCC.dll and the other files

- add this setting in the package.json of some loaded extension to allow 
  setting breakpoints in *.d files:
{
    "contributes": {
        "breakpoints": [
          {
            "language": "d"
          }
        ],
    }
}

- the D expression evaluation is then available as part of the native C++ debugger cppvsdbg.


Mago-MI:

mago-mi project is GDB/MI compatible interface for Mago debugger.
Can be used with IDEs like Eclipse/DDT or DlangIDE.

See MagoMI/mago-mi/README.md for details.
