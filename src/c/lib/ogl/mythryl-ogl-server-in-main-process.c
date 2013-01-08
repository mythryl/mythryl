// mythryl-ogl-server-in-main-process.c
//
// This file handles the C side
// of the Mythryl <-> C interface
// layer for the Mythryl in-process
// Ogl binding.  The Mythryl side
// is implemented by
//
//     src/lib/src/ogl-client-driver-for-server-in-main-process.pkg
//


#include "../../mythryl-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glfw.h>
 
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"
#include "../raise-error.h"

#define GtkWidget void

#define MAX_WIDGETS 1024
#ifdef SOON
static GtkWidget* widget[ MAX_WIDGETS ];	// XXX BUGGO FIXME Should expand in size as needed.
#endif

static char text_buf[ 1024 ];

static void   moan_and_die   (void)   {
    //        ============
    //
    printf( "FATAL src/c/lib/ogl/mythryl-ogl-server-in-main-process.c: %s  exit(1)ing.\n", text_buf );		fflush(stdout);
    exit(1);
}

Val   _lib7_Ogl_ogl_init   (Task* task,  Val arg)   {	// : Void -> Void
    //==================

    if (!glfwInit())   exit( EXIT_FAILURE );

    return HEAP_VOID;
}

void ogl_driver_dummy( void ) {				// This just a test to see if the appropriate libraries are linking; I don't intend to actually call this fn. Public only to keep gcc from muttering about unused code.
    //
    int running = GL_TRUE;

    // Initialize GLFW
    //
    if (!glfwInit())   exit( EXIT_FAILURE );

    // Open an OpenGL window
    //
    if( !glfwOpenWindow( 300,300, 0,0,0,0,0,0, GLFW_WINDOW ) )
    {
	glfwTerminate();

	exit( EXIT_FAILURE );
    }

    // Main loop

    while( running )
    {
	// OpenGL rendering goes here...

	glClear( GL_COLOR_BUFFER_BIT );

	glfwSwapBuffers();				// Swap front and back rendering buffers

	running =  !glfwGetKey( GLFW_KEY_ESC )		// Check if ESC key was pressed or window was closed
                &&  glfwGetWindowParam( GLFW_OPENED );

    }


    glfwTerminate();					// Close window and terminate GLFW

    exit( EXIT_SUCCESS );				// Exit program

}




// We do not want to call Mythryl
// code directly from the C level;
// that would lead to messy problems.
//
// Consequently when Ogl issues a
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
//     src/lib/src/ogl-client-driver-for-server-in-main-process.pkg 
//
#define          QUEUED_VOID_CALLBACK   1
#define          QUEUED_BOOL_CALLBACK   2
#define         QUEUED_FLOAT_CALLBACK   3
#define  QUEUED_BUTTON_PRESS_CALLBACK   4
#define     QUEUED_KEY_PRESS_CALLBACK   5
#define QUEUED_MOTION_NOTIFY_CALLBACK   6
#define        QUEUED_EXPOSE_CALLBACK   7
#define     QUEUED_CONFIGURE_CALLBACK   8

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
        struct {		// button_press;
	    int    widget_id;
	    int    button;
	    double x;
	    double y;
	    int    time;
	    int    modifiers;
        } button_press;

        struct {
	    int  key;
	    int  keycode;
	    int  time;
	    int  modifiers;
        }                     key_press;

        struct {
	    int    widget_id;

	    int    time;
	    double x;
	    double y;
	    int    modifiers;
	    int    is_hint;
        }                     motion_notify;

        struct {
	    int    widget_id;

	    int    count;
	    int    area_x;
	    int    area_y;
	    int    area_wide;
	    int    area_high;
        }                     expose;

        struct {
	    int    widget_id;

	    int    x;
	    int    y;
	    int    wide;
	    int    high;
        }                     configure;

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
    if (callback_queue_is_empty()) {
        strcpy( text_buf, "get_next_queued_callback(): Callback queue is empty.\n" );
        moan_and_die();
    }
    callback_queue_cat = callback_queue_bump( callback_queue_cat );
    return callback_queue[ cat ];
}


