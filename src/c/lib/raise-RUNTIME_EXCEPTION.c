// raise-RUNTIME_EXCEPTION.c

#include "../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#ifdef HAS_STRERROR
#  include <string.h>
#endif
#include <stdio.h>
#include <errno.h>
#include "runtime-base.h"
#include "task.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-globals.h"
#include "lib7-c.h"


#ifndef HAS_STRERROR
    //
    static char*   strerror   (int errnum)   {
        //
        // An implementation of strerror for
        // those systems that do not provide it.

	extern int	sys_nerr;
	extern char	*sys_errlist[];

	if (errnum < 0
        || sys_nerr <= errnum
	){
	      return "<unknown system error>";
	else  return sys_errlist[errnum];

    }
#endif



Val   RaiseSysError (
    //=============
    //
    Task*  task,
    const char*    altMsg,
    const char*    at			// C sourcefile and line number raising this error:  "<foo.c:37>"

) {
    // Raise the Mythryl exception RUNTIME_EXCEPTION, which is defined as:
    //
    //    exception RUNTIME_EXCEPTION (String, Null_Or(System_Error) );
    //
    // We normally get invoked via either the
    // RAISE_SYSERR or RAISE_ERROR macro from
    //
    //     src/c/lib/lib7-c.h 
    //
    // For the time being, we use the errno value as the System_Error; eventually that
    // will be represented by an (Int, String) pair.  If alt_msg is non-zero,
    // then use it as the error string and use NULL for the System_Error.

    int error_number = errno;		// Various calls can trash this value so preserve it early.

    Val	    errno_string;		// From strerror(errno).
    Val	    at_list;			// [] or [ "<foo.c:187>" ].
    Val	    null_or_errno;		// NULL or (THE errno).
    Val	    arg;			// Holds pair (errno_string, null_or_errno).
    Val	    syserr_exception;		// Final return value and exception raised.

    const char*	    msg;
    char	    buf[32];

    if (altMsg != NULL) {
	//
	msg           =  altMsg;
	null_or_errno =  OPTION_NULL;

    } else if ((msg = strerror(error_number)) != NULL) {

	OPTION_THE (task, null_or_errno, TAGGED_INT_FROM_C_INT(error_number))

    } else {

	sprintf(buf, "<unknown error %d>", error_number);
	msg = buf;
	OPTION_THE(task, null_or_errno, TAGGED_INT_FROM_C_INT(error_number));
    }

    #if (defined(DEBUG_OS_INTERFACE) || defined(DEBUG_TRACE_CCALL))
	debug_say ("RaiseSysError: errno = %d, msg = \"%s\"\n",
	    (altMsg != NULL) ? -1 : error_number, msg);
    #endif

    errno_string
	=
	make_ascii_string_from_c_string (task, msg);

    if (at != NULL) {

	Val  at_cstring
            =
            make_ascii_string_from_c_string (task, at);

	LIST_CONS(task, at_list, at_cstring, LIST_NIL);

    } else {

	at_list = LIST_NIL;
    }

    REC_ALLOC2 (task, arg, errno_string, null_or_errno);
    EXN_ALLOC  (task, syserr_exception, PTR_CAST( Val, RUNTIME_EXCEPTION_GLOBAL), arg, at_list);

    // Modify the Lib7 state so that 'syserr_exception'
    // will be raised when Mythryl execution resumes:
    //
    raise_mythryl_exception( task, syserr_exception );		// raise_mythryl_exception	is from    src/c/main/run-mythryl-code-and-runtime-eventloop.c

    return  syserr_exception;
}								// fun RaiseSysError

// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

