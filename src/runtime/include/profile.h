/* profile.h
 *
 */

#ifndef _PROFILE_
#define _PROFILE_

#ifndef PROFILE_QUANTUM_US
#  define PROFILE_QUANTUM_US	10000		/* profile timer quantum in uS */
#endif

extern lib7_val_t	ProfCntArray;

/* Indices into the ProfCntArray for the
 * run-time and garbage collector.
 * These need to track the definitions in
 *
 *     src/lib/std/src/nj/profiling-control.pkg
 */
#define PROF_RUNTIME	INT_CtoLib7(0)		/* ! Must match    runtime_index		from   src/lib/std/src/nj/profiling-control.pkg	*/
#define PROF_MINOR_GC	INT_CtoLib7(1)		/* ! Must match    minor_gc_index		from   src/lib/std/src/nj/profiling-control.pkg	*/
#define PROF_MAJOR_GC	INT_CtoLib7(2)		/* ! Must match    major_gc_index		from   src/lib/std/src/nj/profiling-control.pkg	*/
#define PROF_OTHER	INT_CtoLib7(3)		/* ! Must match    other_index			from   src/lib/std/src/nj/profiling-control.pkg	*/

#endif /* _PROFILE_ */



/* COPYRIGHT (c) 1996 AT&T Research.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
