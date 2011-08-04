// profiler-call-counts.h


#ifndef PROFILER_CALL_COUNTS_H
#define PROFILER_CALL_COUNTS_H

#ifndef MICROSECONDS_PER_SIGVTALRM
#define MICROSECONDS_PER_SIGVTALRM 10000			// This gets used (only) in   src/c/lib/space-and-time-profiling/libmythryl-space-and-time-profiling.c
#endif

extern Val	time_profiling_rw_vector_global;		// This gets set in   src/c/lib/space-and-time-profiling/libmythryl-space-and-time-profiling.c

// Indices into the time_profiling_rw_vector_global
// for the run-time and cleaner.  
// 
// These need to track the definitions in
//
//     src/lib/std/src/nj/runtime-profiling-control.pkg
//
#define PROF_RUNTIME		INT31_FROM_C_INT(0)		// Must match    runtime_index		from   src/lib/std/src/nj/runtime-profiling-control.pkg
#define PROF_MINOR_CLEANING	INT31_FROM_C_INT(1)		// Must match    minor_cleaning_index	from   src/lib/std/src/nj/runtime-profiling-control.pkg
#define PROF_MAJOR_CLEANING	INT31_FROM_C_INT(2)		// Must match    major_cleaning_index	from   src/lib/std/src/nj/runtime-profiling-control.pkg
#define PROF_OTHER		INT31_FROM_C_INT(3)		// Must match    other_index		from   src/lib/std/src/nj/runtime-profiling-control.pkg

#endif // PROFILER_CALL_COUNTS_H



// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

