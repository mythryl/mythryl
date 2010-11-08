/* sgi-mp.c
 *
 * MP support for SGI Challenge machines (Irix 5.x).
 */

#include "../config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <sys/prctl.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <ulocks.h>
#include "runtime-base.h"
#include "runtime-limits.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "tags.h"
#include "runtime-mp.h"
#include "runtime-state.h"
#include "runtime-globals.h"
#include "vproc-state.h"

/* #define ARENA_FNAME  tmpnam(0) */
#define ARENA_FNAME  "/tmp/sml-mp.lock-arena"

#define INT_LIB7inc(n,i)  ((lib7_val_t)INT_CtoLib7(INT_LIB7toC(n) + (i)))
#define INT_LIB7dec(n,i)  (INT_LIB7inc(n,(-i)))

/* forwards */
static mp_lock_t AllocLock ();        
static mp_barrier_t *AllocBarrier();

/* locals */
static usptr_t	*arena;		/* arena for shared sync chunks */
static ulock_t	MP_ArenaLock;	/* must be held to alloc/free a lock */
static ulock_t	MP_ProcLock;	/* must be held to acquire/release procs */

/* globals */
mp_lock_t	MP_GCLock;
mp_lock_t	MP_GCGenLock;
mp_barrier_t	*MP_GCBarrier;
mp_lock_t	MP_TimerLock;


/* MP_Init:
 */
void MP_Init ()
{
  /* set '_utrace = 1;' to debug shared arenas */
    if (usconfig(CONF_LOCKTYPE, US_NODEBUG) == -1) {
	Die ("usconfig failed in MP_Init");
    }
    usconfig(CONF_AUTOGROW, 0);
    if (usconfig(CONF_INITSIZE, 65536) == -1) {
	Die ("usconfig failed in MP_Init");
    }
    if ((arena = usinit(ARENA_FNAME)) == NULL) {
	Die ("usinit failed in MP_Init");
    }

    MP_ArenaLock	= AllocLock();
    MP_ProcLock		= AllocLock();
    MP_GCLock		= AllocLock();
    MP_GCGenLock	= AllocLock();
    MP_TimerLock	= AllocLock();
    MP_GCBarrier	= AllocBarrier();
    ASSIGN(ActiveProcs, INT_CtoLib7(1));

} /* end of MP_Init */


/* MP_ProcId:
 */
mp_pid_t MP_ProcId ()
{

  return getpid ();

} /* end of MP_ProcId */


/* AllocLock:
 *
 * Allocate and initialize a system lock.
 */
static mp_lock_t AllocLock ()
{
    ulock_t	lock;

    if ((lock = usnewlock(arena)) == NULL) {
	Die ("AllocLock: cannot get lock with usnewlock\n");
    }
    usinitlock(lock);
    usunsetlock(lock);

    return lock;

} /* end of AllocLock */
 

/* MP_SetLock:
 */
void MP_SetLock (mp_lock_t lock)
{
    ussetlock(lock);

} /* end of MP_SetLock */


/* MP_UnsetLock:
 */
void MP_UnsetLock (mp_lock_t lock)
{
    usunsetlock(lock);

} /* end of MP_UnsetLock */


/* MP_TryLock:
 */
bool_t MP_TryLock (mp_lock_t lock)
{
    return ((bool_t) uscsetlock(lock, 1));  /* try once */

} /* end of MP_TryLock */


/* MP_AllocLock:
 */
mp_lock_t MP_AllocLock ()
{
    ulock_t lock;

    ussetlock(MP_ArenaLock);
	lock = AllocLock ();
    usunsetlock(MP_ArenaLock);

    return lock;

} /* end of MP_AllocLock */


/*  MP_FreeLock:
 */
void MP_FreeLock (mp_lock_t lock)
{
    ussetlock(MP_ArenaLock);
	usfreelock(lock,arena);
    usunsetlock(MP_ArenaLock);

} /* end of MP_FreeLock */


/* AllocBarrier:
 *
 * Allocate and initialize a system barrier.
 */
static mp_barrier_t *AllocBarrier ()
{
    barrier_t *barrierp;

    if ((barrierp = new_barrier(arena)) == NULL) {
	Die ("cannot get barrier with new_barrier");
    }
    init_barrier(barrierp);

    return barrierp;

} /* end of AllocBarrier */
  
/* MP_AllocBarrier:
 */
mp_barrier_t *MP_AllocBarrier ()
{
    barrier_t *barrierp;

    ussetlock(MP_ArenaLock);
	barrierp = AllocBarrier ();
    usunsetlock(MP_ArenaLock);

    return barrierp;

} /* end of MP_AllocBarrier */

/* MP_FreeBarrier:
 */
void MP_FreeBarrier (mp_barrier_t *barrierp)
{
    ussetlock(MP_ArenaLock);
	free_barrier(barrierp);
    usunsetlock(MP_ArenaLock);

} /* end of MP_FreeBarrier */

/* MP_Barrier:
 */
void MP_Barrier (mp_barrier_t *barrierp, unsigned n)
{
    barrier(barrierp, n);

} /* end of MP_Barrier */

/* MP_ResetBarrier:
 */
void MP_ResetBarrier (mp_barrier_t *barrierp)
{
    init_barrier(barrierp);

} /* end of MP_ResetBarrier */

/* ??? */
static void fixPnum (int n)
{
  /* dummy for now */
}
 

/* MP_MaxProcs:
 */
int MP_MaxProcs ()
{
    return MAX_NUM_PROCS;

} /* end of MP_MaxProcs */


/* ProcMain:
 */
