
#ifndef __SLIM_MEMCOPY_H__
#define __SLIM_MEMCOPY_H__

#include <string.h>

#define MEMCOPY(dst, src, sz)   memcpy(dst, src, sz)
#define MEMCLR(dst, sz)         memset(dst, 0, sz)
#endif