/* c-calls-fns.c
 *
 */

#include "../../config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-state.h"
#include "tags.h"
#include "runtime-heap.h"
#include "runtime-globals.h"
#include "cache-flush.h"

#include "c-calls.h"

/* Layout of a code_header must match offsets in c-entry.asm: */
typedef struct code_header {
    lib7_val_t	the_g;
    char*	argtypes[ N_ARGS ];
    char*	rettype;
    int		nargs;
} code_header_t;

extern Addr_t grabPC();
extern Addr_t grabPCend();

Word_t*		last_entry;   /* Points to the beginning of the last c-entry */
			      /* executed set by grabPC in c-entry.asm */

static code_header_t*   last_code_hdr   = NULL;  /* Last code header used. */

#define CODE_HDR_START(p)   ((code_header_t *)((Byte_t *)(p)-sizeof(code_header_t)))


/* Code for maintaining Lib7 heap chunks (currently
 * only functions) potentially only reachable from C.
 *
 * Currently, we use a list.
 *
 * Chunks on this list persist until the program completes;
 * this is a known space leak...           XXX BUGGO FIXME
 */

lib7_val_t   CInterfaceRootList   = LIST_nil;    /* See gc/call-gc.c and gc/major-gc.c */



static void   register_fn_as_garbage_collection_root   (   lib7_state_t*   lib7_state,
                                                           lib7_val_t*     rp
                                                       )
{
    LIST_cons(lib7_state,CInterfaceRootList,(lib7_val_t) rp,CInterfaceRootList);
#ifdef DEBUG_C_CALLS
    printf("register_fn_as_garbage_collection_root: added %x\n", rp);
#endif
}



static lib7_val_t   saveState   (    lib7_state_t* lib7_state,
                                      lib7_val_t    fate
                                 )
{
    /* link, closure, arg, fate, and misc regs are in mask ... */


    /* Compute space for save record */
    int      n = 0;
    {   Word_t mask = lib7_state -> lib7_liveRegMask;       /* Should also be mask from REQ_CALLC */
        int  i;
	for (i = 0;  mask != 0;  i++, mask >>= 1) {
	    if ((mask & 1))	  ++n;
	}
    }

    /* ... but pc, exception_fate, current_thread, and basereg (if defined) aren't */
    n += 3;

#ifdef BASE_INDEX
    n++;
#endif

    /* Also need to save the liveRegMask. 
     * We'll do this first.       others??
     */
    n++;

#if defined(SOFT_POLL)
#error
#endif

#ifdef DEBUG_C_CALLS
    printf("saveState: size %d\n", n);
#endif

    {   int index = 0;   /* Initialization is redundant but a good precaution. */

	if (!fate) {

	    LIB7_AllocWrite (lib7_state, 0, MAKE_DESC( n, DTAG_record ));
	    index = 1;

	} else {

	    n++;
	    LIB7_AllocWrite (lib7_state, 0, MAKE_DESC( n, DTAG_record));
	    LIB7_AllocWrite (lib7_state, 1, fate);
	    index = 2;
	}

	LIB7_AllocWrite( lib7_state, index++, INT_CtoLib7( lib7_state -> lib7_liveRegMask )           );
	LIB7_AllocWrite( lib7_state, index++,               lib7_state -> lib7_program_counter        );
	LIB7_AllocWrite( lib7_state, index++,               lib7_state -> lib7_exception_fate         );
	LIB7_AllocWrite( lib7_state, index++,               lib7_state -> lib7_current_thread         );
    #ifdef BASE_INDEX
	LIB7_AllocWrite( lib7_state, index++,               lib7_state -> lib7_baseReg                );
    #endif
	{   Word_t mask = lib7_state -> lib7_liveRegMask;
	    int  i;
	    for (i = 0;  mask != 0;  i++, mask >>= 1) {
		if ((mask & 1)) {
		    LIB7_AllocWrite( lib7_state, index++, lib7_state->lib7_roots[ ArgRegMap[ i ]] );
		}
	    }
	}

	ASSERT( index == n+1 );
    }

    return LIB7_Alloc( lib7_state, n );
}

