// runtime-configuration.h
//
// Various limits and default settings for the Mythryl run-time system.


#ifndef RUNTIME_CONFIGURATION_H
#define RUNTIME_CONFIGURATION_H

#include "../mythryl-config.h"
#include "runtime-base.h"

#define DEFAULT_IMAGE				NULL						// NULL means: Try to find in-core image using dlopen/dlsym.

#define MAX_LENGTH_FOR_A_BOOTFILE_PATHNAME	 512
#define MAX_NUMBER_OF_BOOT_FILES		1024


#define DEFAULT_ACTIVE_AGEGROUPS	5

#define MAX_ACTIVE_AGEGROUPS	14								// Should agree with MAX_AGEGROUPS in  sibid.h.

#define DEFAULT_OLDEST_AGEGROUP_RETAINING_FROMSPACE_SIBS_BETWEEN_HEAPCLEANINGS	2		// Keep idle fromspace quires for ages 1 & 2
    //
    // By design each successive agegroup contains buffers ten
    // times larger, heapcleaned ("garbage collected") one-tenth
    // as often:
    //
    //    The agegroup0 buffer  is   about  1MB and heapcleaned about 100 times/sec;
    //    The agegroup1 buffers run about  10MB and heapcleaned about  10 times/sec;
    //    The agegroup2 buffers run about 100MB and heapcleaned about   1 times/sec;
    //
    // During heapcleaning each agegroup requires twice as much ram:
    // A from-space to copy from, and a to-space to copy into.
    // Between heapcleanings, these extra copies are just waste space.
    // The policy question here is:  Is it better to retain these unused
    // buffers between heapcleanings, or to return them to the OS?
    // The considerations are:
    //
    //   o Allocating and returning them are system calls, hence by
    //     rule of thumb take a millisecond or so each.
    //
    //   o In the modern virtual memory environment, "ram" which goes
    //     unused long enough will page out to disk.  Paging it out
    //     to disk takes time -- tens of milliseconds or more -- and
    //     paging it back in when next needed will take just as much
    //     time.  Paging a 100MB buffer out might take seconds.  So
    //     So for a large buffer, returning it to the OS and then
    //     allocating it again when next needed can be MUCH faster
    //     than letting it page to and from disk.
    //
    // Clearly, for very small buffers it is better to just retain them.
    // Equally, for very large buffers it is better to return and re-allocate them.
    // So the question is just at what buffer size we should change policies.
    // The DEFAULT_OLDEST_AGEGROUP_RETAINING_FROMSPACE_SIBS_BETWEEN_HEAPCLEANINGS value here
    // controls this policy; setting it to '2' will result in buffers of 100MB
    // and below being retained and buffers of 1GB and above being returned
    // and re-allocated.
    //
    // This gets used

#define DEFAULT_AGEGROUP0_BUFFER_BYTESIZE	(256 * ONE_K_BINARY)				// Size-in-bytes for the per-core (well, per-hostthread)
												// generation-zero heap buffer.  The 256KB value is
												// ancient (1992?), but at the moment Intel level-two
												// cache sizes seem to range from 256KB to 1024MB, so
												// 256KB still seems a good choice.
												//                         -- 2011-11-01 CrT

// How much bigger should one heap agegroup be
// relative to the next-youngest agegroup?
// Agegroup0 is usually 256KB, in general
// we intend that each agegroup be about
// 10X bigger than the preceding one:								// These values are(only) used in   src/c/heapcleaner/heapcleaner-initialization.c
//
#define DEFAULT_AGEGROUP_SIZE_RATIO1	20							// agegroup-one sib buffers are small -- typically 256KB, to fit in secondary cache -- so we use a larger ratio for agegroup1.
#define DEFAULT_AGEGROUP_SIZE_RATIO2	10
#define DEFAULT_AGEGROUP_SIZE_RATIO	5
#define MAX_SZ1(NSZ)	(6*(NSZ))								// What is '6' here??   -- 2011-12-19 CrT  This is used (only) in   src/c/heapcleaner/heapcleaner-initialization.c


#define CODECHUNK_ALLOCATION_AGEGROUP	2							// Agegroup in which to allocate code chunks.

#define MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS	512						// I believe we need to have MAX_AGEGROUP0_ALLOCATION_SIZE_IN_WORDS * WORD_BYTESIZE < MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER
    //												// -- maybe something stronger than that.  -- 2011-12-20 CrT
    // Size-in-words of a "small chunk."
    // The C allocation routines allocate
    // small chunks in agegroup0,
    // while large chunks are allocated in
    // agegroup 1.

#define MIN_FREE_BYTES_IN_AGEGROUP0_BUFFER	  ((1024 + 128) * WORD_BYTESIZE)		// Referenced only in   src/c/h/heap.h


#define  MAX_C_HEAPCLEANER_ROOTS	16							// Maximum number of global C variables that can be heapcleaner ("garbage collector") roots.

#define  MAX_EXTRA_HEAPCLEANER_ROOTS_PER_HOSTTHREAD	16							// Maximum number of additional roots that can be passed to heapcleaner.

// Number of potential cleaner roots.
// This includes space for C global roots,
// Mythryl roots and the terminating null pointer.
//
#ifdef N_PSEUDO_REGS
    #define N_PSEUDO_ROOTS	N_PSEUDO_REGS
#else
    #define N_PSEUDO_ROOTS	0
#endif

// 
// We must assume that all other hostthreads
// are supplying MAX_EXTRA_HEAPCLEANER_ROOTS_PER_HOSTTHREAD
// in addition to the standard roots.
//
// This #define is referenced only in:
//
//     src/c/heapcleaner/call-heapcleaner.c   						// NROOTS	is from   src/c/h/system-dependent-root-register-indices.h
//
#define MAX_TOTAL_CLEANING_ROOTS	ROUND_UP_TO_POWER_OF_TWO(   MAX_HOSTTHREADS    * (MAX_C_HEAPCLEANER_ROOTS + NROOTS + N_PSEUDO_ROOTS) +	\
						   (MAX_HOSTTHREADS-1) * MAX_EXTRA_HEAPCLEANER_ROOTS_PER_HOSTTHREAD +1,				\
						 8 )

#if NEED_SOFTWARE_GENERATED_PERIODIC_EVENTS  
    //
    // This #define is referenced only in:
    //
    //     src/c/heapcleaner/call-heapcleaner.c
    //     src/c/heapcleaner/hostthread-heapcleaner-stuff.c
    //
    #define PERIODIC_EVENT_TIME_GRANULARITY_IN_NEXTCODE_INSTRUCTIONS   (1 << 10)     // Must be power of 2.
#endif

#endif // RUNTIME_CONFIGURATION_H



// COPYRIGHT (c) 1992 AT&T Bell Laboratories
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released per terms of SMLNJ-COPYRIGHT.


