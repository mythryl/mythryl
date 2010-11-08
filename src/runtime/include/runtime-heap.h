/* runtime-heap.h
 *
 * Macros and routines for allocating heap chunks.
 */

#ifndef _LIB7_HEAP_
#define _LIB7_HEAP_

#ifndef _LIB7_BASE_
#include "runtime-base.h"
#endif

#ifndef _LIB7_VALUES_
#include "runtime-values.h"
#endif

#ifndef _LIB7_STATE_
#include "runtime-state.h"
#endif

#ifndef _TAGS_
#include "tags.h"
#endif

/* Extract info from chunks: */
#define CHUNK_DESC(CHUNK)		REC_SEL((CHUNK), -1)
#define CHUNK_LEN(CHUNK)		GET_LEN(CHUNK_DESC(CHUNK))
#define CHUNK_TAG(CHUNK)		GET_TAG(CHUNK_DESC(CHUNK))


/** The size of a Lib7 record in bytes (including descriptor) **/
#define REC_SZB(n)	(((n)+1)*sizeof(lib7_val_t))


/** Heap allocation macros **/

#define LIB7_AllocWrite(lib7_state, i, x)	((((lib7_state)->lib7_heap_cursor))[i] = (x))

#define LIB7_Alloc(lib7_state, n)	(			\
    ((lib7_state)->lib7_heap_cursor += ((n)+1)),			\
    PTR_CtoLib7((lib7_state)->lib7_heap_cursor - (n)))

#define REF_ALLOC(lib7_state, r, a)	{			\
	lib7_state_t	*__lib7_state = (lib7_state);			\
	lib7_val_t	*__p = __lib7_state->lib7_heap_cursor;	\
	*__p++ = DESC_ref;				\
	*__p++ = (a);					\
	(r) = PTR_CtoLib7(__lib7_state->lib7_heap_cursor + 1);	\
	__lib7_state->lib7_heap_cursor = __p;			\
    }

#define REC_ALLOC1(lib7_state, r, a)	{				\
	lib7_state_t	*__lib7_state = (lib7_state);				\
	lib7_val_t	*__p = __lib7_state->lib7_heap_cursor;		\
	*__p++ = MAKE_DESC(1, DTAG_record);			\
	*__p++ = (a);						\
	(r) = PTR_CtoLib7(__lib7_state->lib7_heap_cursor + 1);		\
	__lib7_state->lib7_heap_cursor = __p;				\
    }

#define REC_ALLOC2(lib7_state, r, a, b)	{			\
	lib7_state_t	*__lib7_state = (lib7_state);				\
	lib7_val_t	*__p = __lib7_state->lib7_heap_cursor;		\
	*__p++ = DESC_pair;					\
	*__p++ = (a);						\
	*__p++ = (b);						\
	(r) = PTR_CtoLib7(__lib7_state->lib7_heap_cursor + 1);		\
	__lib7_state->lib7_heap_cursor = __p;				\
    }

#define REC_ALLOC3(lib7_state, r, a, b, c)	{			\
	lib7_state_t	*__lib7_state = (lib7_state);				\
	lib7_val_t	*__p = __lib7_state->lib7_heap_cursor;		\
	*__p++ = MAKE_DESC(3, DTAG_record);			\
	*__p++ = (a);						\
	*__p++ = (b);						\
	*__p++ = (c);						\
	(r) = PTR_CtoLib7(__lib7_state->lib7_heap_cursor + 1);		\
	__lib7_state->lib7_heap_cursor = __p;				\
    }

#define REC_ALLOC4(lib7_state, r, a, b, c, d)	{			\
	lib7_state_t	*__lib7_state = (lib7_state);				\
	lib7_val_t	*__p = __lib7_state->lib7_heap_cursor;		\
	*__p++ = MAKE_DESC(4, DTAG_record);			\
	*__p++ = (a);						\
	*__p++ = (b);						\
	*__p++ = (c);						\
	*__p++ = (d);						\
	(r) = PTR_CtoLib7(__lib7_state->lib7_heap_cursor + 1);		\
	__lib7_state->lib7_heap_cursor = __p;				\
    }

#define REC_ALLOC5(lib7_state, r, a, b, c, d, e)	{		\
	lib7_state_t	*__lib7_state = (lib7_state);				\
	lib7_val_t	*__p = __lib7_state->lib7_heap_cursor;		\
	*__p++ = MAKE_DESC(5, DTAG_record);			\
	*__p++ = (a);						\
	*__p++ = (b);						\
	*__p++ = (c);						\
	*__p++ = (d);						\
	*__p++ = (e);						\
	(r) = PTR_CtoLib7(__lib7_state->lib7_heap_cursor + 1);		\
	__lib7_state->lib7_heap_cursor = __p;				\
    }

#define REC_ALLOC6(lib7_state, r, a, b, c, d, e, f)	{		\
	lib7_state_t	*__lib7_state = (lib7_state);				\
	lib7_val_t	*__p = __lib7_state->lib7_heap_cursor;		\
	*__p++ = MAKE_DESC(6, DTAG_record);			\
	*__p++ = (a);						\
	*__p++ = (b);						\
	*__p++ = (c);						\
	*__p++ = (d);						\
	*__p++ = (e);						\
	*__p++ = (f);						\
	(r) = PTR_CtoLib7(__lib7_state->lib7_heap_cursor + 1);		\
	__lib7_state->lib7_heap_cursor = __p;				\
    }

