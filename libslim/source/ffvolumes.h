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
 * Gets the mount point name (without colon) for the specified volume.
 */
const TCHAR* const get_mnt(volno_t vol);

/**
 * Gets the IO driver for the specified volume number.
 */
const DISC_INTERFACE *get_disc_io(volno_t vol);

#endif