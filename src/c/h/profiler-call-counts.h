// profiler-call-counts.h


#ifndef PROFILER_CALL_COUNTS_H
#define PROFILER_CALL_COUNTS_H

#ifndef MICROSECONDS_PER_SIGVTALRM
#define MICROSECONDS_PER_SIGVTALRM 10000						// This gets used (only) in   src/c/lib/space-and-time-profiling/libmythryl-space-and-time-profiling.c
#endif

extern Val	time_profiling_rw_vector__global;					// This gets set in   src/c/lib/space-and-time-profiling/libmythryl-space-and-time-profiling.c

// Indices into the time_profiling_rw_vector__global
// for the run-time and cleaner.  
// 
// These need to track the definitions in
//
//     src/lib/std/src/nj/runtime-profiling-control.pkg
//
#define IN_RUNTIME_PROFILE_INDEX		TAGGED_INT_FROM_C_INT(0)		// Must match    in_runtime_profile_index		from   src/lib/std/src/nj/runtime-profiling-control.pkg
#define IN_MINOR_HEAPCLEANER_PROFILE_INDEX	TAGGED_INT_FROM_C_INT(1)		// Must match    in_minor_heapcleaner_profile_index	from   src/lib/std/src/nj/runtime-profiling-control.pkg
#define IN_MAJOR_HEAPCLEANER_PROFILE_INDEX	TAGGED_INT_FROM_C_INT(2)		// Must match    in_major_heapcleaner_profile_index	from   src/lib/std/src/nj/runtime-profiling-control.pkg
#define IN_OTHER_CODE_PROFILE_INDEX		TAGGED_INT_FROM_C_INT(3)		// Must match    in_other_code_profile_index		from   src/lib/std/src/nj/runtime-profiling-control.pkg

#endif // PROFILER_CALL_COUNTS_H



// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

