/* poll.c
 */

/* The run-time code for winix::io::poll.
 *
 * Note that this implementation must
 * satisfy the following two requirements:
 *
 *   1) The list of return items must be
 *      in the same order as the
 *	corresponding list of arguments.
 *
 *   2) Return items must contain no more
 *      information than was queried for.
 *	(This matters when the same
 *      descriptor is covered by multiple
 *      items).
 */

#include "../../config.h"

#include <errno.h>

#include "runtime-unixdep.h"
#if defined(HAS_SELECT)

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif


#if HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif

#elif defined(HAS_POLL)
#  include <stropts.h>
#  include <poll.h>
#else
#  error no support for I/O polling
#endif
#include INCLUDE_TIME_H
#include "runtime-base.h"
#include "lib7-c.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"

/* bit masks for polling descriptors (see src/sml-nj/boot/Unix/os-io.pkg) */
#define READABLE_BIT		0x1
#define WRITABLE_BIT		0x2
#define OOBDABLE_BIT		0x4

static lib7_val_t LIB7_Poll (lib7_state_t *lib7_state, lib7_val_t poll_list, struct timeval *timeout);


/* _lib7_OS_poll : (List (Int, Unt), Null_Or(int32.Int, Int)) -> List (Int, Unt) 
 */
lib7_val_t _lib7_OS_poll (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	    poll_list = REC_SEL(arg, 0);
    lib7_val_t	    timeout  = REC_SEL(arg, 1);
    struct timeval  tv, *tvp;

    if (timeout == OPTION_NONE)
	tvp = NULL;
    else {
	timeout		= OPTION_get(timeout);
	tv.tv_sec	= REC_SELINT32(timeout, 0);
	tv.tv_usec	= REC_SELINT(timeout, 1);
	tvp = &tv;
    }

    return LIB7_Poll (lib7_state, poll_list, tvp);

} /* end of _lib7_OS_poll */


#ifdef HAS_POLL

#ifdef POLLMSG
#define POLL_ERROR	(POLLRDBAND|POLLPRI|POLLHUP|POLLMSG)
#else
#define POLL_ERROR	(POLLRDBAND|POLLPRI|POLLHUP)
#endif

/* LIB7_Poll:
 *
 * The version of the polling operation for systems that provide SVR4 polling.
 */
