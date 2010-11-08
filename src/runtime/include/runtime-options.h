/* runtime-options.h
 *
 * Command-line argument processing.
 */

#ifndef _LIB7_OPTIONS_
#define _LIB7_OPTIONS_

/* Maximum length of option and argument parts of command-line options:
*/
#define MAX_OPT_LEN	64

extern bool_t  is_runtime_option  (char *commandline_arg, char *option, char **arg);
extern int     get_size_option    (int scale, char *size);

#endif /* !_LIB7_OPTIONS_ */



/* COPYRIGHT (c) 1996 AT&T Research.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

