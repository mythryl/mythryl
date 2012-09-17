// poll.c


// The run-time code for winix::io::poll.
//
// Note that this implementation must
// satisfy the following two requirements:
//
//   1) The list of return items must be
//      in the same order as the
//	corresponding list of arguments.
//
//   2) Return items must contain no more
//      information than was queried for.
//	(This matters when the same
//      descriptor is covered by multiple
//      items).


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "system-dependent-unix-stuff.h"
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
#include "raise-error.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"


// Bitmasks for polling descriptors -- see
//     src/sml-nj/boot/Posix/os-io.pkg
//
#define READABLE_BIT		0x1
#define WRITABLE_BIT		0x2
#define OOBDABLE_BIT		0x4


static Val   LIB7_Poll   (Task *task, Val arg, struct timeval *timeout);


// One of the library bindings exported via
//     src/c/lib/posix-os/cfun-list.h
// and thence
//     src/c/lib/posix-os/libmythryl-posix-os.c



Val   _lib7_OS_poll   (Task* task,  Val arg)   {
    //=============
    //
    // Mythryl type:   (List (Int, Unt), Null_Or(one_word_int::Int, Int)) -> List( (Int, Unt)  )
    //
    // 'poll' is the Unix System V equivalent to BSD 'select'.
    //
    // (At the implementation level we use whichever
    // is available on the host OS.) 
    //
    // This fn gets bound as   poll'   in:
    //
    //     src/lib/std/src/posix/winix-io.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_OS_poll");

//  Val	    poll_list = GET_TUPLE_SLOT_AS_VAL(arg, 0);			// We fetch this in LIB7_Poll() now.
    Val	    timeout   = GET_TUPLE_SLOT_AS_VAL(arg, 1);

    struct timeval  tv;
    struct timeval* tvp;

// printf("src/c/lib/posix-os/poll.c: _lib7_OS_poll/TOP\n"); fflush(stdout);
    if (timeout == OPTION_NULL) {
        //
        tvp = NULL;
        //
    } else {
        //
        timeout	= OPTION_GET( timeout );				// OPTION_GET			is from   src/c/h/make-strings-and-vectors-etc.h

									// TUPLE_GET_INT1		is from   src/c/h/make-strings-and-vectors-etc.h
									// GET_TUPLE_SLOT_AS_INT	is from   src/c/h/runtime-values.h

        tv.tv_sec	= TUPLE_GET_INT1(        timeout, 0 );		// Yes, we really are passing tv_sec as a tagged int
        tv.tv_usec	= GET_TUPLE_SLOT_AS_INT( timeout, 1 );		// but passing tv_usec as a boxed unt.
									// Is this sane?  Deponent declines to declare.
// printf("src/c/lib/posix-os/poll.c: _lib7_OS_poll: tv.tv_sec d=%ld tv.tv_usec d=%ld sec*1000000+usec d=%ld\n", tv.tv_sec, tv.tv_usec, tv.tv_sec*1000000+tv.tv_usec); fflush(stdout);

        tvp = &tv;
    }

    return LIB7_Poll( task, arg, tvp );					// See below.
}


#ifdef HAS_POLL

#ifdef POLLMSG
#define POLL_ERROR	(POLLRDBAND|POLLPRI|POLLHUP|POLLMSG)
#else
#define POLL_ERROR	(POLLRDBAND|POLLPRI|POLLHUP)
#endif


