/*
Copyright (c) 2007, ChaN 
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

/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "ff.h"
#include "ffvolumes.h"
#include "tonccpy.h"

#include "diskio.h"
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <nds/disc_io.h>
#include <nds/arm9/cache.h>

#include "cache.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define CHECK_BIT(v, n) (((v) >> (n)) & 1)
#define BIT_SET(n) (1 << (n))

#ifdef DEBUG_NOGBA
#include <nds/debug.h>
#endif

#if SLIM_USE_CACHE
static CACHE *__cache;
static BYTE working_buf[FF_MAX_SS] __attribute__((aligned(32)));
#endif
/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */

DSTATUS disk_initialize(BYTE drv)
{
#if SLIM_USE_CACHE
	// Initialize cache if not already
	if (!__cache)
	{
		__cache = cache_init(SLIM_CACHE_SIZE);
	}
#endif

	if (!init_disc_io(drv))
	{
		return STA_NOINIT;
	}

	return get_disc_io(drv)->isInserted() ? 0 : STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status(
	BYTE drv /* Physical drive nmuber (0..) */
)
{
	const DISC_INTERFACE *disc_io = NULL;
	if ((disc_io = get_disc_io(drv)) != NULL)
	{
		return disc_io->isInserted() ? 0 : STA_NOINIT;
	}
	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read_internal(
	BYTE drv,	  /* Physical drive nmuber (0..) */
	BYTE *buff,	  /* Data buffer to store read data */
	LBA_t sector, /* Sector address (LBA) */
	BYTE count	  /* Number of sectors to read (1..255) */
)
{
	const DISC_INTERFACE *disc_io = NULL;
	if ((disc_io = get_disc_io(drv)) != NULL)
	{
		return disc_io->readSectors(sector, count, buff) ? RES_OK : RES_ERROR;
	}
	return RES_PARERR;
}
#define PRINTF_BINARY_PATTERN_INT8 "%c%c%c%c%c%c%c%c"
#define PRINTF_BYTE_TO_BINARY_INT8(i) \
	(((i)&0x80ll) ? '1' : '0'),       \
		(((i)&0x40ll) ? '1' : '0'),   \
		(((i)&0x20ll) ? '1' : '0'),   \
		(((i)&0x10ll) ? '1' : '0'),   \
		(((i)&0x08ll) ? '1' : '0'),   \
		(((i)&0x04ll) ? '1' : '0'),   \
		(((i)&0x02ll) ? '1' : '0'),   \
		(((i)&0x01ll) ? '1' : '0')

#define PRINTF_BINARY_PATTERN_INT16 \
	PRINTF_BINARY_PATTERN_INT8 PRINTF_BINARY_PATTERN_INT8
#define PRINTF_BYTE_TO_BINARY_INT16(i) \
	PRINTF_BYTE_TO_BINARY_INT8((i) >> 8), PRINTF_BYTE_TO_BINARY_INT8(i)
#define PRINTF_BINARY_PATTERN_INT32 \
	PRINTF_BINARY_PATTERN_INT16 PRINTF_BINARY_PATTERN_INT16
#define PRINTF_BYTE_TO_BINARY_INT32(i) \
	PRINTF_BYTE_TO_BINARY_INT16((i) >> 16), PRINTF_BYTE_TO_BINARY_INT16(i)
#define PRINTF_BINARY_PATTERN_INT64 \
	PRINTF_BINARY_PATTERN_INT32 PRINTF_BINARY_PATTERN_INT32
#define PRINTF_BYTE_TO_BINARY_INT64(i) \
	PRINTF_BYTE_TO_BINARY_INT32((i) >> 32), PRINTF_BYTE_TO_BINARY_INT32(i)

BYTE get_disk_lookahead(DWORD bitmap, BYTE currentSector, BYTE maxCount)
{
	if ((bitmap >> currentSector) == 0)
		return maxCount;
	return MIN(maxCount, __builtin_ctzl(bitmap >> currentSector));
}

DRESULT disk_read(
	BYTE drv,		  /* Physical drive nmuber (0..) */
	BYTE *buff,		  /* Data buffer to store read data */
	LBA_t baseSector, /* Sector address (LBA) */
	BYTE count		  /* Number of sectors to read (1..255) */
)
{
	DRESULT res = RES_PARERR;
	BYTE sectorsRemaining = count;

	if (VALID_DISK(drv))
	{
#if !SLIM_USE_CACHE
		res = disk_read_internal(drv, buff, sector, count);
#else

#ifdef DEBUG_NOGBA
		char buf[256];
		// sprintf(buf, "load: %d sectors from %ld, wbuf: %p, tbuf: %p", count, baseSector, working_buf, buff);
		// nocashMessage(buf);
#endif
		// If caching is disabled, there's no reason to read each sector individually..,
		if (!__cache)
		{
			return disk_read_internal(drv, buff, baseSector, count);
		}

		// If we're only loading one sector, no need to engage more complicated searches
		if (count == 1)
		{
			if (cache_load_sector(__cache, drv, baseSector, buff))
			{
				res = RES_OK;
			}
			else
			{
				// This is a single sector read.
				res = disk_read_internal(drv, working_buf, baseSector, 1);
				cache_store_sector(__cache, drv, baseSector, working_buf);
				tonccpy(buff, working_buf, FF_MAX_SS);
			}
			return res;
		}

		// Optimized path for multi-sector reads to minimize SD card requests
		LBA_t sectorOffset = 0;
		while (sectorsRemaining)
		{
			BYTE sectorsToRead = MIN((BYTE)SECTORS_PER_CHUNK, sectorsRemaining);
#ifdef DEBUG_NOGBA
			// Limiting to chunks of 4 to test correctness
			sectorsToRead = MIN(4, sectorsRemaining);
			sprintf(buf, "load: chunk of %d (%d remaining, total %ld/%d) sectors starting %ld", sectorsToRead, sectorsRemaining,
					sectorOffset, count, baseSector + sectorOffset);
			nocashMessage(buf);
#endif
			// Get bitmap of cached sectors
			// bitmap is rooted at sectorOffset
			const DWORD cacheBitmap = cache_get_existence_bitmap(__cache, drv, baseSector + sectorOffset, sectorsToRead);
			DWORD readBitmap = 0;

#ifdef DEBUG_NOGBA
			sprintf(buf, "chunk %ld..=%ld (%ld): " PRINTF_BINARY_PATTERN_INT32,
					sectorOffset, sectorsToRead + sectorOffset, cacheBitmap, PRINTF_BYTE_TO_BINARY_INT32(cacheBitmap));
			nocashMessage(buf);
#endif

			for (LBA_t i = 0; i < sectorsToRead; i++)
			{
				if (!CHECK_BIT(cacheBitmap, i))
				{
					continue;
				}

#ifdef DEBUG_NOGBA
				sprintf(buf, "LC: sO: %ld, i: %ld", sectorOffset, i);
				nocashMessage(buf);
#endif
				// If the bitmap says cached sector exists, but the cached sector is invalid,
				// then something went horribly wrong. We can recover though, by re-reading
				// the sector.
				// The unreferenced sector will eventually be wiped from CLOCK
				if (!cache_load_sector(__cache, drv, baseSector + i + sectorOffset,
									   &buff[(i + sectorOffset) * FF_MAX_SS]))
				{
#ifdef DEBUG_NOGBA
					sprintf(buf, "unexpected cache miss: %ld, o: %ld",
							baseSector + (sectorOffset + i), sectorOffset);
					nocashMessage(buf);
#endif
				}
				else
				{
					readBitmap |= BIT_SET(i);
					res = RES_OK;
				}
			}

			for (LBA_t i = 0; i < sectorsToRead;)
			{
				if (CHECK_BIT(readBitmap, i))
				{
					i += 1;
					continue;
				}

				BYTE lookaheadCount = get_disk_lookahead(readBitmap, i, sectorsToRead - i);

#ifdef DEBUG_NOGBA
				sprintf(buf, "LU: sO: %ld, i: %ld, n: %d", sectorOffset, i, lookaheadCount);
				nocashMessage(buf);
#endif
				res = disk_read_internal(drv, &buff[(i + sectorOffset) * FF_MAX_SS],
										 baseSector + sectorOffset + i, lookaheadCount);

				if (res != RES_OK)
					return res;

				// Flush SD read range
				DC_FlushRange(&buff[(i + sectorOffset) * FF_MAX_SS], lookaheadCount * FF_MAX_SS);

				// Cache read sectors
				for (int j = 0; j < lookaheadCount; j++)
				{
					readBitmap |= BIT_SET((i + j));
					tonccpy(working_buf, &buff[(i + j + sectorOffset) * FF_MAX_SS], FF_MAX_SS);
					cache_store_sector(__cache, drv, baseSector + sectorOffset + i + j, working_buf);
				}

				i += lookaheadCount;
			}

			// If the number of read sectors in the chunk is wrong
			// we messed up.
			if (__builtin_popcountl(readBitmap) != sectorsToRead)
				return RES_ERROR;

			sectorOffset += sectorsToRead;
			sectorsRemaining -= sectorsToRead;
		}
#ifdef DEBUG_NOGBA
		sprintf(buf, "read completed, read %ld sectors", sectorOffset);
		nocashMessage(buf);
#endif
#endif
	}

	return res;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write(
	BYTE drv,		  /* Physical drive nmuber (0..) */
	const BYTE *buff, /* Data to be written */
	LBA_t sector,	  /* Sector address (LBA) */
	BYTE count		  /* Number of sectors to write (1..255) */
)
{
	const DISC_INTERFACE *disc_io = NULL;
	if ((disc_io = get_disc_io(drv)) != NULL)
	{
		DRESULT res = disc_io->writeSectors(sector, count, buff) ? RES_OK : RES_ERROR;
#if SLIM_USE_CACHE
		for (BYTE i = 0; i < count; i++)
		{
			cache_invalidate_sector(__cache, drv, sector + i);
		}
#endif
		return res;
	}
	return RES_PARERR;
}
#endif /* _READONLY */

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl(
	BYTE drv,  /* Physical drive nmuber (0..) */
	BYTE ctrl, /* Control code */
	void *buff /* Buffer to send/receive control data */
)
{
	const DISC_INTERFACE *disc_io = NULL;
	if ((disc_io = get_disc_io(drv)) != NULL)
	{
		return disc_io->clearStatus() ? RES_OK : RES_ERROR;
	}
	return RES_PARERR;
}

#define MAX_HOUR 23
#define MAX_MINUTE 59
#define MAX_SECOND 59

#define MAX_MONTH 11
#define MIN_MONTH 0
#define MAX_DAY 31
#define MIN_DAY 1

DWORD get_fattime()
{
	struct tm timeParts;
	time_t epochTime;

	if (time(&epochTime) == (time_t)-1)
	{
		return 0;
	}
	localtime_r(&epochTime, &timeParts);

	// Check that the values are all in range.
	// If they are not, return 0 (no timestamp)
	if ((timeParts.tm_hour < 0) || (timeParts.tm_hour > MAX_HOUR))
		return 0;
	if ((timeParts.tm_min < 0) || (timeParts.tm_min > MAX_MINUTE))
		return 0;
	if ((timeParts.tm_sec < 0) || (timeParts.tm_sec > MAX_SECOND))
		return 0;
	if ((timeParts.tm_mon < MIN_MONTH) || (timeParts.tm_mon > MAX_MONTH))
		return 0;
	if ((timeParts.tm_mday < MIN_DAY) || (timeParts.tm_mday > MAX_DAY))
		return 0;

	return (
		(((timeParts.tm_year - 80) & 0x7F) << 25) | // Adjust for MS-FAT base year (1980 vs 1900 for tm_year)
		(((timeParts.tm_mon + 1) & 0xF) << 21) |
		((timeParts.tm_mday & 0x1F) << 16) |
		((timeParts.tm_hour & 0x1F) << 11) |
		((timeParts.tm_min & 0x3F) << 5) |
		((timeParts.tm_sec >> 1) & 0x1F));
}