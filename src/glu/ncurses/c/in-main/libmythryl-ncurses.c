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

Val   do__init   (Task* task,  Val arg)   {	// : Void -> Void
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
void dummy_call_to__queue_up_void_callback() { queue_up_void_callback(1); }		// Can delete this as soon as we have some real calls -- this is just to quiet gcc.

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
										void dummy_call_to__queue_up_bool_callback() { queue_up_bool_callback(1,TRUE); }		// Can delete this as soon as we have some real calls -- this is just to quiet gcc.

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
										void dummy_call_to__queue_up_float_callback() { queue_up_float_callback(1,1.0); }		// Can delete this as soon as we have some real calls -- this is just to quiet gcc.

static int   find_free_callback_id   ()   {
    //       =====================
    static int next_callback_id = 1;

    return next_callback_id++;
}
										void dummy_call_to__find_free_callback_id() { find_free_callback_id(); }		// Can delete this as soon as we have some real calls -- this is just to quiet gcc.

static int               window_size_event_callback_number;
static void GLFWCALL run_window_size_event_callback   (int wide,  int high)   {
    //               ==============================

    Callback_Queue_Entry e;

    e.callback_type    =  QUEUED_INT_PAIR_CALLBACK;
    e.callback_number  =  window_size_event_callback_number;

    e.entry.int_pair.x  = wide;
    e.entry.int_pair.y  = high;
printf("run_window_size_event_callback(wide=%d high=%d)\n",wide,high);

    queue_up_callback(e);
}
										void dummy_call_to__run_window_size_event_callback() { run_window_size_event_callback(1,2); }		// Can delete this as soon as we have some real calls -- this is just to quiet gcc.








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
//     src/glu/ncurses/etc/construction.plan
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


/*  do__set_window_size_event_callback : Session -> Window_Size_Event_Callback -> Void
 */
static Val   do__set_window_size_event_callback (Task* task, Val arg)
{

    int id   =  find_free_callback_id ();
    window_size_event_callback_number =  id;

    glfwSetWindowSizeCallback( run_window_size_event_callback );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/ncurses/etc/construction.plan.*/


/* do__glew_init
 *
 * ncurses-client.api        type:    Session -> Void
 * ncurses-client-driver.api type:   (Session) -> Void
 */
static Val   do__glew_init   (Task* task, Val arg)
{


    GLenum result = glewInit();;
   if (result != GLEW_OK) {
       fprintf(stderr, "Error: '%s'\n", glewGetErrorString(result));
       exit(1);
   }

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/construction.plan. */


/* do__open_window2
 *
 * ncurses-client.api        type:    {  session: Session,  wide: Int, high: Int,  redbits: Int, greenbits: Int, bluebits: Int,  alphabits: Int, depthbits: Int, stencilbits: Int,  fullscreen: Bool } -> Bool
 * ncurses-client-driver.api type:   (Session, Int, Int, Int, Int, Int, Int, Int, Int, Bool) -> Bool
 */
static Val   do__open_window2   (Task* task, Val arg)
{

    int               i0 =                            GET_TUPLE_SLOT_AS_INT( arg, 1);
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);
    int               i4 =                            GET_TUPLE_SLOT_AS_INT( arg, 5);
    int               i5 =                            GET_TUPLE_SLOT_AS_INT( arg, 6);
    int               i6 =                            GET_TUPLE_SLOT_AS_INT( arg, 7);
    int               i7 =                            GET_TUPLE_SLOT_AS_INT( arg, 8);
    int               b8 =                            GET_TUPLE_SLOT_AS_VAL( arg, 9) == HEAP_TRUE;

    int result = glfwOpenWindow(   /*wide*/i0, /*high*/i1,   /*redbits*/i2, /*greenbits*/i3, /*bluebits*/i4,   /*alphabits*/i5, /*depthbits*/i6, /*stencilbits*/i7,   /*fullscreen*/b8 ? GLFW_FULLSCREEN : GLFW_WINDOW );

    return  result ? HEAP_TRUE : HEAP_FALSE;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/construction.plan. */


/* do__open_window
 *
 * ncurses-client.api        type:    {  session: Session,  wide: Int, high: Int } -> Bool
 * ncurses-client-driver.api type:   (Session, Int, Int) -> Bool
 */
static Val   do__open_window   (Task* task, Val arg)
{

    int               i0 =                            GET_TUPLE_SLOT_AS_INT( arg, 1);
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    int result = glfwOpenWindow(   /*wide*/i0, /*high*/i1,   /*redbits*/0, /*greenbits*/0, /*bluebits*/0,   /*alphabits*/0, /*depthbits*/0, /*stencilbits*/0,   /*fullscreen*/GLFW_WINDOW );

    return  result ? HEAP_TRUE : HEAP_FALSE;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/construction.plan. */


/* do__terminate
 *
 * ncurses-client.api        type:    Session -> Void
 * ncurses-client-driver.api type:   (Session) -> Void
 */
static Val   do__terminate   (Task* task, Val arg)
{


    glfwTerminate();

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/construction.plan. */


/* do__swap_buffers
 *
 * ncurses-client.api        type:    Session -> Void
 * ncurses-client-driver.api type:   (Session) -> Void
 */
static Val   do__swap_buffers   (Task* task, Val arg)
{


    glfwSwapBuffers();

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/construction.plan. */


/* do__get_window_param
 *
 * ncurses-client.api        type:    Session -> Bool
 * ncurses-client-driver.api type:   (Session) -> Bool
 */
static Val   do__get_window_param   (Task* task, Val arg)
{


    int result = glfwGetWindowParam( GLFW_OPENED );

    return  result ? HEAP_TRUE : HEAP_FALSE;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/construction.plan. */


/* do__set_window_title
 *
 * ncurses-client.api        type:   (Session, String) -> Void
 * ncurses-client-driver.api type:   (Session, String) -> Void
 */
static Val   do__set_window_title   (Task* task, Val arg)
{

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    glfwSetWindowTitle( s0 );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/construction.plan. */


/* do__set_window_size
 *
 * ncurses-client.api        type:    { session: Session, wide: Int, high: Int } -> Void
 * ncurses-client-driver.api type:   (Session, Int, Int) -> Void
 */
static Val   do__set_window_size   (Task* task, Val arg)
{

    int               i0 =                            GET_TUPLE_SLOT_AS_INT( arg, 1);
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    glfwSetWindowSize( /*wide*/i0, /*high*/i1 );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/construction.plan. */


/* do__set_window_position
 *
 * ncurses-client.api        type:    { session: Session, x: Int, y: Int } -> Void
 * ncurses-client-driver.api type:   (Session, Int, Int) -> Void
 */
static Val   do__set_window_position   (Task* task, Val arg)
{

    int               i0 =                            GET_TUPLE_SLOT_AS_INT( arg, 1);
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    glfwSetWindowPos( /*x*/i0, /*y*/i1 );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/construction.plan. */


/* do__clear
 *
 * ncurses-client.api        type:    {  session: Session,  color_buffer: Bool, depth_buffer: Bool } -> Void
 * ncurses-client-driver.api type:   (Session, Bool, Bool) -> Void
 */
static Val   do__clear   (Task* task, Val arg)
{

    int               b0 =                            GET_TUPLE_SLOT_AS_VAL( arg, 1) == HEAP_TRUE;
    int               b1 =                            GET_TUPLE_SLOT_AS_VAL( arg, 2) == HEAP_TRUE;

    glClear(   (/*color_buffer*/b0 ? GL_COLOR_BUFFER_BIT : 0)  |  (/*depth_buffer*/b1 ? GL_DEPTH_BUFFER_BIT : 0));

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/construction.plan. */


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
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/construction.plan. */


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
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/construction.plan. */


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
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/construction.plan. */


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
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/ncurses/etc/construction.plan. */


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
//     src/glu/ncurses/etc/construction.plan
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
CFUNC("set_window_size_event_callback",           "set_window_size_event_callback",           do__set_window_size_event_callback,                    "Session -> Window_Size_Event_Callback -> Void")
CFUNC("glew_init",                                "glew_init",                                do__glew_init,                                         "Session -> Void")
CFUNC("open_window2",                             "open_window2",                             do__open_window2,                                      "{  session: Session,  wide: Int, high: Int,  redbits: Int, greenbits: Int, bluebits: Int,  alphabits: Int, depthbits: Int, stencilbits: Int,  fullscreen: Bool } -> Bool")
CFUNC("open_window",                              "open_window",                              do__open_window,                                       "{  session: Session,  wide: Int, high: Int } -> Bool")
CFUNC("terminate",                                "terminate",                                do__terminate,                                         "Session -> Void")
CFUNC("swap_buffers",                             "swap_buffers",                             do__swap_buffers,                                      "Session -> Void")
CFUNC("get_window_param",                         "get_window_param",                         do__get_window_param,                                  "Session -> Bool")
CFUNC("set_window_title",                         "set_window_title",                         do__set_window_title,                                 "(Session, String) -> Void")
CFUNC("set_window_size",                          "set_window_size",                          do__set_window_size,                                   "{ session: Session, wide: Int, high: Int } -> Void")
CFUNC("set_window_position",                      "set_window_position",                      do__set_window_position,                               "{ session: Session, x: Int, y: Int } -> Void")
CFUNC("clear",                                    "clear",                                    do__clear,                                             "{  session: Session,  color_buffer: Bool, depth_buffer: Bool } -> Void")
CFUNC("print_hello_world",                        "print_hello_world",                        do__print_hello_world,                                 "Session -> Void")
CFUNC("negate_int",                               "negate_int",                               do__negate_int,                                       "(Session, Int) -> Int")
CFUNC("negate_float",                             "negate_float",                             do__negate_float,                                     "(Session, Float) -> Float")
CFUNC("negate_boolean",                           "negate_boolean",                           do__negate_boolean,                                   "(Session, Bool) -> Bool")
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

