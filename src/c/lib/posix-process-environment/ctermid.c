// ctermid.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>

#include "system-dependent-unix-stuff.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
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
    //     src/lib/std/src/psx/posix-id.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    char* status;
    char  name[ L_ctermid ];

    RELEASE_MYTHRYL_HEAP( task->hostthread, __func__, NULL );
	//
	status = ctermid( name );
	//
    RECOVER_MYTHRYL_HEAP( task->hostthread, __func__ );

    if (status == NULL || *status == '\0') {
        //
	return RAISE_ERROR__MAY_HEAPCLEAN(task, "cannot determine controlling terminal", NULL);
    }
  
    Val result =  make_ascii_string_from_c_string__may_heapclean( task, name, NULL );		// make_ascii_string_from_c_string__may_heapclean	def in    src/c/heapcleaner/make-strings-and-vectors-etc.c


									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
 * released per terms of SMLNJ-COPYRIGHT.
 */