#ifdef OLD
static int   find_free_widget_slot   (void)   {
    //       =====================
    //
    for (int i = 1; i < MAX_WIDGETS; ++i) {		// We reserve 0 as an always-invalid analog to NULL.
        if (!widget[i])  return i;
    }
    sprintf(text_buf, "find_free_widget_slot: All slots full.");
    moan_and_die();
    return 0;						// Can't happen, but keeps gcc quiet.
}
#endif

#ifdef OLD
static int   find_widget_id   (GtkWidget* query_widget)   {
    //       ==============
    for (int i = 1;   i < MAX_WIDGETS;   ++i) {
	//
        if (widget[i] == query_widget)   return i;
    }
    return 0;
}
#endif


#ifdef OLD
static int   get_widget_id   (GtkWidget* query_widget)   {
    //       =============
    int slot =  find_widget_id( query_widget );

    if(!slot) {
	slot = find_free_widget_slot ();
	//
	widget[slot] = (GtkWidget*) query_widget;
    }

    return slot;
}
#endif




#ifdef OLD
Val   _lib7_Ogl_ogl_init   (Task* task,  Val arg)   {	// : Void -> Void
    //==================

    #if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)
	//
	extern char	**commandline_args_without_argv0_or_runtime_args__global;

	int argc = 0;
	while (commandline_args_without_argv0_or_runtime_args__global[argc]) ++argc;    

	if (!gtk_init_check( &argc, &commandline_args_without_argv0_or_runtime_args__global )) {
	    //
	    return RAISE_ERROR__MAY_HEAPCLEAN(task, "ogl_init: failed to initialize GUI support", NULL);
	}

	// XXX BUGGO FIXME: ogl_init_check installs ogl default signal handlers,
	//		    which most likely screws up Mythryl's own signal
	//		    handling no end.  At some point should put some work
	//		    into keeping both happy.
	//
	return HEAP_VOID;
    #else
	extern char*  no_ogl_support_in_runtime;
	//
	return RAISE_ERROR__MAY_HEAPCLEAN(task, no_ogl_support_in_runtime, NULL);
    #endif
}
#endif

// ogl-client.api        type:   (None -- not exported to ogl-client.api level.)
// ogl-client-driver.api type:   Void -> Bool
//
Val   _lib7_Ogl_callback_queue_is_empty   (Task* task,  Val arg)   {
    //=================================
    //
    return  callback_queue_is_empty()
              ?  HEAP_TRUE
              : HEAP_FALSE;
}

// ogl-client.api        type:   (None -- not exported to ogl-client.api level.)
// ogl-client-driver.api type:   Void -> Int
//
Val   _lib7_Ogl_number_of_queued_callbacks   (Task* task,  Val arg)   {
    //====================================
    //
    int result =  number_of_queued_callbacks ();
    //
    return TAGGED_INT_FROM_C_INT( result );
}

// ogl-client.api        type:   (None -- not exported to ogl-client.api level.)
// ogl-client-driver.api type:   Void -> Int
//
Val   _lib7_Ogl_type_of_next_queued_callback   (Task* task,  Val arg)   {
    //======================================
    int result =  type_of_next_queued_callback ();
    //
    return TAGGED_INT_FROM_C_INT( result );
}

