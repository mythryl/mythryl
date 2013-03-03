// libmythryl-ncurses.c

// This file handles the C side
// of the Mythryl <-> C interface
// layer for the Mythryl in-process
// Ncurses binding.  The Mythryl side
// is implemented by
//
//     src/glu/ncurses/src/ncurses-client-driver-for-library-in-main-process.pkg
//
// We get compiled by:
//    src/glu/ncurses/c/in-main/Makefile.in


#include "../../../../c/mythryl-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curses.h>
 
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"


#include "../../../../c/lib/raise-error.h"


static char text_buf[ 1024 ];

static void   moan_and_die   (void)   {
    //        ============
    //
    printf( "FATAL src/c/lib/ncurses/mythryl-ncurses-library-in-main-process.c: %s  exit(1)ing.\n", text_buf );		fflush(stdout);
    exit(1);
}

static Val   do__init   (Task* task,  Val arg)   {	// : Void -> Void
           //========


    return HEAP_VOID;
}




// We do not want to call Mythryl
// code directly from the C level;
// that would lead to messy problems.
//
// Consequently when Ncurses issues a
// widget callback we queue up the
// event and then read it in response
// to calls from the Mythryl side.
//
// Here we implement that queue.

#define MAX_CALLBACK_QUEUE 1024

// Define the different types of
// callback queue entries supported.
//
// WARNING! Must be kept in sync
// with matching declarations in
//
//     src/glu/ncurses/src/ncurses-client-driver-for-library-in-main-process.pkg 
//
#define          QUEUED_VOID_CALLBACK   1
#define          QUEUED_BOOL_CALLBACK   2
#define         QUEUED_FLOAT_CALLBACK   3
#define      QUEUED_INT_PAIR_CALLBACK   4

typedef struct {
    //
    int callback_number;	// This identifies which closure to call on the Mythryl side.
    int callback_type;		// This will be one of the QUEUED_*_CALLBACK values above.
    //
    union {
	//
        int    bool_value;
	//
        double float_value; 
	//
        struct {		// int_pair;
	    int x;
	    int y;
        } int_pair;


    } entry;

} Callback_Queue_Entry;

// Here is the cat-and-mouse queue proper.
// Queue is empty when cat==rat.
// Next slot to read  is  callback_queue[ callback_queue_cat ].
// Next slot to write is  callback_queue[ callback_queue_rat ].
//
static int   callback_queue_cat = 0;
static int   callback_queue_rat = 0;
//
static Callback_Queue_Entry  callback_queue   [ MAX_CALLBACK_QUEUE ];

static int  callback_queue_bump (int at)   {
    //      ===================
    if (at < (MAX_CALLBACK_QUEUE-1))   return at + 1;
    return 0;
}

static int   callback_queue_is_empty   ()   {
    //       =======================
    return  callback_queue_cat == callback_queue_rat;
}

static int   number_of_queued_callbacks   ()   {
    //       ==========================
    if         (callback_queue_rat >= callback_queue_cat) {
        return  callback_queue_rat -  callback_queue_cat;
    }
    return (callback_queue_rat + MAX_CALLBACK_QUEUE) - callback_queue_cat;
}

static int   type_of_next_queued_callback   ()   {
    //       ============================
    if (callback_queue_is_empty()) {
        strcpy( text_buf, "type_of_next_queued_callback(): Callback queue is empty.\n" );
        moan_and_die();
    }
    return callback_queue[ callback_queue_cat ].callback_type;
}

static Callback_Queue_Entry   get_next_queued_callback   ()   {
    //                        ========================
    int cat = callback_queue_cat;
printf("get_next_queued_callback called...  --libmythryl-ncurses.c\n");
    if (callback_queue_is_empty()) {
        strcpy( text_buf, "get_next_queued_callback(): Callback queue is empty.\n" );
        moan_and_die();
    }
    callback_queue_cat = callback_queue_bump( callback_queue_cat );
    return callback_queue[ cat ];
}

static void   queue_up_callback   (Callback_Queue_Entry entry)   {
    //        =================
    int rat = callback_queue_rat;

    callback_queue_rat = callback_queue_bump( callback_queue_rat );

    if (callback_queue_is_empty()) {
	//
        strcpy( text_buf, "queue_up_callback(): Callback queue overflowed.\n" );
        moan_and_die();
    }
    callback_queue[ rat ] = entry;
}

