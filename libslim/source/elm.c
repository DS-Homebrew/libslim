/*
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
*/

#include <sys/iosupport.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <nds/arm9/dldi.h>

#include <ctype.h>
#include <string.h>

#include <wchar.h>
#include <stdlib.h>
#include <wctype.h>

#include <ff.h>
#include <diskio.h>
#include <slim.h>
#include <ffvolumes.h>

int elm_error;

static FATFS _elm[FF_VOLUMES];
// static DWORD clmt[20];

#if FF_MAX_SS == 512 /* Single sector size */
#define ELM_SS(fs) 512U
#elif FF_MAX_SS == 1024 || FF_MAX_SS == 2048 || FF_MAX_SS == 4096 /* Multiple sector size */
#define ELM_SS(fs) ((fs)->ssize)
#else
#error Wrong sector size.
#endif

int _ELM_open_r(struct _reent *r, void *fileStruct, const char *path, int flags, int mode);
int _ELM_close_r(struct _reent *r, void *fd);
ssize_t _ELM_write_r(struct _reent *r, void *fd, const char *ptr, size_t len);
ssize_t _ELM_read_r(struct _reent *r, void *fd, char *ptr, size_t len);
off_t _ELM_seek_r(struct _reent *r, void *fd, off_t pos, int dir);
int _ELM_fstat_r(struct _reent *r, void *fd, struct stat *st);
int _ELM_stat_r(struct _reent *r, const char *file, struct stat *st);
int _ELM_link_r(struct _reent *r, const char *existing, const char *newLink);
int _ELM_unlink_r(struct _reent *r, const char *name);
int _ELM_chdir_r(struct _reent *r, const char *name);
int _ELM_rename_r(struct _reent *r, const char *oldName, const char *newName);
int _ELM_mkdir_r(struct _reent *r, const char *path, int mode);
DIR_ITER *_ELM_diropen_r(struct _reent *r, DIR_ITER *dirState, const char *path);
int _ELM_dirreset_r(struct _reent *r, DIR_ITER *dirState);
int _ELM_dirnext_r(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat);
int _ELM_dirclose_r(struct _reent *r, DIR_ITER *dirState);
int _ELM_statvfs_r(struct _reent *r, const char *path, struct statvfs *buf);
int _ELM_ftruncate_r(struct _reent *r, void *fd, off_t len);
int _ELM_fsync_r(struct _reent *r, void *fd);
int _ELM_rmdir_r(struct _reent *r, const char *name);

typedef struct _DIR_EX_
{
    DIR dir;
    TCHAR name[FF_MAX_LFN + 1];
    size_t namesize;
} DIR_EX;

#define ELM_DEVOPTAB(mount)                     \
    {                                           \
        mount,                                  \
            sizeof(FIL),                        \
            _ELM_open_r,   /* fopen  */         \
            _ELM_close_r,  /* fclose */         \
            _ELM_write_r,  /* fwrite */         \
            _ELM_read_r,   /* fread  */         \
            _ELM_seek_r,   /* fseek  */         \
            _ELM_fstat_r,  /* fstat  */         \
            _ELM_stat_r,   /* stat   */         \
            _ELM_link_r,   /* link   */         \
            _ELM_unlink_r, /* unlink */         \
            _ELM_chdir_r,  /* chdir  */         \
            _ELM_rename_r, /* rename */         \
            _ELM_mkdir_r,  /* mkdir  */         \
            sizeof(DIR_EX),                     \
            _ELM_diropen_r,   /* diropen   */   \
            _ELM_dirreset_r,  /* dirreset  */   \
            _ELM_dirnext_r,   /* dirnext   */   \
            _ELM_dirclose_r,  /* dirclose  */   \
            _ELM_statvfs_r,   /* statvfs   */   \
            _ELM_ftruncate_r, /* ftruncate */   \
            _ELM_fsync_r,     /* fsync     */   \
            NULL,             /* device data */ \
            NULL,                               \
            NULL,                               \
            _ELM_rmdir_r /* rmdir */            \
    }

static const devoptab_t dotab_elm[FF_VOLUMES] =
    {
        ELM_DEVOPTAB(FF_MNT_FC),
        ELM_DEVOPTAB(FF_MNT_SD),
};

static TCHAR CvtBuf[FF_MAX_LFN + 1];

/**
 * Used to be needed to chop off fat: prefix.
 * Now we don't need it, but keep it around
 * as a macro just in case.
 */
