#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "ffvolumes.h"

static const DISC_INTERFACE *_disc_io[FF_VOLUMES] = {NULL};
static bool _disc_io_init[FF_VOLUMES] = {false};

bool configure_disc_io(int vol, const DISC_INTERFACE *disc_io_drv)
{
    if (!VALID_DISK(vol))
        return false;
    if (_disc_io[vol])
        return false;
    _disc_io[vol] = disc_io_drv;
    return true;
}

bool init_disc_io(int vol) 
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

bool deinit_disc_io(int vol) 
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

const DISC_INTERFACE *get_disc_io(int vol)
{
    if (!VALID_DISK(vol))
        return NULL;
    if (!_disc_io[vol])
        return NULL;
    return _disc_io[vol];
}

extern int get_ldnumber(const TCHAR** path);

int get_vol(const TCHAR* mount)
{
    int mnt_len = strlen(mount);
    if (mnt_len < 2)
        return -1;
    
    TCHAR end_chr = mount[mnt_len - 1];
    if (end_chr == ':' || (end_chr == '/' && mount[mnt_len - 2] == ':'))
        return get_ldnumber(&mount);
    return -1;
}
