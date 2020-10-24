# libslim

A revival of libELM for use with the Nintendo DS, based off modern FatFS.

libslim aims to be a lightweight drop-in compatible with libfat in the common modern use case and provides only 2 stdio devices instead of libfat's more flexible options. 

  * `fat:/` for the SLOT-1 flashcart device
  * `sd:/` for the TWL-mode SD card

It provides a restricted subset of libfat's API for the common use case. 

```c
/**
 * Initializes SD card in sd:/ and SLOT-1 flashcart device in fat:/ with the default
 * DISC_INTERFACE, then sets the default directory to sd:/ if available, or 
 * fat:/ otherwise, and then changes directory according to argv if setArgvMagic 
 * is true.
 * 
 * This method differs significantly enough from its equivalent libfat API that
 * you should take caution. Particularly of note is that unlike libfat, the 
 * cluster cache is shared across all mounted devices.
 * 
 * Returns true if either sd:/ or fat:/ is successfully mounted, and false otherwise. * 
 */
bool fatInit(uint32_t cacheSize, bool setArgvMagic);

/**
 * Initializes SD card in sd:/ and SLOT-1 flashcart device in fat:/ with the default
 * DISC_INTERFACE, then sets the default directory to sd:/ if available, or 
 * fat:/ otherwise, and then changes directory according to argv.
 * 
 * Returns true if either sd:/ or fat:/ is successfully mounted, and false otherwise.
 */
bool fatInitDefault(void);

/**
 * Initializes the given mount point with the provided DISC_INTERFACE
 * 
 * - `mount` must be either "sd:/" or "fat:/". Unlike libfat, this function does not
 * support other mount points.
 */
bool fatMountSimple(const char* mount, const DISC_INTERFACE* interface);

/**
 * Unmounts the given mount point, attempting to flush any open files.
 * 
 * Unlike libfat, this function returns true on success.
 */
bool fatUnmount(const char* mount);

/**
 * Gets the volume label of the given mount point.
 * 
 * - `mount` must be either "sd:" or "fat:". Unlike libfat, this function does not
 * support other mount points.
 */
void fatGetVolumeLabel(const char* mount, char *label);

/**
 * Gets the given FAT attributes for the file.
 */
int FAT_getAttr(const char *file);

/**
 * Sets the FAT attributes
 */
int FAT_setAttr(const char *file, uint8_t attr);
```

# Usage

libslim is **not compatible with libfat.**

1. In your Makefile, link against `lslim` instead of `lfat`. This must come before `lnds9`.
2. Include `<slim.h>` instead of `<fat.h>`.
3. If you are using `fatMountSimple` instead of `fatInitDefault` or `fatInit`, **you must add `:/` to each mount point**
   * libfat: `fatMountSimple("sd", get_io_dsisd())`
   * libslim: `fatMountSimple("sd:/", get_io_dsisd())`

## Advanced Usage

