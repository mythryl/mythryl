// system-dependent-signal-stuff.h


#ifndef SYSTEM_DEPENDENT_SIGNAL_STUFF_H
#define SYSTEM_DEPENDENT_SIGNAL_STUFF_H


// Signals_Seen_And_Done_Counts
//
// Counters for pending signals.
// To avoid race conditions
// we keep two counters.
//
// This type appears only in  struct hostthread  in
//
//     src/c/h/runtime-base.h
//
typedef struct {
    //
    unsigned int	seen_count;	// The count of how many signals of this variety
					// have been received. This counter is incremented
					// by the signal handler.

    unsigned int	done_count;	// The count of how many of this kind of
					// signal have been handled.  This counter
					// is incremented by the main thread.
} Signals_Seen_And_Done_Counts;



// The states of Mythryl signal handlers.
//
// These definitions must agree with
// the values used in
//
//     src/lib/std/src/nj/interprocess-signals.pkg
//
#define LIB7_SIG_IGNORE		0
#define LIB7_SIG_DEFAULT	1
#define LIB7_SIG_ENABLED	2

// Utility functions:
//
extern void   choose_signal				  (Hostthread* hostthread);						// choose_signal				is from   src/c/machine-dependent/interprocess-signals.c
extern void   clear_signal_counts			  (Hostthread* hostthread);						// clear_signal_counts				is from   src/c/machine-dependent/interprocess-signals.c
//
extern Val    make_mythryl_signal_handler_arg		  (Task* task,  Val resume[]);						// make_mythryl_signal_handler_arg		is from   src/c/machine-dependent/signal-stuff.c
extern Val    make_posthandler_resumption_fate_from_task  (Task* task,  Val resume[]);						// make_posthandler_resumption_fate_from_task	is from   src/c/machine-dependent/signal-stuff.c
extern void   load_task_from_posthandler_resumption_fate  (Task* task);								// load_task_from_posthandler_resumption_fate	is from   src/c/machine-dependent/signal-stuff.c

// Core signal operations.  Depending on the platform,
// these are implemented in one of:
//
//     src/c/machine-dependent/interprocess-signals.c
//     src/c/machine-dependent/win32-signal.c
//
extern void	pause_until_signal		(Hostthread* hostthread);						//  pause_until_signal	def in   src/c/machine-dependent/interprocess-signals.c
extern void	set_signal_state		(Hostthread* hostthread, int signal_number, int signal_state);
extern int	get_signal_state		(Hostthread* hostthread, int signal_number);
//
extern void	set_signal_mask			(Task* task, Val signal_list);
extern Val	get_signal_mask__may_heapclean	(Task* task, Val arg, Roots*);

#endif // SYSTEM_DEPENDENT_SIGNAL_STUFF_H


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

