// cleaner-statistics.h


#ifndef CLEANER_STATISTICS_H
#define CLEANER_STATISTICS_H

#include "cleaner-statistics-2.h"

/*
###             "Should array indices start at 0 or 1?
###              My compromise of 0.5 was rejected without,
###              I thought, proper consideration."
###
###                            -- Stan Kelly-Bootle
*/

#ifndef KEEP_CLEANER_PAUSE_STATISTICS
    //
    inline void   note_when_cleaning_started            (Heap* heap)               {}
    inline void   note_when_cleaning_completed          (void)                     {}
    inline void   note_active_agegroups_count_for_this_timesample (Unt1 active_agegroups)   {} 

#else					// KEEP_CLEANER_PAUSE_STATISTICS

   
    inline void   note_when_cleaning_started   (Heap* heap)   {
        //        ==============================
	//
        // Called (only) from:    src/c/cleaner/call-cleaner.c

	if (cleaner_statistics_generation_switch) {
	    //
	    Cleaner_Statistics* stats									// Cleaner_Statistics	def in    src/c/h/cleaner-statistics-2.h
		=
		&statistics_buffer[ statistics_buffer_record_count ];						// statistics_buffer		def in    src/c/cleaner/cleaner-initialization.c

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
        // Called (only) from:    src/c/cleaner/call-cleaner.c
	//
	if (cleaner_statistics_generation_switch) {
	    //
	    gettimeofday( &statistics_buffer[ statistics_buffer_record_count ].stop_time, NULL );
	    //
	    STATS_FINISH();
	}
    }

    inline void   note_active_agegroups_count_for_this_timesample   (Unt1 active_agegroups) {
        //
        // Called (only) from:    src/c/cleaner/clean-n-agegroups.c
	//
	if (cleaner_statistics_generation_switch) {
	    //
	    statistics_buffer[ statistics_buffer_record_count ].active_agegroups
		=
		active_agegroups;
	}
    }

#endif 					// KEEP_CLEANER_PAUSE_STATISTICS

#endif	// CLEANER_STATISTICS_H


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

