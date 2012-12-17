// signal-stuff.c
//
// System independent support fns for
// signals and software polling.

#include "../mythryl-config.h"

#include <stdio.h>
#include "runtime-base.h"
#include "runtime-configuration.h"
#include "make-strings-and-vectors-etc.h"
#include "system-dependent-signal-stuff.h"


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
    set_slot_in_nascent_heapchunk(task,  0, MAKE_TAGWORD(10, PAIRS_AND_RECORDS_BTAG));
    set_slot_in_nascent_heapchunk(task,  1, PTR_CAST( Val, resume));
    set_slot_in_nascent_heapchunk(task,  2, task->argument);
    set_slot_in_nascent_heapchunk(task,  3, task->fate);
    set_slot_in_nascent_heapchunk(task,  4, task->current_closure);
    set_slot_in_nascent_heapchunk(task,  5, task->link_register);
    set_slot_in_nascent_heapchunk(task,  6, task->program_counter);
    set_slot_in_nascent_heapchunk(task,  7, task->exception_fate);
    set_slot_in_nascent_heapchunk(task,  8, task->callee_saved_registers[0]);				// John Reppy says not to do: set_slot_in_nascent_heapchunk(task,  8, task->current_thread);
    set_slot_in_nascent_heapchunk(task,  9, task->callee_saved_registers[1]);
    set_slot_in_nascent_heapchunk(task, 10, task->callee_saved_registers[2]);
    //
    return commit_nascent_heapchunk(task, 10);
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


    Hostthread* hostthread = task->hostthread;

    Val run_fate =  make_resumption_fate( task,  resume_after_handling_signal );

    // Allocate the Mythryl signal handler's argument record:
    //
    Val arg = make_three_slot_record( task, 
	//
	TAGGED_INT_FROM_C_INT( hostthread->next_posix_signal_id	),
        TAGGED_INT_FROM_C_INT( hostthread->next_posix_signal_count	),
	run_fate
    );

    #ifdef SIGNAL_DEBUG
	debug_say( "make_mythryl_signal_handler_arg: resumeC = %#x, arg = %#x\n", run_fate, arg );
    #endif

    return arg;
}


void   load_resume_state   (Task* task) {						// Called exactly once, from   src/c/main/run-mythryl-code-and-runtime-eventloop.c
    // =================
    // 
    // Load the Mythryl state with the state preserved
    // in resumption fate made by make_resumption_fate.
    //
    Val* current_closure;

    #ifdef SIGNAL_DEBUG
        debug_say ("load_resume_state:\n");
    #endif

    current_closure = PTR_CAST(Val*, task->current_closure);

    task->argument		= current_closure[1];
    task->fate			= current_closure[2];
    task->current_closure	= current_closure[3];
    task->link_register		= current_closure[4];
    task->program_counter	= current_closure[5];
    task->exception_fate	= current_closure[6];

    // John (Reppy) says current_thread
    // should not be included here...
    //    task->current_thread	= current_closure[7];

    task->callee_saved_registers[0]	= current_closure[7];
    task->callee_saved_registers[1]	= current_closure[8];
    task->callee_saved_registers[2]	= current_closure[9];
}



// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


