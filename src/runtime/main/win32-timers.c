/* win32-timers.c
 *
 * win32 specific interface to times and 
 * an interface to interval timers.
 */

#include "../config.h"

#include <windows.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_TIMEB_H
#include <sys/timeb.h>
#endif

#include "runtime-base.h"
#include "vproc-state.h"
#include "runtime-state.h"
#include "runtime-timer.h"
#include "win32-fault.h"
#include "win32-timers.h"

#include "signal-sysdep.h"
#include "system-signals.h"

#ifndef MP_SUPPORT
#define SELF_VPROC	(VProc[0])
#else
/** for MP_SUPPORT, we'll use SELF_VPROC for now **/
#define SELF_VPROC	(VProc[0])
#endif

/* for computing times */
static struct _timeb start_timeb;

/* for interval timers */
#define WIN32_TIMER_DONE 0
#define WIN32_TIMER_COUNTING 1
#define WIN32_TIMER_HALTED 2

typedef struct {
  HANDLE handle;
  DWORD id;
  int milli_secs;
  void (*action)();
} win32_timer_t;


static
void timer(win32_timer_t *ct)
{
  while (1) {
    (*ct->action)();
    Sleep(ct->milli_secs);
  }
}

static
BOOL create_win32_timer(win32_timer_t *ct,
			void (*f)(),int milli_secs,BOOL suspend)
{
  /* create a thread */
  ct->milli_secs = milli_secs;
  ct->action = f;
  return ((ct->handle = CreateThread(NULL,
				     0,     /* default stack size */
				     (LPTHREAD_START_ROUTINE) timer,
				     ct,
				     suspend ? CREATE_SUSPENDED : 0,
				     &ct->id)) != NULL);
}

static
BOOL destroy_win32_timer(win32_timer_t *ct)
{
  return TerminateThread(ct->handle,1);
}  
  
static
BOOL halt_win32_timer(win32_timer_t *ct)
{
  return SuspendThread(ct->handle) != 0xffffffff;
}

static
BOOL resume_win32_timer(win32_timer_t *ct)
{
  return ResumeThread(ct->handle) != 0xffffffff;
}

static win32_timer_t wt;

bool_t win32StopTimer()
{
  return halt_win32_timer(&wt);
}

bool_t win32StartTimer(int milli_secs)
{
  wt.milli_secs = milli_secs;
  return resume_win32_timer(&wt);
}

static
void win32_fake_sigalrm()
{
  vproc_state_t   *vsp = SELF_VPROC;

  if (SuspendThread(win32_LIB7_thread_handle) == 0xffffffff) {
    Die ("win32_fake_sigalrm: unable to suspend Lib7 thread");
  }

  win32_generic_handler(SIGALRM);

  if (ResumeThread(win32_LIB7_thread_handle) == 0xffffffff) {
    Die ("win32_fake_sigalrm: unable to resume Lib7 thread");
  }
}


/* set_up_timers:
 *
 * system specific timer initialization
 */
void set_up_timers ()
{
  if (!create_win32_timer(&wt,win32_fake_sigalrm,0,TRUE)) {
    Die("set_up_timers: unable to create_win32_timer");
  }
  _ftime(&start_timeb);
} /* end of set_up_timers */


/* get_cpu_time:
 *
 * Get the elapsed user and/or system cpu times in a system independent way.
 */
void get_cpu_time (Time_t *usrT, Time_t *sysT)
{
  struct _timeb now_timeb, elapsed_timeb;

  _ftime(&now_timeb);
  if (now_timeb.millitm < start_timeb.millitm) {
    now_timeb.time--;
    ASSERT(now_timeb.time >= start_timeb.time);
    now_timeb.millitm += 1000;
    ASSERT(now_timeb.millitm > start_timeb.millitm);
  }
  elapsed_timeb.time = now_timeb.time - start_timeb.time;
  elapsed_timeb.millitm = now_timeb.millitm - start_timeb.millitm;
  if (usrT != NULL) {
    usrT->seconds = (Int32_t) elapsed_timeb.time;
    usrT->uSeconds = ((Int32_t) elapsed_timeb.millitm) * 1000;
  }
  if (sysT != NULL) {
    sysT->seconds = sysT->uSeconds = 0;
  }
}

/* end of win32-timers.c */


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

