/* runtime-values.h
 *
 * Basic definitions for representing Lib7 values in C.
 *
 *   INT_LIB7toC(v)	-- convert an unboxed Lib7 value to a Word_t.
 *   INT_CtoLib7(i)	-- convert a Word_t to an unboxed Lib7 value.
 *   PTR_LIB7toC(ty, v)	-- convert a boxed Lib7 value to a (ty *).
 *   PTR_CtoLib7(p)	-- convert (Word_t *p) to an boxed Lib7 value.
 */

#ifndef _LIB7_VALUES_
#define _LIB7_VALUES_

#ifndef _LIB7_BASE_
#include "runtime-base.h"
#endif

/* typedef void *lib7_val_t; */	/* defined in runtime-base.h */

#ifdef BOXED1

#ifndef _ASM_
#define INT_LIB7toC(n)		(((Int_t)(n)) >> 1)
#define INT_CtoLib7(n)		((lib7_val_t)((n) << 1))
#define PTR_LIB7toC(ty,p)	((ty *)(((Addr_t)(p))-1))
#define PTR_CtoLib7(p)		((lib7_val_t)(((Addr_t)(p))+1))
#else
#define INT_CtoLib7(n)		((n)*2)
#endif /* !_ASM_ */

#else

#ifndef _ASM_

/* When the size of a C pointer differs from the size of an Lib7 value, the
 * pointer cast should first convert to a address sized integer before
 * the actual cast.  This causes problems, however, for gcc when used in
 * a static initialization; hence the PTR_CAST macro.
 */
#ifdef SIZES_C64_LIB732
#define PTR_CAST(ty, p)		((ty)(Addr_t)(p))
#else
#define PTR_CAST(ty, p)		((ty)(p))
#endif

#define INT_LIB7toC(n)		(((Int_t)(n)) >> 1)
#define INT_CtoLib7(n)		((lib7_val_t)(((n) << 1) + 1))
#define PTR_LIB7toC(ty,p)		PTR_CAST(ty *, p)
#define PTR_CtoLib7(p)		PTR_CAST(lib7_val_t, p)
#else
#define INT_CtoLib7(n)		(((n)*2)+1)
#endif /* !_ASM_ */

#endif /* BOXED1 */

#ifndef _ASM_

/* Convert an Lib7 pointer to an Addr_t value:
 */
#define PTR_LIB7toADDR(p)		((Addr_t)PTR_LIB7toC(void, p))

/* Lib7 record field selection:
 */
#define REC_SEL(p, i)	      ((PTR_LIB7toC(lib7_val_t, p))[i])
#define REC_SELPTR(ty, p, i)	PTR_LIB7toC(ty, REC_SEL(p, i))
#define REC_SELINT(p, i)	INT_LIB7toC(REC_SEL(p, i))

/* Extract the components of a ro_vector or rw_vector header:
 */
#define GET_SEQ_DATA(p)		REC_SEL(p, 0)
#define GET_SEQ_DATAPTR(ty, p)	REC_SELPTR(ty, p, 0)
#define GET_SEQ_LEN(p)		REC_SELINT(p, 1)

/* Turn a Lib7 string into a C string:
 */
#define STR_LIB7toC(p)		GET_SEQ_DATAPTR(char, p)

/* Extract the code address from a Lib7 closure:
 */
#define GET_CODE_ADDR(c)	(REC_SEL(c, 0))

#endif /* !_ASM_ */


/** Some basic Lib7 values **/
#define LIB7_void		INT_CtoLib7(0)
#define LIB7_false		INT_CtoLib7(0)
#define LIB7_true		INT_CtoLib7(1)
#define LIB7_nil		INT_CtoLib7(0)

#endif /* !_LIB7_VALUES_ */


/* COPYRIGHT (c) 1992 AT&T Bell Laboratories
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

