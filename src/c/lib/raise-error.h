// raise-error.h
//
// Header file for C functions that are callable from Mythryl.
// We define a number of macros for checking return results
// and for raising the RUNTIME_EXCEPTION exception:
//
//	RAISE_SYSERR__MAY_HEAPCLEAN(task, status, extra_roots)
//          #
//	    Raise the RUNTIME_EXCEPTION exception using the
//	    appropriate system error message (on
//	    some systems, status may be an error code).
//
//	RAISE_ERROR__MAY_HEAPCLEAN(task, msg, extra_roots)
//          #
//          Raise the RUNTIME_EXCEPTION exception using the
//	    given message (with NULL for the system
//	    error part).
//
//	RETURN_VAL_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, val, extra_roots)
//          #
//   	    Check status for an error (< 0); if okay,
//	    then return val.  Otherwise raise
//	    RUNTIME_EXCEPTION with the appropriate system
//	    error message.
//
//	RETURN_STATUS_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, extra_roots)
//          #
//	    Check status for an error (< 0); if okay,
//	    then return it as the result (after
//	    converting to a Mythryl Int31).
//
//	RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status)
//          #
// 	    Check status for an error (< 0); if okay,
//	    then return Void.


#ifndef LIB7_C_H
#define LIB7_C_H

#include "system-dependent-stuff.h"
					// raise_error__may_heapclean	def in     src/c/lib/raise-error.c


// Snarfed this trick from
//     http://www.decompile.com/cpp/faq/file_and_line_error_string.htm
//
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)



#ifdef SYSTEM_CALLS_RETURN_ERROR_CODES					// This will be (only) TRUE on MacOS.

Val raise_error__may_heapclean (Task *task, int err, const char *alt_msg, const char *at, Roots* extra_roots);			// raise_error__may_heapclean	def in    src/c/lib/raise-error.c

#define RAISE_SYSERR__MAY_HEAPCLEAN(task, status, extra_roots)		raise_error__may_heapclean((task), (status), NULL,  "<" __FILE__ ":" TOSTRING(__LINE__) ">", extra_roots)
#define RAISE_ERROR__MAY_HEAPCLEAN(task, msg, extra_roots)		raise_error__may_heapclean((task), 0,        (msg), "<" __FILE__ ":" TOSTRING(__LINE__) ">", extra_roots)

#else

Val raise_error__may_heapclean (Task *task, const char *alt_msg, const char *at, Roots* extra_roots);

#define RAISE_SYSERR__MAY_HEAPCLEAN(task, status, extra_roots)		raise_error__may_heapclean((task), NULL,  "<" __FILE__  ":" TOSTRING(__LINE__) ">", extra_roots)
#define RAISE_ERROR__MAY_HEAPCLEAN(task, msg, extra_roots)		raise_error__may_heapclean((task), (msg), "<" __FILE__  ":" TOSTRING(__LINE__) ">", extra_roots)

#endif



// Return a value to the calling Mythryl code,
// but raise an exception if an error occurred.
// NB: 'status' must not have side-effects:
//
#define RETURN_VAL_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task,status,val,extra_roots)	\
	  (((status) < 0)										\
	   ? RAISE_SYSERR__MAY_HEAPCLEAN(task, status, extra_roots)					\
	   : (val)					    						\
	   )

// Return status to the calling Mythryl code,
// but raise an exception if an error occurred:
// NB: 'status' must not have side-effects:
//
#define RETURN_STATUS_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task,status,extra_roots)	\
	\
	RETURN_VAL_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN((task), status, TAGGED_INT_FROM_C_INT(status), extra_roots)

// Return Void to the calling Mythryl code,
// but raise an exception if an error occurred:
// NB: 'status' must not have side-effects:
//
#define RETURN_VOID_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, extra_roots)	\
	\
	RETURN_VAL_EXCEPT_RAISE_SYSERR_ON_NEGATIVE_STATUS__MAY_HEAPCLEAN(task, status, HEAP_VOID, extra_roots)

#endif // LIB7_C_H



// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