static lib7_val_t LIB7_Poll (lib7_state_t *lib7_state, lib7_val_t poll_list, struct timeval *timeout)
{
    int		    tout;
    struct pollfd*   fds;
    struct pollfd*   fdp;
    int		    nfds, i, flag;
    lib7_val_t	    l, item;

    if (timeout == NULL)
	tout = -1;
    else
        /* Convert to miliseconds: */
	tout = (timeout->tv_sec * 1000) + (timeout->tv_usec / 1000);

    /* Count the number of polling items:
     */
    for (l = poll_list, nfds = 0;  l != LIST_nil;  l = LIST_tl(l))
	nfds++;

    /* Allocate the fds vector: */
    fds = NEW_VEC(struct pollfd, nfds);
    CLEAR_MEM (fds, sizeof(struct pollfd)*nfds);

    /* Initialize the polling descriptors:
     */
    for (l = poll_list, fdp = fds;  l != LIST_nil;  l = LIST_tl(l), fdp++) {

	item = LIST_hd(l);

	fdp->fd	= REC_SELINT(item, 0);
	flag    = REC_SELINT(item, 1);

	if ((flag & READABLE_BIT) != 0)  fdp->events |= POLLIN;
	if ((flag & WRITABLE_BIT) != 0)  fdp->events |= POLLOUT;
	if ((flag & OOBDABLE_BIT) != 0)  fdp->events |= POLL_ERROR;
    }

    {   int status;


/*      do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

            status = poll (fds, nfds, tout);

/*      } while (status < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

	if (status < 0) {
	    FREE(fds);
	    return RAISE_SYSERR(lib7_state, status);
	}
	else {
	    for (i = nfds-1, l = LIST_nil;  i >= 0;  i--) {

		fdp = &(fds[i]);

		if (fdp->revents != 0) {

		    flag = 0;

		    if ((fdp->revents & POLLIN    ) != 0)  flag |= READABLE_BIT;
		    if ((fdp->revents & POLLOUT   ) != 0)  flag |= WRITABLE_BIT;
		    if ((fdp->revents & POLL_ERROR) != 0)  flag |= OOBDABLE_BIT;

		    REC_ALLOC2(lib7_state, item, INT_CtoLib7(fdp->fd), INT_CtoLib7(flag));
		    LIST_cons(lib7_state, l, item, l);
		}
	    }
	    FREE(fds);
	    return l;
	}
    }
}

#else /* HAS_SELECT */
#include <fcntl.h>/* 2008-03-15 CrT BUGGO -- DELETEME! Temporary debug hack. */


/* LIB7_Poll:
 *
 * The version of the polling operation for systems that provide BSD select.
 */
static lib7_val_t LIB7_Poll (lib7_state_t *lib7_state, lib7_val_t poll_list, struct timeval *timeout)
{
    fd_set	rset, wset, eset;
    fd_set	*rfds, *wfds, *efds;
    int		maxFD, status, fd, flag;
    lib7_val_t	l, item;

/*printf("src/runtime/c-libs/posix-os/poll.c: Using 'select' implementation\n");*/
    rfds = wfds = efds = NULL;
    maxFD = 0;
    for (l = poll_list;  l != LIST_nil;  l = LIST_tl(l)) {
	item	= LIST_hd(l);
	fd	= REC_SELINT(item, 0);
	flag	= REC_SELINT(item, 1);
	if ((flag & READABLE_BIT) != 0) {
/*int fd_flags = fcntl(fd,F_GETFL,0);*/
	    if (rfds == NULL) {
		rfds = &rset;
		FD_ZERO(rfds);
	    }
/*printf("src/runtime/c-libs/posix-os/poll.c: Will check fd %d for readability. fd flags x=%x O_NONBLOCK x=%x\n",fd,fd_flags,O_NONBLOCK);*/
	    FD_SET (fd, rfds);
	}
	if ((flag & WRITABLE_BIT) != 0) {
	    if (wfds == NULL) {
		wfds = &wset;
		FD_ZERO(wfds);
	    }
/*printf("src/runtime/c-libs/posix-os/poll.c: Will check fd %d for writability.\n",fd);*/
	    FD_SET (fd, wfds);
	}
	if ((flag & OOBDABLE_BIT) != 0) {
	    if (efds == NULL) {
		efds = &eset;
		FD_ZERO(efds);
	    }
/*printf("src/runtime/c-libs/posix-os/poll.c: Will check fd %d for oobdability.\n",fd);*/
	    FD_SET (fd, efds);
	}
	if (fd > maxFD) maxFD = fd;
    }

/*printf("src/runtime/c-libs/posix-os/poll.c: maxFD d=%d timeout x=%x.\n",maxFD,timeout);*/

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        status = select (maxFD+1, rfds, wfds, efds, timeout);

/*  } while (status < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

/*printf("src/runtime/c-libs/posix-os/poll.c: result status d=%d.\n",status);*/

    if (status < 0)
        return RAISE_SYSERR(lib7_state, status);
    else if (status == 0)
	return LIST_nil;
    else {
	lib7_val_t	*resVec = NEW_VEC(lib7_val_t, status);
	int		i;
	int		resFlag;

	for (i = 0, l = poll_list;  l != LIST_nil;  l = LIST_tl(l)) {
	    item	= LIST_hd(l);
	    fd		= REC_SELINT(item, 0);
	    flag	= REC_SELINT(item, 1);
	    resFlag	= 0;
	    if (((flag & READABLE_BIT) != 0) && FD_ISSET(fd, rfds)) {
/*int fd_flags = fcntl(fd,F_GETFL,0);*/
/*printf("src/runtime/c-libs/posix-os/poll.c: fd d=%d is in fact readable. fd flags x=%x O_NONBLOCK x=%x\n",fd,fd_flags,O_NONBLOCK);*/
		resFlag |= READABLE_BIT;
            }
	    if (((flag & WRITABLE_BIT) != 0) && FD_ISSET(fd, wfds)) {
/*printf("src/runtime/c-libs/posix-os/poll.c: fd d=%d is in fact writable.\n",fd);*/
		resFlag |= WRITABLE_BIT;
            }
	    if (((flag & OOBDABLE_BIT) != 0) && FD_ISSET(fd, efds)) {
/*printf("src/runtime/c-libs/posix-os/poll.c: fd d=%d is in fact oobdable.\n",fd);*/
		resFlag |= OOBDABLE_BIT;
            }
	    if (resFlag != 0) {
		REC_ALLOC2 (lib7_state, item, INT_CtoLib7(fd), INT_CtoLib7(resFlag));
		resVec[i++] = item;
	    }
	}

	ASSERT(i == status);

	for (i = status-1, l = LIST_nil;  i >= 0;  i--) {
	    item = resVec[i];
	    LIST_cons (lib7_state, l, item, l);
	}

	FREE(resVec);

	return l;
    }

} /* end of LIB7_Poll */

#endif



/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
