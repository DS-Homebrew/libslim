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

#define SLIM_USE_CACHE 1
/**
/ This option defines how the cache will be implemented
/ 
/ 0 - Cache is disabled
/ 1 - Cache on heap memory is used
/ 
*/

#define SLIM_CACHE_SIZE 64
/* This option defines the default cache size in number of sectors
*/

#define SLIM_DMA_CACHE_STORE 1
/**
/ This option defines whether or not to use DMA to store sectors to the cache
/ 
/ 0 - Uses a CPU memcpy to store sectors
/ 1 - Uses DMA to store sectors
/ 
*/

#if SLIM_USE_CACHE && FF_MAX_SS != FF_MIN_SS
    #error "Cache can only be used for fixed sector size."
#endif

#if SLIM_USE_CACHE
typedef struct cache_s CACHE;

/**
 * Initializes the cache with the specified cache and sector size.
 * 
 * On the default implementation, there is only a single cache instance that 
 * is supported. Hence, this method should be idempotent.
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
 * Preconditions:
 *  - src is readable for exactly FF_MAX_SS bytes
 *    if this is not the case, bad things will happen.
 *
 *  - if SLIM_DMA_CACHE_STORE is defined (default), src is cache (32-byte)-aligned
 *    and in EWRAM (DMA readable)
 */ 
void cache_store_sector(CACHE *cache, BYTE drv, LBA_t sector, const BYTE *src);

/**
 * Invalidates the specified sector 
 * 
 * Returns true if the sector was previously cached and is now invalidated, false otherwise.
 */ 
BOOL cache_invalidate_sector(CACHE *cache, BYTE drv, LBA_t sector);

/**
 * Invalidates all sectors cached for the given drive.
 */ 
void cache_invalidate_all(CACHE *cache, BYTE drv);

BOOL cache_exists(CACHE *cache, BYTE drv, LBA_t sector);
#endif
#endif