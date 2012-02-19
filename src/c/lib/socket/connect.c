// connect.c

#include "../../mythryl-config.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>

       /* For select(): */

       /* According to POSIX.1-2001 */
       #include <sys/select.h>

       /* According to earlier standards */
       #include <sys/time.h>
       #include <sys/types.h>
       #include <unistd.h>


#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"




/*
###        "How often, or on what system, the Thought Police
###         plugged in any individual wire was guesswork.
###
###         It was even conceivable that they watched
###         everybody all the time.
###
###         But at any rate, they could plug in your wire
###         whenever they wanted to."
###
###                             -- George Orwell, 1984
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_connect   (Task* task,  Val arg)   {
    //==================
    //
    // _lib7_Sock_connect: (Socket, Address) -> Void
    //
    // This function gets bound as   connect'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_connect");

    int status;

    int	socket = GET_TUPLE_SLOT_AS_INT( arg, 0 );
    Val	addr   = GET_TUPLE_SLOT_AS_VAL( arg, 1 );
    //
    socklen_t addrlen              =  GET_VECTOR_LENGTH(                         addr );

    {   unsigned char* a = GET_VECTOR_DATACHUNK_AS( unsigned char*, addr );
        char buf[ 1024 ];

										// Translate to hex for log:
										buf[0] = '\0';
										for (int i = 0; i < addrlen; ++i) {
										    //
										    sprintf (buf+strlen(buf), "%02x.", a[i]);
										}
										log_if( "connect.c/top: socket d=%d addrlen d=%d addr s='%s'\n", socket, addrlen, buf );
	// Copy address from Mythryl heap into C stack because
	// we cannot reference the Mythryl heap between
	// RELEASE_MYTHRYL_HEAP and
	// RECOVER_MYTHRYL_HEAP -- the heapcleaner may be
	// moving stuff around on us:
	//
	for (int i = 0; i < addrlen; ++i) {
	    //
	    buf[i] = a[i];
	}

	errno = 0;

	RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_connect", arg );
	    //
	    status =  connect (socket, (struct sockaddr*)buf, addrlen );
	    //
	RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_connect" );
    }


    // NB: Unix Network Programming p135 S5.9 says that
    //     for connect() we cannot just retry on EINTR.
    //     On p452 it says we must instead do a select(),
    //     which will wait until the three-way TCP
    //     handshake either succeeds or fails:
    //
    // Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c
#ifdef SOME_OTHER_TIME
    if (status < 0 && errno == EINTR) {

        int eintr_count = 1;

        int maxfd = socket+1;

	fd_set read_set;
	fd_set write_set;

	do {
	    log_if( "connect.c/mid: Caught EINTR #%d, doing a select() on fd %d\n", eintr_count, socket);

	    FD_ZERO( &read_set);
	    FD_ZERO(&write_set);

	    FD_SET( socket,  &read_set );
	    FD_SET( socket, &write_set );

	    errno = 0;

	    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_connect", arg );
		//
		status = select(maxfd, &read_set, &write_set, NULL, NULL); 
		//
	    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_connect" );

            ++eintr_count;

	} while (status < 0 && errno == EINTR);

	// According to p452, if the connection completes properly
        // the socket will be writable, but if it fails it will be
        // both readable and writable.  On return 'status' is the
        // count of bits set in the fd_sets;  if it is 2, the fd
        // is both readable and writable, implying connect failure.
        // To be on the safe side, in this case I ensure that status
        // is negative and errno set to something valid for a failed
        // connect().  I don't know if this situation is even possible:
        //
	if (status == 2) {
	    status = -1;
	    errno  = ENETUNREACH;	// Possibly ETIMEDOUT would be better?
	}
    }
#endif

    log_if( "connect.c/bot: status d=%d errno d=%d\n", status, errno);

    RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, NULL);		// RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN	is from   src/c/lib/lib7-c.h
}

// EINTR discussion:      (2010-02-26 CrT)
//
// I noticed that the C code was pervasively not retrying
// interrupted "slow" system calls, i.e. those returning
// EINTR, so I made a pass through the code adding retry
// logic.
//
// Unfortunately, the x-kit logic then hangs in recv.c
// and cannot be revived even via ^C. (kill -SIGHUP does
// shut it down.)
//
// John H Reppy tells me in email that not retrying on EINTR
// was a deliberate CML design decision, in part to maintain
// responsiveness to ^C.
//
// The mooted retry logic appears to be that in:
//
//     src/lib/std/src/threadkit/posix/retry-syscall-in-eintr.pkg
//
// I don't understand the relevant code, but it seems to me
// this implies that the system must be spending significant
// amounts of time sitting blocked and idle -- time when it
// could be doing useful computation -- so this seems like a
// design decision which might be right for a lab prototype
// but which cannot possibly be right for production use...?!
//
// It may be that we need to run two POSIX threads, one to block
// on slow system calls when needed, the other to keep the Mythryl
// concurrent programming stuff moving smartly along.
//
// However, that sounds like a whole new ball of worms, and I'm
// immersed at the moment in checking out the X-kit stuff, so for
// the moment I'm just going to back out all the EINTR-retry logic
// I added.
//
// Google does turn up the following email, which indicates that at
// least on Linux, for at least some system calls, SA_RESTART can be
// set to automate restarting of interrupted slow system calls. Since
// this may not work on other kernels, and result in obscure flaky
// problems, it may be more sensible to just program restarts everywhere
// once things have be re-architected to allow that.
//
//     http://lkml.indiana.edu/hypermail/linux/kernel/0807.2/2268.html
//     Re: EINTR under Linux
//     From: Michael Kerrisk 
//     Date:  Mon Jul 21 2008 - 06:43:57 EST 
//     Next message:   Joshua C.: "ACPI: ACPI_CUSTOM_DSDT_INITRD option" 
//     Previous message:   Ramax Lo: "Re: Add to_irq fields to gpiolib (with sample implementation)" 
//     In reply to:   Robert Hancock: "Re: EINTR under Linux" 
//     Messages sorted by: [ date ] [ thread ] [ subject ] [ author ] 
//      On 7/18/08, Robert Hancock <hancockr@xxxxxxx> wrote:
//      > akineko wrote:
//      >
//      > > Hello,
//      > >
//      > > I have a socket program that is running flawlessly under Solaris.
//      > > When I re-compiled it under Linux (CentOS 5.1) and run it, I got the
//      > > following error:
//      > >
//      > > recv() failed: Interrupted system call
//      > >
//      > > This only occurs very infrequently (probably one out of a million
//      > > packets exchanged).
//      > >
//      > > select() in my program is getting EINTR.
//      > >
//      > > From the postings I found in the news group seem suggesting that it is
//      > > due to GC.
//      > >
//      > >
//      > > > The GC sends signals to each thread which causes them all to enter a
//      > stop-the-world state. When the GC
//      > > > is finished, all the threads are resumed. When the threads are
//      > resumed, any that were blocked in a
//      > > > blocking system call (like poll()) will return with EINTR. Normally you
//      > would just retry the system call.
//      > > >
//      > >
//      > > So, I added to check if the errno == EINTR and now my program seems
//      > > working fine.
//      > >
//      > > //
//      > >
//      > > My question I would like to ask in this group is:
//      > > Does this mean any system call under Linux could return empty-hand
//      > > with EINTR due to GC?
//      > > I usually assume fatal if system call returns -1.
//      > > It is quite painful to check all system-call return status.
//      > >
//      > > My second question is:
//      > > Does this can occur in other OS's? (free-BSD, Solaris, ...)
//      > > Or, is this specific to Linux OS?
//      > >
//      >
//      > I'm not sure what the GC you're referring to is, but I assume it's using a
//      > signal handler for that stop signal. If the signal handler is not installed
//      > with the SA_RESTART flag, then if a system call is interrupted by that
//      > signal it will get EINTR instead of being restarted automatically. For some
//      > system calls, EINTR can still occur, for example, see:
//      >
//      > http://www.opengroup.org/onlinepubs/007908775/xsh/select.html
//      >
//      > This is not Linux specific, but the specs allow for some different behavior
//      > between UNIX variants.
//      
//      And the signal.7 page has been very recently updated to include
//      Linux-specific details for most system calls. Have a look here:
//      
//      http://www.kernel.org/doc/man-pages/online/pages/man7/signal.7.html
//      
//      Basically, recv() is restarted if you use SA_RESTART, but select() is
//      never restarted, regardless of SA_RESTART (and POSIX.1 allows this).
//
// Later notes:
//     Drake advises that select() is an abomination and poll() rocks. :-)
//     He points to
//         http://www.madore.org/~david/computers/connect-intr.html
//     which notes that Linux (alone) actually makes connect() restartable,
//     and also
//         http://cr.yp.to/docs/connect.html
//     where DJ Bernstein discusses some nonblocking connect() issues.
//     This paper recommends Linux Real-time signals as superior to select/poll
//     when doing webserver extreme performance optimization:
//         http://www.hpl.hp.com/techreports/2000/HPL-2000-174.pdf
//
//



// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

