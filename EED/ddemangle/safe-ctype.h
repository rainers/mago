/* <ctype.h> replacement macros.

   To avoid conflicts, this header defines the isxxx functions in upper
   case, e.g. ISALPHA not isalpha.  */

#ifndef SAFE_CTYPE_H
#define SAFE_CTYPE_H

#include <ctype.h>

#define ISALPHA   isalpha
#define ISALNUM   isalnum
#define ISBLANK   isblank
#define ISCNTRL   iscntrl
#define ISDIGIT   isdigit
#define ISGRAPH   isgraph
#define ISLOWER   islower
#define ISPRINT   isprint
#define ISPUNCT   ispunct
#define ISSPACE   isspace
#define ISUPPER   isupper
#define ISXDIGIT  isxdigit

#endif /* SAFE_CTYPE_H */
