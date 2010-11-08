/* win32-sigtab.h
 *
 * fake "signals" to make win32 go through.
 * unlike the unix counterpart, this file is not generated -- do not delete!
 */

#ifndef _WIN32_SIGTAB_
#define _WIN32_SIGTAB_

struct {
  int n;
  char *sname,*lname;
} win32SigTab[] = {
  {0, "INTERRUPT", "SIGINT"},
  {1, "ALRAM", "SIGALRM"},
  {2, "TERMINAL", "SIGTERM"},
  {3, "GARBAGE_COLLECTION", "RUNSIG_GC"}
};

#define NUM_SIGS 4


#endif

/* end of win32-sigtab.h */


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