static void   queue_up_void_callback   (int callback)   {
    //        ======================
    Callback_Queue_Entry e;
    //
    e.callback_type   =  QUEUED_VOID_CALLBACK;
    e.callback_number =  callback;
    //
    queue_up_callback(e);
}
void dummy_ncurses_call_to__queue_up_void_callback() { queue_up_void_callback(1); }		// Can delete this as soon as we have some real calls -- this is just to quiet gcc.

static void   queue_up_bool_callback   (int callback, int bool_value)   {
    //        ======================  
    Callback_Queue_Entry e;
    //
    e.callback_type    =  QUEUED_BOOL_CALLBACK;
    e.callback_number  =  callback;
    e.entry.bool_value =  bool_value;
    //
    queue_up_callback( e );
}
										void dummy_ncurses_call_to__queue_up_bool_callback() { queue_up_bool_callback(1,TRUE); }		// Can delete this as soon as we have some real calls -- this is just to quiet gcc.

static void   queue_up_float_callback   (int callback,  double float_value)   {
    //        =======================
    Callback_Queue_Entry e;
    //
    e.callback_type     =  QUEUED_FLOAT_CALLBACK;
    e.callback_number   =  callback;
    e.entry.float_value =  float_value;
    //
    queue_up_callback( e );
}
										void dummy_ncurses_call_to__queue_up_float_callback() { queue_up_float_callback(1,1.0); }		// Can delete this as soon as we have some real calls -- this is just to quiet gcc.

static int   find_free_callback_id   ()   {
    //       =====================
    static int next_callback_id = 1;

    return next_callback_id++;
}
										void dummy_ncurses_call_to__find_free_callback_id() { find_free_callback_id(); }		// Can delete this as soon as we have some real calls -- this is just to quiet gcc.









// ncurses-client.api        type:   (None -- not exported to ncurses-client.api level.)
// ncurses-client-driver.api type:   Void -> Bool
//
static Val   do__callback_queue_is_empty   (Task* task,  Val arg)   {
    //       ===========================
    //
    return  callback_queue_is_empty()
              ?  HEAP_TRUE
              : HEAP_FALSE;
}

// ncurses-client.api        type:   (None -- not exported to ncurses-client.api level.)
// ncurses-client-driver.api type:   Void -> Int
//
static Val   do__number_of_queued_callbacks   (Task* task,  Val arg)   {
    //       ==============================
    //
    int result =  number_of_queued_callbacks ();
    //
    return TAGGED_INT_FROM_C_INT( result );
}

// ncurses-client.api        type:   (None -- not exported to ncurses-client.api level.)
// ncurses-client-driver.api type:   Void -> Int
//
static Val   do__type_of_next_queued_callback   (Task* task,  Val arg)   {
    //       ================================
    int result =  type_of_next_queued_callback ();
    //
    return TAGGED_INT_FROM_C_INT( result );
}

// ncurses-client.api        type:   (None -- not exported to ncurses-client.api level.)
// ncurses-client-driver.api type:   Void -> Int
//
Val   _lib7_Ncurses_get_queued_void_callback   (Task* task,  Val arg)   {
    //==================================
    //
    Callback_Queue_Entry e = get_next_queued_callback ();

    if (e.callback_type != QUEUED_VOID_CALLBACK) {
	//
        strcpy( text_buf, "get_queued_void_callback: Next callback not Void." );
        moan_and_die();
    }
    //
    return TAGGED_INT_FROM_C_INT( e.callback_number );
}


// ncurses-client.api        type:   (None -- not exported to ncurses-client.api level.)
// ncurses-client-driver.api type:   Void -> (Int, Bool)
//
Val   _lib7_Ncurses_get_queued_bool_callback   (Task *task,  Val arg)   {
    //==================================
    //
    Callback_Queue_Entry e = get_next_queued_callback ();
    //
    if (e.callback_type != QUEUED_BOOL_CALLBACK) {
        strcpy( text_buf, "get_queued_bool_callback: Next callback not Bool." );
        moan_and_die();
    }

    set_slot_in_nascent_heapchunk(  task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 2));
    set_slot_in_nascent_heapchunk(  task, 1, TAGGED_INT_FROM_C_INT( e.callback_number ));
    set_slot_in_nascent_heapchunk(  task, 2, e.entry.bool_value ?  HEAP_TRUE : HEAP_FALSE );
    //
    return commit_nascent_heapchunk(task, 2);
}


