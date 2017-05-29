#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <utypes.h>

#define MAJOR    0
#define MINOR    0
#define PATCH    0
#define RELEASE  1
#define VERSION() (RELEASE | (PATCH << 8) | (MINOR << 16) | (MAJOR << 24))
#define VERSION_STR() ("V"##MAJOR##"."##MINOR##"."##PATCH##"."##RELEASE)

#define MALLOC(size) malloc(size)
#define FREE(p) free(p)

//#ifndef bool
//#define bool unsigned int
//#endif

//#ifndef true
//#define true (!!1)
//#endif

//#ifndef false
//#define false (!!0)
//#endif

#define ASSERT(x) do { if (!(x)) exit(0); } while (0)

#endif
