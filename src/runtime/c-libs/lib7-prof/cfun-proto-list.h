/* cfun-proto-list.h
 *
 */

#ifndef _CFUN_PROTO_LIST_
#define _CFUN_PROTO_LIST_

#ifndef _C_LIBRARY_
#  include "c-library.h"
#endif

/* the external definitions for the C functions */
#define CFUNC(NAME, FUNC, LIB7TYPE)	CFUNC_PROTO(NAME, FUNC, LIB7TYPE)
#include "cfun-list.h"
#undef CFUNC

#endif /* !_CFUN_PROTO_LIST_ */


/* COPYRIGHT (c) 1996 AT&T Research
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
