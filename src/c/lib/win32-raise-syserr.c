// win32-raise-syserr.c

#include "../mythryl-config.h"

#include <windows.h>
#include "runtime-base.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-globals.h"
#include "lib7-c.h"


Val   RaiseSysError   (Task* task,  const char* altMsg,  char* at)   {
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

    Val	    s, syserror, arg, exn, atStk;
    const char	    *msg;
    char	    buf[32];
    int             errno = -1;

    if (altMsg != NULL) {
	msg = altMsg;
	syserror = OPTION_NULL;
    } else {
        errno = (int) GetLastError();
	sprintf(buf, "<win32 error code %d>", errno);
	msg = buf;
	syserror =  OPTION_THE(  task,  TAGGED_INT_FROM_C_INT(errno)  );
    }

    s = make_ascii_string_from_c_string__may_heapclean (task, msg);
    if (at == NULL) {
	atStk = LIST_NIL;
    } else {
	Val atMsg = make_ascii_string_from_c_string__may_heapclean (task, at);
	atStk = LIST_CONS(task, atMsg, LIST_NIL);
    }

    arg =  make_two_slot_record( task, s, syserror);

    exn =  MAKE_EXCEPTION (task, PTR_CAST( Val, RUNTIME_EXCEPTION__GLOBAL), arg, atStk);

    raise_mythryl_exception( task, exn );

    return exn;
}


// COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

