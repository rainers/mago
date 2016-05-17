Mago debugger GDB MI compatible command line interface project
==============================================================


Mago is Windows debug engine for D programming language. Used in VisualD MS Visual Studio plugin.

It's the best D debugger for Windows.

GOAL of Mago-MI project: make mago debugger commandline compatible with gdb mi, lldb-mi.

To be used with D language IDEs which support GDB MI interface.

Tested with DlangIDE on Windows.


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
	
	  --interpreter=mi2 Turn on GDB MI interface mode
	
	Operating modes:
	
	  --help           Print this message and then exit
	  --version        Print version information and then exit
	  -v               Verbose output
	  --silent         Don't print version info on startup
	
	Other options:
	
	  --cd=DIR         Change current directory to DIR.
	  --log-file=FILE  Set log file for debugger internal logging.
	  --log-LEVEL=FATAL|ERROR|WARN|INFO|DEBUG|TRACE Set log level for debugger internal logging.



Supported subset of GDB commands
================================

	mago-mi: GDB and GDB-MI compatible interfaces for MAGO debugger.
	
	run                     - start program execution
	continue                - continue program execution
	interrupt               - interrupt program execution
	next                    - step over
	nexti                   - step over by instruction
	step                    - step into
	stepi                   - step into by instruction
	finish                  - step out of current function
	break                   - add breakpoint
	info break              - list breakpoints
	enable N                - enable breakpoint
	disable N               - disable breakpoint
	delete N                - delete breakpoint
	info thread             - thread list with properties
	info threads            - list of thread ids
	backtrace               - list thread stack frames


Supported subset of GDB MI commands
===================================

	mago-mi: GDB and GDB-MI compatible interfaces for MAGO debugger.
	
	-exec-run [--start]     - start program execution
	-exec-continue          - continue program execution
	-exec-interrupt         - interrupt program which is being running
	-exec-next              - step over
	-exec-next-instruction  - step over by instruction
	-exec-step              - step into
	-exec-step-instruction  - step into by instruction
	-exec-finish            - exit function
	-break-insert           - add breakpoint
	-break-list             - list breakpoints
	-break-enable           - enable breakpoint
	-break-disable          - disable breakpoint
	-break-delete           - delete breakpoint
	-thread-info            - thread list with properties
	-thread-list-ids        - list of thread ids
	-stack-list-frames      - list thread stack frames
	-stack-list-variables   - list stack frame variables
	-gdb-exit               - quit debugger
	
	Type quit to exit.

