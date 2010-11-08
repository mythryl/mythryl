/* runtime-timer.h
 */

#ifndef _LIB7_TIMER_
#define _LIB7_TIMER_

#ifndef _LIB7_BASE_
#include "runtime-base.h"
#endif

/* We define our own type to represent time values,
 * since some systems have
 *     struct timeval
 * but others do not.
 */
typedef struct {
    Int32_t	seconds;
    Int32_t	uSeconds;
} Time_t;

extern void get_cpu_time (Time_t *user_t, Time_t *sys_t);
extern void start_garbage_collection_timer (vproc_state_t *vsp);
extern void stop_garbage_collection_timer (vproc_state_t *vsp, long *time);

#endif /* !_LIB7_TIMER_ */


/* COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
