#include "cache.h"
#include <ctype.h>
#include "tonccpy.h"

#include "ff.h"

#define CHECK_BIT(v, p) (((v) >> (p)) & 1)
#define VALID(v) CHECK_BIT(v, 0)
#define REFERENCED(v) CHECK_BIT(v, 1)

typedef struct cache_s
{
    // Bit 0 is validity. If set to 1, then read will succeed.
    // Bit 1 is reference. If set to 0, then on the next round robin, will be evicted.
    BYTE status;
    BYTE pdrv;
    LBA_t sector;
    BYTE data[FF_MAX_SS];
} __attribute__((aligned(4))) CACHE;

static UINT _evictCounter = 0;

static CACHE *__cache = NULL;
static UINT _cacheSize = 0;
static BOOL _cacheDisabled = false;

CACHE *cache_init(UINT cacheSize)
{
    // Cache was previously disabled
    if (_cacheDisabled) {
        return NULL;
    }
    
    // Cache wa previously initialized
    if (__cache) {
        return __cache;
    }

    _cacheSize = cacheSize;

    // Disable cache
    if (cacheSize == 0) {
        _cacheDisabled = true;
        return NULL;
    }

    CACHE *allocedCache = ff_memalloc(sizeof(CACHE) * cacheSize);
    if (allocedCache == NULL) {
        return NULL;
    } else {
        __cache = allocedCache;
    }
  
    toncset(__cache, 0, sizeof(CACHE) * cacheSize);
    return __cache;
}

// Finds the block with the given drv and sector
// Returns -1 if none can be found.
int cache_find_valid_block(CACHE *cache, BYTE drv, LBA_t sector)
{
    if (!cache)
        return -1;

    for (int i = 0; i < _cacheSize; i++) {
        if (VALID(cache[i].status) && cache[i].pdrv == drv && cache[i].sector == sector) {
            return i;
        }
    }
    return -1;
}

BOOL cache_read_sector(CACHE *cache, BYTE drv, LBA_t sector, BYTE *dst)
{
    if (!cache)
        return false;
        
    int i = -1;
    if ((i = cache_find_valid_block(cache, drv, sector)) == -1) {
        return false;
    }

    // Set referenced bit
    cache[i].status |= 0b10;

    // Copy cache
    tonccpy(dst, cache[i].data, FF_MAX_SS);
    return true;
}

void cache_write_sector(CACHE *cache, BYTE drv, LBA_t sector, const BYTE *src)
{   
    if (!cache)
        return;

    int free_block = -1;

    while (free_block < 0) {
        if (!REFERENCED(cache[_evictCounter].status)) {
            free_block = _evictCounter;
        } else {
            // Clear reference bit
            cache[_evictCounter].status &= ~(1U << 1);
        }
        _evictCounter = (_evictCounter % _cacheSize);
    }

    // Set valid, but not referenced.
    // Referenced will be set on first read
    cache[free_block].status = 0b01;
    cache[free_block].pdrv = drv;
    cache[free_block].sector = sector;
    tonccpy(cache[free_block].data, src, FF_MAX_SS);
}

BOOL cache_invalidate_sector(CACHE *cache, BYTE drv, LBA_t sector)
{
    if (!cache)
        return false;

    int i = -1;
    if ((i = cache_find_valid_block(cache, drv, sector)) != -1) {
        cache[i].status = 0;
        return true;
    }

    return false;
}