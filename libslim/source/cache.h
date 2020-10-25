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

#ifndef __SLIM_CACHE_H__
#define __SLIM_CACHE_H__
#include "ff.h"

/**
 * This option defines how the cache will be implemented
 * 
 * 0 - Cache is disabled
 * 1 - Cache in heap memory is used, with working buffer in heap memory
 * 2 - Cache in heap memory is used, with working buffer in .bss
 * 
 */
#define SLIM_USE_CACHE 1

/**
 * This option defines the default cache size in number of sectors
 */
#define SLIM_CACHE_SIZE 64

/**
 * This option defines whether or not to use DMA to store sectors to the cache
 *
 * 0 - Uses a CPU memcpy to store sectors
 * 1 - Uses DMA to store sectors
 * 2 - Uses NDMA to store sectors, on DSi, using CPU memcpy otherwise.
 */
#define SLIM_CACHE_STORE_CPY 0

/**
 * This option configures how to read sectors
 * 
 * 0 - Sectors are read 1 by 1 from the SD card
 * 1 - Sectors are read in chunks, greedily from SD card
 */
#define SLIM_CHUNKED_READS 0

/**
 * This option configures the number of sectors prefetched 
 * on single sector reads. 
 * 
 * This increases WRAM usage by a factor of SLIM_PREFETCH_AMOUNT * 512
 * 
 * 0 - Single sector reads read exactly one sector on a single sector read
 * > 1 - Single sector reads trigger a prefetch of SLIM_PREFETCH_AMOUNT extra sectors into the cache
 */
#define SLIM_PREFETCH_AMOUNT 0

/**
 * This configures the max number of sectors fetched from the SD card per chunk.
 * If this is 0, then defaults to (sizeof(BITMAP_PRIMITIVE) 
 * 
 * Must be 1 < SLIM_SECTORS_PER_CHUNK <= (sizeof(BITMAP_PRIMITIVE) * CHAR_BIT) 
 */ 
#define SLIM_SECTORS_PER_CHUNK 0

/**
 * **YOU SHOULD NOT NEED TO CHANGE THIS OPTION**
 * 
 * Specifies the primitive to use as a bitmap. 
 * Changes the size of the chunk used.
 */
#define BITMAP_PRIMITIVE BYTE

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#if !SLIM_SECTORS_PER_CHUNK
#define SECTORS_PER_CHUNK MAX_SECTORS_PER_CHUNK
#else
#define SECTORS_PER_CHUNK MAX(1, MIN(SLIM_SECTORS_PER_CHUNK, MAX_SECTORS_PER_CHUNK))
#endif

#if SLIM_CHUNKED_READS && (SECTORS_PER_CHUNK < SLIM_PREFETCH_AMOUNT)
#error "Not enough scratch space will be allocated to support the specified prefetch amount."
#endif

#define MAX_SECTORS_PER_CHUNK (sizeof(BITMAP_PRIMITIVE) * CHAR_BIT)

#if SLIM_USE_CACHE && FF_MAX_SS != FF_MIN_SS
    #error "Cache can only be used for fixed sector size."
#endif

#if SLIM_USE_CACHE
typedef struct cache_s CACHE;

/**
 * Initializes the cache with the specified cache and sector size.
 * 
 * On the default implementation, there is only a single cache instance that 
 * is supported. Hence, this method should be idempotent. The default cache uses
 * the GCLOCK eviction algorithm to approximate LFRU without a lot of overhead.  
 * 
 * On success, a valid pointer to a CACHE instance will be returned.
 * 
 * Once initialized, the cache can not be reinitialized, and on subsequent runs will 
 * return the same cache instance.
 * 
 * If cache_init is ever called explicitly with 0, the cache will be disabled,
 * and subsequent calls will return NULL.
 */
CACHE *cache_init(UINT cacheSize);

/**
 * Reads a full sector for the specified drive into dst if it exists.
 *
 * If it is not cached, returns false, and dst is not modified.
 * Otherwise, if the read is successful, returns true.
 * 
 * Preconditions:
 *  - cache is initialized
 *  - dst is not necessarily word aligned.
 * Postconditions: 
 *  - If the sector exists, FF_MAX_SS bytes will be written to dst.
 */
BOOL cache_load_sector(CACHE *cache, BYTE drv, LBA_t sector, BYTE *dst);

/**
 * Caches a full sector for the specified drive.
 * Previous cached data will be cleared before writing.
 * 
 * Required is an input weight that determines how long the
 * sector will stay cached. For sectors with high access times
 * use a higher weight.
 * 
 * Preconditions:
 *  - src is readable for exactly FF_MAX_SS bytes
 *    if this is not the case, bad things will happen.
 *
 *  - if SLIM_DMA_CACHE_STORE is defined (default), src is cache (32-byte)-aligned
 *    and in EWRAM (DMA readable)
 */ 
void cache_store_sector(CACHE *cache, BYTE drv, LBA_t sector, const BYTE *src, BYTE weight);

/**
 * Invalidates the specified sector 
 * 
 * Returns true if the sector was previously cached and is now invalidated, false otherwise.
 */ 
BOOL cache_invalidate_sector(CACHE *cache, BYTE drv, LBA_t sector);

/**
 * Gets the existence of up to count < sizeof(BITMAP_PRIMITIVE) * CHAR_BIT 
 * consecutive sectors starting from sector as a bitmap.
 * 
 * For example, if sectors 4, 5, and 6 were cached, the query was 
 * considered for sector = 3 and count = 8, the returned bitmap will be
 * 0x000E (1110 in the first nibble is set.)
 * 
 * An unset bit does not necessarily mean that the sector is uncached.
 * That may be the case, or it may be the case that it was not within
 * the alloted count.
 */ 
BITMAP_PRIMITIVE cache_get_existence_bitmap(CACHE *cache, BYTE drv, LBA_t sector, BYTE count);
#endif
#endif