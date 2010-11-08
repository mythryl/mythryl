/* build-literals.c
 *
 * COPYRIGHT (c) 1997 Bell Labs, Lucent Technologies.
 */

#include "../config.h"

#include "runtime-base.h"
#include "runtime-heap.h"
#include "heap.h"
#include <string.h>

/* Codes for literal machine instructions (version 1):
 *   INT(i)		0x01 <i>
 *   RAW32[i]		0x02 <i>
 *   RAW32[i1,..,in]	0x03 <n> <i1> ... <in>
 *   RAW64[r]		0x04 <r>
 *   RAW64[r1,..,rn]	0x05 <n> <r1> ... <rn>
 *   STR[c1,..,cn]	0x06 <n> <c1> ... <cn>
 *   LIT(k)		0x07 <k>
 *   VECTOR(n)		0x08 <n>
 *   RECORD(n)		0x09 <n>
 *   RETURN		0xff
 */
#define V1_MAGIC	0x19981022
#define I_INT		0x01
#define I_RAW32		0x2
#define I_RAW32L	0x3
#define I_RAW64		0x4
#define I_RAW64L	0x5
#define I_STR		0x6
#define I_LIT		0x7
#define I_VECTOR	0x8
#define I_RECORD	0x9
#define I_RETURN	0xff

#define _B0(p)		((p)[pc])
#define _B1(p)		((p)[pc+1])
#define _B2(p)		((p)[pc+2])
#define _B3(p)		((p)[pc+3])

#define GET32(p)	\
    ((_B0(p) << 24) | (_B1(p) << 16) | (_B2(p) << 8) | _B3(p))

/* the size of a list cons cell in bytes */
#define CONS_SZB	(WORD_SZB*3)

/* GetDouble:
 */
static double GetDouble (Byte_t *p)
{
    int		i;
    union {
	double		d;
	Byte_t		b[sizeof(double)];
    }		u;

#ifdef BYTE_ORDER_LITTLE
    for (i = sizeof(double)-1;  i >= 0;  i--) {
	u.b[i] = *p++;
    }
#else
    for (i = 0;  i < sizeof(double);  i++) {
	u.b[i] = p[i];
    }
#endif

    return u.d;

} /* end of GetDouble */


/* BuildLiterals:
 *
 * NOTE: we allocate all of the chunks in the first generation, and allocate
 * the vector of literals in the allocation space.
 */
