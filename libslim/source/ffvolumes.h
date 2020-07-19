
#ifndef __SLIM_VOL_H__
#define __SLIM_VOL_H__

#include "ff.h"
#include <nds/disc_io.h>

/**
 * A volume number.
 */
typedef int volno_t;

/**
 * Configures the IO driver for the specified volume.
 * Once configured, can never be reconfigured.
 * 
 * Returns false if the specified volume number is invalid,
 * or an existing IO driver has already been configured for
 * the volume number.
 */
BOOL configure_disc_io(volno_t vol, const DISC_INTERFACE *disc_io_drv);

/**
 * Initializes the IO driver configured in the specified volume number.
 * Returns true if the IO driver has been initialized.
 */
BOOL init_disc_io(volno_t vol);

/**
 * Deinitializes the IO driver configured in the specified volume number.
 * Returns true if the IO driver has been deinitialized.
 */
BOOL deinit_disc_io(volno_t vol);

/**
 * Gets the volume number given the specified mount point.
 */
volno_t get_vol(const TCHAR *mount);

/**
 */
const TCHAR* const get_mnt(volno_t vol);

const DISC_INTERFACE *get_disc_io(volno_t vol);

#endif