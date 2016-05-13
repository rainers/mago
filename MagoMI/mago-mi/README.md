Mago debugger MI commandline interface
======================================

Mago is Windows debug engine for D programming language. Used in VisualD MS Visual Studio plugin.

It's the best D debugger for Windows.

GOAL of Mago-MI project: make mago debugger commandline compatible with gdb mi, lldb-mi.

To be used with D language IDEs which support GDB MI interface.

I'm planning to use it for DlangIDE.


[GDB MI interface documentation](https://sourceware.org/gdb/onlinedocs/gdb/GDB_002fMI.html)


Command line parameters
=======================
*mago-mi --help* shows command line parameters help information.



	This is Mago debugger command line interface. Usage:
	    mago-mi [options] [executable-file]
	    mago-mi [options] --args executable-file [inferior-arguments ...]
	
	Selection of debuggee and its files:
	
	  --args           Arguments after executable-file are passed to inferior
	  --exec=EXECFILE  Use EXECFILE as the executable.
	
	Output and user interface control:
	
	  --interpreter=mi2 Turns on GDB MI interface mode
	
	Operating modes:
	
	  --help           Print this message and then exit
	  --version        Print version information and then exit
	  -v               Verbose output
	
	Other options:
	
	  --cd=DIR         Change current directory to DIR.



Supported subset of GDB MI commands
===================================

