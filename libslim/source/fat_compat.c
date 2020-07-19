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
    * Neither the name of chyyran nor the
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

#include <nds/debug.h>

#ifdef ARGV_SUPPORT
#include <nds/system.h>
#include <strings.h>
#endif


bool fatUnmount(const char *mount)
{
    RemoveDevice(mount);
    if (f_mount(NULL, mount, 1) != FR_OK)
    {
        return false;
    }
    return true;
}

bool fatInitDefault(void)
{
    return fatInit(true);
}

void configureArgv()
{
#ifdef ARGV_SUPPORT
    if (__system_argv->argvMagic == ARGV_MAGIC && __system_argv->argc >= 1 && (strrchr(__system_argv->argv[0], '/') != NULL))
    {
        for (int i = 0; i < FF_VOLUMES; i++)
        {
            const char *mount = get_mnt(i);
            if (!mount)
                continue;

            if (get_disc_io(i) != NULL && strncasecmp(__system_argv->argv[0], mount, strlen(mount)))
            {
                char filePath[MAX_FILENAME_LENGTH];
                char *lastSlash;
                strcpy(filePath, __system_argv->argv[0]);
                lastSlash = strrchr(filePath, '/');

                if (lastSlash != NULL)
                {
                    if (*(lastSlash - 1) == ':')
                        lastSlash++;
                    *lastSlash = '\0';
                }
                chdir(filePath);
                return;
            }
        }
    }
#endif
}

bool fatInit(bool setArgvMagic)
{
    bool sdMounted = false, fatMounted = false;
    fatMounted = fatMountSimple(FF_MNT_FC ":", dldiGetInternal());
    if (isDSiMode())
    {
        nocashMessage("is dsi mode");
        sdMounted = fatMountSimple(FF_MNT_SD ":", get_io_dsisd());
    }
    if (sdMounted)
    {
        chdir("sd:/");
        if (setArgvMagic)
        {
            configureArgv();
        }
        return sdMounted;
    }
    else if (fatMounted)
    {
        chdir("fat:/");
        if (setArgvMagic)
        {
            configureArgv();
        }
        return fatMounted;
    }

    return false;
}