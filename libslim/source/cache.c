/*
Copyright (c) 2020, chyyran
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL CHYYRAN BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "cache.h"
#include <ctype.h>
#include "tonccpy.h"

#include "ff.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <nds/dma.h>
#include <nds/arm9/cache.h>

#ifdef DEBUG_NOGBA
#include <nds/debug.h>
#endif

#define CHECK_BIT(v, p) (((v) >> (p)) & 1)
#define VALID(v) CHECK_BIT(v, 0)
#define REFERENCED(v) CHECK_BIT(v, 1)
#define BIT_SET(n)   (1 << (n))

#define CACHE_LINE_SIZE 32

typedef struct cache_s
{
    BYTE data[FF_MAX_SS] __attribute__((aligned(32)));

    WORD __padding;

    // Bit 0 is validity. If set to 1, then read will succeed.
    // Bit 1 is reference. If set to 0, then on the next round robin, will be evicted.
    BYTE status;
    BYTE pdrv;
    LBA_t sector;
} __attribute__((aligned(4))) CACHE;

static int _evictCounter = 0;

static CACHE *__cache = NULL;
static UINT _cacheSize = 0;
static BOOL _cacheDisabled = false;

CACHE *cache_init(UINT cacheSize)
{
    // Cache was previously disabled
    if (_cacheDisabled)
    {
        return NULL;
    }

    // Cache was previously initialized
    if (__cache)
    {
        return __cache;
    }

    _cacheSize = cacheSize;

    // Disable cache
    if (cacheSize == 0)
    {
        _cacheDisabled = true;
        return NULL;
    }

    CACHE *allocedCache = ff_memalloc(sizeof(CACHE) * cacheSize);
    if (allocedCache == NULL)
    {
        return NULL;
    }
    else
    {
        __cache = allocedCache;
    }

    toncset(__cache, 0, sizeof(CACHE) * cacheSize);
    return __cache;
}

// Finds the block with the given drv and sector
// Returns -1 if none can be found.
static inline int cache_find_valid_block(CACHE *cache, BYTE drv, LBA_t sector)
{
    if (!cache)
        return -1;

    for (int i = 0; i < _cacheSize; i++)
    {
        if (VALID(cache[i].status) && cache[i].pdrv == drv && cache[i].sector == sector)
        {
            return i;
        }
    }
    return -1;
}

BOOL cache_load_sector(CACHE *cache, BYTE drv, LBA_t sector, BYTE *dst)
{
    if (!cache)
        return false;

    int i = -1;
    if ((i = cache_find_valid_block(cache, drv, sector)) == -1)
    {
        return false;
    }

#ifdef DEBUG_NOGBA
    char block[256];
    sprintf(block, "HIT: d: %d, s: %ld, b: %d", drv, sector, i);
    nocashMessage(block);
#endif
    // Set referenced bit
    cache[i].status |= 0b10;

    // Copy cache
    tonccpy(dst, &cache[i].data, FF_MAX_SS);
    return true;
}

void cache_store_sector(CACHE *cache, BYTE drv, LBA_t sector, const BYTE *src)
{
    if (!cache || _cacheSize == 0)
        return;

    int free_block = -1;

    while (free_block < 0)
    {
        if (!REFERENCED(cache[_evictCounter].status))
        {
            free_block = _evictCounter;
        }
        else
        {
            // Clear reference bit
            cache[_evictCounter].status &= ~BIT_SET(1);
        }
        _evictCounter = ((_evictCounter + 1) % _cacheSize);
    }

#ifdef DEBUG_NOGBA
    char block[256];
    sprintf(block, "S: d: %d, s: %ld, fb: %d, cs: %u", drv, sector, free_block, _cacheSize);
    nocashMessage(block);
#endif

    // Set valid and unreferenced
    cache[free_block].status = (BYTE)0b01;
    cache[free_block].pdrv = drv;
    cache[free_block].sector = sector;

#if SLIM_DMA_CACHE_STORE

    DC_FlushRange(src, FF_MAX_SS);

    // Perform safe cache flush
    uint32_t dst = (uint32_t)&cache[free_block].data;
    if (dst % CACHE_LINE_SIZE)
        DC_FlushRange((void *)(dst), 1);
    if ((dst + FF_MAX_SS) % CACHE_LINE_SIZE)
        DC_FlushRange((void *)(dst + FF_MAX_SS), 1);

    dmaCopyWords(0, src, &cache[free_block].data, FF_MAX_SS);
    DC_InvalidateRange(&cache[free_block].data, FF_MAX_SS);

#else
    tonccpy(&cache[free_block].data, src, FF_MAX_SS);
#endif
}

BOOL cache_invalidate_sector(CACHE *cache, BYTE drv, LBA_t sector)
{
    if (!cache)
        return false;

    int i = -1;
    if ((i = cache_find_valid_block(cache, drv, sector)) != -1)
    {
        cache[i].status = (BYTE)0b00;
        return true;
    }

    return false;
}

void cache_invalidate_all(CACHE *cache, BYTE drv)
{
    if (!cache)
        return;
    for (UINT i = 0; i < _cacheSize; i++)
    {
        cache[i].status = (BYTE)00;
    }
}

DWORD cache_get_existence_bitmap(CACHE *cache, BYTE drv, LBA_t sector, BYTE count)
{
    if (!cache)
        return 0;
    if (count > sizeof(DWORD) * CHAR_BIT)
        return 0;

    DWORD bitmap = 0;
    for (int i = 0; i < _cacheSize; i++)
    {
        if (VALID(cache[i].status) && cache[i].pdrv == drv)
        {
            int cachedSector = cache[i].sector;
            int relativeSector = cachedSector - sector;
            if (relativeSector < 0 || relativeSector >= count)
                continue;
            bitmap &= BIT_SET(relativeSector);
        }
    }
    return bitmap;
}