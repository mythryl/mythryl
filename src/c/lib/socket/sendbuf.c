// sendbuf.c


#include "../../mythryl-config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "raise-error.h"
#include "cfun-proto-list.h"

#include "hexdump-if.h"


/*
###         "Railroad carriages are pulled at the enormous speed
###          of fifteen miles per hour by engines which,
###          in addition to endangering life and limb of passengers,
###          roar and snort their way through the countryside,
###          setting fire to the crops, scaring the livestock,
###          and frightening women and children.
###
###         "The Almighty certainly never intended that
###          people should travel at such break-neck speed."
### 
###                -- President Martin Van Buren, 1829
*/



// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_sendbuf   (Task* task,  Val arg)   {
    //==================
    //
    // Mythryl type:
    //     ( Int,	    # socket fd
    //       Wy8Vector,     # byte vector
    //       Int,           # start offset
    //       Int,           # vector length (end offset)
    //       Bool,          # don't-route flag
    //       Bool           # default-oob flag
    //     )
    //     ->
    //     Int
    //
    // Send data from the buffer; bytes is either a rw_vector_of_one_byte_unts.Rw_Vector, or
    // a vector_of_one_byte_unts.vector.  The arguments are: socket, data buffer, start
    // position, number of bytes, OOB flag, and don't_route flag.
    //
    // This fn gets bound as   send_v, send_a   in:
    //
    //     src/lib/std/src/socket/socket-guts.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Sock_sendbuf");

    int   socket    = GET_TUPLE_SLOT_AS_INT( arg, 0);
    Val   buf       = GET_TUPLE_SLOT_AS_VAL( arg, 1);
    int   offset    = GET_TUPLE_SLOT_AS_INT( arg, 2);
    int   nbytes    = GET_TUPLE_SLOT_AS_INT( arg, 3);
    Val   oob       = GET_TUPLE_SLOT_AS_VAL( arg, 4);
    Val   dontroute = GET_TUPLE_SLOT_AS_VAL( arg, 5);									// Last use of 'arg'.

    char* heap_data = HEAP_STRING_AS_C_STRING(buf) + offset;

    // Compute flags parameter:
    //
    int                         flgs  = 0;
    if (oob       == HEAP_TRUE) flgs |= MSG_OOB;
    if (dontroute == HEAP_TRUE) flgs |= MSG_DONTROUTE;

//															log_if( "sendbuf.c/top: socket d=%d nbytes d=%d OOB=%s DONTROUTE=%s\n",
//																socket, nbytes, (oob == HEAP_TRUE) ? "TRUE" : "FALSE", (dontroute == HEAP_TRUE) ? "TRUE" : "FALSE"
//															);
//															hexdump_if( "sendbuf.c/top: Data to send: ", (unsigned char*)heap_data, nbytes );

    errno = 0;

    int n;

    // We cannot reference anything on the Mythryl heap
    // between RELEASE_MYTHRYL_HEAP and RECOVER_MYTHRYL_HEAP
    // because garbage collection might be moving
    // it around, so copy 'heap_data' into C storage: 
    //
    Mythryl_Heap_Value_Buffer  data_buf;
    //
    {   char* c_data =  buffer_mythryl_heap_value( &data_buf, (void*) heap_data, nbytes );

	RELEASE_MYTHRYL_HEAP( task->hostthread, "_lib7_Sock_sendbuf", NULL );
	    //
    /*      do { */	// Backed out 2010-02-26 CrT: See discussion at bottom of src/c/lib/socket/connect.c
		//
		n = send (socket, c_data, nbytes, flgs);
		//
if (errno == EINTR) puts("Error: EINTR in sendbuf.c\n");
    /*      } while (n < 0 && errno == EINTR);	*/	// Restart if interrupted by a SIGALRM or SIGCHLD or whatever.
	    //
	RECOVER_MYTHRYL_HEAP( task->hostthread, "_lib7_Sock_sendbuf" );
//															log_if( "sendbuf.c/bot: n d=%d errno d=%d\n", n, errno );
	unbuffer_mythryl_heap_value( &data_buf );
    }

    RETURN_STATUS_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, n, NULL);
}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

