// heapcleaner-initialization.c
//
// The heapcleaner initialization code.

/*
###         "Weave a circle round him thrice,
###            And close your eyes with holy dread,
###          For he on honey-dew hath fed,
###            And drunk the milk of Paradise."
###
###                          -- Coleridge
*/

#if NEED_HEAPCLEANER_PAUSE_STATISTICS		// Cleaner pause statistics are UNIX dependent.
    #include "system-dependent-unix-stuff.h"
#endif

#include "../mythryl-config.h"

#include <stdarg.h>
#include <string.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "runtime-base.h"
#include "runtime-commandline-argument-processing.h"
#include "runtime-configuration.h"
#include "get-quire-from-os.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "bigcounter.h"
#include "heap.h"
#include "runtime-globals.h"
#include "runtime-timer.h"
#include "heapcleaner-statistics.h"

static int  default_agegroup_size_ratio__local[ MAX_AGEGROUPS ]
    =
  {     DEFAULT_AGEGROUP_SIZE_RATIO1,					// These values are defined in   src/c/h/runtime-configuration.h
	DEFAULT_AGEGROUP_SIZE_RATIO2,
	DEFAULT_AGEGROUP_SIZE_RATIO,
	DEFAULT_AGEGROUP_SIZE_RATIO,
	DEFAULT_AGEGROUP_SIZE_RATIO,
	DEFAULT_AGEGROUP_SIZE_RATIO,
	DEFAULT_AGEGROUP_SIZE_RATIO
    };

#ifdef TWO_LEVEL_MAP
    #  error two level map not supported
#else
Sibid* book_to_sibid__global;
#endif

									// Should this go into heapcleaner-statistics.c ?
Bool	heapcleaner_statistics_generation_switch__global = TRUE;	// If TRUE, then generate stats.
int	heapcleaner_statistics_fd__global = -1;				// The file descriptor to write the data to.
int	heapcleaner_statistics_buffer_record_count__global;				// Number of records in the buffer.

Heapcleaner_Statistics   heapcleaner_statistics_buffer__global [ HEAPCLEANER_STATISTICS_BUFFER_SIZE_IN_RECORDS ];			// Buffer of data.



Heapcleaner_Args*   handle_heapcleaner_commandline_arguments   (char **argv) {
    //              ========================================
    //
    // Parse any heapcleaner args from the user commandline:

    char     option[ MAX_COMMANDLINE_ARGUMENT_PART_LENGTH ];
    char*    option_arg;
    Bool     seen_error = FALSE;
    char*    arg;

    Heapcleaner_Args* params;

    if ((params = MALLOC_CHUNK(Heapcleaner_Args)) == NULL) {
	die("unable to allocate heap_params");
    }

    // We use 0 or "-1" to signify that
    // the default value should be used:
    //
    params->agegroup0_buffer_bytesize = 0;
    params->active_agegroups = -1;
    params->oldest_agegroup_keeping_idle_fromspace_buffers = -1;

    #define MATCH(opt)	(strcmp(opt, option) == 0)
    #define CHECK(opt)	{						\
	if (option_arg[0] == '\0') {					\
	    seen_error = TRUE;						\
	    say_error("missing argument for \"%s\" option\n", opt);		\
	    continue;							\
	}								\
    }			// CHECK

    while ((arg = *argv++) != NULL) {
	//								// is_runtime_option		def in    src/c/main/runtime-options.c
        if (is_runtime_option(arg, option, &option_arg)) {		// A runtime option is one starting with "--runtime-".
	    //
	    if (MATCH("gc-gen0-bufsize")) { 				// Set cleaner agegroup0 buffer size.
		//
		CHECK("gc-gen0-bufsize");

		params->agegroup0_buffer_bytesize
		    =
                    get_size_option( ONE_K_BINARY, option_arg );

		if (params->agegroup0_buffer_bytesize < 0) {
		    //
		    seen_error = TRUE;
		    say_error( "bad argument for \"--runtime-gc-gen0-bufsize\" option\n" );
		}

	    } else if (MATCH("gc-generations")) {

		CHECK("gc-generations");
		params->active_agegroups = atoi(option_arg);
		if (params->active_agegroups < 1)
		    params->active_agegroups = 1;
		else if (params->active_agegroups > MAX_ACTIVE_AGEGROUPS)
		    params->active_agegroups = MAX_ACTIVE_AGEGROUPS;

	    } else if (MATCH("vmcache")) {

		CHECK("vmcache");
		params->oldest_agegroup_keeping_idle_fromspace_buffers = atoi(option_arg);
		if (params->oldest_agegroup_keeping_idle_fromspace_buffers < 0) {
		    params->oldest_agegroup_keeping_idle_fromspace_buffers = 0;
		} else if (params->oldest_agegroup_keeping_idle_fromspace_buffers > MAX_ACTIVE_AGEGROUPS) {
		    params->oldest_agegroup_keeping_idle_fromspace_buffers = MAX_ACTIVE_AGEGROUPS;
		}
	    } else if (MATCH("unlimited-heap")) {

		unlimited_heap_is_enabled__global = TRUE;
	    }
	}

	if (seen_error)  return NULL;
    }								// while

    return params;
}