#define _ELM_realpath(path) (path)

static TCHAR *_ELM_mbstoucs2(const char *src, size_t *len)
{
    mbstate_t ps = {0};
    wchar_t tempChar;
    int bytes;
    TCHAR *dst = CvtBuf;
    while (src)
    {
        bytes = mbrtowc(&tempChar, src, MB_CUR_MAX, &ps);
        if (bytes > 0)
        {
            *dst = (TCHAR)tempChar;
            src += bytes;
            dst++;
        }
        else if (bytes == 0)
        {
            break;
        }
        else
        {
            dst = CvtBuf;
            break;
        }
    }
    *dst = '\0';
    if (len)
        *len = dst - CvtBuf;
    return CvtBuf;
}

static size_t _ELM_ucs2tombs(char *dst, const TCHAR *src)
{
    mbstate_t ps = {0};
    size_t count = 0;
    int bytes;
    char buff[MB_CUR_MAX];
    int i;

    while (*src != '\0')
    {
        bytes = wcrtomb(buff, *src, &ps);
        if (bytes < 0)
        {
            return -1;
        }
        if (bytes > 0)
        {
            for (i = 0; i < bytes; i++)
            {
                *dst++ = buff[i];
            }
            src++;
            count += bytes;
        }
        else
        {
            break;
        }
    }
    *dst = L'\0';
    return count;
}

ssize_t _ELM_errnoparse(struct _reent *r, ssize_t suc, int fail)
{
    int ret = fail;
    switch (elm_error)
    {
    case FR_OK:
        ret = suc;
        break;
    case FR_NO_FILE:
        r->_errno = ENOENT;
        break;
    case FR_NO_PATH:
        r->_errno = ENOENT;
        break;
    case FR_INVALID_NAME:
        r->_errno = EINVAL;
        break;
    case FR_INVALID_DRIVE:
        r->_errno = EINVAL;
        break;
    case FR_EXIST:
        r->_errno = EEXIST;
        break;
    case FR_DENIED:
        r->_errno = EACCES;
        break;
    case FR_NOT_READY:
        r->_errno = ENODEV;
        break;
    case FR_WRITE_PROTECTED:
        r->_errno = EROFS;
        break;
    case FR_DISK_ERR:
        r->_errno = EIO;
        break;
    case FR_INT_ERR:
        r->_errno = EIO;
        break;
    case FR_NOT_ENABLED:
        r->_errno = EINVAL;
        break;
    case FR_NO_FILESYSTEM:
        r->_errno = EIO;
        break;
    }
    return ret;
}

int _ELM_open_r(struct _reent *r, void *fileStruct, const char *path, int flags, int mode)
{
    FIL *fp = (FIL *)fileStruct;
    BYTE m = 0;
    BOOL truncate = false;
    const TCHAR *p = _ELM_mbstoucs2(_ELM_realpath(path), NULL);
    if (flags & O_WRONLY)
    {
        m |= FA_WRITE;
    }
    else if (flags & O_RDWR)
    {
        m |= FA_READ | FA_WRITE;
    }
    else
    {
        m |= FA_READ;
    }
    if (flags & O_CREAT)
    {
        if (flags & O_EXCL)
            m |= FA_CREATE_NEW;
        else if (flags & O_TRUNC)
            m |= FA_CREATE_ALWAYS;
        else
            m |= FA_OPEN_ALWAYS;
    }
    else
    {
        if (flags & O_TRUNC)
            truncate = true;
        m |= FA_OPEN_EXISTING;
    }
    elm_error = f_open(fp, p, m);
    if (elm_error == FR_OK && truncate)
    {
        elm_error = f_truncate(fp);
    }
    if (elm_error == FR_OK && (flags & O_APPEND))
    {
        elm_error = f_lseek(fp, fp->obj.objsize);
    }
#if FF_USE_FASTSEEK
    if (elm_error == FR_OK)
    {
        fp->cltbl = clmt;
        clmt[0] = 20;
        elm_error = f_lseek(fp, CREATE_LINKMAP);
    }
    if (elm_error == FR_OK)
    {
        elm_error = f_lseek(fp, 0);
    }
#endif
    return _ELM_errnoparse(r, (int)fp, -1);
}

int _ELM_close_r(struct _reent *r, void *fd)
{
    FIL *fp = (FIL *)fd;
    elm_error = f_close(fp);
    return _ELM_errnoparse(r, 0, -1);
}

