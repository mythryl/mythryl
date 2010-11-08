/* select.c
 *
 */

#include "../../config.h"

#include "runtime-osdep.h"
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
#include "runtime-heap.h"
#include "tags.h"
#include "runtime-state.h"
#include "ml-signal.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#ifdef HAS_SELECT
static fd_set *ListToFDSet (lib7_val_t fdl, fd_set *fds, int *width);
static lib7_val_t FDSetToList (lib7_state_t *lib7_state, fd_set *fds, int width);
#endif


/* _lib7_IO_select : (int list * int list * int list * (int * int) option)
 *                 -> (int list * int list * int list)
 *
 * Check file descriptors for the readiness of I/O operations.
 */
lib7_val_t _lib7_IO_select (lib7_state_t *lib7_state, lib7_val_t arg)
{
#if ((*defined(HAS_SELECT)) && (*defined(HAS_POLL)))
    return RAISE_ERROR (lib7_state, "LIB7-io.select unsupported");
#else
    lib7_val_t	    rl = REC_SEL(arg, 0);
    lib7_val_t	    wl = REC_SEL(arg, 1);
    lib7_val_t	    el = REC_SEL(arg, 2);
    lib7_val_t	    timeout = REC_SEL(arg, 3);
#ifdef HAS_SELECT
    fd_set	    rset, wset, eset;
    fd_set	    *rfds, *wfds, *efds;
    int		    width = 0, status;
    struct timeval  t, *tp;

    rfds = ListToFDSet (rl, &rset, &width);
    wfds = ListToFDSet (wl, &wset, &width);
    efds = ListToFDSet (el, &eset, &width);

    if (isBOXED(timeout)) {
	timeout = REC_SEL(timeout, 0);  /* strip the THE */
	t.tv_sec = REC_SELINT(timeout, 0);
	t.tv_usec = REC_SELINT(timeout, 1);
	tp = &t;
    }
    else
	tp = 0;

#else /* HAS_POLL */
    struct pollfd   *fds;
    int		    nr, nw, ne, nfds, i, t, status;

#define COUNT(cntr, l) {					\
	lib7_val_t	__p = (l);				\
	for (cntr = 0;  __p != LIST_nil;  __p = LIST_tl(__p))	\
	    cntr++;						\
    }
#define INSERT(req, l) {					\
	lib7_val_t	__p = (l);				\
	while (__p != LIST_nil) {				\
	    fds[i].fd = INT_LIB7toC(LIST_hd(__p));		\
	    fds[i].events = (req);				\
	    i++;						\
	    __p = LIST_tl(__p);					\
	}							\
    }

    COUNT(nr, rl);
    COUNT(nw, wl);
    COUNT(ne, el);
    nfds = nr+nw+ne;
    fds = NEW_VEC(struct pollfd, nfds);
    i = 0;
    INSERT(POLLIN, rl);
    INSERT(POLLOUT, wl);
#ifdef POLLMSG
    INSERT(POLLRDBAND|POLLPRI|POLLMSG|POLLHUP, el);
#else
    INSERT(POLLRDBAND|POLLPRI|POLLHUP, el);
#endif

    if (isBOXED(timeout)) {
	long	sec, usec;
	timeout = REC_SEL(timeout, 0);  /* strip the THE */
	sec = REC_SELINT(timeout, 0);
	usec = REC_SELINT(timeout, 1);
	t = (usec/1000 + sec*1000);
    }
    else
	t = INFTIM;
#endif

    if (lib7_state->lib7_inSigHandler || lib7_state->lib7_maskSignals
    || ((*SETJMP (lib7_state->lib7_syscallEnv)) &&
	(((lib7_state->lib7_ioWaitFlag = TRUE), (lib7_state->ml_numPendingSigs == 0)))))
    {
#ifdef HAS_SELECT
	DO_SYSCALL (select (width, rfds, wfds, efds, tp), status);
#else /* HAS_POLL */
	DO_SYSCALL (poll (fds, nfds, t), status);
#endif
	lib7_state->lib7_ioWaitFlag = FALSE;
    }
    else {
#ifdef HAS_POLL
	FREE (fds);
#endif
	BackupLib7Cont(lib7_state);
      /* re-enable signals */
	RESET_SIG_MASK();
	return lib7_state->lib7_argument;
    }

    if (status == -1) {
#ifdef HAS_POLL
	FREE (fds);
#endif
	return RAISE_SYSERR (lib7_state, status);
    }
    else {
	lib7_val_t	    rfdl, wfdl, efdl, res;

	if (status == 0)
	    rfdl = wfdl = efdl = LIST_nil;
	else {
#ifdef HAS_SELECT
	    rfdl = FDSetToList (lib7_state, rfds, width);
	    wfdl = FDSetToList (lib7_state, wfds, width);
	    efdl = FDSetToList (lib7_state, efds, width);
#else /* HAS_POLL */
#define BUILD_RESULT(l,n)	{				\
	l = LIST_nil;						\
	while ((status > 0) && (n > 0)) {				\
	    if (fds[i].revents != 0) {				\
		status--;						\
		LIST_cons(lib7_state, l, INT_CtoLib7(fds[i].fd), l);	\
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
	REC_ALLOC3 (lib7_state, res, rfdl, wfdl, efdl);

#ifdef HAS_POLL
	FREE (fds);
#endif

	return res;
    }
#endif
} /* end of _lib7_IO_select */


#ifdef HAS_SELECT

/* ListToFDSet:
 *
 * Map a Lib7 list of file descriptors to a fd_set.
 */
static fd_set *ListToFDSet (lib7_val_t fdl, fd_set *fds, int *width)
{
    register int    fd, maxfd = -1;

    FD_ZERO(fds);
    while (fdl != LIST_nil) {
	fd = INT_LIB7toC(LIST_hd(fdl));
	if (fd > maxfd)
	    maxfd = fd;
	FD_SET (fd, fds);
	fdl = LIST_tl(fdl);
    }

    if (maxfd >= 0) {
	if (maxfd >= *width)
	    *width = maxfd+1;
	return fds;
    }
    else
	return (fd_set *)0;

} /* end of ListToFDSet */

/* FDSetToList:
 *
 * Map a fd_set to a Lib7 list of ready file descriptors.
 */
static lib7_val_t FDSetToList (lib7_state_t *lib7_state, fd_set *fds, int width)
{
    register lib7_val_t p;
    register int    i;

    if (fds == NULL)
	return LIST_nil;

    for (i = 0, p = LIST_nil;  i < width;  i++) {
	if (FD_ISSET(i, fds))
	    LIST_cons (lib7_state, p, INT_CtoLib7(i), p);
    }

    return p;

} /* end of FDSetToList */

#endif /* HAS_SELECT */


/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
