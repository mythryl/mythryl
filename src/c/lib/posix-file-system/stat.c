// stat.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#include <stdio.h>
#include <string.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
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

    mode  =  make_one_word_unt(task,  (Val_Sized_Unt)((buf->st_mode) & MODE_BITS)  );
    ino   =  make_one_word_unt(task,  (Val_Sized_Unt)(buf->st_ino)                 );
    dev   =  make_one_word_unt(task,  (Val_Sized_Unt)(buf->st_dev)                 );
    nlink =  make_one_word_unt(task,  (Val_Sized_Unt)(buf->st_nlink)               );
    uid   =  make_one_word_unt(task,  (Val_Sized_Unt)(buf->st_uid)                 );
    gid   =  make_one_word_unt(task,  (Val_Sized_Unt)(buf->st_gid)                 );
    atime =  make_one_word_int(task,                  buf->st_atime		   );
    mtime =  make_one_word_int(task,                  buf->st_mtime		   );
    ctime =  make_one_word_int(task,                  buf->st_ctime		   );

    // Allocate the stat record:
    //
    set_slot_in_nascent_heapchunk(task,  0, MAKE_TAGWORD(11, PAIRS_AND_RECORDS_BTAG));
    set_slot_in_nascent_heapchunk(task,  1, TAGGED_INT_FROM_C_INT(ftype));
    set_slot_in_nascent_heapchunk(task,  2, mode);
    set_slot_in_nascent_heapchunk(task,  3, ino);
    set_slot_in_nascent_heapchunk(task,  4, dev);
    set_slot_in_nascent_heapchunk(task,  5, nlink);
    set_slot_in_nascent_heapchunk(task,  6, uid);
    set_slot_in_nascent_heapchunk(task,  7, gid);
    set_slot_in_nascent_heapchunk(task,  8, TAGGED_INT_FROM_C_INT(buf->st_size));
    set_slot_in_nascent_heapchunk(task,  9, atime);
    set_slot_in_nascent_heapchunk(task, 10, mtime);
    set_slot_in_nascent_heapchunk(task, 11, ctime);
    sr = commit_nascent_heapchunk(task, 11);

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

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_stat");

    int status;

    struct stat     buf;

    char*           heap_path = HEAP_STRING_AS_C_STRING(arg);

    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  path_buf;
    //
    {	char* c_path
	    = 
	    buffer_mythryl_heap_value( &path_buf, (void*) heap_path, strlen( heap_path ) +1 );		// '+1' for terminal NUL on string.

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_stat", arg );
	    //
	    status =  stat( c_path, &buf );
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_stat" );

	unbuffer_mythryl_heap_value( &path_buf );
    }

    if (status < 0)   return RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);

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

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_fstat");

    int status;

    int           fd = TAGGED_INT_TO_C_INT(arg);
    struct stat   buf;

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_fstat", arg );
	//
	status =  fstat( fd, &buf );
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_fstat" );

    if (status < 0)   return RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);

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

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_lstat");

    int status;

    struct stat     buf;

    char*           heap_path = HEAP_STRING_AS_C_STRING(arg);

    // We cannot reference anything on the Mythryl
    // heap between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy heap_path into C storage: 
    //
    Mythryl_Heap_Value_Buffer  path_buf;
    //
    {	char* c_path
	    = 
	    buffer_mythryl_heap_value( &path_buf, (void*) heap_path, strlen( heap_path ) +1 );		// '+1' for terminal NUL on string.

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_lstat", arg );
	    //
	    status = lstat(c_path, &buf);
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_lstat" );

	unbuffer_mythryl_heap_value( &path_buf );
    }

    if (status < 0)   return RAISE_SYSERR__MAY_HEAPCLEAN(task, statusf, NULL);

    return  mkStatRep( task, &buf );
}




// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

