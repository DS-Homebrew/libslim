
#ifndef __SLIM_VOL_H__
#define __SLIM_VOL_H__

#include "ff.h"
#include <nds/disc_io.h>

bool configure_disc_io(int vol, const DISC_INTERFACE *disc_io_drv);

bool init_disc_io(int vol);

bool deinit_disc_io(int vol);

int get_vol(const TCHAR *mount);

const DISC_INTERFACE *get_disc_io(int vol);

#endif