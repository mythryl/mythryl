// errmsg.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"



// One of the library bindings exported via
//     src/c/lib/posix-error/cfun-list.h
// and thence
//     src/c/lib/posix-error/libmythryl-posix-error.c



Val   _lib7_P_Error_errmsg   (Task* task, Val arg)   {
    //====================
    //
    // Mythryl type:   Int -> String
    //
    // Return the OS-dependent error message associated with error.
    //
    // This fn gets bound as   errmsg   in:
    //
    //     src/lib/std/src/psx/posix-error.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int errnum =  TAGGED_INT_TO_C_INT( arg );
    Val result;

    #if defined( HAS_STRERROR )
	//
	char* msg = strerror( errnum );
	//
	if (msg != 0) {
	    //
	    result = make_ascii_string_from_c_string__may_heapclean( task, msg, NULL );				// make_ascii_string_from_c_string__may_heapclean	def in    src/c/heapcleaner/make-strings-and-vectors-etc.c
	} else {
	    char     buf[64];
	    sprintf( buf, "<unknown error %d>", errnum);				// XXX SUCKO FIXME should use a modern fn proof against buffer overrun.
	    result = make_ascii_string_from_c_string__may_heapclean (task, buf, NULL );
	}
    #else
	if (0 <= errnum  &&  errnum < sys_nerr) {
	    //
	    result = make_ascii_string_from_c_string__may_heapclean (task, sys_errlist[errnum], NULL );
	    //
	} else {
	    //
	    char     buf[64];
	    snprintf( buf, 64, "<unknown error %d>", errnum);
	    result = make_ascii_string_from_c_string__may_heapclean (task, buf, NULL );
	}
    #endif
									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);

    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

