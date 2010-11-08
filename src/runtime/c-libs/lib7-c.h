/* lib7-c.h
 *
 * Header file for C functions that are callable from lib7.  This defines
 * a number of macros for checking return results and for raising the SYSTEM_ERROR
 * exception:
 *
 *	RAISE_SYSERR(lib7_state, status)
 *          #
 *	    Raise the SYSTEM_ERROR exception using the
 *	    appropriate system error message (on
 *	    some systems, status may be an error code).
 *
 *	RAISE_ERROR(lib7_state, msg)
 *          #
 *          Raise the SYSTEM_ERROR exception using the
 *	    given message (with NULL for the system
 *	    error part).
 *
 *	CHECK_RETURN_VAL(lib7_state, status, val)
 *          #
 *   	    Check status for an error (< 0); if okay,
 *	    then return val.  Otherwise raise
 *	    SYSTEM_ERROR with the appropriate system
 *	    error message.
 *
 *	CHECK_RETURN(lib7_state, status)
 *          #
 *	    Check status for an error (< 0); if okay,
 *	    then return it as the result (after
 *	    converting to an Lib7 int).
 *
 *	CHECK_RETURN_UNIT(lib7_state, status)
 *          #
 * 	    Check status for an error (< 0); if okay,
 *	    then return Void.
 */

#ifndef _LIB7_C_
#define _LIB7_C_

#ifndef _LIB7_OSDEP_
#include "runtime-osdep.h"
#endif
					/* RaiseSysError	is in     src/runtime/c-libs/unix-raise-syserr.c
					*/

/* Snarfed this trick from
 *     http://www.decompile.com/cpp/faq/file_and_line_error_string.htm
 */
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifdef SYSCALL_RET_ERR

lib7_val_t RaiseSysError (lib7_state_t *lib7_state, int err, const char *alt_msg, const char *at);
#define RAISE_SYSERR(lib7_state, status)				\
	RaiseSysError((lib7_state), (status), NULL, "<" __FILE__ ":" TOSTRING(__LINE__) ">")
#define RAISE_ERROR(lib7_state, msg)	\
	RaiseSysError((lib7_state), 0, (msg), "<" __FILE__  ":" TOSTRING(__LINE__) ">")

#else
lib7_val_t RaiseSysError (lib7_state_t *lib7_state, const char *alt_msg, const char *at);
#define RAISE_SYSERR(lib7_state, status)	\
	RaiseSysError((lib7_state), NULL, "<" __FILE__  ":" TOSTRING(__LINE__) ">")
#define RAISE_ERROR(lib7_state, msg)	\
	RaiseSysError((lib7_state), (msg), "<" __FILE__  ":" TOSTRING(__LINE__) ">")

#endif

/* return a value to the calling Lib7 code, but raise an exception if an error
 * occured.
 */
#define CHECK_RETURN_VAL(lib7_state,status,val)	{			\
	if ((status) < 0)						\
	    return RAISE_SYSERR(lib7_state, status);			\
	else								\
	    return (val);						\
    }

/* Return status to the calling Lib7 code, but raise an exception if an error occurred */
#define CHECK_RETURN(lib7_state,status)	{				\
	int	__sts = (status);					\
	CHECK_RETURN_VAL((lib7_state), __sts, INT_CtoLib7(__sts))	\
    }

/* return Void to the calling Lib7 code, but raise an exception if an error occured */
#define CHECK_RETURN_UNIT(lib7_state,status)				\
	CHECK_RETURN_VAL(lib7_state, status, LIB7_void)

#endif /* !_Lib7_C_ */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

