// export-fun.c

#include "../../config.h"

#include "system-dependent-stuff.h"
#include <stdio.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "task.h"
#include "heap-io.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"


// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c


Val   _lib7_runtime_export_fun   (Task* task,  Val arg)   {    // : 
    //========================
    //
    // Mythryl type:  (String, ((String, List(String)) -> Int)) -> Void
    // or maybe:      (String, (List(String) -> Void)) -> Void              XXX BUGGO FIXME figure out which, then if needed also update   src/c/lib/heap/cfun-list.h
    //
    // Save the current heap in a diskfile, tweaked
    // to begin execution with the given function
    // when next loaded and executed.
    //
    // This fn get bound to   spawn_to_disk'   in:
    //
    //     src/lib/std/src/nj/export.pkg
    //

    char	cwd[      1024 ];
    char	filename[ 1024 ];

    Val	lib7_filename = GET_TUPLE_SLOT_AS_VAL( arg, 0 );
    Val	funct          = GET_TUPLE_SLOT_AS_VAL( arg, 1 );

    FILE*	file;
    int		status;

    if (!getcwd( cwd, 1024 )) { strcpy( cwd, "." ); }

    strcpy( filename, HEAP_STRING_AS_C_STRING(lib7_filename) );

    fprintf(
        stderr,
        "\n                            export-fun.c:   Writing   executable (heap image) %s/%s\n\n",
        cwd,
        filename
    );

    if (!(file = fopen(filename, "wb"))) {
      return RAISE_ERROR( task, "Unable to open file for writing");
    }

    status =  export_fn_image( task, funct, file );				// export_fn_image	def in   src/c/cleaner/export-heap.c

    fclose (file);

    if (status == SUCCESS) 	print_stats_and_exit( 0 );
    else	                die( "Export-fn call failed" );

    // NB: It would be nice to raise a RUNTIME_EXCEPTION exception here,
    // but the Mythryl state has been trashed as a side-effect of the
    // export operation.
    //	    return RAISE_ERROR(task, "export failed");

    exit(1);			// Cannot execute; just here to prevent a compiler warning.
}


// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