// ogl-client.api        type:   (None -- not exported to ogl-client.api level.)
// ogl-client-driver.api type:   Void -> Int
//
Val   _lib7_Ogl_get_queued_void_callback   (Task* task,  Val arg)   {
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


// ogl-client.api        type:   (None -- not exported to ogl-client.api level.)
// ogl-client-driver.api type:   Void -> (Int, Bool)
//
Val   _lib7_Ogl_get_queued_bool_callback   (Task *task,  Val arg)   {
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


// ogl-client.api        type:   (None -- not exported to ogl-client.api level.)
// ogl-client-driver.api type:   Void -> (Int, Float)
//
Val   _lib7_Ogl_get_queued_float_callback   (Task* task, Val arg)  {
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


// ogl-client.api        type:   (None -- not exported to ogl-client.api level.)
// ogl-client-driver.api type:   Void -> (Int,     Int,   Int,   Float, Float, Int, Int)
//                                 callback widget button x      y      time modifiers
//
Val   _lib7_Ogl_get_queued_button_press_callback   (Task *task, Val arg)   {
    //==========================================
    Callback_Queue_Entry e = get_next_queued_callback ();

    if (e.callback_type != QUEUED_BUTTON_PRESS_CALLBACK) {
        strcpy( text_buf, "get_queued_button_press_callback: Next callback not Button_Press." );
        moan_and_die();
    }

    Val boxed_x =  make_float64(task, e.entry.button_press.x );
    Val boxed_y =  make_float64(task, e.entry.button_press.y );

    set_slot_in_nascent_heapchunk(  task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 7)                );
    set_slot_in_nascent_heapchunk(  task, 1, TAGGED_INT_FROM_C_INT( e.callback_number              ));
    set_slot_in_nascent_heapchunk(  task, 2, TAGGED_INT_FROM_C_INT( e.entry.button_press.widget_id ));
    set_slot_in_nascent_heapchunk(  task, 3, TAGGED_INT_FROM_C_INT( e.entry.button_press.button    ));
    set_slot_in_nascent_heapchunk(  task, 4, boxed_x                                      );
    set_slot_in_nascent_heapchunk(  task, 5, boxed_y                                      );
    set_slot_in_nascent_heapchunk(  task, 6, TAGGED_INT_FROM_C_INT( e.entry.button_press.time      ));
    set_slot_in_nascent_heapchunk(  task, 7, TAGGED_INT_FROM_C_INT( e.entry.button_press.modifiers ));
    //
    return commit_nascent_heapchunk(task, 7);
}



// ogl-client.api        type:   (None -- not exported to ogl-client.api level.)
// ogl-client-driver.api type:   Void -> (Int,     Int,   Int,    Int, Int)
//                                 callback key    keycode time modifiers
//
Val   _lib7_Ogl_get_queued_key_press_callback   (Task *task,  Val arg)   {
    //=======================================
    //
    Callback_Queue_Entry e = get_next_queued_callback ();

    if (e.callback_type != QUEUED_KEY_PRESS_CALLBACK) {
        strcpy( text_buf, "get_queued_key_press_callback: Next callback not Key_Press." );
        moan_and_die();
    }

    set_slot_in_nascent_heapchunk(  task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 5)                 );
    set_slot_in_nascent_heapchunk(  task, 1, TAGGED_INT_FROM_C_INT( e.callback_number           ));
    set_slot_in_nascent_heapchunk(  task, 2, TAGGED_INT_FROM_C_INT( e.entry.key_press.key       ));
    set_slot_in_nascent_heapchunk(  task, 3, TAGGED_INT_FROM_C_INT( e.entry.key_press.keycode   ));
    set_slot_in_nascent_heapchunk(  task, 4, TAGGED_INT_FROM_C_INT( e.entry.key_press.time      ));
    set_slot_in_nascent_heapchunk(  task, 5, TAGGED_INT_FROM_C_INT( e.entry.key_press.modifiers ));
    //
    return commit_nascent_heapchunk(task, 5);
}



// ogl-client.api        type:   (None -- not exported to ogl-client.api level.)
// ogl-client-driver.api type:   Void -> (Int,     Int,  Float, Float, Int,      Bool)
//                                 callback time  x      y      modifiers is_hint
//
Val   _lib7_Ogl_get_queued_motion_notify_callback   (Task *task,  Val arg)   {
    //===========================================
    //
    Callback_Queue_Entry e =  get_next_queued_callback ();
    //
    if (e.callback_type != QUEUED_MOTION_NOTIFY_CALLBACK) {
        strcpy( text_buf, "get_queued_motion_notify_callback: Next callback not Motion_Notify." );
        moan_and_die();
    }

    Val boxed_x =  make_float64(task, e.entry.motion_notify.x );
    Val boxed_y =  make_float64(task, e.entry.motion_notify.y );

    set_slot_in_nascent_heapchunk(  task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 7)                 );
    set_slot_in_nascent_heapchunk(  task, 1, TAGGED_INT_FROM_C_INT( e.callback_number               ));
    set_slot_in_nascent_heapchunk(  task, 2, TAGGED_INT_FROM_C_INT( e.entry.motion_notify.widget_id ));
    set_slot_in_nascent_heapchunk(  task, 3, TAGGED_INT_FROM_C_INT( e.entry.motion_notify.time      ));
    set_slot_in_nascent_heapchunk(  task, 4, boxed_x                                       );
    set_slot_in_nascent_heapchunk(  task, 5, boxed_y                                       );
    set_slot_in_nascent_heapchunk(  task, 6, TAGGED_INT_FROM_C_INT( e.entry.motion_notify.modifiers ));
    set_slot_in_nascent_heapchunk(  task, 7, e.entry.motion_notify.is_hint ? HEAP_TRUE : HEAP_FALSE );
    //
    return commit_nascent_heapchunk(task, 7);
}



