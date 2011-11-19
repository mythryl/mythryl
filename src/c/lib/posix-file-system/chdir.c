// chdir.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif


// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_chdir   (Task* task,  Val arg) {
    //=====================
    //
    // Mythryl type:   String -> Void
    //
    // Change working directory.
    //
    // This fn gets bound as   change_directory   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

    char* dir = HEAP_STRING_AS_C_STRING( arg );

//  CEASE_USING_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_chdir" );
	//
        int status = chdir( dir );			// NB: Before uncommenting CEASE/BEGIN here, we'd need to copy 'dir' into a C buffer.
	//
//  BEGIN_USING_MYTHRYL_HEAP( task->pthread, "_lib7_P_FileSys_chdir" );
    //
    CHECK_RETURN_UNIT(task, status)
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

