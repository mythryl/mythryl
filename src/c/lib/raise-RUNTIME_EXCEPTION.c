// raise-RUNTIME_EXCEPTION.c

#include "../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#ifdef HAS_STRERROR
#  include <string.h>
#endif
#include <stdio.h>
#include <errno.h>
#include "runtime-base.h"
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



Val   raise_sys_error__may_heapclean (
    //==============================
    //
    Task*	    task,
    const char*	    altMsg,
    const char*     at,			// C sourcefile and line number raising this error:  "<foo.c:37>"
    Roots*	    extra_roots
) {
    // Raise the Mythryl exception RUNTIME_EXCEPTION, which is defined as:
    //
    //    exception RUNTIME_EXCEPTION (String, Null_Or(System_Error) );
    //
    // We normally get invoked via either the
    // RAISE_SYSERR__MAY_HEAPCLEAN or RAISE_ERROR__MAY_HEAPCLEAN macro from
    //
    //     src/c/lib/lib7-c.h 
    //
    // For the time being, we use the errno value as the System_Error; eventually that
    // will be represented by an (Int, String) pair.  If alt_msg is non-zero,
    // then use it as the error string and use NULL for the System_Error.

    int error_number = errno;		// Various calls can trash this value so preserve it early.


    const char*	    msg;
    char	    buf[32];

    Val  null_or_errno;

    if (altMsg != NULL) {
	//
	msg           =  altMsg;
	null_or_errno =  OPTION_NULL;

    } else if ((msg = strerror(error_number)) != NULL) {

        null_or_errno =  OPTION_THE( task, TAGGED_INT_FROM_C_INT(error_number) );

    } else {

	sprintf(buf, "<unknown error %d>", error_number);
	msg = buf;
	null_or_errno =  OPTION_THE(  task,  TAGGED_INT_FROM_C_INT(error_number)  );
    }

    #if (defined(DEBUG_OS_INTERFACE) || defined(DEBUG_TRACE_CCALL))
	debug_say ("RaiseSysError: errno = %d, msg = \"%s\"\n",
	    (altMsg != NULL) ? -1 : error_number, msg);
    #endif

    Roots roots1 = { &null_or_errno, extra_roots };

    Val errno_string = make_ascii_string_from_c_string__may_heapclean (task, msg, &roots1 );

    Val at_list;			// [] or [ "<foo.c:187>" ].
    //
    if (at != NULL) {
        //
	Roots roots2 = { &errno_string, &roots1 };

	Val at_cstring
            =
	    make_ascii_string_from_c_string__may_heapclean (task, at, &roots2 );

	at_list = LIST_CONS(task, at_cstring, LIST_NIL);

    } else {

	at_list = LIST_NIL;
    }

    Val arg = make_two_slot_record( task,  errno_string, null_or_errno);

    Val syserr_exception =   MAKE_EXCEPTION(task, PTR_CAST( Val, RUNTIME_EXCEPTION__GLOBAL), arg, at_list);

    // Modify the task state so that 'syserr_exception'
    // will be raised when Mythryl execution resumes:
    //
    raise_mythryl_exception( task, syserr_exception );		// raise_mythryl_exception	is from    src/c/main/run-mythryl-code-and-runtime-eventloop.c

    return  syserr_exception;
}								// fun raise_sys_error__may_heapclean

// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