ssize_t _ELM_write_r(struct _reent *r, void *fd, const char *ptr, size_t len)
{
#if !FF_FS_READONLY
    FIL *fp = (FIL *)fd;
    UINT written = 0;
    elm_error = f_write(fp, ptr, len, &written);
    return _ELM_errnoparse(r, written, -1);
#else
    r->_errno = ENOSYS;
    return -1;
#endif
}

ssize_t _ELM_read_r(struct _reent *r, void *fd, char *ptr, size_t len)
{
    FIL *fp = (FIL *)fd;
    UINT read = 0;
    elm_error = f_read(fp, ptr, (UINT)len, &read);
    return (_ELM_errnoparse(r, read, -1) == -1 ? -1 : (ssize_t)read);
}

off_t _ELM_seek_r(struct _reent *r, void *fd, off_t pos, int whence)
{
#if FF_FS_MINIMIZE < 3
    FIL *f = (FIL *)fd;
    FSIZE_t off;

    switch (whence)
    {
    case SEEK_SET:
        off = 0;
        break;
    case SEEK_CUR:
        off = f_tell(f);
        break;
    case SEEK_END:
        off = f_size(f);
        break;
    default:
        r->_errno = EINVAL;
        return -1;
    }

    if (pos < 0 && off < -pos)
    {
        /* don't allow seek to before the beginning of the file */
        r->_errno = EINVAL;
        return -1;
    }

    elm_error = f_lseek(f, off+(FSIZE_t)pos);
    return (_ELM_errnoparse(r, f->fptr, -1) == -1 ? -1 : (off_t)(off + (FSIZE_t)pos));
#else
    r->_errno = ENOSYS;
    return -1;
#endif
}

int _ELM_fstat_r(struct _reent *r, void *fd, struct stat *st)
{
#if FF_FS_MINIMIZE < 1
    FIL *fp = (FIL *)fd;
    memset(st, 0, sizeof(*st));
    st->st_nlink = 1;
    st->st_uid = 1;
    st->st_gid = 2;
    st->st_size = fp->obj.objsize;
    st->st_spare4[0] = fp->obj.attr;
    return 0;
#else
    r->_errno = ENOSYS;
    return -1;
#endif
}

static void _ELM_fileinfo_to_stat(const FILINFO *info, struct stat *st)
{
    /* Date of last modification */
    struct tm date;
    memset(st, 0, sizeof(struct stat));
    date.tm_mday = info->fdate & 31;
    date.tm_mon = ((info->fdate >> 5) & 15) - 1;
    date.tm_year = ((info->fdate >> 9) & 127) - 1980 + 1900;

    date.tm_sec = (info->ftime << 1) & 63;
    date.tm_min = (info->ftime >> 5) & 63;
    date.tm_hour = (info->ftime >> 11) & 31;

    st->st_atime = st->st_mtime = st->st_ctime = mktime(&date);
    st->st_size = (off_t)info->fsize;
    st->st_nlink = 1;

    if (info->fattrib & AM_DIR)
    {
        st->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
    }
    else
    {
        st->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    }
}

static int _ELM_chk_mounted(int disk)
{
    if (VALID_DISK(disk))
    {
        if (!_elm[disk].fs_type)
        {
            FILINFO fi;
            TCHAR path[3];
            const TCHAR *path_ptr = path;
            path[0] = L'0' + disk;
            path[1] = L':';
            path[2] = 0;
            f_stat(path_ptr, &fi);
        }
        return _elm[disk].fs_type;
    }
    return false;
}

static void _ELM_disk_to_stat(int disk, struct stat *st)
{
    memset(st, 0, sizeof(*st));
    if (_ELM_chk_mounted(disk))
    {
        st->st_mode = S_IFDIR;
        st->st_nlink = 1;
        st->st_uid = 1;
        st->st_gid = 2;
    }
}

int _ELM_stat_r(struct _reent *r, const char *file, struct stat *st)
{
#if FF_FS_MINIMIZE < 1
    size_t len = 0;
    TCHAR *p = _ELM_mbstoucs2(_ELM_realpath(file), &len);

    // chop off trailing slash
    if (p[len - 1] == L'/')
        p[len - 1] = L'\0';

    int vol;
    if ((vol = get_vol(p)) != -1)
    {
        _ELM_disk_to_stat(vol, st);
        return _ELM_errnoparse(r, 0, -1);
    }

    FILINFO fi;
    fi.fsize = sizeof(fi.fname) / sizeof(fi.fname[0]);
    elm_error = f_stat(p, &fi);
    _ELM_fileinfo_to_stat(&fi, st);
    return _ELM_errnoparse(r, 0, -1);
#else
    r->_errno = ENOSYS;
    return -1;
#endif
}

