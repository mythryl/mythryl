/* timers.c
 *
 * OS independent timer routines; these rely on a OS dependent implementation
 * of the following function:
 *
 *	void get_cpu_time (Time_t *user_t, Time_t *sys_t);
 */

#include "../config.h"

#include "runtime-base.h"
#include "vproc-state.h"
#include "runtime-timer.h"


/* reset_timers:
 *
 * Clear the GC timers.
 */
void reset_timers (vproc_state_t *vsp)
{
    vsp->vp_gcTime->seconds = 0;
    vsp->vp_gcTime->uSeconds = 0;

} /* end of reset_timers. */


/* start_garbage_collection_timer:
 */
void start_garbage_collection_timer (vproc_state_t *vsp)
{
    get_cpu_time (vsp->vp_gcTime0, NULL);

} /* end of start_garbage_collection_timer */


/* stop_garbage_collection_timer:
 *
 * Stop the garbage collection timer and update the cumulative garbage collection
 * time.  If time is not NULL, then return the time (in ms.) spent since
 * the start of the GC.
 */
void stop_garbage_collection_timer (vproc_state_t *vsp, long *time)
{
    int			sec, usec;
    Time_t		t1;
    Time_t		*gt0 = vsp->vp_gcTime0;
    Time_t		*gt = vsp->vp_gcTime;

    get_cpu_time (&t1, NULL);

    sec = t1.seconds - gt0->seconds;
    usec = t1.uSeconds - gt0->uSeconds;

    if (time != NULL) {
	if (usec < 0) {
	    sec--; usec += 1000000;
	}
	else if (usec > 1000000) {
	    sec++; usec -= 1000000;
	}
	*time = (usec/1000 + sec*1000);
    }

    sec = gt->seconds + sec;
    usec = gt->uSeconds + usec;
    if (usec < 0) {
	sec--; usec += 1000000;
    }
    else if (usec > 1000000) {
	sec++; usec -= 1000000;
    }
    gt->seconds = sec;
    gt->uSeconds = usec;

} /* end of stop_garbage_collection_timer */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

