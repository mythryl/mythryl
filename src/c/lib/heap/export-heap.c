// export-heap.c

#include "../../mythryl-config.h"

#include "system-dependent-stuff.h"
#include <stdio.h>
#include <string.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-io.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c



Val   _lib7_runtime_export_heap   (Task* task,  Val arg)   {
    //=========================
    //
    // Mythryl type: String -> Bool
    //
    // Export the world to the given file and return FALSE.
    // The exported version returns TRUE when restarted.
    //
    // This fn gets bound to   export_heap   in:
    //
    //     src/lib/std/src/nj/export.pkg

    char  fname[ 1024 ];
    FILE* file;

    strcpy(fname, HEAP_STRING_AS_C_STRING(arg)); // XXX BUGGO FIXME no buffer overflow check!

    fprintf(stderr,"\n");
    fprintf(stderr,"------------------------------------------------------------------------------------------------------\n");
    fprintf(stderr," export-heap.c:_lib7_runtime_export_heap:   Writing file '%s'\n",fname);
    fprintf(stderr,"------------------------------------------------------------------------------------------------------\n");

    if (!(file = fopen(fname, "wb"))) {
        //
        return RAISE_ERROR(task, "unable to open file for writing");
    }

    task->argument = HEAP_TRUE;

    int status =   export_heap_image( task, file );					// export_heap_image		def in    src/c/heapcleaner/export-heap.c

    fclose (file);

    if (status == SUCCESS)   return HEAP_FALSE;
    else                     return RAISE_ERROR( task, "export failed");
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

