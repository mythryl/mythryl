/* runtime-globals.h
 *
 * These are global reference variables allocated in the run-time system that
 * are visible to the Lib7 tasks.
 */

#ifndef _LIB7_GLOBALS_
#define _LIB7_GLOBALS_

#ifndef _LIB7_VALUES_
#include "runtime-values.h"
#endif

extern lib7_val_t	*CRoots[];
extern int	NumCRoots;

/* "current function" hook for profiling */
extern lib7_val_t	_ProfCurrent[];
#define ProfCurrent PTR_CtoLib7(_ProfCurrent+1)

extern lib7_val_t	_PervasiveStruct[];		/* Pointer to the pervasive package */
#define PervasiveStruct PTR_CtoLib7(_PervasiveStruct+1)

extern lib7_val_t _LIB7SignalHandler[];
#define Lib7SignalHandler PTR_CtoLib7(_LIB7SignalHandler+1)

extern lib7_val_t SYSTEM_ERROR_id0[];
#define SysErrId PTR_CtoLib7(SYSTEM_ERROR_id0+1)

extern lib7_val_t runtimeCompileUnit;
#ifdef ASM_MATH
extern lib7_val_t MathVec;
#endif

extern lib7_val_t _Div_id0[];
#define DivId		PTR_CtoLib7(_Div_id0+1)

extern lib7_val_t _Overflow_id0[];
#define OverflowId	PTR_CtoLib7(_Overflow_id0+1)

#if defined(ASM_MATH)
extern lib7_val_t _Ln_id0[];
#define LnId PTR_CtoLib7(_Ln_id0+1)
extern lib7_val_t _Sqrt_id0[];
#define SqrtId PTR_CtoLib7(_Sqrt_id0+1)
#endif

extern lib7_val_t sigh_resume[];
extern lib7_val_t *sigh_return_c;
extern lib7_val_t pollh_resume[];
extern lib7_val_t *pollh_return_c;
extern lib7_val_t callc_v[];
extern lib7_val_t handle_v[];
extern lib7_val_t *return_c;

extern lib7_val_t _LIB7PollHandler[];
#define Lib7PollHandler PTR_CtoLib7(_LIB7PollHandler+1)

/** polling and MP references **/
extern lib7_val_t _PollFreq0[];
#define PollFreq PTR_CtoLib7(_PollFreq0+1)
extern lib7_val_t _PollEvent0[];
#define PollEvent PTR_CtoLib7(_PollEvent0+1)
extern lib7_val_t _ActiveProcs0[];
#define ActiveProcs PTR_CtoLib7(_ActiveProcs0+1)

/* Initialize the C function list */
extern void InitCFunList ();

/* Record the C symbols that are visible to Lib7 */
extern void record_globals ();

/* Initialize the Lib7 globals that are supported by the runtime system */
extern void allocate_globals (lib7_state_t *lib7_state);

/* Bind a C function */
extern lib7_val_t BindCFun (char *moduleName, char *funName);

#ifdef SIZES_C64_LIB732
/* patch the 32-bit addresses */
extern void PatchAddresses ();
#endif

#endif /* !_LIB7_GLOBALS_ */


/* COPYRIGHT (c) 1992 AT&T Bell Laboratories
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