static Val   LIB7_Poll   (Task* task,  Val arg, struct timeval* timeout)   {
    //       ========= 
    //
    //
    // The version of the polling operation for systems that provide SVR4 polling.

    Val	    poll_list = GET_TUPLE_SLOT_AS_VAL(arg, 0);

    struct pollfd*   fds;
    struct pollfd*   fdp;

    int tout;
    int nfds;
    int i;
    int flag;

    Val l;
    Val item;

// printf("src/c/lib/posix-os/poll.c: Using 'poll' implementation\n"); fflush(stdout);
    if (timeout == NULL)   tout = -1;
    else	           tout = (timeout->tv_sec * 1000) + (timeout->tv_usec / 1000);        // Convert to miliseconds.

    // Count the number of polling items:
    //
    for(nfds = 0, l = poll_list;  l != LIST_NIL;  l = LIST_TAIL(l)) {
        nfds++;
    }

    // Allocate the fds vector:
    //
    fds = MALLOC_VEC(struct pollfd, nfds);
    CLEAR_MEMORY (fds, sizeof(struct pollfd)*nfds);

    // Initialize the polling descriptors:
    //
    for (l = poll_list, fdp = fds;  l != LIST_NIL;  l = LIST_TAIL(l), fdp++) {
	//
	item = LIST_HEAD(l);

	fdp->fd	= GET_TUPLE_SLOT_AS_INT(item, 0);
	flag    = GET_TUPLE_SLOT_AS_INT(item, 1);

	if ((flag & READABLE_BIT) != 0)  fdp->events |= POLLIN;
	if ((flag & WRITABLE_BIT) != 0)  fdp->events |= POLLOUT;
	if ((flag & OOBDABLE_BIT) != 0)  fdp->events |= POLL_ERROR;
    }

    {   int status;


	RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_OS_poll", NULL );
	    //
/**/        do { /**/						// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c
								// Restored   2012-06-11 CrT as an experiment.
		status = poll (fds, nfds, tout);
		//
/**/        } while (status < 0 && errno == EINTR);	/**/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_OS_poll" );


	if (status < 0) {
	    //
	    FREE(fds);
	    return RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);
	    //
	} else {
	    //
	    for (i = nfds-1, l = LIST_NIL;  i >= 0;  i--) {
		//
		fdp = &(fds[i]);
		//
		if (fdp->revents != 0) {
		    //
		    flag = 0;
		    //
		    if ((fdp->revents & POLLIN    ) != 0)  flag |= READABLE_BIT;
		    if ((fdp->revents & POLLOUT   ) != 0)  flag |= WRITABLE_BIT;
		    if ((fdp->revents & POLL_ERROR) != 0)  flag |= OOBDABLE_BIT;
		    //
		    item =  make_two_slot_record( task,  TAGGED_INT_FROM_C_INT(fdp->fd), TAGGED_INT_FROM_C_INT(flag) );

		    l = LIST_CONS(task, item, l);
		}
	    }
	    FREE(fds);
	    return l;
	}
    }
}

#else // HAS_SELECT
// #include <fcntl.h>/* 2008-03-15 CrT BUGGO -- DELETEME! Temporary debug hack. */


#undef  RESULT_VECTOR_BUF_SIZE
#define RESULT_VECTOR_BUF_SIZE 256

