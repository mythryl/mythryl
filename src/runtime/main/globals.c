/* globals.c
 *
 */

#include "../config.h"

#include "runtime-base.h"
#include "machine-id.h"
#include "runtime-values.h"
#include "tags.h"
#include "runtime-globals.h"
#include "runtime-heap.h"
#include "runtime-limits.h"
#include "c-globals-table.h"

#ifdef SIZES_C64_LIB732
void PatchAddresses ();
#endif

#ifndef SIZES_C64_LIB732

typedef struct {
	lib7_val_t	desc;
	char		*s;
	lib7_val_t	len;
} lib7_string_t;

#define LIB7_STRING(id, s)				\
    lib7_string_t id = {					\
	DESC_string,					\
	s,						\
	INT_CtoLib7(sizeof(s)-1)				\
    }

/* Exceptions are identified by (string ref) values */
#define LIB7_EXNID(ex,name)				\
    LIB7_STRING(CONCAT(ex,_s), name);			\
    lib7_val_t CONCAT(ex,_id0) [2] = {			\
	DESC_ref,					\
	PTR_CtoLib7(&(CONCAT(ex,_s).s))			\
    }

#define ASM_CLOSURE(name)				\
    extern lib7_val_t CONCAT(name,_a)[];			\
    lib7_val_t CONCAT(name,_v)[2] = {			\
	MAKE_DESC(1,DTAG_record),			\
	PTR_CtoLib7(CONCAT(name,_a))			\
    }

#else /* SIZES_C64_LIB732 */
/* When the size of Addr_t is bigger than the size of an Word_t, we need
 * to dynamically patch the static Lib7 chunks.
 */

typedef struct {
	lib7_val_t	desc;
	lib7_val_t	s;
	lib7_val_t	len;
} lib7_string_t;

#define LIB7_STRING(id,s)					\
    static char CONCAT(id,_data)[] = s;			\
    lib7_string_t id = {					\
	DESC_string, LIB7_void, INT_CtoLib7(sizeof(s)-1)	\
    }

#define PATCH_LIB7_STRING(id)				\
    id.s = PTR_CtoLib7(CONCAT(id,_data))

/* Exceptions are identified by (string ref) values */
#define LIB7_EXNID(ex,name)				\
    LIB7_STRING(CONCAT(ex,_s),name);			\
    lib7_val_t CONCAT(ex,_id0) [2] = { DESC_ref, }

#define PATCH_LIB7_EXNID(ex)				\
    PATCH_LIB7_STRING(CONCAT(ex,_s));			\
    CONCAT(ex,_id0)[1] = PTR_CtoLib7(&(CONCAT(ex,_s).s))

#define ASM_CLOSURE(name)				\
    extern lib7_val_t CONCAT(name,_a)[];			\
    lib7_val_t CONCAT(name,_v)[2] = {			\
	MAKE_DESC(1, DTAG_record),			\
    }

#define PATCH_ASM_CLOSURE(name)				\
    CONCAT(name,_v)[1] = PTR_CtoLib7(CONCAT(name,_a))

#endif


#if (CALLEESAVE > 0)
#define ASM_CONT(name) 							\
    extern lib7_val_t CONCAT(name,_a)[];					\
    lib7_val_t *CONCAT(name,_c) = (lib7_val_t *)(CONCAT(name,_a))
#else
#define ASM_CONT(name)							\
    ASM_CLOSURE(name);							\
    lib7_val_t *CONCAT(name,_c) = (lib7_val_t *)(CONCAT(name,_v)+1)
#endif

/* machine identification strings */
LIB7_STRING(machine_id, MACHINE_ID);


ASM_CLOSURE(array);
ASM_CLOSURE(bind_cfun);
ASM_CLOSURE(callc);
ASM_CLOSURE(create_b);
ASM_CLOSURE(create_r);
ASM_CLOSURE(create_s);
ASM_CLOSURE(create_v);
ASM_CLOSURE(floor);
ASM_CLOSURE(logb);
ASM_CLOSURE(scalb);
ASM_CLOSURE(try_lock);
ASM_CLOSURE(unlock);
ASM_CLOSURE(handle);

