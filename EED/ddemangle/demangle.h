#include <stdlib.h>

#define XNEWVEC(T, n)       malloc(sizeof(T)*(n))
#define XRESIZEVEC(T, p, n) realloc(p, sizeof(T)*(n))
#define XDELETEVEC(p)       free(p)

#define DMGL_NO_OPTS	 0		/* For readability... */
#define DMGL_PARAMS	 (1 << 0)	/* Include function args */
#define DMGL_ANSI	 (1 << 1)	/* Include const, volatile, etc */
#define DMGL_JAVA	 (1 << 2)	/* Demangle as Java rather than C++. */
#define DMGL_VERBOSE	 (1 << 3)	/* Include implementation details.  */
#define DMGL_TYPES	 (1 << 4)	/* Also try to demangle type encodings.  */