// ncurses-client.api        type:   (None -- not exported to ncurses-client.api level.)
// ncurses-client-driver.api type:   Void -> (Int, Float)
//
Val   _lib7_Ncurses_get_queued_float_callback   (Task* task, Val arg)  {
    //===================================
    //
    Callback_Queue_Entry e = get_next_queued_callback ();
    //
    if (e.callback_type != QUEUED_FLOAT_CALLBACK) {
        strcpy( text_buf, "get_queued_float_callback: Next callback not Float." );
        moan_and_die();
    }

    double d =  e.entry.float_value;

    Val boxed_double =   make_float64(task, d );					// make_float64		is from   src/c/h/make-strings-and-vectors-etc.h

    set_slot_in_nascent_heapchunk(  task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 2));
    set_slot_in_nascent_heapchunk(  task, 1, TAGGED_INT_FROM_C_INT( e.callback_number ));
    set_slot_in_nascent_heapchunk(  task, 2, boxed_double );
    return commit_nascent_heapchunk(task, 2);
}


// ncurses-client.api        type:   (None -- not exported to ncurses-client.api level.)
// ncurses-client-driver.api type:   Void -> (Int,     Int)
//                                 callback x      y
//
static Val   do__get_queued_int_pair_callback   (Task *task, Val arg)   {
    //       ================================
    Callback_Queue_Entry e = get_next_queued_callback ();

printf("do__get_queued_int_pair_callback called\n");
    if (e.callback_type != QUEUED_INT_PAIR_CALLBACK) {
        strcpy( text_buf, "get_queued_int_pair_callback: Next callback not Int_Pair." );
        moan_and_die();
    }

printf("do__get_queued_int_pair_callback called returning a record.\n");
    set_slot_in_nascent_heapchunk(  task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 3)      );
    set_slot_in_nascent_heapchunk(  task, 1, TAGGED_INT_FROM_C_INT( e.callback_number    ));
    set_slot_in_nascent_heapchunk(  task, 2, TAGGED_INT_FROM_C_INT( e.entry.int_pair.x	 ));
    set_slot_in_nascent_heapchunk(  task, 3, TAGGED_INT_FROM_C_INT( e.entry.int_pair.y ));
    //
    return commit_nascent_heapchunk(task, 3);
}



/////////////////////////////////////////////////////////////////////////////////////
// The following stuff gets built from paragraphs in
//     src/glu/ncurses/etc/ncurses-construction.plan
// via logic in
//     src/lib/make-library-glue/make-library-glue.pkg
//
// Paragraphs like
//     build-a: plain-fn
//     fn-name:
//     fn-type:
//     libcall:
// drive the code-build path
//  build_plain_fn
//  -> build_plain_fun_for_'libmythryl_xxx_c'
//     -> build_fun_header_for_'libmythryl_xxx_c'
//      + build_fun_arg_loads_for__'libmythryl_xxx_c'
//      + build_fun_body_for__'libmythryl_xxx_c'
//      + build_fun_trailer_for_'libmythryl_xxx_c'
// 
// Paragraphs like
//     build-a: callback-fn
//     fn-name:
//     fn-type:
// drive the code-build path
//     mlb::BUILD_A ("callback-fn", build_callback_function)			# In src/glu/ncurses/sh/make-ncurses-glue
//  ->  build_callback_function							# In src/glu/ncurses/sh/make-ncurses-glue
//  ->  build_set_callback_fn_for_'libmythryl_xxx_c'				# In src/glu/ncurses/sh/make-ncurses-glue
//  ->  r.to_libmythryl_xxx_c_funs						# In src/lib/make-library-glue/make-library-glue.pkg
//
/* Do not edit this or following lines -- they are autobuilt.  (patchname="functions") */
/* do__print_hello_world
 *
 * ncurses-client.api        type:    Session -> Void
 * ncurses-client-driver.api type:   (Session) -> Void
 */
