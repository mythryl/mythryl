// listerrors.c
//
// Return the list of system constants that represents the known error codes.


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

extern Sysconsts	errno_table__global;				// See  src/c/lib/posix-error/tbl-errno.c



Val   _lib7_P_Error_listerrors   (Task* task,  Val arg)   {
    //========================
    //
    // Mythryl type:   Int -> List(Sys_Const)
    //
    // This fn gets bound in:
    //
    //     src/lib/std/src/psx/posix-error.pkg   

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_P_Error_listerrors");

    return   dump_table_as_system_constants_list__may_heapclean( task, &errno_table__global, NULL );		// dump_table_as_system_constants_list__may_heapclean		def in    src/c/heapcleaner/make-strings-and-vectors-etc.c
}


// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

