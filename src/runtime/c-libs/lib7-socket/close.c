/* close.c
 *
 */

#include "../../config.h"

#include <errno.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#include "print-if.h"

/* _lib7_Sock_close : Socket -> Void
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t

_lib7_Sock_close (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		status;
    int         fd      =  INT_LIB7toC(arg);

    /* FIXME:  Architecture dependencies code should probably moved to
       sockets-osdep.h */

    print_if( "close.c/top: fd d=%d\n", fd );
    errno = 0;

#if defined(OPSYS_WIN32)
    status = closesocket(fd);
#else
/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        status = close(fd);

/*  } while (status < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/
#endif

    print_if( "close.c/bot: status d=%d errno d=%d\n", status, errno);

    CHECK_RETURN_UNIT(lib7_state, status);
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