static void   clear_heapcleaner_statistics   (Heap* heap)  {
    //        ============================
    //
    ZERO_BIGCOUNTER( &heap->total_bytes_allocated );
    //
    for     (int age = 0;  age < MAX_AGEGROUPS;   ++age) {
	for (int ilk = 0;  ilk < MAX_PLAIN_SIBS;  ++ilk) {
	    //
	    ZERO_BIGCOUNTER( &heap->total_bytes_copied_to_sib[ age ][ ilk ] );
	}
    }
}



void   set_up_heap   (			// Create and initialize the heap.
    // ===========
    //
    Task*              task,
    Bool               is_boot,
    Heapcleaner_Args*  params
) {
    // We are called (only) from
    //
    //     make_task()
    // in
    //     src/c/main/runtime-state.c

    int		max_size = 0;		// Initialized only to suppress a gcc -Wall warning.

    Heap*	heap;
    Agegroup*	ag;

    Quire*  quire;

    Val* agegroup0_master_buffer;

    // Default any parameters unspecified by user:
    //
    if (params->agegroup0_buffer_bytesize == 0)  params->agegroup0_buffer_bytesize  = DEFAULT_AGEGROUP0_BUFFER_BYTESIZE;		// From   src/c/h/runtime-configuration.h
    if (params->active_agegroups           < 0)  params->active_agegroups           = DEFAULT_ACTIVE_AGEGROUPS;				// From   src/c/h/runtime-configuration.h

    if (params->oldest_agegroup_keeping_idle_fromspace_buffers < 0) {
        params->oldest_agegroup_keeping_idle_fromspace_buffers =  DEFAULT_OLDEST_AGEGROUP_KEEPING_IDLE_FROMSPACE_BUFFERS;		// From   src/c/h/runtime-configuration.h
    }

    // First we initialize the underlying memory system:
    //
    set_up_quire_os_interface ();						// set_up_quire_os_interface	def in   src/c/ram/get-quire-from-mach.c
										// set_up_quire_os_interface	def in   src/c/ram/get-quire-from-mmap.c
										// set_up_quire_os_interface	def in   src/c/ram/get-quire-from-win32.c

    // Allocate a ram region to hold
    // the book_to_sibid__global and agegroup0 buffer:
    //
    {   long	book2sibid_bytesize;

	#ifdef TWO_LEVEL_MAP
	    #error two level map not supported
	#else
		book2sibid_bytesize = BOOK2SIBID_TABLE_SIZE_IN_SLOTS * sizeof( Sibid );
	#endif

	quire
	    =
            obtain_quire_from_os(
		//
		MAX_PTHREADS * params->agegroup0_buffer_bytesize
                +
                book2sibid_bytesize
           );

	if (quire == NULL) 	   die ("Unable to allocate ram region for book_to_sibid__global");

	book_to_sibid__global = (Sibid*) BASE_ADDRESS_OF_QUIRE( quire );

	agegroup0_master_buffer = (Val*) (((Punt)book_to_sibid__global) + book2sibid_bytesize);
    }

    // Initialize the book_to_sibid__global:
    //
    #ifdef TWO_LEVEL_MAP
        #error two level map not supported
    #else
	for (int i = 0;  i < BOOK2SIBID_TABLE_SIZE_IN_SLOTS;  i++) {
	    //
	    book_to_sibid__global[ i ] = UNMAPPED_BOOK_SIBID;
	}
    #endif

    // Initialize heap descriptor:
    //
    heap = MALLOC_CHUNK(Heap);
    //
    memset ((char*)heap, 0, sizeof(Heap));
    //
    for (int age = 0;  age < MAX_AGEGROUPS;  age++) {
	//
	if (age == 0) {   max_size = MAX_SZ1( params->agegroup0_buffer_bytesize * MAX_PTHREADS );	// MAX_SZ1 just multiplies by 6 (why??) -- def in  src/c/h/runtime-configuration.h
	} else {          max_size = (5 * max_size)/2;
	    //
	    if (max_size > 64 * ONE_MEG_BINARY)  {				// WTF? This silliness probably needs to Just Die.  XXX BUGGO FIXME. -- 2011-11-01 CrT
                max_size = 64 * ONE_MEG_BINARY;
	    }
	}

	ag			=
	heap->agegroup[ age ]	=  MALLOC_CHUNK( Agegroup );

	ag->heap		=  heap;
	ag->age			=  age + 1;
	ag->heapcleanings_count	=  0;

	ag->target_heapcleaning_frequency_ratio
	    =
	    default_agegroup_size_ratio__local[ age ];

	ag->heapcleanings_count_of_younger_agegroup_during_last_heapcleaning
	    =
	    0;
	//
	ag->tospace_quire			= NULL;
	ag->fromspace_quire		= NULL;
	ag->retained_fromspace_quire		= NULL;
	ag->coarse_inter_agegroup_pointers_map	= NULL;

	for (int s = 0;  s < MAX_PLAIN_SIBS;  s++) {				// MAX_PLAIN_SIBS		def in    src/c/h/sibid.h
	    //
	    ag->sib[ s ] = MALLOC_CHUNK( Sib );
	    //
	    ag->sib[ s ]->tospace_bytesize              = 0;
	    ag->sib[ s ]->requested_sib_buffer_bytesize = 0;
	    ag->sib[ s ]->soft_max_bytesize             = max_size;
	    //
	    ag->sib[ s ]->id =   MAKE_SIBID( age+1, s+1, 0);
	}
	for (int s = 0;  s < MAX_HUGE_SIBS;  s++) {			// MAX_HUGE_SIBS		def in    src/c/h/sibid.h
	    //
	    ag->hugechunks[ s ] = NULL;					// s = 0 == CODE__HUGE_SIB	def in    src/c/h/sibid.h
	}
    }

    for (int age = 0;   age < params->active_agegroups;   age++) {
	//
	int k = (age == params->active_agegroups -1)
                     ?  age
                     :  age+1;

	for (int ilk = 0;  ilk < MAX_PLAIN_SIBS;  ilk++) {
	    //
	    heap->agegroup[ age ]->sib[ ilk ]->sib_for_promoted_chunks
                =
                heap->agegroup[ k ]->sib[ ilk ];
	}
    }

    heap->oldest_agegroup_keeping_idle_fromspace_buffers
	=
	params->oldest_agegroup_keeping_idle_fromspace_buffers;

    heap->active_agegroups			= params->active_agegroups;
    //
    heap->agegroup0_heapcleanings_count		= 0;
    heap->hugechunk_ramregion_count		= 0;
    heap->hugechunk_ramregions			= NULL;
    //
    heap->hugechunk_freelist			= MALLOC_CHUNK( Hugechunk );
    heap->hugechunk_freelist->chunk		= (Punt)0;
    //
    heap->hugechunk_freelist->bytesize		= 0;
    heap->hugechunk_freelist->hugechunk_state	= FREE_HUGECHUNK;
    heap->hugechunk_freelist->prev		= heap->hugechunk_freelist;
    //
    heap->hugechunk_freelist->next		= heap->hugechunk_freelist;
    //
    heap->weak_pointers_forwarded_during_heapcleaning		= NULL;

    // Initialize new space:
    //
    heap->quire       =  quire;
    //
    heap->agegroup0_master_buffer    =  agegroup0_master_buffer;
    //
    heap->agegroup0_master_buffer_bytesize
        =
        params->agegroup0_buffer_bytesize * MAX_PTHREADS;			// "* MAX_PTHREADS" because it gets partitioned into MAX_PTHREADS separate buffers by
										// partition_agegroup0_buffer_between_pthreads() in   src/c/heapcleaner/pthread-heapcleaner-stuff.c
    //
    set_book2sibid_entries_for_range (
	//
	book_to_sibid__global,
	(Val*) book_to_sibid__global,
	BYTESIZE_OF_QUIRE( heap->quire ),
	AGEGROUP0_SIBID
    );


    clear_heapcleaner_statistics( heap );


    //
    if (heapcleaner_statistics_fd__global > 0) {
	//	
      Cleaner_Statistics_Header   header;									// Cleaner_Statistics_Header		is from   src/c/h/heapcleaner-statistics-2.h
	//
	ZERO_BIGCOUNTER( &heap->total_bytes_allocated );
	//
	header.mask = STATMASK_ALLOC
		    | STATMASK_NGENS
		    | STATMASK_START
		    | STATMASK_STOP;

	header.is_new_runtime = 1;
	//
	header.agegroup0_buffer_bytesize = params->agegroup0_buffer_bytesize;
	header.active_agegroups        = params->active_agegroups;
	//
	{   struct timeval tv;
	    //
	    gettimeofday ( &tv, NULL);
	    //
	    header.start_time.seconds  =  tv.tv_sec;	
	    header.start_time.uSeconds =  tv.tv_usec;	
	};
	//
	write( heapcleaner_statistics_fd__global, (char*)&header, sizeof( Cleaner_Statistics_Header ) );
    }


    if (is_boot) {
	//
	// Create agegroup 1's to-space:
	//
        for (int s = 0;  s < MAX_PLAIN_SIBS;  s++) {
	    //
	    heap->agegroup[ 0 ]->sib[ s ]->tospace_bytesize
                =
                BOOKROUNDED_BYTESIZE( 2 * (heap->agegroup0_master_buffer_bytesize / MAX_PTHREADS) );
	}

	if (allocate_and_partition_an_agegroup( heap->agegroup[0] ) == FAILURE)	    die ("unable to allocate initial agegroup 1 buffer\n");

	for (int ilk = 0;  ilk < MAX_PLAIN_SIBS;  ilk++) {
	    //
	    heap->agegroup[ 0 ]->sib[ ilk ]->fromspace_oldstuff_end
		=
		heap->agegroup[ 0 ]->sib[ ilk ]->tospace_start;
	}
    }

    // Initialize the cleaner-related
    // parts of the Mythryl state:
    //
    task->heap	                          =  heap;
    task->heap_allocation_buffer          =  task->heap->agegroup0_master_buffer;
    task->heap_allocation_pointer         =  task->heap->agegroup0_master_buffer;
    task->heap_allocation_buffer_bytesize =  heap->agegroup0_master_buffer_bytesize / MAX_PTHREADS; 

    #if NEED_SOFTWARE_GENERATED_PERIODIC_EVENTS
	//
	reset_heap_allocation_limit_for_software_generated_periodic_events( task );
    #else
	task->heap_allocation_limit
	    =
	    HEAP_ALLOCATION_LIMIT( task );
    #endif
}										// fun set_up_heap



// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


