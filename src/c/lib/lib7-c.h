// lib7-c.h
//
// Header file for C functions that are callable from Mythryl.
// We define a number of macros for checking return results
// and for raising the RUNTIME_EXCEPTION exception:
//
//	RAISE_SYSERR(task, status)
//          #
//	    Raise the RUNTIME_EXCEPTION exception using the
//	    appropriate system error message (on
//	    some systems, status may be an error code).
//
//	RAISE_ERROR(task, msg)
//          #
//          Raise the RUNTIME_EXCEPTION exception using the
//	    given message (with NULL for the system
//	    error part).
//
//	CHECK_RETURN_VAL(task, status, val)
//          #
//   	    Check status for an error (< 0); if okay,
//	    then return val.  Otherwise raise
//	    RUNTIME_EXCEPTION with the appropriate system
//	    error message.
//
//	CHECK_RETURN(task, status)
//          #
//	    Check status for an error (< 0); if okay,
//	    then return it as the result (after
//	    converting to an Lib7 int).
//
//	CHECK_RETURN_UNIT(task, status)
//          #
// 	    Check status for an error (< 0); if okay,
//	    then return Void.


#ifndef LIB7_C_H
#define LIB7_C_H

#include "system-dependent-stuff.h"
					// RaiseSysError	is in     src/c/lib/raise-RUNTIME_EXCEPTION.c


// Snarfed this trick from
//     http://www.decompile.com/cpp/faq/file_and_line_error_string.htm
//
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifdef SYSTEM_CALLS_RETURN_ERROR_CODES

Val RaiseSysError (Task *task, int err, const char *alt_msg, const char *at);			// RaiseSysError	def in    src/c/lib/raise-RUNTIME_EXCEPTION.c
#define RAISE_SYSERR(task, status)				\
	RaiseSysError((task), (status), NULL, "<" __FILE__ ":" TOSTRING(__LINE__) ">")
#define RAISE_ERROR(task, msg)	\
	RaiseSysError((task), 0, (msg), "<" __FILE__  ":" TOSTRING(__LINE__) ">")

#else
Val RaiseSysError (Task *task, const char *alt_msg, const char *at);
#define RAISE_SYSERR(task, status)	\
	RaiseSysError((task), NULL, "<" __FILE__  ":" TOSTRING(__LINE__) ">")
#define RAISE_ERROR(task, msg)	\
	RaiseSysError((task), (msg), "<" __FILE__  ":" TOSTRING(__LINE__) ">")

#endif

// Return a value to the calling Lib7 code,
// but raise an exception if an error occurred.
//
#define CHECK_RETURN_VAL(task,status,val)	{			\
	if ((status) < 0)						\
	    return RAISE_SYSERR(task, status);			\
	else								\
	    return (val);						\
    }

// Return status to the calling Lib7 code,
// but raise an exception if an error occurred:
//
#define CHECK_RETURN(task,status)	{				\
	int	__sts = (status);					\
	CHECK_RETURN_VAL((task), __sts, INT31_FROM_C_INT(__sts))	\
    }

// Return Void to the calling Lib7 code,
// but raise an exception if an error occurred:
//
#define CHECK_RETURN_UNIT(task,status)				\
	CHECK_RETURN_VAL(task, status, HEAP_VOID)

#endif // LIB7_C_H



// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