// ogl-client.api        type:   (None -- not exported to ogl-client.api level.)
// ogl-client-driver.api type:   Void -> (Int,     Int,   Int,  Int,   Int,   Int,      Int)
//                                 callback widget count area_x area_y area_wide area_high
//
Val   _lib7_Ogl_get_queued_expose_callback   (Task *task,  Val arg)   {
    //====================================
    //
    Callback_Queue_Entry e = get_next_queued_callback ();
    //
    if (e.callback_type != QUEUED_EXPOSE_CALLBACK) {
        strcpy( text_buf, "get_queued_expose_callback: Next callback not Expose." );
        moan_and_die();
    }

    set_slot_in_nascent_heapchunk(  task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 7)              );
    set_slot_in_nascent_heapchunk(  task, 1, TAGGED_INT_FROM_C_INT( e.callback_number        ));
    set_slot_in_nascent_heapchunk(  task, 2, TAGGED_INT_FROM_C_INT( e.entry.expose.widget_id ));
    set_slot_in_nascent_heapchunk(  task, 3, TAGGED_INT_FROM_C_INT( e.entry.expose.count     ));
    set_slot_in_nascent_heapchunk(  task, 4, TAGGED_INT_FROM_C_INT( e.entry.expose.area_x    ));
    set_slot_in_nascent_heapchunk(  task, 5, TAGGED_INT_FROM_C_INT( e.entry.expose.area_y    ));
    set_slot_in_nascent_heapchunk(  task, 6, TAGGED_INT_FROM_C_INT( e.entry.expose.area_wide ));
    set_slot_in_nascent_heapchunk(  task, 7, TAGGED_INT_FROM_C_INT( e.entry.expose.area_high ));
    //
    return commit_nascent_heapchunk(task, 7);
}



// ogl-client.api        type:   (None -- not exported to ogl-client.api level.)
// ogl-client-driver.api type:   Void -> (Int,     Int,   Int, Int, Int, Int)
//                                 callback widget x    y    wide high
//
Val   _lib7_Ogl_get_queued_configure_callback   (Task *task, Val arg)   {
    //=======================================
    //
    Callback_Queue_Entry e = get_next_queued_callback ();

    if (e.callback_type != QUEUED_CONFIGURE_CALLBACK) {
        strcpy( text_buf, "get_queued_configure_callback: Next callback not Configure." );
        moan_and_die();
    }

    set_slot_in_nascent_heapchunk(  task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 6)                 );
    set_slot_in_nascent_heapchunk(  task, 1, TAGGED_INT_FROM_C_INT( e.callback_number           ));
    set_slot_in_nascent_heapchunk(  task, 2, TAGGED_INT_FROM_C_INT( e.entry.configure.widget_id ));
    set_slot_in_nascent_heapchunk(  task, 3, TAGGED_INT_FROM_C_INT( e.entry.configure.x         ));
    set_slot_in_nascent_heapchunk(  task, 4, TAGGED_INT_FROM_C_INT( e.entry.configure.y         ));
    set_slot_in_nascent_heapchunk(  task, 5, TAGGED_INT_FROM_C_INT( e.entry.configure.wide      ));
    set_slot_in_nascent_heapchunk(  task, 6, TAGGED_INT_FROM_C_INT( e.entry.configure.high      ));
    //
    return commit_nascent_heapchunk(task, 6);
}


