/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

// wrapper to add file into project that is generated into the intermediate folder

#ifdef MAGOREMOTE
    #include "MagoRemoteCmd_s.c"
#else
    #include "MagoRemoteCmd_c.c"
#endif