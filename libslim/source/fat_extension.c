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

/*
 * libfat compatibility API
 */

#define ARGV_SUPPORT

#include <sys/iosupport.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <nds/arm9/dldi.h>

#include <stdlib.h>
#include <string.h>

#include <ff.h>
#include <diskio.h>
#include <slim.h>
#include <ffvolumes.h>

#ifdef ARGV_SUPPORT
#include <nds/system.h>
#include <strings.h>
#include <limits.h>
#endif

#include "charset.h"
#include "cache.h"

bool configureDefault(const char *root)
{
    volno_t vol = get_vol(root);
    if (vol == -1)
        return false;
    const char *mount = get_mnt(vol);
    char filePath[PATH_MAX];
    strcpy(filePath, mount);
    strcat(filePath, ":/");
#ifdef ARGV_SUPPORT
    if (__system_argv->argvMagic == ARGV_MAGIC && __system_argv->argc >= 1 && (strrchr(__system_argv->argv[0], '/') != NULL))
    {
        if (get_disc_io(vol) != NULL && strncasecmp(__system_argv->argv[0], mount, strlen(mount)))
        {
            char *lastSlash;
            strcpy(filePath, __system_argv->argv[0]);
            lastSlash = strrchr(filePath, '/');

            if (lastSlash != NULL)
            {
                if (*(lastSlash - 1) == ':')
                    lastSlash++;
                *lastSlash = '\0';
            }
        }
    }
#endif
    chdir(filePath);
    return true;
}

bool configureCache(uint32_t cacheSize)
{
    return cache_init(cacheSize) != NULL;
}
