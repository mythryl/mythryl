/* runtime-base.h
 *
 */

#ifndef _LIB7_BASE_
#define _LIB7_BASE_

/* macro concatenation (ANSI CPP) */
#ifndef CONCAT /* assyntax.h also defines CONCAT */
#  define CONCAT(a,b)	a ## b
#endif
#define CONCAT3(a,b,c)	a ## b ## c

#define ONE_K		1024
#define ONE_MEG 	(ONE_K*ONE_K)

/* The generated file runtime-sizes.h defines various size macros, and
 * the following types:
 *
 * Int16_t	-- 16-bit signed integer
 * Int32_t	-- 32-bit signed integer
 * Int64_t	-- 64-bit signed integer (64-bit machines only)
 * Unsigned16_t	-- 16-bit unsigned integer
 * Unsigned32_t	-- 32-bit unsigned integer
 * Unsigned64_t	-- 64-bit unsigned integer (64-bit machines only)
 * Byte_t	-- unsigned 8-bit integer.
 * Word_t	-- unsigned integer that is large enough to hold an Lib7 value.
 * Int_t	-- signed integer that is large enough to hold an Lib7 value.
 * Addr_t	-- an unsigned integer that is large enough to hold an address.
 */
#ifndef _LIB7_SIZES_
#include "runtime-sizes.h"
#endif

/* the size of a pair */
#define PAIR_SZB	(2*WORD_SZB)
/* the number of Word_t's per double */
#define REALD_SZW	(REALD_SZB / WORD_SZB)
/* the number of Word_t's per pair chunk */
#define PAIR_SZW	2
/* the number of Word_t's per special chunk */
#define SPECIAL_SZW	2

/* convert a number of bytes to an even number of words */
#define BYTES_TO_WORDS(N)	(((N)+(WORD_SZB-1)) >> LOG_BYTES_PER_WORD)

/* convert a number of doubles to an even number of words */
#define DOUBLES_TO_WORDS(N)	((N) * REALD_SZW)

/* on 32-bit machines it is useful to align doubles on 8-byte boundries */
#ifndef SIZES_C64_LIB764
#  define ALIGN_REALDS
#endif


#ifndef _ASM_

#include <stdlib.h>

typedef Int32_t bool_t;
#ifndef TRUE		/* Some systems already define TRUE and FALSE */
#  define TRUE 1
#  define FALSE 0
#endif

typedef Int32_t status_t;
#define SUCCESS 1
#define FAILURE 0

/* Assertions for debugging: */
#ifdef ASSERT_ON
extern void AssertFail (const char *a, const char *file, int line);
/* #define ASSERT(A)	((A) ? ((void)0) : AssertFail(#A, __FILE__, __LINE__)) */
#define ASSERT(A)	{ if (!(A)) AssertFail(#A, __FILE__, __LINE__); }
#else
#define ASSERT(A)	{ }
#endif

/* Convert a bigendian 32-bit quantity into the host machine's representation. */
#if defined(BYTE_ORDER_BIG)
#  define BIGENDIAN_TO_HOST(x)	(x)
#elif defined(BYTE_ORDER_LITTLE)
   extern Unsigned32_t SwapBytes (Unsigned32_t x);
#  define BIGENDIAN_TO_HOST(x)	SwapBytes(x)
#else
#  error must define endianess
#endif

/* round i up to the nearest multiple of n, where n is a power of 2 */
#define ROUNDUP(i, n)		(((i)+((n)-1)) & ~((n)-1))


/* extract the bitfield of width WIDTH starting at position POS from I */
#define XBITFIELD(I,POS,WIDTH)		(((I) >> (POS)) & ((1<<(WIDTH))-1))

/* aliases for malloc/free, so that we can easily replace them */
#define MALLOC(size)	malloc(size)
#define _FREE		free
#define FREE(p)		_FREE(p)

/* Allocate a new C chunk of type t. */
#define NEW_CHUNK(t)	((t *)MALLOC(sizeof(t)))
/* Allocate a new C array of type t chunks. */
#define NEW_VEC(t,n)	((t *)MALLOC((n)*sizeof(t)))