static void   restore_state   (   lib7_state_t*   lib7_state,
                                  lib7_val_t      state,
                                  int              holds_fate
                              )
{
    int n = CHUNK_LEN( state );

    /* link, closure, arg, fate, and misc regs are in mask ... */
    /* ... but pc, exception_fate, current_thread, and basereg (if defined) aren't */
    /* and also need the liveRegMask. get this first */
    /* others?? */

    /* Set index to first or second 
     * slot in 'state' record:
     */
    int index = !!holds_fate;               /* Maybe skip function ptr. */

#ifdef DEBUG_C_CALLS
    printf("restore_state: state size %d\n", n);
#endif

    lib7_state -> lib7_liveRegMask            = REC_SELINT( state, index++ );
    lib7_state -> lib7_program_counter        = REC_SEL(    state, index++ );
    lib7_state -> lib7_exception_fate         = REC_SEL(    state, index++ );
    lib7_state -> lib7_current_thread         = REC_SEL(    state, index++ );

#ifdef BASE_INDEX
    lib7_state -> lib7_baseReg                = REC_SEL(    state, index++ );
#endif

    {   Word_t mask = lib7_state->lib7_liveRegMask;

        int  i;
	for (i = 0;    mask;    i++, mask >>= 1) {

	    if ((mask & 1)) {
		lib7_state -> lib7_roots [ ArgRegMap [i] ]   =   REC_SEL( state, index++ );
	    }
        }
    }

    ASSERT( index == n );
}



static void   setup_lib7_state  (   lib7_state_t*   lib7_state,
                                     lib7_val_t      f,
                                     lib7_val_t      arg
                                 )
{
#if (CALLEESAVE == 0)
    extern lib7_val_t return_a[];
#endif

    /* save necessary state from current lib7_state in calleesave register */
#if (CALLEESAVE > 0)
    lib7_state->lib7_calleeSave(1) = saveState(lib7_state,NULL);
    lib7_state->lib7_fate = PTR_CtoLib7(return_c);
#else
    lib7_state->lib7_fate = saveState(lib7_state,PTR_CtoLib7(return_a));
#endif

    /* inherit exception_fate (?) */
    /* leave lib7_state->lib7_exception_fate as is */
    lib7_state->lib7_current_thread = LIB7_void;
    lib7_state->lib7_argument	= arg;
    lib7_state->lib7_closure	= f;
    lib7_state->lib7_program_counter		=
    lib7_state->lib7_link_register	= GET_CODE_ADDR(f);
}

static void   restore_lib7_state   (lib7_state_t* lib7_state)
{
    /* Restore previous lib7_state: */
#if (CALLEESAVE > 0)
    restore_state( visible_lib7_state, visible_lib7_state->lib7_calleeSave(1), FALSE );
#else
    restore_state( visible_lib7_state, visible_lib7_state->lib7_fate, TRUE   );
#endif
}

static   Word_t convert_result_to_c   (lib7_state_t *lib7_state,code_header_t *chp,lib7_val_t val)
{
    Word_t p, *q = &p;
    char *t = chp->rettype;
    int err;

    /* front-end of interface guarantees that ret is a valid 
     * return value for a C function: Word_t or some pointer
     */
    err = datumLib7toC(lib7_state,&t,&q,val);
    if (err)
	/* need better error reporting here ... */
	Die("convert_result_to_c: error converting return value to C");
    /* return C result*/
    return p;
}


int   no_args_entry   ()
{

    /* Entry points;  Must be visible to c-entry.asm. */


#ifdef DEBUG_C_CALLS
    printf("no_args_entry: entered\n");
#endif
    last_code_hdr = CODE_HDR_START(last_entry);
#ifdef DEBUG_C_CALLS
    printf("no_args_entry: nargs in header is %d\n", last_code_hdr->nargs);
#endif

    /* Set up lib7_state for RunLib7 evaluation of (f LIST_nil) */
    setup_lib7_state(visible_lib7_state, last_code_hdr->the_g, LIST_nil);

    /* Call Lib7 fn, returns an lib7_val_t (which is cdata) */
#ifdef DEBUG_C_CALLS
    printf("no_arg_entry: calling Lib7 from C\n");
#endif
    RunLib7 (visible_lib7_state);

    
#ifdef DEBUG_C_CALLS
    printf("no_args_entry: return value is %d\n", visible_lib7_state->lib7_argument);
#endif

    {   lib7_val_t ret = visible_lib7_state->lib7_argument;

	restore_lib7_state( visible_lib7_state );

	return convert_result_to_c( visible_lib7_state, last_code_hdr, ret );
    }
}