static Val   do__print_hello_world   (Task* task, Val arg)
{


    fprintf(stderr,"Hello, world!\n");

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/ncurses-construction.plan. */


/* do__negate_int
 *
 * ncurses-client.api        type:   (Session, Int) -> Int
 * ncurses-client-driver.api type:   (Session, Int) -> Int
 */
static Val   do__negate_int   (Task* task, Val arg)
{

    int               i0 =                            GET_TUPLE_SLOT_AS_INT( arg, 1);

    int result = -i0;

    return TAGGED_INT_FROM_C_INT(result);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/ncurses-construction.plan. */


/* do__negate_float
 *
 * ncurses-client.api        type:   (Session, Float) -> Float
 * ncurses-client-driver.api type:   (Session, Float) -> Float
 */
static Val   do__negate_float   (Task* task, Val arg)
{

    double            f0 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 1)));

    double d = -f0;

    return  make_float64(task, d );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/ncurses-construction.plan. */


/* do__negate_boolean
 *
 * ncurses-client.api        type:   (Session, Bool) -> Bool
 * ncurses-client-driver.api type:   (Session, Bool) -> Bool
 */
static Val   do__negate_boolean   (Task* task, Val arg)
{

    int               b0 =                            GET_TUPLE_SLOT_AS_VAL( arg, 1) == HEAP_TRUE;

    int result = !b0;

    return  result ? HEAP_TRUE : HEAP_FALSE;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/ncurses-construction.plan. */


/* do__addch
 *
 * ncurses-client.api        type:   (Session, Int) -> Void
 * ncurses-client-driver.api type:   (Session, Int) -> Void
 */
static Val   do__addch   (Task* task, Val arg)
{

    int               i0 =                            GET_TUPLE_SLOT_AS_INT( arg, 1);

    addch(i0);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/ncurses-construction.plan. */


/* do__cbreak
 *
 * ncurses-client.api        type:    Session -> Void
 * ncurses-client-driver.api type:   (Session) -> Void
 */
static Val   do__cbreak   (Task* task, Val arg)
{


    cbreak();

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/ncurses-construction.plan. */


/* do__endwin
 *
 * ncurses-client.api        type:    Session -> Void
 * ncurses-client-driver.api type:   (Session) -> Void
 */
static Val   do__endwin   (Task* task, Val arg)
{


    endwin();

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/ncurses-construction.plan. */


/* do__getch
 *
 * ncurses-client.api        type:    Session -> Int
 * ncurses-client-driver.api type:   (Session) -> Int
 */
static Val   do__getch   (Task* task, Val arg)
{


    int result = getch();

    return TAGGED_INT_FROM_C_INT(result);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/ncurses-construction.plan. */


/* do__has_colors
 *
 * ncurses-client.api        type:    Session -> Bool
 * ncurses-client-driver.api type:   (Session) -> Bool
 */
static Val   do__has_colors   (Task* task, Val arg)
{


    int result = has_colors();

    return  result ? HEAP_TRUE : HEAP_FALSE;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/ncurses-construction.plan. */


/* do__initscr
 *
 * ncurses-client.api        type:    Session -> Void
 * ncurses-client-driver.api type:   (Session) -> Void
 */
static Val   do__initscr   (Task* task, Val arg)
{


    initscr();

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/ncurses-construction.plan. */


/* Do not edit this or preceding lines -- they are autobuilt. */
/////////////////////////////////////////////////////////////////////////////////////



/////////////// old libmythryl-ncurses.c contents follow //////////////////////////////////

#include "../../../../c/mythryl-config.h"

#include "runtime-base.h"
#include "mythryl-callable-c-libraries.h"
#include "raise-error.h"


// This section lists the directory library of C functions that are callable from Mythryl.

// This table ultimately gets searched by
//
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"ncurses"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 13, 2008"
#endif

// The table of C functions and their Mythryl names:
//
#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, NAME2, FUNC, LIB7TYPE)
static Mythryl_Name_With_C_Function CFunTable[] = {


CFUNC("init","init",	do__init,		"Void -> Void")

CFUNC("callback_queue_is_empty","callback_queue_is_empty",	   do__callback_queue_is_empty,		"Void -> Bool")
CFUNC("number_of_queued_callbacks","number_of_queued_callbacks",	   do__number_of_queued_callbacks,	"Void -> Int")
CFUNC("type_of_next_queued_callback","type_of_next_queued_callback",	   do__type_of_next_queued_callback,	"Void -> Int")
CFUNC("get_queued_int_pair_callback","get_queued_button_press_callback",  do__get_queued_int_pair_callback,	"Void -> (Int, Int, Int)")  // Void -> (callback_number, x, y)


/////////////////////////////////////////////////////////////////////////////////////
// The following stuff gets built from paragraphs in
//     src/glu/ncurses/etc/ncurses-construction.plan
// via logic in
//     src/lib/make-library-glue/make-library-glue.pkg
//
// Paragraphs like
//     build-a: plain-fn
//     fn-name:
//     fn-type:
//     libcall:
// drive the code-build path
//   build_plain_function
//     -> build_table_entry_for_'libmythryl_xxx_c
//        -> to_libmythryl_xxx_c_table
// 
// Paragraphs like
//     build-a: callback-fn
//     fn-name:
//     fn-type:
//     lowtype:
// drive the code-build path
//   mlb::BUILD_A ("callback-fn", build_callback_function)				# In src/glu/ncurses/sh/make-ncurses-glue
//   ->  build_callback_function							# In src/glu/ncurses/sh/make-ncurses-glue
//       ->  build_set_callback_fn_for_'libmythryl_xxx_c'				# In src/glu/ncurses/sh/make-ncurses-glue
//           ->  r.build_table_entry_for_'libmythryl_xxx_c' (c_fn_name, fn_type);
//
/* Do not edit this or following lines -- they are autobuilt.  (patchname="table") */
CFUNC("print_hello_world",                        "print_hello_world",                        do__print_hello_world,                                 "Session -> Void")
CFUNC("negate_int",                               "negate_int",                               do__negate_int,                                       "(Session, Int) -> Int")
CFUNC("negate_float",                             "negate_float",                             do__negate_float,                                     "(Session, Float) -> Float")
CFUNC("negate_boolean",                           "negate_boolean",                           do__negate_boolean,                                   "(Session, Bool) -> Bool")
CFUNC("addch",                                    "addch",                                    do__addch,                                            "(Session, Int) -> Void")
CFUNC("cbreak",                                   "cbreak",                                   do__cbreak,                                            "Session -> Void")
CFUNC("endwin",                                   "endwin",                                   do__endwin,                                            "Session -> Void")
CFUNC("getch",                                    "getch",                                    do__getch,                                             "Session -> Int")
CFUNC("has_colors",                               "has_colors",                               do__has_colors,                                        "Session -> Bool")
CFUNC("initscr",                                  "initscr",                                  do__initscr,                                           "Session -> Void")
/* Do not edit this or preceding lines -- they are autobuilt. */
/////////////////////////////////////////////////////////////////////////////////////

	CFUNC_NULL_BIND
    };
#undef CFUNC



// The Ncurses library:
//
// Our record                Libmythryl_Ncurses
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries__local []
// in                        src/c/lib/mythryl-callable-c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Ncurses = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              ==================
    CLIB_NAME,
    CLIB_VERSION,
    CLIB_DATE,
    NULL,
    CFunTable
};


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.






/*
##########################################################################
#   The following is support for outline-minor-mode in emacs.		 #
#  ^C @ ^T hides all Text. (Leaves all headings.)			 #
#  ^C @ ^A shows All of file.						 #
#  ^C @ ^Q Quickfolds entire file. (Leaves only top-level headings.)	 #
#  ^C @ ^I shows Immediate children of node.				 #
#  ^C @ ^S Shows all of a node.						 #
#  ^C @ ^D hiDes all of a node.						 #
#  ^HFoutline-mode gives more details.					 #
#  (Or do ^HI and read emacs:outline mode.)				 #
#									 #
# Local variables:							 #
# mode: outline-minor							 #
# outline-regexp: "[A-Za-z]"			 		 	 #
# End:									 #
##########################################################################
*/

