Allowing symbol names longer than 255 characters in CodeView
------------------------------------------------------------

Important: This is a non-standard extension.

Because the CodeView debug information format limits the names of symbols to 
255 characters, the D compiler DMD uses a new encoding that allows for names 
that are longer than the official limit.

The following is the special encoding is used for non-public symbols.

1. If the name is less than or equal to 255 characters, then the standard 
   encoding is used, where there is a 1 byte length prefix.
2. Otherwise, the bytes of the name are laid out as follows:
   a. 1 byte: 255
   b. 1 byte: 0
   c. 2 bytes: the real name length (little endian)
   d. * bytes: the real name characters