static Val   LIB7_Poll   (Task* task,  Val arg, struct timeval* timeout)   {
    //       =========
    //
    // The version of the polling operation for systems that provide BSD select.

    Val	    poll_list = GET_TUPLE_SLOT_AS_VAL(arg, 0);

    fd_set  rset;
    fd_set  wset;
    fd_set  eset;

    fd_set* rfds = NULL;
    fd_set* wfds = NULL;
    fd_set* efds = NULL;

    int	    maxFD = -1;
    int     status;

    int     fd;
    int     flag;

    Val	    l;
    Val     item;

								// printf("src/c/lib/posix-os/poll.c: Using 'select' implementation\n"); fflush(stdout);

    								// When using select() just to sleep, first arg to select() should be zero.
    for (l = poll_list;  l != LIST_NIL;  l = LIST_TAIL(l)) {
        //
	item	= LIST_HEAD(l);

	fd	= GET_TUPLE_SLOT_AS_INT(item, 0);
	flag	= GET_TUPLE_SLOT_AS_INT(item, 1);

	if ((flag & READABLE_BIT) != 0) {
								// int fd_flags = fcntl(fd,F_GETFL,0);
	    if (rfds == NULL) {
		rfds = &rset;
		FD_ZERO(rfds);
	    }
								// printf("src/c/lib/posix-os/poll.c: Will check fd %d for readability. fd flags x=%x O_NONBLOCK x=%x\n",fd,fd_flags,O_NONBLOCK);  fflush(stdout);
	    FD_SET (fd, rfds);
	}
	if ((flag & WRITABLE_BIT) != 0) {
	    if (wfds == NULL) {
		wfds = &wset;
		FD_ZERO(wfds);
	    }
								// printf("src/c/lib/posix-os/poll.c: Will check fd %d for writability.\n",fd);  fflush(stdout);
	    FD_SET (fd, wfds);
	}
	if ((flag & OOBDABLE_BIT) != 0) {
	    if (efds == NULL) {
		efds = &eset;
		FD_ZERO(efds);
	    }
								// printf("src/c/lib/posix-os/poll.c: Will check fd %d for oobdability.\n",fd);   fflush(stdout);
	    FD_SET (fd, efds);
	}
	if (fd > maxFD) maxFD = fd;
    }

								// printf("src/c/lib/posix-os/poll.c: maxFD d=%d\n",maxFD); fflush(stdout);

/**/  do { /**/							// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c
											// Restored 2012-08-07 CrT

	RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_OS_poll", &arg );
	    //
	    status = select (maxFD+1, rfds, wfds, efds, timeout);
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_OS_poll" );

 /**/  } while (status < 0 && errno == EINTR);	/**/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.

// printf("src/c/lib/posix-os/poll.c: result status d=%d.\n",status); fflush(stdout);

    if (status < 0)
        return RAISE_SYSERR__MAY_HEAPCLEAN(task, status, NULL);
    else if (status == 0)
	return LIST_NIL;
    else {
	// Figure number of bytes required for return value.
	// This is a 'status'-length list of intpairs.
	// Each CONS cell and intpair takes three words (two
	// for payload, one for implicit tagword) so:
	//
	int     bytes_needed  =  status * 6 * sizeof(Val);

	if (need_to_call_heapcleaner (task, bytes_needed)) {
	    //
	    Roots roots1 = { &arg, NULL };
	    //
	    call_heapcleaner_with_extra_roots (task, 0, roots1 );
	}

	Val     result_vector_buf[ RESULT_VECTOR_BUF_SIZE ];
	Val*	result_vector =  ( RESULT_VECTOR_BUF_SIZE >= status) ? result_vector_buf : MALLOC_VEC(Val, status);
	int	i;
	int	result_bitflags;

    	poll_list = GET_TUPLE_SLOT_AS_VAL(arg, 0);		// Re-fetch poll_list because heapcleaner may have moved it between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP or during explicit call.

	for (i = 0, l = poll_list;  l != LIST_NIL;  l = LIST_TAIL(l)) {
	    //
	    item	= LIST_HEAD(l);

	    fd		= GET_TUPLE_SLOT_AS_INT(item, 0);
	    flag	= GET_TUPLE_SLOT_AS_INT(item, 1);

	    result_bitflags	= 0;

	    if (((flag & READABLE_BIT) != 0) && FD_ISSET(fd, rfds))   result_bitflags |= READABLE_BIT;
	    if (((flag & WRITABLE_BIT) != 0) && FD_ISSET(fd, wfds))   result_bitflags |= WRITABLE_BIT;
	    if (((flag & OOBDABLE_BIT) != 0) && FD_ISSET(fd, efds))   result_bitflags |= OOBDABLE_BIT;

	    if (result_bitflags != 0) {
		item = make_two_slot_record( task, TAGGED_INT_FROM_C_INT(fd), TAGGED_INT_FROM_C_INT(result_bitflags) );			// make_two_slot_record		is from   src/c/h/make-strings-and-vectors-etc.h
		result_vector[i++] = item;
	    }
	}

	ASSERT(i == status);

	for (i = status-1, l = LIST_NIL;  i >= 0;  i--) {
	    item = result_vector[i];
	    l = LIST_CONS (task, item, l);
	}

// Overrunning the heap limit should be impossible with added
// 	if (need_to_call_heapcleaner (task, bytes_needed)) {
// call above, but let's be paranoid given that we've
// been getting a lot of segfaults out of this fn during
// the 2012-01 -> 2012-08 timeframe:
if (task->heap_allocation_pointer
 >= task->heap_allocation_limit)
{
 char* bar = "====================================================================\n";
 fprintf(stderr,"\n");
 fprintf(stderr,bar);
 fprintf(stderr,bar);
 fprintf(stderr,bar);
 fprintf("poll.c: task->heap_allocation_pointer %p > task->heap_allocation_limit %p!\n", task->heap_allocation_pointer, task->heap_allocation_limit);
 fprintf(stderr,bar);
 fprintf(stderr,bar);
 fprintf(stderr,bar);
 _exit(1);
}

	if (status > RESULT_VECTOR_BUF_SIZE) {
	    //
	    FREE( result_vector );
	}

	return l;
    }
}						// fun LIB7_Poll
#undef  RESULT_VECTOR_BUF_SIZE

#endif



// COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

