// readdir.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#if HAVE_DIRENT_H
    #include <dirent.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-file-system/cfun-list.h
// and thence
//     src/c/lib/posix-file-system/libmythryl-posix-file-system.c



Val   _lib7_P_FileSys_readdir   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:   Ckit_Dirstream -> String
    //
    // Return the next filename from the directory stream.
    //
    // This fn gets bound as   readdir'   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-file.pkg
    //     src/lib/std/src/posix-1003.1b/posix-file-system-64.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_FileSys_readdir");

    struct dirent* dirent;
    
    while (TRUE) {
	errno = 0;

	RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_P_FileSys_readdir", NULL );
	    //
	    dirent = readdir(PTR_CAST(DIR*, arg));					// Note that 'arg' points into the C heap not the Mythryl heap -- check src/c/lib/posix-file-system/opendir.c 
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_P_FileSys_readdir" );

	if (dirent == NULL) {
	    if (errno != 0)  return RAISE_SYSERR__MAY_HEAPCLEAN(task, -1, NULL);	// Error occurred.
	    else	     return ZERO_LENGTH_STRING__GLOBAL;				// End of stream.
	} else {
	    char	*cp = dirent->d_name;

// SML/NJ drops "." and ".." at this point,
//           but that is alien to posix culture,
//           so I've commented it out:			-- 2008-02-23 CrT
//

//	    if ((cp[0] == '.')
//	    && ((cp[1] == '\0') || ((cp[1] == '.') && (cp[2] == '\0'))))
//		continue;
//	    else
//
	    return make_ascii_string_from_c_string__may_heapclean (task, cp, NULL);
	}
    }
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

