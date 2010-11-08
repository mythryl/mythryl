/* error.c
 *
 * Run-time system error messages.
 */

#include "../config.h"

#include <stdio.h>
#include <stdarg.h>
#include "runtime-base.h"

extern FILE	*DebugF;

#ifdef TARGET_BYTECODE
extern FILE	*BC_stdout;
#endif


/* say:
 * Print a message to the standard output.
 */
void say (char *fmt, ...)
{
    va_list	ap;

    va_start (ap, fmt);
    vfprintf (stdout, fmt, ap);
    va_end(ap);
    fflush (stdout);

} /* end of say */

/* SayDebug:
 * Print a message to the debug output stream.
 */
void SayDebug (char *format, ...)
{
    va_list	ap;

    va_start (ap, format);
    vfprintf (DebugF, format, ap);
    va_end(ap);
    fflush (DebugF);

} /* end of SayDebug */

/* Error:
 * Print an error message.
 */
void Error (char *fmt, ...)
{
    va_list	ap;

    va_start (ap, fmt);
    fprintf (stderr, "%s: Error -- ", Lib7CommandName);
    vfprintf (stderr, fmt, ap);
    va_end(ap);

} /* end of Error */


/* Die:
 * Print an error message and then exit.
 */
void Die (char *fmt, ...)
{
    va_list	ap;

    va_start (ap, fmt);
    fprintf (stderr, "%s: Fatal error -- ", Lib7CommandName);
    vfprintf (stderr, fmt, ap);
    fprintf (stderr, "\n");
    va_end(ap);

#if (defined(TARGET_BYTECODE) && defined(INSTR_HISTORY))
    {
	extern void PrintRegs (FILE *);
	extern void PrintInstrHistory (FILE *);

	PrintRegs (BC_stdout);
	PrintInstrHistory (BC_stdout);
    }
#endif

#ifdef MP_SUPPORT
    MP_Shutdown ();
#endif

    Exit (1);

} /* end of Die */


#ifdef ASSERT_ON
/* AssertFail:
 *
 * Print an assertion failure message.
 */
void AssertFail (const char *a, const char *file, int line)
{
    fprintf (stderr, "%s: Assertion failure (%s) at \"%s:%d\"\n",
	Lib7CommandName, a, file, line);

#ifdef MP_SUPPORT
    MP_Shutdown ();
#endif

    Exit (2);

} /* end of AssertFail */
#endif


/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