/* clear memory */
#define CLEAR_MEM(m, size)	(memset((m), 0, (size)))

/* The size of a page in the BIBOP memory map (in bytes) */
#define BIBOP_PAGE_SZB		((Addr_t)(64*ONE_K))
#define RND_HEAP_CHUNK_SZB(SZ)	ROUNDUP(SZ,BIBOP_PAGE_SZB)

/** C types used in the run-time system **/
#ifdef SIZES_C64_LIB732
typedef Unsigned32_t lib7_val_t;
#else
typedef struct { Word_t v[1]; } lib7_chunk_t; /* something for an lib7_val_t to point to */
typedef lib7_chunk_t *lib7_val_t;
#endif
typedef struct vproc_state vproc_state_t;
typedef struct lib7_state lib7_state_t;
typedef struct heap heap_t;


/* In C, system constants are usually integers.  We represent these in the Lib7
 * system as (Int, String) pairs, where the integer is the C constant, and the
 * string is a short version of the symbolic name used in C (e.g., the constant
 * EINTR might be represented as (4, "INTR")).
 */
typedef struct {	/* The representation of system constants */
    int		id;
    char	*name;
} sys_const_t;

typedef struct {	/* a table of system constants. */
    int		numConsts;
    sys_const_t	*consts;
} sysconst_table_t;


/* run-time system messages */
extern void say (char *fmt, ...);
extern void SayDebug (char *fmt, ...);
extern void Error (char *, ...);
extern void Exit (int code);
extern void Die (char *, ...);

/* heap_params is an abstract type, whose representation depends on the
 * particular GC being used.
 */
typedef struct heap_params heap_params_t;

extern heap_params_t* ParseHeapParams (char** argv);
extern lib7_state_t* AllocLib7state (bool_t is_boot, heap_params_t* params);

extern void load_oh7_files        (const char* o7files_to_load_filename, const char* heap_file_to_write, heap_params_t* params);
extern void load_and_run_heap_image (const char* heap_image_to_run_filename,    heap_params_t* params);

extern void InitLib7state (lib7_state_t *lib7_state);
extern void SaveCState (lib7_state_t *lib7_state, ...);
extern void RestoreCState (lib7_state_t *lib7_state, ...);
extern void set_up_timers ();
extern void reset_timers (vproc_state_t *vsp);
extern lib7_val_t ApplyLib7Fn (lib7_state_t *lib7_state, lib7_val_t f, lib7_val_t arg, bool_t useCont);
extern void RunLib7 (lib7_state_t *lib7_state);
extern void RaiseLib7Exception (lib7_state_t *lib7_state, lib7_val_t exn);
extern void set_up_fault_handlers ();

#ifdef SOFT_POLL
extern void ResetPollLimit (lib7_state_t *lib7_state);
#endif


/* These are two views of the command line arguments; raw_args is essentially
 * argv[].  commandline_arguments is argv[] with runtime system arguments stripped
 * out (e.g., those of the form --runtime-xxx[=yyy]).
 */
extern char	**raw_args;
extern char	**commandline_arguments;	/* does not include the command name (argv[0]) */
extern char	*Lib7CommandName;	/* the command name used to invoke the runtime */
extern int	verbosity;
extern bool_t   show_code_chunk_comments;
extern bool_t	GCMessages;
extern bool_t	UnlimitedHeap;

/* The table of virtual processor Lib7 states */
extern vproc_state_t	*VProc[];
extern int		NumVProcs;

#endif /* !_ASM_ */

#ifndef HEAP_IMAGE_SYMBOL
#define HEAP_IMAGE_SYMBOL       "lib7_heap_image"
#define HEAP_IMAGE_LEN_SYMBOL   "lib7_heap_image_len"
#endif

#endif /* !_LIB7_BASE_ */


/* COPYRIGHT (c) 1992 AT&T Bell Laboratories
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