lib7_val_t BuildLiterals (lib7_state_t *lib7_state, Byte_t *lits, int len)
{
    int		pc = 0;
    Word_t	magic, maxDepth;
    lib7_val_t	stk;
    lib7_val_t	res;
    Int32_t	i, j, n;
    double	d;
    int		availSpace, spaceReq;

/* A check that the available space is sufficient for the literal chunk that
 * we are about to allocate.  Note that the conce cell has already been accounted
 * for in availSpace (but not in spaceReq).
 */
#define GC_CHECK									\
    do {										\
	if ((spaceReq > availSpace) && need_to_collect_garbage(lib7_state, spaceReq+CONS_SZB)) {		\
	    collect_garbage_with_extra_roots (lib7_state, 0, (lib7_val_t *)&lits, &stk, NULL);	\
	    availSpace = 0;								\
	}										\
	else										\
	    availSpace -= spaceReq;							\
    } while (0)

#ifdef DEBUG_LITERALS
SayDebug("BuildLiterals: lits = %#x, len = %d\n", lits, len);
#endif
    if (len <= 8) return LIB7_nil;

    magic = GET32(lits); pc += 4;
    maxDepth = GET32(lits); pc += 4;

    if (magic != V1_MAGIC) {
	Die("bogus literal magic number %#x", magic);
    }

    stk = LIB7_nil;
    availSpace = 0;
    for (;;) {
	ASSERT(pc < len);
	availSpace -= CONS_SZB;	/* space for stack cons cell */
	if (availSpace < ONE_K) {
	    if (need_to_collect_garbage(lib7_state, 64*ONE_K)) {
		collect_garbage_with_extra_roots (lib7_state, 0, (lib7_val_t *)&lits, &stk, NULL);
            }
	    availSpace = 64*ONE_K;
	}
	switch (lits[pc++]) {
	  case I_INT:
	    i = GET32(lits); pc += 4;
#ifdef DEBUG_LITERALS
SayDebug("[%2d]: INT(%d)\n", pc-5, i);
#endif
	    LIST_cons(lib7_state, stk, INT_CtoLib7(i), stk);
	    break;
	  case I_RAW32:
	    i = GET32(lits); pc += 4;
#ifdef DEBUG_LITERALS
SayDebug("[%2d]: RAW32[%d]\n", pc-5, i);
#endif
	    INT32_ALLOC(lib7_state, res, i);
	    LIST_cons(lib7_state, stk, res, stk);
	    availSpace -= 2*WORD_SZB;
	    break;
	  case I_RAW32L:
	    n = GET32(lits); pc += 4;
#ifdef DEBUG_LITERALS
SayDebug("[%2d]: RAW32L(%d) [...]\n", pc-5, n);
#endif
	    ASSERT(n > 0);
	    spaceReq = 4*(n+1);
	    GC_CHECK;
	    LIB7_AllocWrite (lib7_state, 0, MAKE_DESC(n, DTAG_raw32));
	    for (j = 1;  j <= n;  j++) {
		i = GET32(lits); pc += 4;
		LIB7_AllocWrite (lib7_state, j, (lib7_val_t)i);
	    }
	    res = LIB7_Alloc (lib7_state, n);
	    LIST_cons(lib7_state, stk, res, stk);
	    break;
	  case I_RAW64:
	    d = GetDouble(&(lits[pc]));  pc += 8;
	    REAL64_ALLOC(lib7_state, res, d);
#ifdef DEBUG_LITERALS
SayDebug("[%2d]: RAW64[%f] @ %#x\n", pc-5, d, res);
#endif
	    LIST_cons(lib7_state, stk, res, stk);
	    availSpace -= 4*WORD_SZB;	/* extra 4 bytes for alignment padding  */
	    break;
	  case I_RAW64L:
	    n = GET32(lits); pc += 4;
#ifdef DEBUG_LITERALS
SayDebug("[%2d]: RAW64L(%d) [...]\n", pc-5, n);
#endif
	    ASSERT(n > 0);
	    spaceReq = 8*(n+1);
	    GC_CHECK;
#ifdef ALIGN_REALDS
	  /* Force REALD_SZB alignment (descriptor is off by one word) */
	    lib7_state->lib7_heap_cursor = (lib7_val_t *)((Addr_t)(lib7_state->lib7_heap_cursor) | WORD_SZB);
#endif
	    j = 2*n; /* number of words */
	    LIB7_AllocWrite (lib7_state, 0, MAKE_DESC(j, DTAG_raw64));
	    res = LIB7_Alloc (lib7_state, j);
	    for (j = 0;  j < n;  j++) {
		PTR_LIB7toC(double, res)[j] = GetDouble(&(lits[pc]));  pc += 8;
	    }
	    LIST_cons(lib7_state, stk, res, stk);
	    break;

	  case I_STR:
	    n = GET32(lits); pc += 4;
#ifdef DEBUG_LITERALS
SayDebug("[%2d]: STR(%d) [...]", pc-5, n);
#endif
	    if (n == 0) {
#ifdef DEBUG_LITERALS
SayDebug("\n");
#endif
		LIST_cons(lib7_state, stk, LIB7_string0, stk);
		break;
	    }
	    j = BYTES_TO_WORDS(n+1);  /* include space for '\0' */
	  /* the space request includes space for the data-chunk header word and
	   * the sequence header chunk.
	   */
	    spaceReq = WORD_SZB*(j+1+3);
	    GC_CHECK;
	  /* allocate the data chunk */
	    LIB7_AllocWrite(lib7_state, 0, MAKE_DESC(j, DTAG_raw32));
	    LIB7_AllocWrite (lib7_state, j, 0);  /* so word-by-word string equality works */
	    res = LIB7_Alloc (lib7_state, j);
#ifdef DEBUG_LITERALS
SayDebug(" @ %#x (%d words)\n", res, j);
#endif
	    memcpy (PTR_LIB7toC(void, res), &lits[pc], n); pc += n;
	  /* allocate the header chunk */
	    SEQHDR_ALLOC(lib7_state, res, DESC_string, res, n);
	  /* push on stack */
	    LIST_cons(lib7_state, stk, res, stk);
	    break;
	  case I_LIT:
	    n = GET32(lits); pc += 4;
	    for (j = 0, res = stk;  j < n;  j++) {
		res = LIST_tl(res);
	    }
#ifdef DEBUG_LITERALS
SayDebug("[%2d]: LIT(%d) = %#x\n", pc-5, n, LIST_hd(res));
#endif
	    LIST_cons(lib7_state, stk, LIST_hd(res), stk);
	    break;
	  case I_VECTOR:
	    n = GET32(lits); pc += 4;
#ifdef DEBUG_LITERALS
SayDebug("[%2d]: VECTOR(%d) [", pc-5, n);
#endif
	    if (n == 0) {
#ifdef DEBUG_LITERALS
SayDebug("]\n");
#endif
		LIST_cons(lib7_state, stk, LIB7_vector0, stk);
		break;
	    }
	  /* the space request includes space for the data-chunk header word and
	   * the sequence header chunk.
	   */
	    spaceReq = WORD_SZB*(n+1+3);
	    GC_CHECK;
	  /* allocate the data chunk */
	    LIB7_AllocWrite(lib7_state, 0, MAKE_DESC(n, DTAG_ro_vec_data));
	  /* top of stack is last element in vector */
	    for (j = n;  j > 0;  j--) {
		LIB7_AllocWrite(lib7_state, j, LIST_hd(stk));
		stk = LIST_tl(stk);
	    }
	    res = LIB7_Alloc(lib7_state, n);
	  /* allocate the header chunk */
	    SEQHDR_ALLOC(lib7_state, res, DESC_polyvec, res, n);
#ifdef DEBUG_LITERALS
SayDebug("...] @ %#x\n", res);
#endif
	    LIST_cons(lib7_state, stk, res, stk);
	    break;
	  case I_RECORD:
	    n = GET32(lits); pc += 4;
#ifdef DEBUG_LITERALS
SayDebug("[%2d]: RECORD(%d) [", pc-5, n);
#endif
	    if (n == 0) {
#ifdef DEBUG_LITERALS
SayDebug("]\n");
#endif
		LIST_cons(lib7_state, stk, LIB7_void, stk);
		break;
	    }
	    else {
		spaceReq = 4*(n+1);
		GC_CHECK;
		LIB7_AllocWrite(lib7_state, 0, MAKE_DESC(n, DTAG_record));
	    }
	  /* top of stack is last element in record */
	    for (j = n;  j > 0;  j--) {
		LIB7_AllocWrite(lib7_state, j, LIST_hd(stk));
		stk = LIST_tl(stk);
	    }
	    res = LIB7_Alloc(lib7_state, n);
#ifdef DEBUG_LITERALS
SayDebug("...] @ %#x\n", res);
#endif
	    LIST_cons(lib7_state, stk, res, stk);
	    break;
	  case I_RETURN:
	    ASSERT(pc == len);
#ifdef DEBUG_LITERALS
SayDebug("[%2d]: RETURN(%#x)\n", pc-5, LIST_hd(stk));
#endif
	    return (LIST_hd(stk));
	    break;
	  default:
	    Die ("bogus literal opcode #%x @ %d", lits[pc-1], pc-1);
	} /* switch */
    } /* while */

} /* end of BuildLiterals */