ASM_CONT(return);
ASM_CONT(sigh_return);
ASM_CONT(pollh_return);


/* A ref cell initialized to Void. */
#define REFCELL(z)	lib7_val_t z[2] = {DESC_ref, LIB7_void}

REFCELL(_ProfCurrent);
REFCELL(_PervasiveStruct);
REFCELL(_LIB7SignalHandler);
REFCELL(_LIB7PollHandler);
REFCELL(_PollEvent0);
REFCELL(_PollFreq0);
REFCELL(_ActiveProcs0);

lib7_val_t		runtimeCompileUnit = LIB7_void;
#ifdef ASM_MATH
lib7_val_t		MathVec = LIB7_void;
#endif

/* aggregate packages of length zero */
const char _LIB7_string0_data[1]  = {0};
lib7_val_t _LIB7_string0[3]		= {DESC_string, PTR_CtoLib7(_LIB7_string0_data), INT_CtoLib7(0)};
lib7_val_t _LIB7_vector0[3]		= {DESC_polyvec, LIB7_void, INT_CtoLib7(0)};

LIB7_EXNID(_Div,"DIVIDE_BY_ZERO");
LIB7_EXNID(_Overflow,"OVERFLOW");
LIB7_EXNID(SYSTEM_ERROR, "SYSTEM_ERROR");

extern lib7_val_t externlist0[];

#ifdef ASM_MATH
LIB7_EXNID(_Ln,"Ln");
LIB7_EXNID(_Sqrt,"Sqrt");
#endif


/* A table of pointers to global C variables that are potential roots. */
lib7_val_t	*CRoots[MAX_C_ROOTS] = {
    &runtimeCompileUnit,
    _PervasiveStruct+1,
    _LIB7SignalHandler+1,
    _LIB7PollHandler+1,
#ifdef ASM_MATH
    &MathVec,
#else
    NULL,
#endif
    NULL, NULL
};
#ifdef ASM_MATH
int		NumCRoots = 5;
#else
int		NumCRoots = 4;
#endif



