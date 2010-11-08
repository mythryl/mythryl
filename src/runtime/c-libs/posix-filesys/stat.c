/* stat.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#define MODE_BITS (S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID)

/* mkStatRep:
 *
 * This makes a representation of the struct stat to be returned
 * to the Lib7 side. It is a tuple with the following fields:
 *
 *    file_type : int
 *    mode      : word
 *    ino       : word
 *    dev       : word
 *    nlink     : word
 *    uid       : word
 *    gid       : word
 *    size      : int
 *    atime     : int32.Int
 *    mtime     : int32.Int
 *    ctime     : int32.Int
 */
static lib7_val_t mkStatRep (lib7_state_t *lib7_state, struct stat *buf)
{
    int		    ftype;
    lib7_val_t        mode, ino, dev, uid, gid, nlink, sr, atime, mtime, ctime;

#if ((S_IFDIR != 0x4000) || (S_IFCHR != 0x2000) || (S_IFBLK != 0x6000) || (S_IFREG != 0x8000) || (S_IFIFO != 0x1000) || (S_IFLNK != 0xA000) || (S_IFSOCK != 0xC000))
    if (S_ISDIR(buf->st_mode)) ftype = 0x4000;
    else if (S_ISCHR(buf->st_mode)) ftype = 0x2000;
    else if (S_ISBLK(buf->st_mode)) ftype = 0x6000;
    else if (S_ISREG(buf->st_mode)) ftype = 0x8000;
    else if (S_ISFIFO(buf->st_mode)) ftype = 0x1000;
#ifdef S_ISLNK
    else if (S_ISLNK(buf->st_mode)) ftype = 0xA000;
#endif
#ifdef S_ISSOCK
    else if (S_ISSOCK(buf->st_mode)) ftype = 0xC000;
#endif
    else ftype = 0;
#else
    ftype = buf->st_mode & 0xF000;
#endif

    WORD_ALLOC (lib7_state, mode, (Word_t)((buf->st_mode) & MODE_BITS));
    WORD_ALLOC (lib7_state, ino, (Word_t)(buf->st_ino));
    WORD_ALLOC (lib7_state, dev, (Word_t)(buf->st_dev));
    WORD_ALLOC (lib7_state, nlink, (Word_t)(buf->st_nlink));
    WORD_ALLOC (lib7_state, uid, (Word_t)(buf->st_uid));
    WORD_ALLOC (lib7_state, gid, (Word_t)(buf->st_gid));
    INT32_ALLOC (lib7_state, atime, buf->st_atime);
    INT32_ALLOC (lib7_state, mtime, buf->st_mtime);
    INT32_ALLOC (lib7_state, ctime, buf->st_ctime);

  /* allocate the stat record */
    LIB7_AllocWrite(lib7_state,  0, MAKE_DESC(11, DTAG_record));
    LIB7_AllocWrite(lib7_state,  1, INT_CtoLib7(ftype));
    LIB7_AllocWrite(lib7_state,  2, mode);
    LIB7_AllocWrite(lib7_state,  3, ino);
    LIB7_AllocWrite(lib7_state,  4, dev);
    LIB7_AllocWrite(lib7_state,  5, nlink);
    LIB7_AllocWrite(lib7_state,  6, uid);
    LIB7_AllocWrite(lib7_state,  7, gid);
    LIB7_AllocWrite(lib7_state,  8, INT_CtoLib7(buf->st_size));
    LIB7_AllocWrite(lib7_state,  9, atime);
    LIB7_AllocWrite(lib7_state, 10, mtime);
    LIB7_AllocWrite(lib7_state, 11, ctime);
    sr = LIB7_Alloc(lib7_state, 11);

    return sr;

} /* end of mkStatRep */

/* _lib7_P_FileSys_stat : String -> statrep
 *
 * Query file status given file name.
 */
lib7_val_t _lib7_P_FileSys_stat (lib7_state_t *lib7_state, lib7_val_t arg)
{
    char            *path = STR_LIB7toC(arg);
    int		    status;
    struct stat     buf;

    status = stat(path, &buf);

    if (status < 0)
        return RAISE_SYSERR(lib7_state, status);

    return (mkStatRep(lib7_state, &buf));
}


/* _lib7_P_FileSys_fstat : word -> statrep
 *
 * Query file status given file descriptor.
 */
lib7_val_t _lib7_P_FileSys_fstat (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int             fd = INT_LIB7toC(arg);
    int		    status;
    struct stat     buf;

    status = fstat(fd, &buf);

    if (status < 0)
        return RAISE_SYSERR(lib7_state, status);

    return (mkStatRep(lib7_state, &buf));
}


/* _lib7_P_FileSys_lstat : String -> statrep
 *
 * Query file status given file name, but do not follow
 * symbolic links.
 */
lib7_val_t _lib7_P_FileSys_lstat (lib7_state_t *lib7_state, lib7_val_t arg)
{
    char            *path = STR_LIB7toC(arg);
    int		    status;
    struct stat     buf;

    status = lstat(path, &buf);

    if (status < 0)
        return RAISE_SYSERR(lib7_state, status);

    return (mkStatRep(lib7_state, &buf));
}




/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
