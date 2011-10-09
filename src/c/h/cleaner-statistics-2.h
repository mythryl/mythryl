// cleaner-statistics-2.h


#ifndef CLEANER_STATISTICS_2_H
#define CLEANER_STATISTICS_2_H

#ifndef RUNTIME_BASE_H
    typedef  unsigned int  Unt1;
#endif

#include "runtime-timer.h"
#include "bigcounter.h"

// Cleaner_Statistics_Header
//
typedef struct {				// 
    Time	start_time;			// Time of initialization.   Type 'Time' is defined in   src/c/h/runtime-timer.h
    Unt1	mask;				// Bitmask telling which things were measured.
    Unt1	is_new_runtime;			// TRUE iff this is the new runtime new runtime parameters.
    Unt1	agegroup0_buffer_bytesize;	// Size of the allocation space.
    Unt1	active_agegroups;		// Number of agegroups.
    //						// Old runtime parameters.
    Unt1	softmax;
    Unt1	ratio;
    Unt1	pad[8];				// Pad to 64 bytes.
    //
} Cleaner_Statistics_Header;


// Cleaner_Statistics
//
typedef struct {
    Bigcounter	bytes_allocated;		// Allocation count in bytes.
    Unt1	active_agegroups;		// Number of agegroups cleaned.
    Time	start_time;	
    Time	stop_time;	
    Unt1	pad[9];				// Pad to 64 bytes.
    //
} Cleaner_Statistics;

// Maskbits in header
//
#define STATMASK_ALLOC	0x01
#define STATMASK_NGENS	0x02
#define STATMASK_START	0x04
#define STATMASK_STOP	0x08




//
#define STATISTICS_BUFFER_SIZE_IN_RECORDS	(2048/sizeof(Cleaner_Statistics))

extern Bool	    cleaner_statistics_generation_switch;	// If TRUE, generate statistics.
extern int	    cleaner_statistics_fd;			// The file descriptor to write the data to.

extern Cleaner_Statistics   statistics_buffer[];		// Buffer for data.
extern int	    statistics_buffer_record_count;					// Number of records in the buffer.

// Flush out any records in the buffer:
//
#define STATS_FLUSH_BUF()	{						\
	if (statistics_buffer_record_count >= 0) {							\
	    write (cleaner_statistics_fd, (char*)statistics_buffer, statistics_buffer_record_count*sizeof(Cleaner_Statistics));	\
	    statistics_buffer_record_count = 0;							\
	}									\
    }

#define STATS_FINISH()	{							\
	if (++statistics_buffer_record_count >= STATISTICS_BUFFER_SIZE_IN_RECORDS) {					\
	    write (cleaner_statistics_fd, (char *)statistics_buffer, STATISTICS_BUFFER_SIZE_IN_RECORDS*sizeof(Cleaner_Statistics));	\
	    statistics_buffer_record_count = 0;							\
	}									\
    }



#endif		// CLEANER_STATISTICS_2_H



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