#ifdef OLD
Val   _lib7_Ogl_get_widget_allocation   (Task* task,  Val arg)   {		// : Widget -> (Int, Int, Int, Int)
    //===============================
    //
    #if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

	GtkWidget*        w0 =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

	w0 = GTK_WIDGET( w0 );		// Verify user gave us something appropriate.

	int x    =  w0->allocation.x;
	int y    =  w0->allocation.y;
	int wide =  w0->allocation.width;
	int high =  w0->allocation.height;

	set_slot_in_nascent_heapchunk(  task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 4));
	set_slot_in_nascent_heapchunk(  task, 1, TAGGED_INT_FROM_C_INT( x          ));
	set_slot_in_nascent_heapchunk(  task, 2, TAGGED_INT_FROM_C_INT( y          ));
	set_slot_in_nascent_heapchunk(  task, 3, TAGGED_INT_FROM_C_INT( wide       ));
	set_slot_in_nascent_heapchunk(  task, 4, TAGGED_INT_FROM_C_INT( high       ));
	return commit_nascent_heapchunk(task, 4);
    #else
	extern char* no_gtk_support_in_runtime;
	//
	return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
    #endif
}
#endif

#ifdef OLD
Val   _lib7_Ogl_make_dialog   (Task* task,  Val arg)   {	//  Void -> (Int, Int, Int)       # (dialog, vbox, action_area)
    //=====================
    //
    #if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)
	//
	int dialog;
	int vbox;
	int action_area;
	//
	dialog      = find_free_widget_slot ();   widget[dialog]      = gtk_dialog_new();
	vbox        = find_free_widget_slot ();   widget[vbox]        = GTK_DIALOG( widget[dialog] )->vbox;
	action_area = find_free_widget_slot ();   widget[action_area] = GTK_DIALOG( widget[dialog] )->action_area;
	//
	set_slot_in_nascent_heapchunk(  task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 3));
	set_slot_in_nascent_heapchunk(  task, 1, TAGGED_INT_FROM_C_INT( dialog     ));
	set_slot_in_nascent_heapchunk(  task, 2, TAGGED_INT_FROM_C_INT( vbox       ));
	set_slot_in_nascent_heapchunk(  task, 3, TAGGED_INT_FROM_C_INT( action_area));
	return commit_nascent_heapchunk(task, 3);
    #else
	extern char* no_gtk_support_in_runtime;
	//
	return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
    #endif
}
#endif


#ifdef OLD
Val   _lib7_Ogl_unref_object   (Task* task,  Val arg)   {		//  : Int -> Void       # Widget -> Void
    //======================
    //
    #if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)
	//
	GtkWidget*        w0 =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

	g_object_unref( G_OBJECT( w0 ) );

	widget[ get_widget_id( w0 ) ] = 0;

	return HEAP_VOID;
    #else
	extern char*  no_gtk_support_in_runtime;
	//
	return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
    #endif
}
#endif


#ifdef OLD
Val   _lib7_Ogl_run_eventloop_once   (Task *task, Val arg)   {	// : Bool -> Bool       # Bool -> Bool
    //============================
    //
    #if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)
	//
	int block_until_event = TAGGED_INT_TO_C_INT(arg);

	int quit_called = gtk_main_iteration_do( block_until_event );

	return quit_called ? HEAP_TRUE : HEAP_FALSE;
    #else
	extern char* no_gtk_support_in_runtime;
	//
	return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
    #endif
}
#endif





/* Do not edit this or following lines -- they are autogenerated by make-ogl-glue. */
/* Do not edit this or preceding lines -- they are autogenerated by make-ogl-glue. */




// Code by Jeff Prothero: Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

