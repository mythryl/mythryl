/* osval.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include <signal.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "tags.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "../posix-error/posix-name-val.h"

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

/* _lib7_P_Signal_osval : String -> word
 *
 * Return the OS-dependent, compile-time constant specified by the string.
 */
lib7_val_t _lib7_P_Signal_osval (lib7_state_t *lib7_state, lib7_val_t arg)
{
    name_val_t      *res;
    
    res = _lib7_posix_nv_lookup (STR_LIB7toC(arg), values, NUMELMS);
    if (res)
	return INT_CtoLib7(res->val);
    else {
        return RAISE_ERROR(lib7_state, "system constant not defined");
    }

} /* end of _lib7_P_Signal_osval */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
