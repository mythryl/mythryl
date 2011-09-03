// OLDselect.c


#include "../../config.h"

#include "system-dependent-stuff.h"
#if defined(HAS_SELECT)

#if HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif

#if HAVE_SYS_TIME_H
    #include <sys/time.h>
#endif

#elif defined(HAS_POLL)
#include <stropts.h>
#include <poll.h>
#endif
#include <signal.h>
#include <setjmp.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "heap-tags.h"
#include "task.h"
#include "ml-signal.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#ifdef HAS_SELECT
  static fd_set *ListToFDSet (Val fdl, fd_set *fds, int *width);
  static Val FDSetToList (Task *task, fd_set *fds, int width);
#endif


Val   _lib7_IO_select   (Task* task,  Val arg)   {
    //===============
    //
    // Mythryl type: (List(Int), List(Int), List(Int), Null_Or( (Int,Int) ))
    //               ->
    //               (List(Int), List(Int), List(Int))
    //
    // Check file descriptors for the readiness of I/O operations.

#if ((*defined(HAS_SELECT)) && (*defined(HAS_POLL)))
    return RAISE_ERROR (task, "LIB7-io.select unsupported");
#else
    Val	    rl = GET_TUPLE_SLOT_AS_VAL(arg, 0);
    Val	    wl = GET_TUPLE_SLOT_AS_VAL(arg, 1);
    Val	    el = GET_TUPLE_SLOT_AS_VAL(arg, 2);
    Val	    timeout = GET_TUPLE_SLOT_AS_VAL(arg, 3);
#ifdef HAS_SELECT
    fd_set	    rset, wset, eset;
    fd_set	    *rfds, *wfds, *efds;
    int		    width = 0, status;
    struct timeval  t, *tp;

    rfds = ListToFDSet (rl, &rset, &width);
    wfds = ListToFDSet (wl, &wset, &width);
    efds = ListToFDSet (el, &eset, &width);

    if (IS_POINTER(timeout)) {
	timeout = GET_TUPLE_SLOT_AS_VAL(timeout, 0);  /* strip the THE */
	t.tv_sec = GET_TUPLE_SLOT_AS_INT(timeout, 0);
	t.tv_usec = GET_TUPLE_SLOT_AS_INT(timeout, 1);
	tp = &t;
    }
    else
	tp = 0;

#else // HAS_POLL
    struct pollfd   *fds;
    int		    nr, nw, ne, nfds, i, t, status;

#define COUNT(cntr, l) {					\
	Val	__p = (l);				\
	for (cntr = 0;  __p != LIST_NIL;  __p = LIST_TAIL(__p))	\
	    cntr++;						\
    }
#define INSERT(req, l) {					\
	Val	__p = (l);				\
	while (__p != LIST_NIL) {				\
	    fds[i].fd = TAGGED_INT_TO_C_INT(LIST_HEAD(__p));		\
	    fds[i].events = (req);				\
	    i++;						\
	    __p = LIST_TAIL(__p);					\
	}							\
    }

    COUNT(nr, rl);
    COUNT(nw, wl);
    COUNT(ne, el);
    nfds = nr+nw+ne;
    fds = MALLOC_VEC(struct pollfd, nfds);
    i = 0;
    INSERT(POLLIN, rl);
    INSERT(POLLOUT, wl);
#ifdef POLLMSG
    INSERT(POLLRDBAND|POLLPRI|POLLMSG|POLLHUP, el);
#else
    INSERT(POLLRDBAND|POLLPRI|POLLHUP, el);
#endif

    if (IS_POINTER(timeout)) {
	long	sec, usec;
	timeout = GET_TUPLE_SLOT_AS_VAL(timeout, 0);  		// Strip the THE.
	sec = GET_TUPLE_SLOT_AS_INT(timeout, 0);
	usec = GET_TUPLE_SLOT_AS_INT(timeout, 1);
	t = (usec/1000 + sec*1000);
    }
    else
	t = INFTIM;
#endif

    if (task->lib7_inSigHandler || task->lib7_maskSignals
    || ((*SETJMP (task->lib7_syscallEnv)) &&
	(((task->lib7_ioWaitFlag = TRUE), (task->ml_numPendingSigs == 0)))))
    {
#ifdef HAS_SELECT
	DO_SYSCALL (select (width, rfds, wfds, efds, tp), status);
#else // HAS_POLL
	DO_SYSCALL (poll (fds, nfds, t), status);
#endif
	task->lib7_ioWaitFlag = FALSE;
    }
    else {
#ifdef HAS_POLL
	FREE (fds);
#endif
	BackupLib7Cont(task);
      // re-enable signals
	RESET_SIG_MASK();
	return task->argument;
    }

    if (status == -1) {
#ifdef HAS_POLL
	FREE (fds);
#endif
	return RAISE_SYSERR (task, status);
    }
    else {
	Val	    rfdl, wfdl, efdl, res;

	if (status == 0)
	    rfdl = wfdl = efdl = LIST_NIL;
	else {
#ifdef HAS_SELECT
	    rfdl = FDSetToList (task, rfds, width);
	    wfdl = FDSetToList (task, wfds, width);
	    efdl = FDSetToList (task, efds, width);
#else // HAS_POLL
#define BUILD_RESULT(l,n)	{				\
	l = LIST_NIL;						\
	while ((status > 0) && (n > 0)) {				\
	    if (fds[i].revents != 0) {				\
		status--;						\
		LIST_CONS(task, l, TAGGED_INT_FROM_C_INT(fds[i].fd), l);	\
	    }							\
	    n--;  i++;						\
	}							\
    }
	    i = 0;
	    BUILD_RESULT(rfdl, nr);
	    BUILD_RESULT(wfdl, nw);
	    BUILD_RESULT(efdl, ne);
#endif
	}
	REC_ALLOC3 (task, res, rfdl, wfdl, efdl);

#ifdef HAS_POLL
	FREE (fds);
#endif

	return res;
    }
#endif
}					// fun _lib7_IO_select


#ifdef HAS_SELECT


static fd_set*   ListToFDSet   (Val fdl,  fd_set* fds,  int* width)   {
    //           ===========
    //
    // Mythryl type:
    //
    // Map a Lib7 list of file descriptors to a fd_set.

    register int    fd, maxfd = -1;

    FD_ZERO(fds);
    while (fdl != LIST_NIL) {
	fd = TAGGED_INT_TO_C_INT(LIST_HEAD(fdl));
	if (fd > maxfd)
	    maxfd = fd;
	FD_SET (fd, fds);
	fdl = LIST_TAIL(fdl);
    }

    if (maxfd < 0) return (fd_set *)0;

    if (*width <= maxfd)
	*width  = maxfd+1;

    return fds;
}


static Val   FDSetToList   (Task* task,  fd_set* fds,  int width)   {
    //       ===========
    //
    // Mythryl type:
    //
    // Map a fd_set to a Lib7 list of ready file descriptors.


    register Val  p;
    register int  i;

    if (fds == NULL)   return LIST_NIL;

    for (i = 0, p = LIST_NIL;  i < width;  i++) {
	//
	if (FD_ISSET(i, fds)) {
	    //
	    LIST_CONS (task, p, TAGGED_INT_FROM_C_INT(i), p);
        }
    }

    return p;
}

#endif // HAS_SELECT


// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

