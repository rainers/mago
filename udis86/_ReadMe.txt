The version is 1.7.2 + additional commits up to commit 
56ff6c87c11de0ffa725b14339004820556e343d from https://github.com/vmt/udis86

The original distribution of udis86 comes with files for building on UNIX-like 
systems and test suite. I left those files out and built my own Visual Studio projects.
The source depends on a file config.h which is generated from the original build 
files based on the file config.h.in. I copied config.h.in to config.h and changed it
as needed.

You can find the project page online here:
http://sourceforge.net/projects/udis86
