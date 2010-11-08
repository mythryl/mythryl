/* unix-sigtable.c
 *
 * NOTE: this file is generated --- do not edit!!!
 */


#include "../config.h"

static sys_const_t SigInfo[NUM_SIGS] = {
    { SIGHUP, "HUP" },
    { SIGINT, "INTERRUPT" },
    { SIGQUIT, "QUIT" },
    { SIGPIPE, "PIPE" },
    { SIGALRM, "ALARM" },
    { SIGTERM, "TERMINAL" },
    { SIGUSR1, "USR1" },
    { SIGUSR2, "USR2" },
    { SIGCHLD, "CHLD" },
    { SIGWINCH, "WINCH" },
    { SIGURG, "URG" },
    { SIGIO, "IO" },
    { SIGTSTP, "TSTP" },
    { SIGCONT, "CONT" },
    { SIGTTIN, "TTIN" },
    { SIGTTOU, "TTOU" },
    { SIGVTALRM, "VTALRM" },
  /* Run-time signals */
    { RUNSIG_GC, "GARBAGE_COLLECTION" },
};
static sysconst_table_t SigTable = {
    /* numConsts */ NUM_SYSTEM_SIGS,
    /* consts */    SigInfo
};


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 */
