// close.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"




// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_close   (Task* task,  Val arg)   {
    //================
    //
    // Mythryl type: Socket -> Void
    //
    // This function gets bound as   close'   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

    int		status;
    int         fd      =  TAGGED_INT_TO_C_INT(arg);

    // XXX BUGGO FIXME:  Architecture dependencies code should
    // probably moved to       sockets-osdep.h

									log_if( "close.c/top: fd d=%d\n", fd );
    errno = 0;

    RELEASE_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_close", arg );
	//
	#if defined(OPSYS_WIN32)
	    status = closesocket(fd);
	#else
	/*  do { */							// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c

		status = close(fd);

	/*  } while (status < 0 && errno == EINTR);	*/		// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.
	#endif
	//
    RECOVER_MYTHRYL_HEAP( task->pthread, "_lib7_Sock_close" );

									log_if( "close.c/bot: status d=%d errno d=%d\n", status, errno);

    CHECK_RETURN_UNIT(task, status);
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

