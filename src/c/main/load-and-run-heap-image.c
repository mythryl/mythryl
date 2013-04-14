// load-and-run-heap-image.c

#include "../mythryl-config.h"

#include <stdio.h>
#include "runtime-base.h"
#include "heapcleaner.h"
#include "heap-io.h"

// This fun is called (only) from:
//
//     src/c/main/runtime-main.c
// 
void   load_and_run_heap_image__may_heapclean (
    // ======================================
    //
    const char*	        heap_image_to_run_filename,
    Heapcleaner_Args*   heap_parameters,
    Roots*              extra_roots
) {
    // Load a heap image from a file and resume execution.
    //
    // The arguments
    //
    //     agegroup0_buffer_bytesize
    //     active_agegroups
    //     oldest_agegroup_retaining_fromspace_sibs_between_heapcleanings
    //
    // are possible command-line overrides of the heap parameters
    // specified in the image being imported.
    //
    // (Non-negative values signify override.)
    //
    // This function is called in only one place, in
    //     src/c/main/runtime-main.c

    Task* task
	=
	import_heap_image__may_heapclean(									// import_heap_image__may_heapclean				is from   src/c/heapcleaner/import-heap.c
	    //
	    heap_image_to_run_filename,
	    heap_parameters,
	    extra_roots
	);


    set_up_fault_handlers ();

    #ifdef SIZES_C_64_MYTHRYL_32
	// Patch the 32-bit addresses:
        //
	patch_static_heapchunk_32_bit_addresses ();
    #endif

	run_mythryl_task_and_runtime_eventloop__may_heapclean( task, extra_roots );				// run_mythryl_task_and_runtime_eventloop__may_heapclean	def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c
}


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.


