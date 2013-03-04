// listerrors.c
//
// Return the list of system constants that represents the known error codes.


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

extern Sysconsts	errno_sysconsts_table__global;				// errno_sysconsts_table__global	is from   src/c/lib/posix-error/errno-sysconsts-table.c



Val   _lib7_P_Error_listerrors   (Task* task,  Val arg)   {
    //========================
    //
    // Mythryl type:   Int -> List(Sys_Const)
    //
    // This fn gets bound in:
    //
    //     src/lib/std/src/psx/posix-error.pkg   

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    Val result =  dump_table_as_system_constants_list__may_heapclean( task, &errno_sysconsts_table__global, NULL );		// dump_table_as_system_constants_list__may_heapclean		def in    src/c/heapcleaner/make-strings-and-vectors-etc.c

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.

