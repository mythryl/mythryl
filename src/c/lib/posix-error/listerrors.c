// listerrors.c
//
// Return the list of system constants that represents the known error codes.


#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

extern System_Constants_Table	errno_table_global;				// See  src/c/lib/posix-error/tbl-errno.c



Val   _lib7_P_Error_listerrors   (Task* task,  Val arg)   {
    //========================
    //
    // Mythryl type:   Int -> List(Sys_Const)
    //
    // This fn gets bound in:
    //
    //     src/lib/std/src/posix-1003.1b/posix-error.pkg   

    return   dump_table_as_system_constants_list( task, &errno_table_global );			// dump_table_as_system_constants_list		def in    src/c/cleaner/make-strings-and-vectors-etc.c
}


// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

