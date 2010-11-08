/* export-fun.c
 *
 */

#include "../../config.h"

#include "runtime-osdep.h"
#include <stdio.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-state.h"
#include "heap-io.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"


lib7_val_t   _lib7_runtime_export_fun   (   lib7_state_t*   lib7_state,
                                              lib7_val_t      arg
                                          )
{
    /* _lib7_runtime_export_fun : (String * ((String * String list) -> int)) -> Void
     *
     * Save the current heap in a diskfile, tweaked
     * to begin execution with the given function
     * when next loaded and executed.
     */

    char	cwd[      1024 ];
    char	filename[ 1024 ];

    lib7_val_t	lib7_filename = REC_SEL( arg, 0 );
    lib7_val_t	funct          = REC_SEL( arg, 1 );

    FILE*	file;
    int		status;

    if (!getcwd( cwd, 1024 )) { strcpy( cwd, "." ); }

    strcpy( filename, STR_LIB7toC(lib7_filename) );

    fprintf(
        stderr,
        "\n    .../c-libs/lib7-runtime/export-fun.c:   Writing   executable (heap image) %s/%s\n\n",
        cwd,
        filename
    );

    if (!(file = fopen(filename, "wb"))) {
      return RAISE_ERROR( lib7_state, "Unable to open file for writing");
    }

    status = ExportFnImage (lib7_state, funct, file);
    fclose (file);

    if (status == SUCCESS) 	Exit (0);
    else	                Die ("Export-fn call failed");

    /* NB: It would be nice to raise a SYSTEM_ERROR exception here,
     * but the Lib7 state has been trashed as a side-effect of
     * the export operation.
     *	    return RAISE_ERROR(lib7_state, "export failed");
     */
}


/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
