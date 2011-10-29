// stat.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#define MODE_BITS (S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID)



static Val   mkStatRep   (Task* task,  struct stat* buf)   {
    //       =========
    //
    // Mythryl type:
    //
    // This makes a representation of the struct stat to be returned
    // to the Lib7 side. It is a tuple with the following fields:
    //
    //    file_type : int
    //    mode      : word
    //    ino       : word
    //    dev       : word
    //    nlink     : word
    //    uid       : word
    //    gid       : word
    //    size      : int
    //    atime     : one_word_int.Int
    //    mtime     : one_word_int.Int
    //    ctime     : one_word_int.Int

    int  ftype;
    Val  mode, ino, dev, uid, gid, nlink, sr, atime, mtime, ctime;

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

    WORD_ALLOC (task, mode, (Val_Sized_Unt)((buf->st_mode) & MODE_BITS));
    WORD_ALLOC (task, ino, (Val_Sized_Unt)(buf->st_ino));
    WORD_ALLOC (task, dev, (Val_Sized_Unt)(buf->st_dev));
    WORD_ALLOC (task, nlink, (Val_Sized_Unt)(buf->st_nlink));
    WORD_ALLOC (task, uid, (Val_Sized_Unt)(buf->st_uid));
    WORD_ALLOC (task, gid, (Val_Sized_Unt)(buf->st_gid));
    INT1_ALLOC (task, atime, buf->st_atime);
    INT1_ALLOC (task, mtime, buf->st_mtime);
    INT1_ALLOC (task, ctime, buf->st_ctime);

    // Allocate the stat record:
    //
    LIB7_AllocWrite(task,  0, MAKE_TAGWORD(11, PAIRS_AND_RECORDS_BTAG));
    LIB7_AllocWrite(task,  1, TAGGED_INT_FROM_C_INT(ftype));
    LIB7_AllocWrite(task,  2, mode);
    LIB7_AllocWrite(task,  3, ino);
    LIB7_AllocWrite(task,  4, dev);
    LIB7_AllocWrite(task,  5, nlink);
    LIB7_AllocWrite(task,  6, uid);
    LIB7_AllocWrite(task,  7, gid);
    LIB7_AllocWrite(task,  8, TAGGED_INT_FROM_C_INT(buf->st_size));
    LIB7_AllocWrite(task,  9, atime);
    LIB7_AllocWrite(task, 10, mtime);
    LIB7_AllocWrite(task, 11, ctime);
    sr = LIB7_Alloc(task, 11);

    return sr;
}



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_stat   (Task* task,  Val arg)   {
    //====================
    //
    // Mythryl type:   String -> Statrep
    //
    // Query file status given file name.
    //
    // This fn gets bound as   stat'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg

    char*           path = HEAP_STRING_AS_C_STRING(arg);
    struct stat     buf;

    int status =  stat( path, &buf );

    if (status < 0)   return RAISE_SYSERR(task, status);

    return  mkStatRep( task, &buf );
}



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_fstat   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type:  Unt -> statrep
    //
    // Query file status given file descriptor.
    //
    // This fn gets bound as   fstat'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg

    int           fd = TAGGED_INT_TO_C_INT(arg);
    struct stat   buf;

    int status =  fstat( fd, &buf );

    if (status < 0)   return RAISE_SYSERR(task, status);

    return  mkStatRep( task, &buf );
}


// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_lstat   (Task* task,  Val arg)   {
    //=====================
    //
    // Mythryl type: String -> statrep
    //
    // Query file status given file name, but do not follow
    // symbolic links.
    //
    // This fn gets bound as   lstat'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg

    char*           path = HEAP_STRING_AS_C_STRING(arg);
    struct stat     buf;

    int status = lstat(path, &buf);

    if (status < 0)   return RAISE_SYSERR(task, status);

    return  mkStatRep( task, &buf );
}




// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

