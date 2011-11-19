// closedir.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_closedir   (Task* task,  Val arg) {
    //========================
    //
    // Mythryl type:  Chunk -> Void
    //
    // Close a directory stream.
    //
    // This fn gets bound as   closedir'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

//  CEASE_USING_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_closedir" );
	//
        int status = closedir(PTR_CAST(DIR*, arg));				// NB: Before uncommenting CEASE/BEGIN here, we'd have to copy 'arg' to a C buffer.
	//
//  BEGIN_USING_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_closedir" );
    //
    CHECK_RETURN_UNIT(task,status)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