int _ELM_link_r(struct _reent *r, const char *existing, const char *newLink)
{
    r->_errno = ENOSYS;
    return -1;
}

int _ELM_unlink_r(struct _reent *r, const char *path)
{
#if (FF_FS_MINIMIZE < 1) && (!FF_FS_READONLY)
    size_t len = 0;
    TCHAR *p = _ELM_mbstoucs2(_ELM_realpath(path), &len);
    if (p[len - 1] == L'/')
        p[len - 1] = L'\0';

    elm_error = f_unlink(p);
    return _ELM_errnoparse(r, 0, -1);
#else
    r->_errno = ENOSYS;
    return -1;
#endif
}

int _ELM_rmdir_r(struct _reent *r, const char *path)
{
#if !FF_FS_RPATH_DOTENTRY
    return _ELM_unlink_r(r, path);
#else
    r->_errno = ENOSYS;
    return -1;
#endif
}

int _ELM_chdir_r(struct _reent *r, const char *path)
{
#if FF_FS_RPATH
    size_t len = 0;
    TCHAR *p = _ELM_mbstoucs2(_ELM_realpath(path), &len);
    TCHAR *drive = strchr(p, ':');

    if (drive != NULL)
    {
        TCHAR _drive = drive[1];
        drive[1] = '\0';
        elm_error = f_chdrive(p);
        if (elm_error)
            return _ELM_errnoparse(r, 0, -1);
        drive[1] = _drive;
    }

    /* 
     * FatFs expects no trailing slash 
     * drive[2] != '\0' is a check to ensure 
     * parent drive is not root.
     */
    if (drive[2] != '\0' && (len > 1 && p[len - 1] == L'/'))
    {
        p[len - 1] = L'\0';
        --len;
    }

    elm_error = f_chdir(p);
    return _ELM_errnoparse(r, 0, -1);
#else
    r->_errno = ENOSYS;
    return -1;
#endif
}

int _ELM_rename_r(struct _reent *r, const char *path, const char *pathp)
{
#if (FF_FS_MINIMIZE < 1) && (!FF_FS_READONLY)
    TCHAR p[FF_MAX_LFN + 1];
    const TCHAR *pp;
    memcpy(p, _ELM_mbstoucs2(_ELM_realpath(path), NULL), sizeof(p));
    pp = _ELM_mbstoucs2(_ELM_realpath(pathp), NULL);
    if ((pp[0] == L'0' || pp[0] == L'1') && pp[1] == L':')
        pp += 2;
    elm_error = f_rename(p, pp);
    return _ELM_errnoparse(r, 0, -1);
#else
    r->_errno = ENOSYS;
    return -1;
#endif
}

int _ELM_mkdir_r(struct _reent *r, const char *path, int mode)
{
#if (FF_FS_MINIMIZE < 1) && (!FF_FS_READONLY)
    const TCHAR *p = _ELM_mbstoucs2(_ELM_realpath(path), NULL);
    elm_error = f_mkdir(p);
    return _ELM_errnoparse(r, 0, -1);
#else
    r->_errno = ENOSYS;
    return -1;
#endif
}

DIR_ITER *_ELM_diropen_r(struct _reent *r, DIR_ITER *dirState, const char *path)
{
#if FF_FS_MINIMIZE < 2
    size_t len = 0;
    TCHAR *p = _ELM_mbstoucs2(_ELM_realpath(path), &len);

    if (len > 1 && p[len - 1] == L'/')
    {
        p[len - 1] = L'\0';
        --len;
    }

    DIR_EX *dir = (DIR_EX *)dirState->dirStruct;
    memcpy(dir->name, (void *)p, (len + 1) * sizeof(TCHAR));
    dir->namesize = len + 1;
    elm_error = f_opendir(&(dir->dir), p);
    return (DIR_ITER *)_ELM_errnoparse(r, (int)dirState, 0);
#else
    r->_errno = ENOSYS;
    return 0;
#endif
}

int _ELM_dirreset_r(struct _reent *r, DIR_ITER *dirState)
{
#if FF_FS_MINIMIZE < 2
    DIR_EX *dir = (DIR_EX *)dirState->dirStruct;
    elm_error = f_readdir(&(dir->dir), NULL);
    return _ELM_errnoparse(r, 0, -1);
#else
    r->_errno = ENOSYS;
    return -1;
#endif
}

