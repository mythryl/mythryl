/* timeofday.c
 *
 * See wikipedia article
 *
 *     http://en.wikipedia.org/wiki/Time_Stamp_Counter
 *
 * Among other things it recommends the newer
 *
 *    clock_gettime( CLOCK_MONOTONIC );
 *    clock_gettime( CLOCK_PROCESS_CPUTIME_ID );
 *    clock_gettime( CLOCK_THREAD_CPUTIME_ID );
 *
 * calls on Linux, points to the MIT-contributed file  cycle.h   at
 *
 *     http://www.fftw.org/cycle.h
 *
 * for getting accurate times portably, and points to the HPET page
 *
 *     http://en.wikipedia.org/wiki/HPET
 *
 */

#include "../../config.h"

#  include "runtime-osdep.h"
#if defined(HAS_GETTIMEOFDAY)
#  if defined(OPSYS_WIN32)
#if HAVE_SYS_TYPES_H
#    include <sys/types.h>
#endif
#if HAVE_SYS_TIMEB_H
#    include <sys/timeb.h>
#endif
#  else
#if HAVE_SYS_TIME_H
#    include <sys/time.h>
#endif
#  endif
#else
#  error no timeofday mechanism
#endif   
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"

/* _lib7_time_gettimeofday -- time as (seconds,microseconds).
 *
 * We break this out in a function separate from
 * _lib7_Time_timeofday so as to have it available
 * at the C level for the benefit of modules like
 * src/runtime/c-libs/lib7-socket/print-if.c
 */
int   _lib7_time_gettimeofday   (int* microseconds)
{
    int			c_sec;
    int			c_usec;

#ifdef HAS_GETTIMEOFDAY
#if defined(OPSYS_UNIX)
    {
	struct timeval	t;

	gettimeofday (&t, NULL);
	c_sec = t.tv_sec;
	c_usec = t.tv_usec;
    }
#elif defined(OPSYS_WIN32)
  /* we could use Win32 GetSystemTime/SystemTimetoFileTime here,
   * but the conversion routines for 64-bit 100-ns values
   * (in the keyed_map dll) are non-Win32s
   *
   * we'll use time routines from the C runtime for now.
   */
    {
	struct _timeb t;

	_ftime(&t);
	c_sec = t.time;
	c_usec = t.millitm*1000;
    }
#else
#error timeofday not defined for OS
#endif
#else
#error no timeofday mechanism
#endif

    *microseconds = c_usec;
    return c_sec;
}


/* _lib7_Time_timeofday : Void -> (int32::Int, Int)
 *
 * Return the time of day.
 * NOTE: gettimeofday() is not POSIX (time() returns seconds, and is POSIX
 * and ISO C).
 */
lib7_val_t _lib7_Time_timeofday (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int			c_seconds;
    int			c_microseconds;
    lib7_val_t		lib7_seconds;
    lib7_val_t		result;

    c_seconds = _lib7_time_gettimeofday( &c_microseconds );

    INT32_ALLOC(lib7_state, lib7_seconds, c_seconds);
    REC_ALLOC2 (lib7_state, result, lib7_seconds, INT_CtoLib7(c_microseconds));

    return result;
}



/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