void   allocate_globals   (lib7_state_t* lib7_state)
{
    lib7_val_t	RunVec;
    lib7_val_t    CStruct;

#ifdef SIZES_C64_LIB732
    PatchAddresses ();
#endif

    /* Allocate the RunVec: */
#define RUNVEC_SZ	12
    LIB7_AllocWrite(lib7_state,  0, MAKE_DESC(RUNVEC_SZ, DTAG_record));
    LIB7_AllocWrite(lib7_state,  1, PTR_CtoLib7(array_v+1));
    LIB7_AllocWrite(lib7_state,  2, PTR_CtoLib7(bind_cfun_v+1));
    LIB7_AllocWrite(lib7_state,  3, PTR_CtoLib7(callc_v+1));
    LIB7_AllocWrite(lib7_state,  4, PTR_CtoLib7(create_b_v+1));
    LIB7_AllocWrite(lib7_state,  5, PTR_CtoLib7(create_r_v+1));
    LIB7_AllocWrite(lib7_state,  6, PTR_CtoLib7(create_s_v+1));
    LIB7_AllocWrite(lib7_state,  7, PTR_CtoLib7(create_v_v+1));
    LIB7_AllocWrite(lib7_state,  8, PTR_CtoLib7(floor_v+1));
    LIB7_AllocWrite(lib7_state,  9, PTR_CtoLib7(logb_v+1));
    LIB7_AllocWrite(lib7_state, 10, PTR_CtoLib7(scalb_v+1));
    LIB7_AllocWrite(lib7_state, 11, PTR_CtoLib7(try_lock_v+1));
    LIB7_AllocWrite(lib7_state, 12, PTR_CtoLib7(unlock_v+1));
    RunVec = LIB7_Alloc(lib7_state, RUNVEC_SZ);

    /* Allocate the CStruct: */
#define CSTRUCT_SZ	12
    LIB7_AllocWrite(lib7_state,  0, MAKE_DESC(CSTRUCT_SZ, DTAG_record));
    LIB7_AllocWrite(lib7_state,  1, RunVec);
    LIB7_AllocWrite(lib7_state,  2, DivId);
    LIB7_AllocWrite(lib7_state,  3, OverflowId);
    LIB7_AllocWrite(lib7_state,  4, SysErrId);
    LIB7_AllocWrite(lib7_state,  5, ProfCurrent);		/* prof_current in src/lib/core/init/runtime-system.api		*/
    LIB7_AllocWrite(lib7_state,  6, PollEvent);			/* poll_event	in src/lib/core/init/runtime-system.api		*/
    LIB7_AllocWrite(lib7_state,  7, PollFreq);			/* poll_freq	in src/lib/core/init/runtime-system.api		*/
    LIB7_AllocWrite(lib7_state,  8, Lib7PollHandler);		/* poll_handler	in src/lib/core/init/runtime-system.api		*/
    LIB7_AllocWrite(lib7_state,  9, ActiveProcs);		/* active_procs	in src/lib/core/init/runtime-system.api		*/
    LIB7_AllocWrite(lib7_state, 10, PervasiveStruct);		/* pstruct	in src/lib/core/init/runtime-system.api		*/
    LIB7_AllocWrite(lib7_state, 11, Lib7SignalHandler);		/* sighandler	in src/lib/core/init/runtime-system.api		*/
    LIB7_AllocWrite(lib7_state, 12, LIB7_vector0);		/* vector0	in src/lib/core/init/runtime-system.api		*/
    CStruct = LIB7_Alloc(lib7_state, CSTRUCT_SZ);

    /* Allocate 1-elem SRECORD just containing the CStruct: */
    REC_ALLOC1(lib7_state, runtimeCompileUnit, CStruct);

#ifdef ASM_MATH
#define MATHVEC_SZ	8
    LIB7_AllocWrite(lib7_state,  0, MAKE_DESC(MATHVEC_SZ, DTAG_record));
    LIB7_AllocWrite(lib7_state,  1, LnId);
    LIB7_AllocWrite(lib7_state,  2, SqrtId);
    LIB7_AllocWrite(lib7_state,  3, PTR_CtoLib7(arctan_v+1));
    LIB7_AllocWrite(lib7_state,  4, PTR_CtoLib7(cos_v+1));
    LIB7_AllocWrite(lib7_state,  5, PTR_CtoLib7(exp_v+1));
    LIB7_AllocWrite(lib7_state,  6, PTR_CtoLib7(ln_v+1));
    LIB7_AllocWrite(lib7_state,  7, PTR_CtoLib7(sin_v+1));
    LIB7_AllocWrite(lib7_state,  8, PTR_CtoLib7(sqrt_v+1));
    MathVec = LIB7_Alloc(lib7_state, MATHVEC_SZ);
#endif

}          /* allocate_globals */


/* record_globals:
 *
 * Record all global symbols that may be referenced from the Lib7 heap.
 */
