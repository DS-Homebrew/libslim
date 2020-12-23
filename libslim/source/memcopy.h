
#ifndef __SLIM_MEMCOPY_H__
#define __SLIM_MEMCOPY_H__

#include <string.h>
#include <tonccpy.h>

#define MEMCOPY(dst, src, sz)   tonccpy(dst, src, sz)
#define MEMSET(dst, val, sz)    memset(dst, val, sz)
#define MEMCLR(dst, sz)         MEMSET(dst, 0, sz)

#endif