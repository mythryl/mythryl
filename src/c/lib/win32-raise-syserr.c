// win32-raise-syserr.c

#include "../mythryl-config.h"

#include <windows.h>
#include "runtime-base.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-globals.h"
#include "raise-error.h"


Val   raise_sys_error__may_heapclean   (Task* task,  const char* altMsg,  char* at, Roots* extra_roots)   {
    //
    // RaiseSysError:
    //
    // Raise the Mythryl exception SysErr, which has the spec:
    //
    //    exception RUNTIME_EXCEPTION (String, Null_Or(System_Error))
    //
    // We use the last win32-api error value as the System_Error; eventually that
    // will be represented by an (int * String) pair.  If alt_msg is non-zero,
    // then use it as the error string and use NULL for the System_Error.


    const char	    *msg;
    char	    buf[32];
    int             errno = -1;

    Val	    syserror;

    if (altMsg != NULL) {
	msg = altMsg;
	syserror = OPTION_NULL;
    } else {
        errno = (int) GetLastError();
	sprintf(buf, "<win32 error code %d>", errno);
	msg = buf;
	syserror =  OPTION_THE(  task,  TAGGED_INT_FROM_C_INT(errno)  );
    }

    Roots roots1 = { &syserror, extra_roots };

    Val string = make_ascii_string_from_c_string__may_heapclean (task, msg, &roots1 );

    Val   at_stk;

    if (at == NULL) {
	//
	at_stk = LIST_NIL;
	//
    } else {
	//
	Roots roots2 =  { &string, &roots1 };
	//
	Val at_msg =  make_ascii_string_from_c_string__may_heapclean (task, at, &roots2 );
	//
	at_stk = LIST_CONS(task, at_msg, LIST_NIL);
    }

    Val arg =  make_two_slot_record( task, string, syserror);

    Val exn =  MAKE_EXCEPTION (task, PTR_CAST( Val, RUNTIME_EXCEPTION__GLOBAL), arg, at_stk);

    raise_mythryl_exception( task, exn );

    return exn;
}


// COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

