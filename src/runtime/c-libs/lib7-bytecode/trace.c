/* trace.c
 *
 * Trace facility for the bytecode interpreter.
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"

#ifdef TARGET_BYTECODE
#  include "memory-trace.h"
#endif

#ifdef INSTR_TRACE
extern bool_t		traceOn;
#endif

#ifdef FULL_HIST
extern bool_t       fullHistOn;
#endif

/* lib7_start_trace:
 */
lib7_val_t lib7_start_trace (lib7_state_t *lib7_state, lib7_val_t *arg)
{
#if defined(TARGET_BYTECODE)
#ifdef FULL_HIST
if (fullHistOn == FALSE) printf("*** TURNING FULL_HIST ON ***\n");
else printf("*** START\n");
    fullHistOn = TRUE;
#endif
#ifdef INSTR_TRACE
    traceOn = TRUE;
#endif
#ifdef DO_MEMORY_TRACE
    MemOp_Start (lib7_state);
#endif
#endif

} /* end of lib7_start_trace */

/* lib7_stop_trace:
 */
lib7_val_t lib7_stop_trace (lib7_state_t *lib7_state, lib7_val_t *arg)
{
#if defined(TARGET_BYTECODE)
#ifdef INSTR_TRACE
    traceOn = FALSE;
#endif
#ifdef DO_MEMORY_TRACE
    MemOp_Stop (lib7_state);
#endif
#endif

} /* end of lib7_stop_trace */



/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

