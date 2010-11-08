/* load-and-run-heap-image.c
 *
 */

#include "../config.h"

#include <stdio.h>
#include "runtime-base.h"
#include "runtime-state.h"
#include "gc.h"
#include "heap-io.h"


void   load_and_run_heap_image (    const char*     heap_image_to_run_filename,
                                    heap_params_t*  heap_parameters
                               )
{
    /* Load a heap image from a file and resume execution.
     *
     * The arguments allocSz, numGens and cacheGen are
     * possible command-line overrides of the heap parameters
     * specified in the image being imported.
     *
     * (Non-negative values signify override.)
     */

    lib7_state_t* lib7_state = ImportHeapImage( heap_image_to_run_filename, heap_parameters );

#ifdef HEAP_MONITOR
    if (set_up_heap_monitor( lib7_state -> lib7_heap ) == FAILURE) {
	Die("Unable to start heap monitor.");
    }
#endif

    set_up_fault_handlers ();

#ifdef SIZES_C64_LIB732
    /* Patch the 32-bit addresses: */
    PatchAddresses ();
#endif

    RunLib7( lib7_state );
}


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