int   some_args_entry   (Word_t first, ... )
{
    va_list       ap;
    lib7_val_t   lp   = LIST_nil;
    lib7_val_t   ret;
    Word_t        next;
    int           i;
    lib7_val_t   args   [N_ARGS];

#ifdef DEBUG_C_CALLS
    printf("some_args_entry: entered\n");
#endif
    last_code_hdr = CODE_HDR_START(last_entry);
#ifdef DEBUG_C_CALLS
    printf("some_args_entry: nargs in header is %d\n", last_code_hdr->nargs);
    printf("arg 0: %x\n",first);
#endif
    ret = datumCtoLib7(visible_lib7_state,last_code_hdr->argtypes[0],first,&lp);
    LIST_cons(visible_lib7_state,lp,ret,lp);
    va_start(ap,first);
    for (i = 1; i < last_code_hdr->nargs; i++) {
	next = va_arg(ap,Word_t);
#ifdef DEBUG_C_CALLS
	printf("arg %d: %x\n",i,next);
#endif
	ret = datumCtoLib7(visible_lib7_state,last_code_hdr->argtypes[i],next,&lp);
	LIST_cons(visible_lib7_state,lp,ret,lp);
    }
    va_end(ap);

    /* lp is backwards */
    lp = revLib7List(lp,LIST_nil);

    /* setup lib7_state for RunLib7 evaluation of (f lp) */
    setup_lib7_state(visible_lib7_state, last_code_hdr->the_g, lp);

    /* call Lib7 fn, returns an lib7_val_t (which is cdata) */
#ifdef DEBUG_C_CALLS
    printf("some_arg_entry: calling Lib7 from C\n");
#endif
    RunLib7 (visible_lib7_state);

    
#ifdef DEBUG_C_CALLS
    printf("some_args_entry: return value is %d\n", visible_lib7_state->lib7_argument);
#endif

    ret = visible_lib7_state->lib7_argument;

    restore_lib7_state(visible_lib7_state);

    return convert_result_to_c( visible_lib7_state, last_code_hdr, ret );
}



static void*   build_entry   (   lib7_state_t*   lib7_state,
                                 code_header_t    h
                             )
{
    int szb = ((Byte_t*) grabPCend) - ((Byte_t*) grabPC);
    Byte_t* p;


#ifdef DEBUG_C_CALLS
    printf ("grabPC=%lx, grabPCend=%lx, code size is %d\n", 
 	    grabPC, grabPCend, szb);
    printf ("code_header size is %d\n", sizeof(code_header_t));
#endif
    ASSERT((sizeof(code_header_t) & 0x3) == 0);

    p = (Byte_t *) memalign(sizeof(Word_t),szb+sizeof(code_header_t));

    *(code_header_t *)p = h;

    register_fn_as_garbage_collection_root( lib7_state,&(((code_header_t*)p)->the_g) );

    /* NB: to free this thing, we'll have to subtract sizeof(code_header_t) */
    p += sizeof(code_header_t);
#ifdef DEBUG_C_CALLS
    printf ("new code starts at %x and ends at %x\n", p, p+szb);
#endif
    memcpy (p, (void *)grabPC, szb);
    FlushICache(p,szb);
    return p;   
}



Word_t   mk_C_function   (   lib7_state_t*   lib7_state,
		             lib7_val_t      f,
                             int              nargs,
                             char*            argtypes  [],
                             char*            rettype
                         )
{   code_header_t   ch;
    int             i;

    /* Create a code header.
     * This will be copied by build entry:
     */
    ch.the_g = f;
    ch.nargs  = nargs;

    for (i = 0; i < nargs; i++) {
	ch.argtypes[i] = argtypes[i];  /* argtypes[i] is a copy we can have */
    }
    ch.rettype = rettype;              /* rettype is a copy we can have */

    /* Build and return a C entry for f: */
    return (Word_t) build_entry( lib7_state, ch );
}



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

