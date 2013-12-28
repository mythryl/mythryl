// mythryl-callable-c-libraries.h
//
// Here we define (some of) the machinery needed to make
// the C libraries listed in
//
//     src/c/lib/mythryl-callable-c-libraries-list.h
//
// available to Mythryl-level code.
//
// For the Mythryl side of this see:
//     src/lib/std/src/unsafe/mythryl-callable-c-library-interface.pkg

#ifndef MYTHRYL_CALLABLE_C_LIBRARY_H
#define MYTHRYL_CALLABLE_C_LIBRARY_H


// A pointer to a library initialization function;
// it is passed the (argc,argv) list of command-line arguments.
//
typedef void (*Clibrary_Initialization_Function) (int, char**);


// A pointer to a Mythryl-callable C function:
//
typedef Val (*Mythryl_Callable_C_Function) (Task*, Val);


// Mythryl_Name_With_C_Function
//
typedef struct {
    //
    const char*                   name;
    const char*                   nickname;	// See Hashtable_Entry comment in src/c/heapcleaner/mythryl-callable-cfun-hashtable.c
    Mythryl_Callable_C_Function	  cfunc;
    const char*                   mythryl_type;
    //
} Mythryl_Name_With_C_Function;
    //
    // A Mythryl-callable C function together with its
    // Mythryl-visible name.  The function may be accessed
    // by name on the Mythryl side via the
    //
    //     find_c_function { lib_name => "my_library", fun_name => "my_function" };
    //
    // function from
    //     src/lib/std/src/unsafe/mythryl-callable-c-library-interface.pkg

// The representation of a library of Mythryl-callable C functions:
//
typedef struct {
    //
    const char*  library_name;
    const char*  version;
    const char*  date;
    //
    Clibrary_Initialization_Function  initialize_mythryl_callable_c_library;		// Initialization function, else NULL.
    Mythryl_Name_With_C_Function*     vector_of_mythryl_names_and_c_functions;		// Vector of name-fn pairs, terminated by a {0, 0} record.
    //
} Mythryl_Callable_C_Library;


// A C function prototype declaration:
//
#define CFUNC_PROTO(NAME, FUNC, LIB7TYPE)	\
	extern Val FUNC (Task *task, Val arg);

// Create a Mythryl-name/C-function pair:
//
#define CFUNC_BIND(NAME, NAME2, FUNC, LIB7TYPE)	\
    { NAME, NAME2, FUNC, LIB7TYPE },

// The terminator for the Mythryl-Name/C-function vector:
//
#define CFUNC_NULL_BIND		{ NULL, NULL, NULL }

#endif						// MYTHRYL_CALLABLE_C_LIBRARY_H


// COPYRIGHT (c) 1994 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.