libslim uses [FatFS](http://elm-chan.org/fsw/ff/00index_e.html) as a backend FAT implementation, and is fully configurable
via `ffconf.h`. For more information on each configuration option, see the [FatFS Documentation](http://elm-chan.org/fsw/ff/doc/config.html).

In addition, libslim adds the following compile-time options, toggleable via defines, in addition to those provided by FatFS.

### FatFS Options

#### `FF_FS_RPATH_DOTENTRY`

**Default:** `1` (Enabled)

Configures filtering of `.` and `..` entries in `readdir`. 

* Setting to `0` will filter out dot entries in `readdir`. This is the default behaviour of FatFS, but **is not the default behaviour of libslim**
* Setting this to `1` will show dot entries in `readdir`. This is the default option. 
  
Enabling this option (by default) will disable support for `rmdir`.

#### `FF_USE_FASTSEEK`

**Default:** `1` (Enabled)

Configures the use of the FAT cluster chain caching (["fastseek"](http://elm-chan.org/fsw/ff/doc/lseek.html)) feature in FatFS. 

Fastseek will only be enabled for files opened in read-only (`"r"`) to ensure proper functionality of `truncate` and `fwrite`. It will heap-allocate 96 bytes of memory per opened file, freed on `fclose`. 

### Cache Options

libslim uses a comparatively more lightweight GCLOCK-based cache with many configuration options that can be tweaked to fit a particular use case. 

#### `SLIM_USE_CACHE`

**Default:** `1` (Enabled, with prefetch buffer in heap memory)

Configures the use of the sector cache. This should be enabled for most use cases. If you decide not to use the cache, the recommended route is to use runtime cache configuration instead of disabling the cache through this define. The cache uses `(512 * SLIM_PREFETCH_AMOUNT) + (CACHE_SIZE * 544)`  bytes of memory, heap allocated on first mount.


* Setting to `0` will disable the cache.
* Setting to `1` will enable the cache in heap memory, with the prefetch buffer in heap memory.
* Setting to `2` will enable the cache in heap memory, with the prefetch buffer in `.bss`. 
  
#### `SLIM_CACHE_SIZE`

**Default:** `64`

Configures the size (in number of sectors) of the cache. By default, 64 sectors will be cached, if not otherwise set at runtime. The default cache size of 64 results in a total reserved heap space of 34KiB used for the cache, and 512 bytes in `.bss` used as a working buffer.

#### `SLIM_DMA_CACHE_STORE`

**Default:** `0` (Disabled)

Configures whether or not to use DMA copies to write to the cache. May result in a speedup if you need frequent streaming
disk access, but is not generally needed, and may cause issues with IRQ sensitive code.

#### `SLIM_CHUNKED_READS`

**Default:** `1` (Enabled)

Configures whether or not libslim will maximize the size of SD card reads when caching is enabled. 

If caching is enabled, and `SLIM_CHUNKED_READS` is disabled, sectors will be read one-by-one from the SD card using a separate request for each sector. This could result in degraded performance, but will take up a smaller code space. 

If caching is enabled, and `SLIM_CHUNKED_READS` is enabled, cached sectors will first be read, then uncached sectors are 'greedily' read in chunks of up to 32 sectors per SD card request, and used to 'fill in the blanks', to minimize the number of SD card accesses while still accounting for cached sectors.

If caching is disabled either in runtime or via `SLIM_USE_CACHE`, this has no effect, and libslim will always read the full number of requested sectors in a single request.

#### `SLIM_PREFETCH_AMOUNT`

**Default:** `1`

Configures the number of sectors to prefetch on single-sector read requests. The goal is the minimize the number of
SD card requests. Single sector reads often occur during initialization or directory read requests, and often require multiple blocks that may come in a separate read request. Increasing the amount of prefetched sectors may help with this.

`.bss` RAM usage increases by a factor of `512 * SLIM_PREFETCH_AMOUNT`.

### Runtime Configuration API
libslim provides a runtime configuration API that does not have an exact analogue in libfat.

#### Default Device
The default device is automatically configured if the `fatInit` or `fatInitDefault` APIs are used, as is done in libfat. However, the `fatMountSimple` API differs significantly from libfat, and does not set the default device.

If `fatMountSimple` is used to mount a drive, the default device should be configured with `configureDefault(const char *root)`,
where `root` is either `sd:/` or `fat:/`. This will also properly set ARGV as is the default behaviour of libfat.

#### Cache Size
Cache size must be configured **before any mount points have created** with `configureCache(uint32_t cacheSize)`, where
cache size is the **number of sectors**, not the number of pages as in libfat. If cache is not configured, then the default cache size (`SLIM_CACHE_SIZE`) will be used.

The cache can be disabled by configuring a cache size of 0 before any mount points have been created. Note that once the cache size is set, it can not be changed, and subsequent calls to this function will have no effect. 

## Versioning
libslim is not formally versioned. We encourage you to integrate libslim into your projects via adding this repository as a submodule. The subset of the libfat API that libslim provides will remain stable and unchanged. No guarantees can be made for the runtime configuration API, but it will be unlikely to change.

The latest revision of devkitARM that libslim was tested against is **r55** (July 2020).

# Legal

This work is an amalgamation of works by multiple people over several years. Unless otherwise indicated, 
this work is licensed under a 3-clause BSD license as provided in LICENSE. 

## Third Party Licenses 

Licenses of component works are provided below.

### libELM by yellowoodgoblin
```
Copyright (c) 2009-2011, yellow wood goblin
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the yellow wood goblin nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL YELLOW WOOD GOBLIN BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

### FatFs by ChaN
```
/*----------------------------------------------------------------------------/
/  FatFs - FAT file system module  R0.14                     (C)ChaN, 2019
/-----------------------------------------------------------------------------/
/ FatFs module is a generic FAT file system module for small embedded systems.
/ This is a free software that opened for education, research and commercial
/ developments under license policy of following trems.
/
/  Copyright (C) 2019, ChaN, all right reserved.
/
/ * The FatFs module is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-----------------------------------------------------------------------------/
```