/* gettime.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "vproc-state.h"
#include "runtime-state.h"
#include "runtime-timer.h"
#include "cfun-proto-list.h"

/* _lib7_Time_gettime : Void -> (int32.Int * int * int32.Int * int * int32.Int * int)
 *
 * Return this process's CPU time consumption
 * so far, broken down as:
 *     User-mode          seconds and microseconds.
 *     Kernel-mode        seconds and microseconds.
 *     Garbage collection seconds and microseconds.
 * used by this process so far.
 */
lib7_val_t _lib7_Time_gettime (lib7_state_t *lib7_state, lib7_val_t arg)
{
    Time_t		usr;					/* User-mode   time consumption as reported by os.				*/
    Time_t		sys;					/* Kernel-mode time consumption as reported by os.				*/

    lib7_val_t		usr_seconds;
    lib7_val_t		sys_seconds;
    lib7_val_t		gc_seconds;

    lib7_val_t		result;					/* For result 6-vector.								*/

    vproc_state_t	*vsp = lib7_state->lib7_vproc;

								/* On posix: get_cpu_time()	def in   src/runtime/main/unix-timers.c		*/
								/* On win32: get_cpu_time()      def in   src/runtime/main/win32-timers.c	*/
    get_cpu_time (&usr, &sys);

    INT32_ALLOC (lib7_state, usr_seconds, usr.seconds            );
    INT32_ALLOC (lib7_state, sys_seconds, sys.seconds            );
    INT32_ALLOC (lib7_state, gc_seconds,  vsp->vp_gcTime->seconds);

    REC_ALLOC6 (lib7_state, result,
	usr_seconds, INT_CtoLib7(usr.uSeconds),
	sys_seconds, INT_CtoLib7(sys.uSeconds),
	gc_seconds,  INT_CtoLib7(vsp->vp_gcTime->uSeconds)
    );

    return result;
}



/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