void record_globals ()
{
  /* Misc. */
    RecordCSymbol ("handle",		PTR_CtoLib7(handle_v+1));
    RecordCSymbol ("return",		PTR_CtoLib7(return_c));
#if (CALLEESAVE == 0)
    RecordCSymbol ("return_a",		PTR_CtoLib7(return_a));
#endif

  /* RunVec */
    RecordCSymbol ("RunVec.array",	PTR_CtoLib7(array_v+1));
    RecordCSymbol ("RunVec.bind_cfun",	PTR_CtoLib7(bind_cfun_v+1));
    RecordCSymbol ("RunVec.callc",	PTR_CtoLib7(callc_v+1));
    RecordCSymbol ("RunVec.create_b",	PTR_CtoLib7(create_b_v+1));
    RecordCSymbol ("RunVec.create_r",	PTR_CtoLib7(create_r_v+1));
    RecordCSymbol ("RunVec.create_s",	PTR_CtoLib7(create_s_v+1));
    RecordCSymbol ("RunVec.create_v",	PTR_CtoLib7(create_v_v+1));
    RecordCSymbol ("RunVec.floor",	PTR_CtoLib7(floor_v+1));
    RecordCSymbol ("RunVec.logb",	PTR_CtoLib7(logb_v+1));
    RecordCSymbol ("RunVec.scalb",	PTR_CtoLib7(scalb_v+1));
    RecordCSymbol ("RunVec.try_lock",	PTR_CtoLib7(try_lock_v+1));
    RecordCSymbol ("RunVec.unlock",	PTR_CtoLib7(unlock_v+1));

  /* CStruct */
    RecordCSymbol ("CStruct.DivId",		DivId);
    RecordCSymbol ("CStruct.OverflowId",	OverflowId);
    RecordCSymbol ("CStruct.SysErrId",		SysErrId);
    RecordCSymbol ("CStruct.machine_id",	PTR_CtoLib7(machine_id.s));
    RecordCSymbol ("CStruct.PervasiveStruct",	PervasiveStruct);
    RecordCSymbol ("CStruct.Lib7SignalHandler",Lib7SignalHandler);
    RecordCSymbol ("CStruct.vector0",		LIB7_vector0);
    RecordCSymbol ("CStruct.profCurrent",	ProfCurrent);
    RecordCSymbol ("CStruct.Lib7PollHandler",     Lib7PollHandler);
    RecordCSymbol ("CStruct.pollEvent",		PollEvent);
    RecordCSymbol ("CStruct.pollFreq",		PollFreq);
    RecordCSymbol ("CStruct.activeProcs",	ActiveProcs);

  /* null string */
    RecordCSymbol ("string0",			LIB7_string0);

#if defined(ASM_MATH)
  /* MathVec */
    RecordCSymbol ("MathVec.LnId",	LnId);
    RecordCSymbol ("MathVec.SqrtId",	SqrtId);
    RecordCSymbol ("MathVec.arctan",	PTR_CtoLib7(arctan_v+1));
    RecordCSymbol ("MathVec.cos",	PTR_CtoLib7(cos_v+1));
    RecordCSymbol ("MathVec.exp",	PTR_CtoLib7(exp_v+1));
    RecordCSymbol ("MathVec.ln",	PTR_CtoLib7(ln_v+1));
    RecordCSymbol ("MathVec.sin",	PTR_CtoLib7(sin_v+1));
    RecordCSymbol ("MathVec.sqrt",	PTR_CtoLib7(sqrt_v+1));
#endif

} /* end of record_globals. */

#ifdef SIZES_C64_LIB732

/* PatchAddresses:
 *
 * On machines where the size of Addr_t is bigger than the size of an Word_t,
 * we need to dynamically patch the static Lib7 chunks.
 */
void PatchAddresses ()
{
    PATCH_LIB7_STRING(machine_id);

    PATCH_LIB7_EXNID(_Div);
    PATCH_LIB7_EXNID(_Overflow);
    PATCH_LIB7_EXNID(SYSTEM_ERROR);

    PATCH_ASM_CLOSURE(array);
    PATCH_ASM_CLOSURE(bind_cfun);
    PATCH_ASM_CLOSURE(callc);
    PATCH_ASM_CLOSURE(create_b);
    PATCH_ASM_CLOSURE(create_r);
    PATCH_ASM_CLOSURE(create_s);
    PATCH_ASM_CLOSURE(create_v);
    PATCH_ASM_CLOSURE(floor);
    PATCH_ASM_CLOSURE(logb);
    PATCH_ASM_CLOSURE(scalb);
    PATCH_ASM_CLOSURE(try_lock);
    PATCH_ASM_CLOSURE(unlock);
    PATCH_ASM_CLOSURE(handle);

#if (CALLEESAVE <= 0)
    PATCH_ASM_CLOSURE(return);
    PATCH_ASM_CLOSURE(sigh_return);
#endif

} /* end of PatchAddresses */

#endif /* SIZES_C64_LIB732 */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

