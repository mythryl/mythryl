/* export-heap.c
 *
 */

#include "../../config.h"

#include "runtime-osdep.h"
#include <stdio.h>
#include <string.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-state.h"
#include "heap-io.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"


lib7_val_t   _lib7_runtime_export_heap   (   lib7_state_t*   lib7_state,
                                               lib7_val_t arg
                                           )
{
    /* _lib7_runtime_export_heap : String -> Bool
     *
     * Export the world to the given file and return false (the exported version
     * returns true).
     */

    char	fname[1024];
    FILE	*file;

    strcpy(fname, STR_LIB7toC(arg)); /* XXX BUGGO FIXME no buffer overflow check! */

    fprintf(stderr,"\n");
    fprintf(stderr,"------------------------------------------------------------------------------------------------------\n");
    fprintf(stderr," export-heap.c:_lib7_runtime_export_heap:   Writing file '%s'\n",fname);
    fprintf(stderr,"------------------------------------------------------------------------------------------------------\n");

    if (!(file = fopen(fname, "wb"))) {
      return RAISE_ERROR(lib7_state, "unable to open file for writing");
    }

    lib7_state->lib7_argument = LIB7_true;

    {   int status = ExportHeapImage (lib7_state, file);
        fclose (file);

	if (status == SUCCESS)   return LIB7_false;
	else                     return RAISE_ERROR( lib7_state, "export failed");
    }
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
