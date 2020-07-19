
#ifndef __FF_VOL_H__
#define __FF_VOL_H__

#define VALID_DISK(disk) (disk == FF_VOL_FC || disk == FF_VOL_SD)

/*----------------------------------------------------------------------------/
/ FatFs Mount Points
/----------------------------------------------------------------------------*/

/* VOL is the volume number, and MNT i the mount point. */
#define FF_VOL_FC           0
#define FF_MNT_FC           "fat"

#define FF_VOL_SD           1
#define FF_MNT_SD           "sd"

/* Number of volumes (logical drives) to be used. (1-10) */
#define FF_VOLUMES		    2

#define FF_VOLUME_STRS		FF_MNT_FC, FF_MNT_SD
#endif