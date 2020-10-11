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

#include <stdlib.h>
#include <string.h>

#include "charset.h"
#include "ffvolumes.h"

static const DISC_INTERFACE *_disc_io[FF_VOLUMES] = {NULL};
static BOOL _disc_io_init[FF_VOLUMES] = {false};

BOOL configure_disc_io(volno_t vol, const DISC_INTERFACE *disc_io_drv)
{
    if (!VALID_DISK(vol))
        return false;
    if (_disc_io[vol])
        return false;
    _disc_io[vol] = disc_io_drv;
    return true;
}

BOOL init_disc_io(volno_t vol) 
{
    if (!VALID_DISK(vol))
        return false;
    if (!_disc_io[vol])
        return false;
    if (_disc_io_init[vol])
        return true;
    _disc_io_init[vol] = _disc_io[vol]->startup();
    return _disc_io_init[vol];
}

BOOL deinit_disc_io(volno_t vol) 
{
    if (!VALID_DISK(vol))
        return false;
    if (!_disc_io[vol])
        return false;
    if (!_disc_io_init[vol])
        return true;
    _disc_io_init[vol] = !_disc_io[vol]->shutdown();
    return !_disc_io_init[vol];
}

const DISC_INTERFACE *get_disc_io(volno_t vol)
{
    if (!VALID_DISK(vol))
        return NULL;
    if (!_disc_io[vol])
        return NULL;
    return _disc_io[vol];
}

extern int get_ldnumber(const TCHAR** path);
extern const char* const VolumeStr[FF_VOLUMES];

volno_t get_vol(const char* mount)
{
    int mnt_len = strlen(mount);
    if (mnt_len < 2)
        return -1;
    size_t len = 0;
    char end_chr = mount[mnt_len - 1];
    if (end_chr == ':' || (end_chr == '/' && mount[mnt_len - 2] == ':'))
    {
        const TCHAR *m = _ELM_mbstoucs2(mount, &len);
        return get_ldnumber(&m);
    }
    return -1;
}

const char* const get_mnt(volno_t vol)
{
    if (!VALID_DISK(vol))
        return NULL;
    return VolumeStr[vol];
}
