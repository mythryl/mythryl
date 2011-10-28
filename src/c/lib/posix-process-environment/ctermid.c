// ctermid.c


#include "../../config.h"

#include "system-dependent-unix-stuff.h"
#include <stdio.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-process-environment/cfun-list.h
// and thence
//     src/c/lib/posix-process-environment/libmythryl-posix-process-environment.c



Val   _lib7_P_ProcEnv_ctermid   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:   Void -> String
    //
    // Return pathname of controlling terminal.
    //
    // This fn gets bound as   ctermid   in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-id.pkg

    char                    name[ L_ctermid ];
    char* status = ctermid( name );

    if (status == NULL || *status == '\0') {
	return RAISE_ERROR(task, "cannot determine controlling terminal");
    }
  
    return   make_ascii_string_from_c_string( task, name );			// make_ascii_string_from_c_string	def in    src/c/heapcleaner/make-strings-and-vectors-etc.c
}



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
 * released under Gnu Public Licence version 3.
 */
