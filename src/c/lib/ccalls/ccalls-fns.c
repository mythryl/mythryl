// ccalls-fns.c

#include "../../config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "task.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "runtime-globals.h"
#include "flush-instruction-cache-system-dependent.h"

#include "ccalls.h"

// Layout of a code_header must match offsets in c-entry.asm:
//
typedef struct {
    Val	the_g;
    char*	argtypes[ N_ARGS ];
    char*	rettype;
    int		nargs;
} Code_Header;

extern Punt  grabPC ();
extern Punt  grabPCend ();

Val_Sized_Unt*	last_entry;		// Points to the beginning of the last c-entry
					// executed set by grabPC in c-entry.asm 

static Code_Header*   last_code_header_used_local   = NULL;  // Last code header used.

static Task*	visible_task_local = NULL;				// Used to expose Task to C code. Used only in this file and in src/c/lib/ccalls/ccalls-fns.c

void   set_visible_task   (Task* visible_task)   {
    // ================
    //
    // Called only from one place   in   src/c/lib/ccalls/ccalls.c
    //
    visible_task_local =  visible_task;
}

#define CODE_HDR_START(p)   ((Code_Header*) ((Unt8*) (p)-sizeof(Code_Header)))


// Code for maintaining Mythryl heap chunks (currently
// only functions) potentially only reachable from C.
//
// Currently, we use a list.
//
// Chunks on this list persist until the program completes;
// this is a known space leak...           XXX BUGGO FIXME
//
// This variable is referenced only in this file and in
//
//     src/c/heapcleaner/call-cleaner.c
//
Val   mythryl_functions_referenced_from_c_code_global =   LIST_NIL;	// See  src/c/heapcleaner/call-cleaner.c
    //===============================================			// and  src/c/heapcleaner/clean-n-agegroups.c


static void   register_fn_as_cleaning_root   (
    //        ================================
    //
    Task*  task,
    Val*   rp
){
    LIST_CONS(task,mythryl_functions_referenced_from_c_code_global,(Val) rp,mythryl_functions_referenced_from_c_code_global);
    //
    #ifdef DEBUG_C_CALLS
	printf("register_fn_as_cleaning_root: added %x\n", rp);
    #endif
}



static Val   save_state   (
    //       ==========
    //
    Task* task,
    Val   fate
){
    // link, closure, arg, fate, and misc regs are in mask ...

    // Compute space for save record:
    //
    int      n = 0;
    {   Val_Sized_Unt mask = task -> lib7_liveRegMask;       // Should also be mask from REQUEST_CALL_CFUN
	for (int i = 0;  mask != 0;  i++, mask >>= 1) {
	    if ((mask & 1))	  ++n;
	}
    }

    // ... but pc, exception_fate, current_thread, and basereg (if defined) are not.
    n += 3;

    #ifdef BASE_INDEX
	n++;
    #endif

    // Also need to save the liveRegMask. 
    // We'll do this first.       others??
    //
    n++;

    #if defined(SOFTWARE_GENERATED_PERIODIC_EVENTS)
        #error
    #endif

    #ifdef DEBUG_C_CALLS
	printf("save_state: size %d\n", n);
    #endif

    int index = 0;		// Initialization is redundant but a good precaution.

    if (!fate) {
	//
	LIB7_AllocWrite (task, 0, MAKE_TAGWORD( n, PAIRS_AND_RECORDS_BTAG ));

	index = 1;

    } else {

	n++;
	LIB7_AllocWrite (task, 0, MAKE_TAGWORD( n, PAIRS_AND_RECORDS_BTAG));
	LIB7_AllocWrite (task, 1, fate);
	index = 2;
    }

    LIB7_AllocWrite( task, index++, TAGGED_INT_FROM_C_INT( task -> lib7_liveRegMask )           );
    LIB7_AllocWrite( task, index++,               task -> program_counter        );
    LIB7_AllocWrite( task, index++,               task -> exception_fate         );
    LIB7_AllocWrite( task, index++,               task -> thread         );
#ifdef BASE_INDEX
    LIB7_AllocWrite( task, index++,               task -> lib7_baseReg                );
#endif
    {   Val_Sized_Unt mask = task -> lib7_liveRegMask;
	int  i;
	for (i = 0;  mask != 0;  i++, mask >>= 1) {
	    if ((mask & 1)) {
		LIB7_AllocWrite( task, index++, task->lib7_roots[ ArgRegMap[ i ]] );
	    }
	}
    }

    ASSERT( index == n+1 );

    return LIB7_Alloc( task, n );
}