int _ELM_dirnext_r(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *st)
{
#if FF_FS_MINIMIZE < 2
    FILINFO fi;
    fi.fsize = sizeof(fi.fname) / sizeof(fi.fname[0]);
    DIR_EX *dir = (DIR_EX *)dirState->dirStruct;
    elm_error = f_readdir(&(dir->dir), &fi);
    if (elm_error != FR_OK)
        return _ELM_errnoparse(r, 0, -1);
    if (!fi.fname[0])
        return -1;
#ifdef FF_USE_LFN
    _ELM_ucs2tombs(filename, fi.fname);
#else
    strcpy(filename, fi.fname);
#endif
    if (st != NULL)
    {
        TCHAR path[FF_MAX_LFN + 1];
        size_t len = 0;
        memcpy(path, dir->name, (dir->namesize - 1) * sizeof(TCHAR));
        path[dir->namesize - 1] = L'/';
        memcpy(path + dir->namesize, _ELM_mbstoucs2(filename, &len), (len + 1) * sizeof(TCHAR));
        _ELM_fileinfo_to_stat(&fi, st);
    }
    return 0;
#else
    r->_errno = ENOSYS;
    return -1;
#endif
}

int _ELM_dirclose_r(struct _reent *r, DIR_ITER *dirState)
{
    return _ELM_dirreset_r(r, dirState);
}

int _ELM_statvfs_r(struct _reent *r, const char *path, struct statvfs *buf)
{
    size_t len = 0;
    TCHAR *p = _ELM_mbstoucs2(_ELM_realpath(path), &len);

    int vol;

    if ((vol = get_vol(p) != -1))
    {
        DWORD nclust;
        FATFS *fat = &_elm[vol];
        elm_error = f_getfree(p, &nclust, &fat);

        buf->f_bsize = ELM_SS(fat);
        buf->f_frsize = ELM_SS(fat);

        buf->f_blocks = (fat->n_fatent - 2) * fat->csize;
        buf->f_bfree = nclust * fat->csize;
        buf->f_bavail = buf->f_bfree;

        buf->f_files = (fat->n_fatent - 2) * fat->csize;
        buf->f_ffree = nclust * fat->csize;
        buf->f_favail = buf->f_bfree;
        buf->f_fsid = fat->fs_type;

        buf->f_namemax = FF_MAX_LFN;
        buf->f_flag = ST_NOSUID /* No support for ST_ISUID and ST_ISGID file mode bits */
                      | (FF_FS_READONLY ? ST_RDONLY /* Read only file system */ : 0);
        return _ELM_errnoparse(r, 0, -1);
    }
    return -1;
}

int _ELM_ftruncate_r(struct _reent *r, void *fd, off_t len)
{
#if (_FS_MINIMIZE < 1) && (!FF_FS_READONLY)
    FIL *fp = (FIL *)fd;
    int ptr = fp->fptr;
    elm_error = f_lseek(fp, len);
    if (elm_error != FR_OK)
        return _ELM_errnoparse(r, 0, -1);
    if (ptr > len)
        ptr = len;
    elm_error = f_truncate(fp);
    if (elm_error != FR_OK)
        return _ELM_errnoparse(r, 0, -2);
    fp->fptr = ptr;
    return 0;
#else
    r->_errno = ENOSYS;
    return -1;
#endif
}

int _ELM_fsync_r(struct _reent *r, void *fd)
{
#if !FF_FS_READONLY
    elm_error = f_sync((FIL *)fd);
    return _ELM_errnoparse(r, 0, -1);
#else
    r->_errno = ENOSYS;
    return -1;
#endif
}

WCHAR ff_convert(WCHAR src, UINT dir)
{
    (void)dir;
    if (src >= 0x80)
        src = '+';
    return src;
}

// This has to be here for _ELM_chk_mounted.
bool fatMountSimple(const char *mount, const DISC_INTERFACE *interface)
{
    int vol = get_vol(mount);
    if (vol == -1)
        return false;
    configure_disc_io(vol, interface);
    if (f_mount(&(_elm[vol]), mount, 1) != FR_OK)
    {
        return false;
    }
    if (!_ELM_chk_mounted(vol))
    {
        return false;
    }
    AddDevice(&dotab_elm[vol]);
    return true;
}
