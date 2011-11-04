// heapcleaner-statistics.h


#ifndef CLEANER_STATISTICS_H
#define CLEANER_STATISTICS_H

#include "../mythryl-config.h"
#include "heapcleaner-statistics-2.h"

/*
###             "Should array indices start at 0 or 1?
###              My compromise of 0.5 was rejected without,
###              I thought, proper consideration."
###
###                            -- Stan Kelly-Bootle
*/

#if !NEED_HEAPCLEANER_PAUSE_STATISTICS
    //
    inline void   note_when_heapcleaning_started            (Heap* heap)               {}
    inline void   note_when_cleaning_completed          (void)                     {}
    inline void   note_active_agegroups_count_for_this_timesample (Unt1 active_agegroups)   {} 

#else					// NEED_HEAPCLEANER_PAUSE_STATISTICS

   
    inline void   note_when_heapcleaning_started   (Heap* heap)   {
        //        ==============================
	//
        // Called (only) from:    src/c/heapcleaner/call-heapcleaner.c

	if (heapcleaner_statistics_generation_switch__global) {
	    //
	    Heapcleaner_Statistics* stats										// Heapcleaner_Statistics		def in    src/c/h/heapcleaner-statistics-2.h
		=
		&heapcleaner_statistics_buffer__global[ heapcleaner_statistics_buffer_record_count__global ];						// heapcleaner_statistics_buffer__global		def in    src/c/heapcleaner/heapcleaner-initialization.c

	    Punt  bytes
		=
                (Punt) task->heap_allocation_pointer
		-
		(Punt) heap->agegroup0_buffer;

	    INCREASE_BIGCOUNTER( &total_bytes_allocated, bytes );

	    stats->bytes_allocated
		=
		heap->total_bytes_allocated;

	    stats->active_agegroups = 0;

	    gettimeofday( &stats->start_time, NULL );
	}
    }

    inline void   note_when_cleaning_completed   (void)   {
        //
        // Called (only) from:    src/c/heapcleaner/call-heapcleaner.c
	//
	if (heapcleaner_statistics_generation_switch__global) {
	    //
	    gettimeofday( &heapcleaner_statistics_buffer__global[ heapcleaner_statistics_buffer_record_count__global ].stop_time, NULL );
	    //
	    STATS_FINISH();
	}
    }

    inline void   note_active_agegroups_count_for_this_timesample   (Unt1 active_agegroups) {
        //
        // Called (only) from:    src/c/heapcleaner/heapclean-n-agegroups.c
	//
	if (heapcleaner_statistics_generation_switch__global) {
	    //
	    heapcleaner_statistics_buffer__global[ heapcleaner_statistics_buffer_record_count__global ].active_agegroups
		=
		active_agegroups;
	}
    }

#endif 					// NEED_HEAPCLEANER_PAUSE_STATISTICS

#endif	// CLEANER_STATISTICS_H


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

