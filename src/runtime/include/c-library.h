/* c-library.h
 *
 */

#ifndef _C_LIBRARY_
#define _C_LIBRARY_

/* a pointer to a library initialization function; it is passed the
 * list of command-line arguments.
 */
typedef void (*clib_init_fn_t) (int, char **);

/* a pointer to an Lib7 callable C function */
typedef lib7_val_t (*cfunc_t) (lib7_state_t *, lib7_val_t);

/* an element in the table of name/function pairs. */
typedef struct {
    const char	    *name;
    cfunc_t	    cfunc;
} cfunc_naming_t;

/* The representation of a library of Lib7 callable C functions */
typedef struct {
    const char	    *libName;	/* the library name */
    const char	    *version;
    const char	    *date;
    clib_init_fn_t  initFn;	/* an optional initialization function */
    cfunc_naming_t *cfuns;	/* the list of C function namings, which is */
				/* terminated by {0, 0}. */
} c_library_t;


/* A C function prototype declaration */
#define CFUNC_PROTO(NAME, FUNC, LIB7TYPE)	\
	extern lib7_val_t FUNC (lib7_state_t *lib7_state, lib7_val_t arg);

/* A C function naming */
#define CFUNC_BIND(NAME, FUNC, LIB7TYPE)	\
    { NAME, FUNC },

/* the terminator for a C function list */
#define CFUNC_NULL_BIND		{ NULL, NULL }

#endif /* !_C_LIBRARY_ */


/* COPYRIGHT (c) 1994 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