static void ProcMain (void *vlib7_state)
{
    lib7_state_t *lib7_state = (lib7_state_t *) vlib7_state;

  /* needs to be done  
    fixPnum(lib7_state->pnum);
    setup_signals(lib7_state, TRUE);
   */
  /* spin until we get our id (from return of call to NewProc) */
    while (lib7_state->lib7_vproc->vp_mpSelf == NULL) {
#ifdef MP_DEBUG
	SayDebug("[waiting for self]\n");
#endif
	continue;
    }
#ifdef MP_DEBUG
    SayDebug ("[new proc main: releasing lock]\n");
#endif
    MP_UnsetLock (MP_ProcLock); /* implicitly handed to us by the parent */
    RunLib7 (lib7_state);                 /* should never return */
    Die ("proc returned after run_lib7() in ProcMain().\n");

} /* end of ProcMain */


/* NewProc:
 */
static int NewProc (lib7_state_t *state)
{
    int ret, error;

    ret = sproc(ProcMain, PR_SALL, (void *)state);
    if (ret == -1) {
	extern int errno;

	error = oserror();	/* this is potentially a problem since */
				/* each thread should have its own errno. */
				/* see sgi man pages for sproc */
	Error ("error=%d,errno=%d\n", error, errno);
	Error ("[warning NewProc: %s]\n",strerror(error));
    } 

    return ret;
}


/* MP_AcquireProc:
 */
lib7_val_t MP_AcquireProc (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_state_t *p;
    vproc_state_t *vsp;
    lib7_val_t v = REC_SEL(arg, 0);
    lib7_val_t f = REC_SEL(arg, 1);
    int i;

#ifdef MP_DEBUG
    SayDebug("[acquiring proc]\n");
#endif
    MP_SetLock(MP_ProcLock);
  /* search for a suspended proc to reuse */
    for (i = 0;
	(i < NumVProcs) && (VProc[i]->vp_mpState != MP_PROC_SUSPENDED);
	i++
    )
	continue;
#ifdef MP_DEBUG
    SayDebug("[checking for suspended processor]\n");
#endif
    if (i == NumVProcs) {
	if (DEREF(ActiveProcs) == INT_CtoLib7(MAX_NUM_PROCS)) {
	    MP_UnsetLock(MP_ProcLock);
	    Error("[processors maxed]\n");
	    return LIB7_false;
	}
#ifdef MP_DEBUG
	SayDebug("[checking for NO_PROC]\n");
#endif
      /* search for a slot in which to put a new proc */
	for (i = 0;
	    (i < NumVProcs) && (VProc[i]->vp_mpState != MP_PROC_NO_PROC);
	    i++
	)
	    continue;
	if (i == NumVProcs) {
	    MP_UnsetLock(MP_ProcLock);
	    Error("[no processor to allocate]\n");
	    return LIB7_false;
	}
    }
#ifdef MP_DEBUG
    SayDebug("[using processor at index %d]\n", i);
#endif
  /* use processor at index i */
    vsp = VProc[i];
    p = vsp->vp_state;

    p->lib7_exceptionn_fate	= PTR_CtoLib7(handle_v+1);
    p->lib7_argument		= LIB7_void;
    p->lib7_fate		= PTR_CtoLib7(return_c);
    p->lib7_closure		= f;
    p->lib7_program_counter	= 
    p->lib7_link_register	= GET_CODE_ADDR(f);
    p->lib7_current_thread	= v;
  
    if (vsp->vp_mpState == MP_PROC_NO_PROC) {
      /* assume we get one */
	ASSIGN(ActiveProcs, INT_LIB7inc(DEREF(ActiveProcs), 1));
	if ((vsp->vp_mpSelf = NewProc(p)) != -1) {
#ifdef MP_DEBUG
	    SayDebug ("[got a processor]\n");
#endif
	    vsp->vp_mpState = MP_PROC_RUNNING;
	  /* NewProc will release MP_ProcLock */
	    return LIB7_true;
	}
	else {
	    ASSIGN(ActiveProcs, INT_LIB7dec(DEREF(ActiveProcs), 1));
	    MP_UnsetLock(MP_ProcLock);
	    return LIB7_false;
	}      
    }
    else {
	vsp->vp_mpState = MP_PROC_RUNNING;
#ifdef MP_DEBUG
	SayDebug ("[reusing a processor]\n");
#endif
	MP_UnsetLock(MP_ProcLock);
	return LIB7_true;
    }

} /* end of MP_AcquireProc */

/* MP_ReleaseProc:
 */
void MP_ReleaseProc (lib7_state_t *lib7_state)
{
#ifdef MP_DEBUG
    SayDebug("[release_proc: suspending]\n");
#endif
    collect_garbage(lib7_state,1);
    MP_SetLock(MP_ProcLock);
    lib7_state->lib7_vproc->vp_mpState = MP_PROC_SUSPENDED;
    MP_UnsetLock(MP_ProcLock);
    while (lib7_state->lib7_vproc->vp_mpState == MP_PROC_SUSPENDED) {
      /* need to be continually available for gc */
	collect_garbage( lib7_state, 1 );
    }
#ifdef MP_DEBUG
    SayDebug("[release_proc: resuming]\n");
#endif
    RunLib7(lib7_state);
    Die ("return after RunLib7(lib7_state) in mp_release_proc\n");

} /* end of MP_ReleaseProc */


/* MP_ActiveProcs:
 */
int MP_ActiveProcs ()
{
    int ap;

    MP_SetLock(MP_ProcLock);
    ap = INT_LIB7toC(DEREF(ActiveProcs));
    MP_UnsetLock(MP_ProcLock);

    return ap;

} /* end of MP_ActiveProcs */


/* MP_Shutdown:
 */
void MP_Shutdown ()
{
    usdetach(arena);

} /* end of MP_Shutdown */



/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

