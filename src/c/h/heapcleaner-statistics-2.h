// heapcleaner-statistics-2.h


#ifndef HEAPCLEANER_STATISTICS_2_H
#define HEAPCLEANER_STATISTICS_2_H

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


// Heapcleaner_Statistics
//
typedef struct {
    Bigcounter	bytes_allocated;		// Allocation count in bytes.
    Unt1	active_agegroups;		// Number of agegroups cleaned.
    Time	start_time;	
    Time	stop_time;	
    Unt1	pad[9];				// Pad to 64 bytes.
    //
} Heapcleaner_Statistics;

// Maskbits in header
//
#define STATMASK_ALLOC	0x01
#define STATMASK_NGENS	0x02
#define STATMASK_START	0x04
#define STATMASK_STOP	0x08




//
#define HEAPCLEANER_STATISTICS_BUFFER_SIZE_IN_RECORDS	(2048/sizeof(Heapcleaner_Statistics))

extern Bool	    heapcleaner_statistics_generation_switch__global;	// If TRUE, generate statistics.
extern int	    heapcleaner_statistics_fd__global;			// The file descriptor to write the data to.

extern Heapcleaner_Statistics   heapcleaner_statistics_buffer__global[];		// Buffer for data.
extern int	    heapcleaner_statistics_buffer_record_count__global;					// Number of records in the buffer.

// Flush out any records in the buffer:
//
#define STATS_FLUSH_BUF()	{						\
	if (heapcleaner_statistics_buffer_record_count__global >= 0) {							\
	    write (heapcleaner_statistics_fd__global, (char*)heapcleaner_statistics_buffer__global, heapcleaner_statistics_buffer_record_count__global*sizeof(Heapcleaner_Statistics));	\
	    heapcleaner_statistics_buffer_record_count__global = 0;							\
	}									\
    }

#define STATS_FINISH()	{							\
	if (++heapcleaner_statistics_buffer_record_count__global >= HEAPCLEANER_STATISTICS_BUFFER_SIZE_IN_RECORDS) {					\
	    write (heapcleaner_statistics_fd__global, (char *)heapcleaner_statistics_buffer__global, HEAPCLEANER_STATISTICS_BUFFER_SIZE_IN_RECORDS*sizeof(Heapcleaner_Statistics));	\
	    heapcleaner_statistics_buffer_record_count__global = 0;							\
	}									\
    }



#endif		// HEAPCLEANER_STATISTICS_2_H



// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