static void   restore_state   (
    //        =============
    //
    Task*  task,
    Val    state,
    int    holds_fate
){
    int n = CHUNK_LENGTH( state );

    // link, closure, arg, fate, and misc regs are in mask ...
    // ... but pc, exception_fate, current_thread, and basereg (if defined) are not.
    // and also need the liveRegMask. get this first.
    // others??			XXX BUGGO FIXME

    // Set index to first or second 
    // slot in 'state' record:
    ///
    int index = !!holds_fate;               // Maybe skip function pointer. 

    #ifdef DEBUG_C_CALLS
	printf("restore_state: state size %d\n", n);
    #endif

    task -> lib7_liveRegMask	= GET_TUPLE_SLOT_AS_INT( state, index++ );
    task -> program_counter	= GET_TUPLE_SLOT_AS_VAL(    state, index++ );
    task -> exception_fate	= GET_TUPLE_SLOT_AS_VAL(    state, index++ );
    task -> thread		= GET_TUPLE_SLOT_AS_VAL(    state, index++ );

    #ifdef BASE_INDEX
	task -> lib7_baseReg                = GET_TUPLE_SLOT_AS_VAL(    state, index++ );
    #endif

    Val_Sized_Unt mask =  task->lib7_liveRegMask;

    for (int i = 0;    mask;    i++, mask >>= 1) {
	//
	if ((mask & 1)) {
	    //
	    task -> lib7_roots [ ArgRegMap [i] ]   =   GET_TUPLE_SLOT_AS_VAL( state, index++ );
	}
    }

    ASSERT( index == n );
}



static void   set_up_task  (
    //        ===========
    //
    Task*   task,
    Val     f,
    Val     arg
){
    #if CALLEE_SAVED_REGISTERS_COUNT == 0
	//
	extern Val return_to_c_level_asm[];
    #endif

    // Save necessary state from current task in calleesave register:
    //
    #if (CALLEE_SAVED_REGISTERS_COUNT > 0)
	task->callee_saved_registers(1) = save_state(task,NULL);
	task->fate = PTR_CAST( Val, return_to_c_level_c);
    #else
	task->fate = save_state(task,PTR_CAST( Val, return_to_c_level_asm));
    #endif

    // Inherit exception_fate (?)		XXX BUGGO FIXME
    // leave task->exception_fate as is:
    //
    task->thread   =  HEAP_VOID;
    task->argument =  arg;
    task->closure  =  f;
    //
    task->program_counter =
    task->link_register	  = GET_CODE_ADDRESS_FROM_CLOSURE( f );
}


static void   restore_task   (Task* task)   {
    //        ============
    //
    // Restore previous task:
    //
    #if (CALLEE_SAVED_REGISTERS_COUNT > 0)
	//
	restore_state( visible_task_local, visible_task_local->callee_saved_registers(1), FALSE );
    #else
	restore_state( visible_task_local, visible_task_local->fate,                      TRUE  );
    #endif
}

static   Val_Sized_Unt   convert_result_to_c   (Task* task,  Code_Header* chp,  Val val)   {
    //                   ===================
    //
    Val_Sized_Unt  p;
    Val_Sized_Unt* q = &p;

    char* t = chp->rettype;
    int err;

    // frontend of interface guarantees that ret is a valid 
    // return value for a C function: Val_Sized_Unt or some pointer
    //
    err = convert_mythryl_value_to_c(task,&t,&q,val);							// convert_mythryl_value_to_c		def in    c/lib/ccalls/ccalls.c
    //
    if (err)   die("convert_result_to_c: error converting return value to C");	// Need better error reporting here XXX BUGGO FIXME.

    return p;    // Return C result.
}


int   no_args_entry   (void)   {
    //=============
    //
    // Entry points;  Must be visible to c-entry.asm.

    #ifdef DEBUG_C_CALLS
	printf("no_args_entry: entered\n");
    #endif
	last_code_header_used_local = CODE_HDR_START(last_entry);
    #ifdef DEBUG_C_CALLS
	printf("no_args_entry: nargs in header is %d\n", last_code_header_used_local->nargs);
    #endif

    // Set up task for run_mythryl_task_and_runtime_eventloop evaluation of (f LIST_NIL):
    //
    set_up_task( visible_task_local, last_code_header_used_local->the_g, LIST_NIL );

    // Call Mythryl fn, returns an Val (which is cdata):
    //
    #ifdef DEBUG_C_CALLS
	printf("no_arg_entry: calling Mythryl from C\n");
    #endif
    //
    run_mythryl_task_and_runtime_eventloop( visible_task_local );							// run_mythryl_task_and_runtime_eventloop		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c

    
    #ifdef DEBUG_C_CALLS
	printf("no_args_entry: return value is %d\n", visible_task_local->argument);
    #endif

    Val result = visible_task_local->argument;

    restore_task( visible_task_local );

    return convert_result_to_c( visible_task_local, last_code_header_used_local, result );

}



