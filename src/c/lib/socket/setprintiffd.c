// setprintiffd.c


#include "../../mythryl-config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "raise-error.h"
#include "cfun-proto-list.h"




// One of the library bindings exported via
//     src/c/lib/socket/cfun-list.h
// and thence
//     src/c/lib/socket/libmythryl-socket.c



Val   _lib7_Sock_setprintiffd   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:   Int -> Void
    //
    // This fn gets bound as   set_printif_fd   in:
    //
    //     src/lib/std/src/socket/internet-socket--premicrothread.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    int fd    =  TAGGED_INT_TO_C_INT(arg);

    log_if_fd = fd;

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return HEAP_VOID;
}


// COPYRIGHT (c) 2010 by Jeff Prothero,
// released per terms of SMLNJ-COPYRIGHT.
