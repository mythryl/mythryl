/* runtime-mp.h
 *
 */

#ifndef _LIB7_MP_
#define _LIB7_MP_

#include "../config.h"

/* The status of a virtual processor */
typedef enum {
    MP_PROC_RUNNING,	/* processor is running   */
    MP_PROC_SUSPENDED,	/* processor is suspended */
    MP_PROC_NO_PROC	/* no processor allocated */
} vproc_status_t;

#ifdef MP_SUPPORT

#if !defined(SOFT_POLL) || !defined(MP_GCPOLL)
#  error MP runtime currently requires polling support
#endif

/*** OS dependent stuff ***/

#if defined(OPSYS_IRIX5)

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <sys/prctl.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <ulocks.h>

typedef ulock_t mp_lock_t;		/* A lock */
typedef barrier_t mp_barrier_t;		/* A barrier */
typedef int mp_pid_t;                   /* A process id */

#else
#  error MP not supported for this system
#endif


/*** Generic MP interface ***/

extern int MP_StartCollect(lib7_state_t *);
extern void MP_FinishCollect(lib7_state_t *,int);
extern lib7_val_t *mpExtraRoots[];

extern void MP_SetLock (mp_lock_t lock);
extern void MP_UnsetLock (mp_lock_t lock);
extern bool_t MP_TryLock (mp_lock_t lock);
extern mp_lock_t MP_AllocLock ();
extern void MP_FreeLock (mp_lock_t lock);

extern mp_barrier_t *MP_AllocBarrier ();
extern void MP_FreeBarrier (mp_barrier_t *barrierp);
extern void MP_Barrier (mp_barrier_t *barrierp, unsigned n);
extern void MP_ResetBarrier (mp_barrier_t *barrierp);

extern mp_pid_t MP_ProcId (void);
extern int MP_MaxProcs ();
extern lib7_val_t MP_AcquireProc (lib7_state_t *lib7_state, lib7_val_t arg);
extern void MP_ReleaseProc (lib7_state_t *lib7_state);
extern int MP_ActiveProcs ();
extern void MP_Init (void);
extern void MP_Shutdown (void);

extern mp_lock_t	MP_GCLock;
extern mp_lock_t	MP_GCGenLock;
extern mp_lock_t	MP_TimerLock;
extern mp_barrier_t	*MP_GCBarrier;

#define BEGIN_CRITICAL_SECT(LOCK)	{ MP_SetLock(LOCK); {
#define END_CRITICAL_SECT(LOCK)		} MP_UnsetLock(LOCK); }
#define ACQUIRE_LOCK(LOCK)		MP_SetLock(LOCK);
#define RELEASE_LOCK(LOCK)		MP_UnsetLock(LOCK);

#else /* !MP_SUPPORT */

#define BEGIN_CRITICAL_SECT(LOCK)	{
#define END_CRITICAL_SECT(LOCK)		}
#define ACQUIRE_LOCK(LOCK)		/* no operation */
#define RELEASE_LOCK(LOCK)		/* no operation */

#endif /* MP_SUPPORT */

#endif /* !_LIB7_MP_ */



/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
