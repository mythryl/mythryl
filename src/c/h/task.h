// task.h
//
// This is the C view of the state of a Mythryl computation.


#ifndef TASK_H
#define TASK_H

#include "runtime-base.h"
#include "system-dependent-root-register-indices.h"

#if (!defined(BIGCOUNTER_H) && defined(ICOUNT))
    #include "bigcounter.h"
#endif

#define CALLEE_SAVED_REGISTERS_COUNT	3


// The core Mythryl state vector
//

/* typedef  struct task  Task; */					// Defined in runtime-base.h

struct task {
    //
    Heap*	heap;							// The heap for this Mythryl task.
    Pthread* 	pthread;						// The Pthread on which it is running. If you change the offset of this field you'll probably need to change:
									//     pthread_offtask   in   src/lib/compiler/back/low/main/intel32/machine-properties-intel32.pkg
    // Mythryl registers:
    //
    Val*	heap_allocation_pointer;				// We allocate heap memory just by advancing this pointer.
    Val*	heap_allocation_limit;					// When heap_allocation_pointer reaches this point it is time to call the heapcleaner.
    //
    Val		argument;						// Argument to current function/closure. Since we treat calling and returning as the same thing, this will also hold the result of the call.
    Val		fate;							// "Return address".
    Val		closure;						// Currently executing closure ("function").
    //
    Val		link_register;						// A valid program counter value -- initially at least entrypoint in 'closure'.
    Val		program_counter;					// Address of Mythryl code to execute; when
									// calling a Mythryl function from C, this
									// holds the same value as the link_register.

    Val		exception_fate;						// Exception handler (?)
    Val		current_thread;
    Val		callee_saved_registers[ CALLEE_SAVED_REGISTERS_COUNT ];

    Val		heap_changelog;						// The cons-list of updates to the heap. These are allocated on the heap at each update, used by heapcleaner to detect (new) intergenerational pointers.

    Val		fault_exception;					// The exception packet for a hardware fault.
    Val_Sized_Unt  faulting_program_counter;				// The program counter of the faulting instruction.

    #if NEED_SOFTWARE_GENERATED_PERIODIC_EVENTS
	//
	Val*	real_heap_allocation_limit;				// We ab/use heapchecks to generate events by setting an artificially small heap_allocation_limit value; in such cases this holds the real value.
	Bool	software_generated_periodic_event_is_pending;
	Bool	in_software_generated_periodic_event_handler;
    #endif
};


#endif // TASK_H



// COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