int   some_args_entry   (Val_Sized_Unt first, ... )   {
    //===============
    //
    va_list  ap;

    Val   lp   = LIST_NIL;
    Val   result;

    Val_Sized_Unt        next;
    int           i;
    Val   args   [N_ARGS];

#ifdef DEBUG_C_CALLS
    printf("some_args_entry: entered\n");
#endif
    last_code_header_used_local = CODE_HDR_START(last_entry);
#ifdef DEBUG_C_CALLS
    printf("some_args_entry: nargs in header is %d\n", last_code_header_used_local->nargs);
    printf("arg 0: %x\n",first);
#endif
    result = convert_c_value_to_mythryl(visible_task_local,last_code_header_used_local->argtypes[0],first,&lp);
    LIST_CONS(visible_task_local,lp,result,lp);
    va_start(ap,first);
    for (i = 1; i < last_code_header_used_local->nargs; i++) {
	next = va_arg(ap,Val_Sized_Unt);
#ifdef DEBUG_C_CALLS
	printf("arg %d: %x\n",i,next);
#endif
	result = convert_c_value_to_mythryl(visible_task_local,last_code_header_used_local->argtypes[i],next,&lp);
	LIST_CONS(visible_task_local,lp,result,lp);
    }
    va_end(ap);

    // lp is backwards:
    //
    lp = revLib7List(lp,LIST_NIL);

    // Set up task for run_mythryl_task_and_runtime_eventloop evaluation of (f lp):
    //
    set_up_task( visible_task_local, last_code_header_used_local->the_g, lp );

    // Call Mythryl fn, returns an Val (which is cdata):
    //
    #ifdef DEBUG_C_CALLS
	printf("some_arg_entry: calling Lib7 from C\n");
    #endif
    //
    run_mythryl_task_and_runtime_eventloop( visible_task_local );								// run_mythryl_task_and_runtime_eventloop		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c

    #ifdef DEBUG_C_CALLS
	printf("some_args_entry: return value is %d\n", visible_task_local->argument);
    #endif

    result = visible_task_local->argument;

    restore_task( visible_task_local );

    return convert_result_to_c( visible_task_local, last_code_header_used_local, result );
}



static void*   build_entry   (									// Called only from make_c_function(), below.
    //         ===========
    //
    Task*   task,
    Code_Header    h
){
    int szb = ((Unt8*) grabPCend) - ((Unt8*) grabPC);						// grabPC		def in    src/c/lib/ccalls/c-entry.asm
    Unt8* p;


    #ifdef DEBUG_C_CALLS
	printf ("grabPC=%lx, grabPCend=%lx, code size is %d\n", grabPC, grabPCend, szb);
	printf ("code_header size is %d\n", sizeof(Code_Header));
    #endif

    ASSERT((sizeof(Code_Header) & 0x3) == 0);

    p = (Unt8*)  memalign( sizeof(Val_Sized_Unt), szb + sizeof(Code_Header) );

    *(Code_Header*) p = h;

    register_fn_as_cleaning_root( task,&(((Code_Header*)p)->the_g) );

    // NB: to free this thing, we'll have to
    // subtract sizeof(Code_Header)
    //
    p += sizeof(Code_Header);
    #ifdef DEBUG_C_CALLS
	printf ("new code starts at %x and ends at %x\n", p, p+szb);
    #endif
    memcpy (p, (void *)grabPC, szb);
    flush_instruction_cache(p,szb);
    return p;   
}



Val_Sized_Unt   make_c_function   (
    //          ===============
    //
    Task*   task,
    Val     f,
    int     nargs,
    char*   argtypes  [],
    char*   rettype
){
    // We get called from only one place, in
    //
    //     src/c/lib/ccalls/ccalls.c

    Code_Header   ch;
    int             i;

    // Create a code header.
    // This will be copied by build entry:
    //
    ch.the_g =  f;
    ch.nargs =  nargs;

    for (i = 0; i < nargs; i++) {
	//
	ch.argtypes[i] = argtypes[i];  // argtypes[i] is a copy we can have.
    }
    ch.rettype = rettype;              // rettype is a copy we can have.

    // Build and return a C entry for f:
    //
    return (Val_Sized_Unt) build_entry( task, ch );
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


