// win32-signal.c
//
// when "signals" are supported in win32, they'll go here.


#include "../mythryl-config.h"

#include "system-dependent-signal-get-set-etc.h"
#include "runtime-base.h"
#include "runtime-configuration.h"
#include "make-strings-and-vectors-etc.h"
#include "system-dependent-signal-stuff.h"
#include "runtime-globals.h"

#include "win32-sigtable.c"

#define SELF_HOSTTHREAD	(pth__get_hostthread_by_ptid( pth__get_hostthread_ptid() ))

void   pause_until_signal   (Hostthread* hostthread) {
    // ==================
    // Suspend the given Hostthread until a signal is received.
    //
    #ifdef WIN32_DEBUG
	debug_say("win32:pause_until_signal: returning without pause\n");
    #endif
} 


void   set_signal_state   (Hostthread* hostthread,  int signal_number,  int signal_state) {
    // ================
    //
    #ifdef WIN32_DEBUG
	debug_say("win32:set_signal_state: not setting state for signal %d\n", signal_number);
    #endif
}



int   get_signal_state   (Hostthread* hostthread, int signal_number) {
    //================

    #ifdef WIN32_DEBUG
	debug_say("win32:get_signal_state: returning state for signal %d as LIB7_SIG_DEFAULT\n", signal_number);
    #endif

    return LIB7_SIG_DEFAULT;
}  


void   set_signal_mask   (Task* task, Val sigList)   {
    // =============
    //
    // Set the signal mask to the given list of signals.  The sigList has the
    // type: "sysconst list option", with the following semantics -- see
    //
    //     src/lib/std/src/nj/interprocess-signals.pkg
    //
    //	NULL	-- the empty mask
    //	THE[]	-- mask all signals
    //	THE l	-- the signals in l are the mask
    //
#ifdef WIN32_DEBUG
    debug_say("win32:SetSigMask: not setting mask\n");
#endif
}


Val   get_signal_mask__may_heapclean   (Task* task, Val arg, Roots* extra_roots)   {
    //==============================
    //
    // Return the current signal mask (only those signals supported by Lib7); like
    // set_signal_mask, the result has the following semantics:
    //	NULL	-- the empty mask
    //	THE[]	-- mask all signals
    //	THE l	-- the signals in l are the mask
    //
#ifdef WIN32_DEBUG
    debug_say("win32:get_signal_mask__may_heapclean: returning mask as NULL\n");
#endif
    return OPTION_NULL;
}


// COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


