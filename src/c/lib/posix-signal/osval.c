// osval.c


#include "../../mythryl-config.h"

#include "system-dependent-unix-stuff.h"
#include <signal.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"
#include "cfun-proto-list.h"
#include "../posix-error/posix-name-val.h"

// This is (presumably) the list of signals specified by Posix 1003.1.
// NB: We compute an accurate per-system signal list as
// SigTable[]  in src/c/config/posix-signals.c

static name_val_t values [] = {
  {"abrt", SIGABRT},
  {"alrm", SIGALRM},
  {"bus",  SIGBUS},
  {"chld", SIGCHLD},
  {"cont", SIGCONT},
  {"fpe",  SIGFPE},
  {"hup",  SIGHUP},
  {"ill",  SIGILL},
  {"int",  SIGINT},
  {"kill", SIGKILL},
  {"pipe", SIGPIPE},
  {"quit", SIGQUIT},
  {"segv", SIGSEGV},
  {"stop", SIGSTOP},
  {"term", SIGTERM},
  {"tstp", SIGTSTP},
  {"ttin", SIGTTIN},
  {"ttou", SIGTTOU},
  {"usr1", SIGUSR1},
  {"usr2", SIGUSR2},
};

#define NUMELMS ((sizeof values)/(sizeof (name_val_t)))



// One of the library bindings exported via
//     src/c/lib/posix-signal/cfun-list.h
// and thence
//     src/c/lib/posix-signal/libmythryl-posix-signal.c
// to
//     src/lib/std/src/psx/posix-signal.pkg


Val   _lib7_P_Signal_osval   (Task* task,  Val arg)   {
    //====================
    //
    // Mythryl type:   String -> Unt
    //
    // Return the OS-dependent, compile-time constant specified by the string.
    //
    // This fn gets bound as   osval   in
    //
    //     src/lib/std/src/psx/posix-signal.pkg

									    ENTER_MYTHRYL_CALLABLE_C_FN(__func__);

    name_val_t*  resultt = _lib7_posix_nv_lookup (HEAP_STRING_AS_C_STRING(arg), values, NUMELMS);

    if (!resultt)	return RAISE_ERROR__MAY_HEAPCLEAN(task, "system constant not defined", NULL);

    Val result = TAGGED_INT_FROM_C_INT( resultt->val );

									    EXIT_MYTHRYL_CALLABLE_C_FN(__func__);
    return result;
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