#define SEQHDR_ALLOC(lib7_state, r, desc, data, len)	{		\
	lib7_state_t	*__lib7_state = (lib7_state);			\
	lib7_val_t	*__p = __lib7_state->lib7_heap_cursor;		\
	*__p++ = (desc);						\
	*__p++ = (data);						\
	*__p++ = INT_CtoLib7(len);					\
	(r) = PTR_CtoLib7(__lib7_state->lib7_heap_cursor + 1);		\
	__lib7_state->lib7_heap_cursor = __p;				\
    }

#ifdef ALIGN_REALDS
#define REAL64_ALLOC(lib7_state, r, d) {				\
	lib7_state_t	*__lib7_state = (lib7_state);				\
	lib7_val_t	*__p = __lib7_state->lib7_heap_cursor;		\
	__p = (lib7_val_t *)((Addr_t)__p | WORD_SZB);		\
	*__p++ = DESC_reald;					\
	(r) = PTR_CtoLib7(__p);					\
	*(double *)__p = (d);					\
	__p += REALD_SZW;					\
	__lib7_state->lib7_heap_cursor = __p;				\
    }
#else
#define REAL64_ALLOC(lib7_state, r, d) {				\
	lib7_state_t	*__lib7_state = (lib7_state);				\
	lib7_val_t	*__p = __lib7_state->lib7_heap_cursor;		\
	__p = (lib7_val_t *)((Addr_t)__p | WORD_SZB);		\
	(r) = PTR_CtoLib7(__p);					\
	*(double *)__p = (d);					\
	__p += REALD_SZW;					\
	__lib7_state->lib7_heap_cursor = __p;				\
    }
#endif

#define EXN_ALLOC(lib7_state, ex, id, val, where) \
	REC_ALLOC3(lib7_state, ex, id, val, where)

/** Boxed word values **/
#define WORD_LIB7toC(w)		(*PTR_LIB7toC(Word_t, w))
#define WORD_ALLOC(lib7_state, p, w)	{				\
	lib7_state_t	*__lib7_state = (lib7_state);				\
	lib7_val_t	*__p = __lib7_state->lib7_heap_cursor;		\
	*__p++ = MAKE_DESC(1, DTAG_raw32);			\
	*__p++ = (lib7_val_t)(w);					\
	(p) = PTR_CtoLib7(__lib7_state->lib7_heap_cursor + 1);		\
	__lib7_state->lib7_heap_cursor = __p;				\
    }
#define REC_SELWORD(p, i)	(*REC_SELPTR(Word_t, p, i))
#define INT32_LIB7toC(i)		(*PTR_LIB7toC(Int32_t, i))
#define INT32_ALLOC(lib7_state, p, i)	WORD_ALLOC(lib7_state, p, i)
#define REC_SELINT32(p, i)	(*REC_SELPTR(Int32_t, p, i))

/** Lib7 lists **/
#define LIST_hd(p)		REC_SEL(p, 0)
#define LIST_tl(p)		REC_SEL(p, 1)
#define LIST_nil		INT_CtoLib7(0)
#define LIST_isNull(p)		((p) == LIST_nil)
#define LIST_cons(lib7_state, r, a, b)	REC_ALLOC2(lib7_state, r, a, b)

/** Lib7 references **/
#define DEREF(r)		REC_SEL(r, 0)
#define ASSIGN(r, x)		(PTR_LIB7toC(lib7_val_t, r)[0] = (x))

/** Lib7 options **/
#define OPTION_NONE             INT_CtoLib7(0)
#define OPTION_SOME(lib7_state, r, a)  REC_ALLOC1(lib7_state, r, a)
#define OPTION_get(r)		REC_SEL(r, 0)

/** External routines **/
extern lib7_val_t LIB7_CString (lib7_state_t *lib7_state, const char *v);
extern lib7_val_t LIB7_CStringList (lib7_state_t *lib7_state, char **strs);
extern lib7_val_t LIB7_AllocString (lib7_state_t *lib7_state, int len);
extern lib7_val_t LIB7_AllocCode (lib7_state_t *lib7_state, int len);
extern lib7_val_t LIB7_AllocBytearray (lib7_state_t *lib7_state, int len);
extern lib7_val_t LIB7_AllocRealdarray (lib7_state_t *lib7_state, int len);
extern lib7_val_t LIB7_AllocArray (lib7_state_t *lib7_state, int len, lib7_val_t initVal);
extern lib7_val_t LIB7_AllocVector (lib7_state_t *lib7_state, int len, lib7_val_t initVal);
extern lib7_val_t LIB7_AllocRaw32 (lib7_state_t *lib7_state, int len);
extern void       LIB7_ShrinkRaw32 (lib7_state_t *lib7_state, lib7_val_t v, int nWords);
extern lib7_val_t LIB7_AllocRaw64 (lib7_state_t *lib7_state, int len);

extern lib7_val_t LIB7_SysConst (lib7_state_t *lib7_state, sysconst_table_t *table, int id);
extern lib7_val_t LIB7_SysConstList (lib7_state_t *lib7_state, sysconst_table_t *table);
extern lib7_val_t LIB7_AllocCData (lib7_state_t *lib7_state, int nbytes);
extern lib7_val_t LIB7_CData (lib7_state_t *lib7_state, void *data, int nbytes);

extern lib7_val_t BuildLiterals (lib7_state_t *lib7_state, Byte_t *lits, int len);

extern lib7_val_t _LIB7_string0[];
extern lib7_val_t _LIB7_vector0[];
#define LIB7_string0	PTR_CtoLib7(_LIB7_string0+1)
#define LIB7_vector0	PTR_CtoLib7(_LIB7_vector0+1)

#endif /* !_LIB7_HEAP_ */


/* COPYRIGHT (c) 1992 AT&T Bell Laboratories
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

