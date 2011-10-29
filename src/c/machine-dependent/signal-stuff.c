// signal-stuff.c
//
// System independent support fns for
// signals and software polling.

#include "../mythryl-config.h"

#include <stdio.h>
#include "runtime-base.h"
#include "runtime-configuration.h"
#include "task.h"
#include "pthread-state.h"
#include "make-strings-and-vectors-etc.h"
#include "system-dependent-signal-stuff.h"
#include "system-signals.h"


void   choose_signal   (Pthread* pthread)   {
    // =============
    // 
    // Caller guarantees that at least one Unix signal has been
    // seen at the C level but not yet handled at the Mythryl
    // level.  Our job is to find and return the number of
    // that signal plus the number of times it has fired at
    // the C level since last being handled at the Mythry level.
    //
    // Choose which signal to pass to the Mythryl-level handler
    // and set up the Mythryl state vector accordingly.
    //
    // This function gets called (only) from
    //
    //     src/c/main/run-mythryl-code-and-runtime-eventloop.c
    //
    // WARNING: This should be called with signals masked
    // to avoid race conditions.

    int i, j, delta;

    // Scan the signal counts looking for
    // a signal that needs to be handled.
    //
    // The 'seen_count' field for a signal gets
    // incremented once for each incoming signal
    // in   c_signal_handler()   in
    //
    //     src/c/machine-dependent/posix-signal.c
    //
    // Here we increment the matching 'done_count' field
    // each time we invoke appropriate handling for that
    // signal;  thus, the difference between the two
    // gives the number of pending instances of that signal
    // currently needing to be handled.
    //
    // For fairness we scan for signals round-robin style, using
    //
    //     pthread->posix_signal_rotor
    //
    // to remember where we left off scanning, so we can pick
    // up from there next time:	

    i = pthread->posix_signal_rotor;
    j = 0;
    do {
	ASSERT (j++ < NUM_SIGS);

	i++;

	// Wrap circularly around the signal vector:
	//
	if (i == MAX_POSIX_SIGNALS)
            i = MIN_SYSTEM_SIG;

	// Does this signal have pending work? (Nonzero == "yes"):
	//
	delta = pthread->posix_signal_counts[i].seen_count - pthread->posix_signal_counts[i].done_count;

    } while (delta == 0);

    pthread->posix_signal_rotor = i;		// Next signal to scan on next call to this fn.

    // Record the signal to process
    // and how many times it has fired
    // since last being handled at the
    // Mythryl level:
    //
    pthread->next_posix_signal_id  = i;
    pthread->next_posix_signal_count = delta;

    // Mark this signal as 'done':
    //
    pthread->posix_signal_counts[i].done_count  += delta;
    pthread->all_posix_signals.done_count += delta;

    #ifdef SIGNAL_DEBUG
        debug_say ("choose_signal: sig = %d, count = %d\n", pthread->next_posix_signal_id, pthread->next_posix_signal_count);
    #endif
}


Val   make_resumption_fate   (				// Called once from this file, once from   src/c/main/run-mythryl-code-and-runtime-eventloop.c
    //==================== 
    //
    Task* task,
    Val*  resume					// Either   resume_after_handling_signal   or   resume_after_handling_software_generated_periodic_event
){							// from a platform-dependent assembly file like    src/c/machine-dependent/prim.intel32.asm  
    // 
    // Build the resume fate for a signal or poll event handler.
    // This closure contains the address of the resume entry-point and
    // the registers from the Mythryl state.
    //
    // Caller guarantees us roughly 4KB available space.
    //
    // This gets called from make_mythryl_signal_handler_arg() below,
    // and also from  src/c/main/run-mythryl-code-and-runtime-eventloop.c

    // Allocate the resumption closure:
    //
    LIB7_AllocWrite(task,  0, MAKE_TAGWORD(10, PAIRS_AND_RECORDS_BTAG));
    LIB7_AllocWrite(task,  1, PTR_CAST( Val, resume));
    LIB7_AllocWrite(task,  2, task->argument);
    LIB7_AllocWrite(task,  3, task->fate);
    LIB7_AllocWrite(task,  4, task->closure);
    LIB7_AllocWrite(task,  5, task->link_register);
    LIB7_AllocWrite(task,  6, task->program_counter);
    LIB7_AllocWrite(task,  7, task->exception_fate);
    LIB7_AllocWrite(task,  8, task->callee_saved_registers[0]);				// John Reppy says not to do: LIB7_AllocWrite(task,  8, task->thread);
    LIB7_AllocWrite(task,  9, task->callee_saved_registers[1]);
    LIB7_AllocWrite(task, 10, task->callee_saved_registers[2]);
    //
    return LIB7_Alloc(task, 10);
}


Val   make_mythryl_signal_handler_arg   (
    //=============================== 
    //
    Task* task,
    Val*  resume_after_handling_signal
){
    // We're handling a POSIX inteprocess signal for
    //
    //     src/c/main/run-mythryl-code-and-runtime-eventloop.c
    //
    // Depending on platform,    resume_after_handling_signal
    // is from one of
    //     src/c/machine-dependent/prim.intel32.asm
    //     src/c/machine-dependent/prim.intel32.masm
    //     src/c/machine-dependent/prim.sun.asm
    //     src/c/machine-dependent/prim.pwrpc32.asm
    //
    // Our job is to build the Mythryl argument record for
    // the Mythryl signal handler.  The handler has type
    //
    //   posix_interprocess_signal_handler : (Int, Int, Fate(Void)) -> X
    //
    // where
    //     The first  argument is  the signal id 		// For example SIGALRM,
    //     the second argument is  the signal count		// I.e., number of times signal has been recieved since last handled.
    //     the third  argument is  the resumption fate.
    //
    // The return type is X because the Mythryl
    // signal handler should never return.
    //
    // NOTE: Maybe this should be combined with choose_signal???	XXX BUGGO FIXME


    Pthread* pthread = task->pthread;

    Val resume_fate =  make_resumption_fate( task,  resume_after_handling_signal );

    // Allocate the Mythryl signal handler's argument record:
    //
    Val	              arg;
    REC_ALLOC3( task, arg,
	//
	TAGGED_INT_FROM_C_INT( pthread->next_posix_signal_id		),
        TAGGED_INT_FROM_C_INT( pthread->next_posix_signal_count	),
	resume_fate
    );

    #ifdef SIGNAL_DEBUG
	debug_say( "make_mythryl_signal_handler_arg: resumeC = %#x, arg = %#x\n", resume_fate, arg );
    #endif

    return arg;
}


void   load_resume_state   (Task* task) {						// Called exactly once, from   src/c/main/run-mythryl-code-and-runtime-eventloop.c
    // =================
    // 
    // Load the Mythryl state with the state preserved
    // in resumption fate made by make_resumption_fate.
    //
    Val* contClosure;

    #ifdef SIGNAL_DEBUG
        debug_say ("load_resume_state:\n");
    #endif

    contClosure = PTR_CAST(Val*, task->closure);

    task->argument		= contClosure[1];
    task->fate			= contClosure[2];
    task->closure		= contClosure[3];
    task->link_register		= contClosure[4];
    task->program_counter	= contClosure[5];
    task->exception_fate	= contClosure[6];

    // John (Reppy) says current_thread
    // should not be included here...
    //    task->thread	= contClosure[7];

    task->callee_saved_registers[0]	= contClosure[7];
    task->callee_saved_registers[1]	= contClosure[8];
    task->callee_saved_registers[2]	= contClosure[9];
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


