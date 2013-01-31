// libmythryl-gtk.c
//
//
// This file handles the C side
// of the Mythryl <-> C interface
// layer for the Mythryl in-process
// Gtk binding.  The Mythryl side
// is implemented by
//
//     src/glu/gtk/src/gtk-client-driver-for-library-in-main-process.pkg
//
//
// We get compiled by:
//    src/glu/opengl/c/in-main/Makefile.in


// ########### NOTE! #############
// When resuming work on this project,
// should take a look at
//
//     src/lib/c-glue/gtk/sml-gtk-runtime.c
//
// to see what they did.

#include "../../../../c/mythryl-config.h"

#include <stdio.h>
#include <string.h>

#if (defined(HAVE_GTK_2_0_GTK_GTK_H) || defined(HAVE_GTK_GTK_H))
    
  // Don't do this: #include <gtk-2.0/gtk/gtk.h> because the command `pkg-config --cflags gtk+-2.0`
  // returns the path of the gtk-2.0 directory.  the only reason it worked before is because of a 
  // coincidence that the directory containing the gtk-2.0 was also in the list of include paths
  // which happens on some systems like Linux.  On OpenBSD, it doesn't, so we have to just trust that
  // pkg-config returns the correct path for including gtk/gtk.h.

   #include <gtk/gtk.h>

#else
    #error "No GTK Library Installed"
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"

#include "../../../../c/lib/raise-error.h"

#define MAX_WIDGETS 1024
static GtkWidget* widget[ MAX_WIDGETS ];	// XXX BUGGO FIXME Should expand in size as needed.


static char text_buf[ 1024 ];

static void   moan_and_die   (void)   {
    //        ============
    //
    printf( "FATAL src/c/lib/gtk/mythryl-gtk-library-in-main-process.c: %s  exit(1)ing.\n", text_buf );		fflush(stdout);
    exit(1);
}




// We do not want to call Mythryl
// code directly from the C level;
// that would lead to messy problems.
//
// Consequently when Gtk issues a
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
//     src/glu/gtk/src/gtk-client-driver-for-library-in-main-process.pkg 
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

static int   find_widget_id   (GtkWidget* query_widget)   {
    //       ==============
    for (int i = 1;   i < MAX_WIDGETS;   ++i) {
	//
        if (widget[i] == query_widget)   return i;
    }
    return 0;
}


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


static int   find_free_callback_id   ()   {
    //       =====================
    static int next_callback_id = 1;

    return next_callback_id++;
}


// These are operationally identical, but for debugging
// it is convenient to keep them separate:
//
static void run_clicked_callback(     GtkWidget* widget, gpointer user_data) {  queue_up_void_callback( (int)user_data ); }
static void run_pressed_callback(     GtkWidget* widget, gpointer user_data) {  queue_up_void_callback( (int)user_data ); }
static void run_enter_callback  (     GtkWidget* widget, gpointer user_data) {  queue_up_void_callback( (int)user_data ); }
static void run_leave_callback  (     GtkWidget* widget, gpointer user_data) {  queue_up_void_callback( (int)user_data ); }
static void run_release_callback(     GtkWidget* widget, gpointer user_data) {  queue_up_void_callback( (int)user_data ); }
static void run_activate_callback(    GtkWidget* widget, gpointer user_data) {  queue_up_void_callback( (int)user_data ); }

static gboolean run_destroy_callback( GtkObject* object, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_realize_callback( GtkWidget* widget, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }

static gboolean   run_button_press_event_callback   (GtkWidget* widget,  GdkEventButton* event,  gpointer user_data)   {
    //            ===============================
    int type = 0;

    switch (event->type) {
    case GDK_BUTTON_PRESS:  type = 1; break;
    case GDK_2BUTTON_PRESS: type = 2; break;
    case GDK_3BUTTON_PRESS: type = 3; break;
    default:
	sprintf (text_buf, "run_button_press_event: type value '%d' is not supported.\n", event->type); moan_and_die();
    }

    Callback_Queue_Entry e;

    e.callback_type    =  QUEUED_BUTTON_PRESS_CALLBACK;
    e.callback_number  =  (int) user_data;

    e.entry.button_press.widget_id = get_widget_id( (GtkWidget*) event->window );
    e.entry.button_press.button    =                             event->button;
    e.entry.button_press.x         =                             event->x;
    e.entry.button_press.y         =                             event->y;
    e.entry.button_press.time      =                             event->time;
    e.entry.button_press.modifiers =                             event->state;

    queue_up_callback(e);

    return TRUE;

}


static gboolean   run_motion_notify_event_callback   (GtkWidget* widget,  GdkEventMotion* event,  gpointer user_data)   {
    //            ================================

    Callback_Queue_Entry e;

    e.callback_type    =  QUEUED_MOTION_NOTIFY_CALLBACK;
    e.callback_number  =  (int) user_data;


    e.entry.motion_notify.widget_id = get_widget_id ((GtkWidget*) (event->window));

    e.entry.motion_notify.time      = event->time;
    e.entry.motion_notify.x         = event->x;
    e.entry.motion_notify.y         = event->y;
    e.entry.motion_notify.modifiers = event->state; 
    e.entry.motion_notify.is_hint   = event->is_hint;

    queue_up_callback(e);

    return TRUE;
}


static gboolean   run_expose_event_callback   (GtkWidget* w,  GdkEventExpose* event,  gpointer user_data)   {
    //            =========================

    Callback_Queue_Entry e;

    e.callback_type    =  QUEUED_EXPOSE_CALLBACK;
    e.callback_number  =  (int) user_data;


    e.entry.expose.widget_id =  find_widget_id( w );

    e.entry.expose.count     =  event->count;
    e.entry.expose.area_x    =  event->area.x;
    e.entry.expose.area_y    =  event->area.y;
    e.entry.expose.area_wide =  event->area.width;
    e.entry.expose.area_high =  event->area.height;

    queue_up_callback(e);

    return TRUE;
}


static gboolean   run_configure_event_callback   ( GtkWidget* widget,  GdkEventConfigure* event,  gpointer user_data)   {
    //            ============================

    Callback_Queue_Entry e;

    e.callback_type    =  QUEUED_CONFIGURE_CALLBACK;
    e.callback_number  =  (int) user_data;


    e.entry.configure.widget_id =  find_widget_id( widget );

    e.entry.configure.x         =  event->x;
    e.entry.configure.y         =  event->y;
    e.entry.configure.wide      =  event->width;
    e.entry.configure.high      =  event->height;


    queue_up_callback(e);

    return TRUE;
}


static gboolean   run_key_press_event_callback   (GtkWidget* widget,  GdkEventKey* event,  gpointer user_data)   {
    //            ============================

    Callback_Queue_Entry e;

    e.callback_type    =  QUEUED_KEY_PRESS_CALLBACK;
    e.callback_number  =  (int) user_data;


    e.entry.key_press.key       =  event->keyval;
    e.entry.key_press.keycode   =  event->hardware_keycode;
    e.entry.key_press.time      =  event->time;
    e.entry.key_press.modifiers =  event->state;


    queue_up_callback(e);

    return TRUE;
}

static gboolean run_button_release_event_callback	(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_scroll_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_delete_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_key_release_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_enter_notify_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_leave_notify_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_focus_in_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_focus_out_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_map_event_callback			(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_unmap_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_property_notify_event_callback	(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_selection_clear_event_callback	(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_selection_request_event_callback	(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_selection_notify_event_callback	(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_proximity_in_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_proximity_out_event_callback	(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_client_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_no_expose_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }
static gboolean run_window_state_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  queue_up_void_callback( (int)user_data );  return TRUE; }

// This one returns a boolean value:
//
static void   run_toggled_callback   (GtkToggleButton* widget,  gpointer user_data)   {
    //        ====================
    //
    gboolean is_set =  gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );
    //
    queue_up_bool_callback( (int)user_data, is_set );
}


// This one returns a double value:
//
static void   run_value_changed_callback   (GtkAdjustment* adjustment,  gpointer user_data)   {
    //        ==========================
    //
    double value = gtk_adjustment_get_value( GTK_ADJUSTMENT(adjustment) );
    //
    queue_up_float_callback( (int)user_data, value );
}



Val   _lib7_Gtk_gtk_init   (Task* task,  Val arg)   {	// : Void -> Void
    //==================

    #if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)
	//
	extern char	**commandline_args_without_argv0_or_runtime_args__global;

	int argc = 0;
	while (commandline_args_without_argv0_or_runtime_args__global[argc]) ++argc;    

	if (!gtk_init_check( &argc, &commandline_args_without_argv0_or_runtime_args__global )) {
	    //
	    return RAISE_ERROR__MAY_HEAPCLEAN(task, "gtk_init: failed to initialize GUI support", NULL);
	}

	// XXX BUGGO FIXME: gtk_init_check installs gtk default signal handlers,
	//		    which most likely screws up Mythryl's own signal
	//		    handling no end.  At some point should put some work
	//		    into keeping both happy.
	//
	return HEAP_VOID;
    #else
	extern char*  no_gtk_support_in_runtime;
	//
	return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
    #endif
}

static int   int_to_arrow_direction   (int arrow_direction)   {
    //       ======================
    //
    switch (arrow_direction) {
        //
    case 0:  return GTK_ARROW_UP;    
    case 1:  return GTK_ARROW_DOWN;    
    case 2:  return GTK_ARROW_LEFT;    
    case 3:  return GTK_ARROW_RIGHT;
    default:
        sprintf( text_buf, "int_to_arrow_direction: bad arg %d.", arrow_direction );
        moan_and_die();
    }
    //
    return 0;	// Just to quiet compilers.
}

static int   int_to_shadow_style   (int shadow_style)   {
    //       ===================
    //
    switch (shadow_style) {
        //
    case 0:  return GTK_SHADOW_NONE;
    case 1:  return GTK_SHADOW_IN;
    case 2:  return GTK_SHADOW_OUT;
    case 3:  return GTK_SHADOW_ETCHED_IN;
    case 4:  return GTK_SHADOW_ETCHED_OUT;
    default:
        sprintf( text_buf, "int_to_shadow_style: bad arg %d.", shadow_style );
        moan_and_die();
    }
    return 0;			// Just to quiet compilers.
}

static int   int_to_policy   (int policy)   {
    //       =============
    switch (policy) {
	//
    case 0:  return GTK_POLICY_AUTOMATIC;
    case 1:  return GTK_POLICY_ALWAYS;
    default:
        sprintf( text_buf, "int_to_policy: bad arg %d.", policy );
        moan_and_die();
    }
    return 0;			// Just to quiet compilers.
}

static int   int_to_event_mask   (int i1)    {
    //       =================
    //
    int mask =  0;

    if (i1 & (1 <<  0))    mask |= GDK_EXPOSURE_MASK;
    if (i1 & (1 <<  1))    mask |= GDK_POINTER_MOTION_MASK;
    if (i1 & (1 <<  2))    mask |= GDK_POINTER_MOTION_HINT_MASK;
    if (i1 & (1 <<  3))    mask |= GDK_BUTTON_MOTION_MASK;
    if (i1 & (1 <<  4))    mask |= GDK_BUTTON1_MOTION_MASK;
    if (i1 & (1 <<  5))    mask |= GDK_BUTTON2_MOTION_MASK;
    if (i1 & (1 <<  6))    mask |= GDK_BUTTON3_MOTION_MASK;
    if (i1 & (1 <<  7))    mask |= GDK_BUTTON_PRESS_MASK;
    if (i1 & (1 <<  8))    mask |= GDK_BUTTON_RELEASE_MASK;
    if (i1 & (1 <<  9))    mask |= GDK_KEY_PRESS_MASK;
    if (i1 & (1 << 10))    mask |= GDK_KEY_RELEASE_MASK;
    if (i1 & (1 << 11))    mask |= GDK_ENTER_NOTIFY_MASK;
    if (i1 & (1 << 12))    mask |= GDK_LEAVE_NOTIFY_MASK;
    if (i1 & (1 << 13))    mask |= GDK_FOCUS_CHANGE_MASK;
    if (i1 & (1 << 14))    mask |= GDK_STRUCTURE_MASK;
    if (i1 & (1 << 15))    mask |= GDK_PROPERTY_CHANGE_MASK;
    if (i1 & (1 << 16))    mask |= GDK_PROXIMITY_IN_MASK;
    if (i1 & (1 << 17))    mask |= GDK_PROXIMITY_OUT_MASK;
    //
    return mask;
}

static int   int_to_justification   (int i1)   {
    //       ====================
    //
    switch (i1) {
	//
    case 0:	i1 = GTK_JUSTIFY_LEFT;	break;
    case 1:	i1 = GTK_JUSTIFY_RIGHT;	break;
    case 2:	i1 = GTK_JUSTIFY_CENTER;	break;
    case 3:	i1 = GTK_JUSTIFY_FILL;	break;
    default:
        sprintf( text_buf, "do_set_label_justification: bad arg %d.", i1 );
        moan_and_die();
    }

    return i1;
}

static int    int_to_position   (int i1)   {
    //        ===============
    //
    switch (i1) {
	//
    case 0:	i1 = GTK_POS_LEFT;	break;
    case 1:	i1 = GTK_POS_RIGHT;	break;
    case 2:	i1 = GTK_POS_TOP;	break;
    case 3:	i1 = GTK_POS_BOTTOM;	break;
    default:
        sprintf( text_buf, "do_set_scale_value_position: bad position arg %d.", i1 );
        moan_and_die();
    }
    //
    return i1;
}

static int   int_to_metric   (int i1)   {
    //       ============= 
    //
    switch (i1) {
	//
    case 0:	i1 = GTK_PIXELS;		break;
    case 1:	i1 = GTK_INCHES;		break;
    case 2:	i1 = GTK_CENTIMETERS;	break;
    default:
      sprintf( text_buf, "do_set_ruler_metric: bad arg %d.", i1 );
      moan_and_die();
    }
    //
    return i1;
}

static int   int_to_range_update_policy   (int i1)   {
    //       ==========================
    //
    switch (i1) {
	//
    case 0: i1 = GTK_UPDATE_CONTINUOUS;	break;
    case 1: i1 = GTK_UPDATE_DISCONTINUOUS;	break;
    case 2: i1 = GTK_UPDATE_DELAYED;		break;
    default:
        sprintf( text_buf, "do_set_range_update_policy: bad policy arg %d.", i1 );
        moan_and_die();
    }
    //
    return i1;
}


// gtk-client.api        type:   (None -- not exported to gtk-client.api level.)
// gtk-client-driver.api type:   Void -> Bool
//
Val   _lib7_Gtk_callback_queue_is_empty   (Task* task,  Val arg)   {
    //=================================
    //
    return  callback_queue_is_empty()
              ?  HEAP_TRUE
              : HEAP_FALSE;
}

// gtk-client.api        type:   (None -- not exported to gtk-client.api level.)
// gtk-client-driver.api type:   Void -> Int
//
Val   _lib7_Gtk_number_of_queued_callbacks   (Task* task,  Val arg)   {
    //====================================
    //
    int result =  number_of_queued_callbacks ();
    //
    return TAGGED_INT_FROM_C_INT( result );
}

// gtk-client.api        type:   (None -- not exported to gtk-client.api level.)
// gtk-client-driver.api type:   Void -> Int
//
Val   _lib7_Gtk_type_of_next_queued_callback   (Task* task,  Val arg)   {
    //======================================
    int result =  type_of_next_queued_callback ();
    //
    return TAGGED_INT_FROM_C_INT( result );
}

// gtk-client.api        type:   (None -- not exported to gtk-client.api level.)
// gtk-client-driver.api type:   Void -> Int
//
Val   _lib7_Gtk_get_queued_void_callback   (Task* task,  Val arg)   {
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


// gtk-client.api        type:   (None -- not exported to gtk-client.api level.)
// gtk-client-driver.api type:   Void -> (Int, Bool)
//
Val   _lib7_Gtk_get_queued_bool_callback   (Task *task,  Val arg)   {
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


// gtk-client.api        type:   (None -- not exported to gtk-client.api level.)
// gtk-client-driver.api type:   Void -> (Int, Float)
//
Val   _lib7_Gtk_get_queued_float_callback   (Task* task, Val arg)  {
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


// gtk-client.api        type:   (None -- not exported to gtk-client.api level.)
// gtk-client-driver.api type:   Void -> (Int,     Int,   Int,   Float, Float, Int, Int)
//                                 callback widget button x      y      time modifiers
//
Val   _lib7_Gtk_get_queued_button_press_callback   (Task *task, Val arg)   {
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



// gtk-client.api        type:   (None -- not exported to gtk-client.api level.)
// gtk-client-driver.api type:   Void -> (Int,     Int,   Int,    Int, Int)
//                                 callback key    keycode time modifiers
//
Val   _lib7_Gtk_get_queued_key_press_callback   (Task *task,  Val arg)   {
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



// gtk-client.api        type:   (None -- not exported to gtk-client.api level.)
// gtk-client-driver.api type:   Void -> (Int,     Int,  Float, Float, Int,      Bool)
//                                 callback time  x      y      modifiers is_hint
//
Val   _lib7_Gtk_get_queued_motion_notify_callback   (Task *task,  Val arg)   {
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



// gtk-client.api        type:   (None -- not exported to gtk-client.api level.)
// gtk-client-driver.api type:   Void -> (Int,     Int,   Int,  Int,   Int,   Int,      Int)
//                                 callback widget count area_x area_y area_wide area_high
//
Val   _lib7_Gtk_get_queued_expose_callback   (Task *task,  Val arg)   {
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



// gtk-client.api        type:   (None -- not exported to gtk-client.api level.)
// gtk-client-driver.api type:   Void -> (Int,     Int,   Int, Int, Int, Int)
//                                 callback widget x    y    wide high
//
Val   _lib7_Gtk_get_queued_configure_callback   (Task *task, Val arg)   {
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



Val   _lib7_Gtk_get_widget_allocation   (Task* task,  Val arg)   {		// : Widget -> (Int, Int, Int, Int)
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


Val   _lib7_Gtk_get_window_pointer   (Task* task,  Val arg)   {		//  : Widget -> (Int, Int, Int)       # (x, y, modifiers)
    //============================
    //
    #if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)
	//
	GtkWidget*       w0 =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

	w0 = GTK_WIDGET( w0 );		// Verify user gave us something appropriate.

	{
	    int             x;
	    int             y;
	    GdkModifierType modifiers;

    /*      GdkWindow* result_window = */  gdk_window_get_pointer (GDK_WINDOW(w0), &x, &y, &modifiers); 

	    set_slot_in_nascent_heapchunk(  task, 0, MAKE_TAGWORD(PAIRS_AND_RECORDS_BTAG, 3));
	    set_slot_in_nascent_heapchunk(  task, 1, TAGGED_INT_FROM_C_INT( x          ));
	    set_slot_in_nascent_heapchunk(  task, 2, TAGGED_INT_FROM_C_INT( y          ));
	    set_slot_in_nascent_heapchunk(  task, 3, TAGGED_INT_FROM_C_INT( modifiers  ));
	    return commit_nascent_heapchunk(task, 3);
	}
    #else
	extern char* no_gtk_support_in_runtime;
	//
	return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime,NULL);
    #endif
}



Val   _lib7_Gtk_make_dialog   (Task* task,  Val arg)   {	//  Void -> (Int, Int, Int)       # (dialog, vbox, action_area)
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



Val   _lib7_Gtk_unref_object   (Task* task,  Val arg)   {		//  : Int -> Void       # Widget -> Void
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



Val   _lib7_Gtk_run_eventloop_once   (Task *task, Val arg)   {	// : Bool -> Bool       # Bool -> Bool
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




/////////////////////////////////////////////////////////////////////////////////////
// The following stuff gets built from paragraphs in
//     src/glu/gtk/etc/library-glue.plan
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
//      + build_fun_body_for__'libmythryl_xxx_c'			# Optionally invokes new_widget_custom_body_plain_fun_mainprocess
//                                                              # or                     widget_custom_body_plain_fun_mainprocess,
//                                                              # from src/glu/gtk/sh/make-gtk-glue
//      + build_fun_trailer_for_'libmythryl_xxx_c'
// 
// Paragraphs like
//     build-a: callback-fn
//     fn-name:
//     fn-type:
//     lowtype:
// drive the code-build path
//   mlb::BUILD_A ("callback-fn", build_callback_function)			# In src/glu/gtk/sh/make-gtk-glue
//   ->  build_callback_function						# In src/glu/gtk/sh/make-gtk-glue
//       ->  build_set_callback_fn_for_'libmythryl_xxx_c'			# In src/glu/gtk/sh/make-gtk-glue
//           ->  r.to_libmythryl_xxx_c_funs					# In src/lib/make-library-glue/make-library-glue.pkg
//
/* Do not edit this or following lines -- they are autobuilt. */
/* do__make_window
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_window   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_window_new( GTK_WINDOW_TOPLEVEL );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_label
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
static Val   do__make_label   (Task* task, Val arg)
{

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_label_new( /*label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_status_bar_context_id
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Int
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Int
 */
static Val   do__make_status_bar_context_id   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    int result = gtk_statusbar_get_context_id( GTK_STATUSBAR(/*status_bar*/w0), /*description*/s1);

    return TAGGED_INT_FROM_C_INT(result);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_menu
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_menu   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_menu_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_option_menu
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_option_menu   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_option_menu_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_menu_bar
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_menu_bar   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_menu_bar_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_combo_box
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_combo_box   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_combo_box_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_text_combo_box
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_text_combo_box   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_combo_box_new_text ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_frame
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
static Val   do__make_frame   (Task* task, Val arg)
{

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_frame_new (*/*label*/s0 ? /*label*/s0 : NULL);

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_button
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_button   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_button_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_button_with_label
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
static Val   do__make_button_with_label   (Task* task, Val arg)
{

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_button_new_with_label( /*label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_button_with_mnemonic
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
static Val   do__make_button_with_mnemonic   (Task* task, Val arg)
{

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_button_new_with_mnemonic( /*mnemonic_label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_toggle_button
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_toggle_button   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_toggle_button_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_toggle_button_with_label
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
static Val   do__make_toggle_button_with_label   (Task* task, Val arg)
{

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_toggle_button_new_with_label( /*label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_toggle_button_with_mnemonic
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
static Val   do__make_toggle_button_with_mnemonic   (Task* task, Val arg)
{

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_toggle_button_new_with_mnemonic( /*mnemonic_label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_check_button
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_check_button   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_check_button_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_check_button_with_label
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
static Val   do__make_check_button_with_label   (Task* task, Val arg)
{

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_check_button_new_with_label ( /*label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_check_button_with_mnemonic
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
static Val   do__make_check_button_with_mnemonic   (Task* task, Val arg)
{

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_check_button_new_with_mnemonic( /*mnemonic_label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_menu_item
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_menu_item   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_menu_item_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_menu_item_with_label
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
static Val   do__make_menu_item_with_label   (Task* task, Val arg)
{

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_menu_item_new_with_label( /*label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_menu_item_with_mnemonic
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
static Val   do__make_menu_item_with_mnemonic   (Task* task, Val arg)
{

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_menu_item_new_with_mnemonic( /*mnemonic_label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_first_radio_button
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_first_radio_button   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_radio_button_new (NULL);

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_next_radio_button
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*new Widget*)
 */
static Val   do__make_next_radio_button   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON(/*sib*/w0));

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_first_radio_button_with_label
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
static Val   do__make_first_radio_button_with_label   (Task* task, Val arg)
{

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_radio_button_new_with_label(NULL,/*label*/s0);

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_next_radio_button_with_label
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Int (*new Widget*)
 */
static Val   do__make_next_radio_button_with_label   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_radio_button_new_with_label_from_widget ( GTK_RADIO_BUTTON(/*sib*/w0), /*label*/s1 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_first_radio_button_with_mnemonic
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
static Val   do__make_first_radio_button_with_mnemonic   (Task* task, Val arg)
{

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_radio_button_new_with_mnemonic(NULL,/*label*/s0);

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_next_radio_button_with_mnemonic
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Int (*new Widget*)
 */
static Val   do__make_next_radio_button_with_mnemonic   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_radio_button_new_with_mnemonic_from_widget ( GTK_RADIO_BUTTON(/*sib*/w0), /*label*/s1 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_arrow
 *
 * gtk-client.api        type:   (Session, Arrow_Direction, Shadow_Style) -> Widget
 * gtk-client-driver.api type:   (Session, Int, Int) -> Int (*new Widget*)
 */
static Val   do__make_arrow   (Task* task, Val arg)
{

    int               i0 =                            GET_TUPLE_SLOT_AS_INT( arg, 1);
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_arrow_new( int_to_arrow_direction(/*arrow_direction_to_int arrow_direction*/i0), int_to_shadow_style(/*shadow_style_to_int shadow_style*/i1) );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_arrow
 *
 * gtk-client.api        type:   (Session, Widget, Arrow_Direction, Shadow_Style) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Void
 */
static Val   do__set_arrow   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    gtk_arrow_set( GTK_ARROW(/*arrow*/w0), int_to_arrow_direction(/*arrow_direction_to_int arrow_direction*/i1), int_to_shadow_style(/*shadow_style_to_int shadow_style*/i2) );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_horizontal_box
 *
 * gtk-client.api        type:   (Session, Bool, Int)   ->   Widget
 * gtk-client-driver.api type:   (Session, Bool, Int) -> Int (*new Widget*)
 */
static Val   do__make_horizontal_box   (Task* task, Val arg)
{

    int               b0 =                            GET_TUPLE_SLOT_AS_VAL( arg, 1) == HEAP_TRUE;
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_hbox_new ( /*homogeneous*/b0, /*spacing*/i1 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_vertical_box
 *
 * gtk-client.api        type:   (Session, Bool, Int)   ->   Widget
 * gtk-client-driver.api type:   (Session, Bool, Int) -> Int (*new Widget*)
 */
static Val   do__make_vertical_box   (Task* task, Val arg)
{

    int               b0 =                            GET_TUPLE_SLOT_AS_VAL( arg, 1) == HEAP_TRUE;
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_vbox_new ( /*homogeneous*/b0, /*spacing*/i1 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_horizontal_button_box
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_horizontal_button_box   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_hbutton_box_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_vertical_button_box
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_vertical_button_box   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_vbutton_box_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_table
 *
 * gtk-client.api        type:    { session: Session,   rows: Int,   cols: Int,   homogeneous: Bool }   ->   Widget
 * gtk-client-driver.api type:   (Session, Int, Int, Bool) -> Int (*new Widget*)
 */
static Val   do__make_table   (Task* task, Val arg)
{

    int               i0 =                            GET_TUPLE_SLOT_AS_INT( arg, 1);
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               b2 =                            GET_TUPLE_SLOT_AS_VAL( arg, 3) == HEAP_TRUE;

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_table_new ( /*rows*/i0, /*cols*/i1, /*homogeneous*/b2 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_event_box
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_event_box   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_event_box_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_image_from_file
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
static Val   do__make_image_from_file   (Task* task, Val arg)
{

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_image_new_from_file( /*filename*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_horizontal_separator
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_horizontal_separator   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_hseparator_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_vertical_separator
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_vertical_separator   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_vseparator_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_layout_container
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_layout_container   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_layout_new (NULL, NULL);

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__layout_put
 *
 * gtk-client.api        type:    { session: Session,  layout: Widget,  kid: Widget,  x: Int,  y: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int, Int) -> Void
 */
static Val   do__layout_put   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);

    gtk_layout_put( GTK_LAYOUT(/*layout*/w0), GTK_WIDGET(/*kid*/w1), /*x*/i2, /*y*/i3);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__layout_move
 *
 * gtk-client.api        type:    { session: Session, layout: Widget,  kid: Widget,  x: Int,  y: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int, Int) -> Void
 */
static Val   do__layout_move   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);

    gtk_layout_move( GTK_LAYOUT(/*layout*/w0), GTK_WIDGET(/*kid*/w1), /*x*/i2, /*y*/i3);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_fixed_container
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_fixed_container   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_fixed_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__fixed_put
 *
 * gtk-client.api        type:    { session: Session, layout: Widget,  kid: Widget,  x: Int,  y: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int, Int) -> Void
 */
static Val   do__fixed_put   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);

    gtk_fixed_put(   GTK_FIXED(/*layout*/w0), GTK_WIDGET(/*kid*/w1), /*x*/i2, /*y*/i3);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__fixed_move
 *
 * gtk-client.api        type:    { session: Session, layout: Widget,  kid: Widget,  x: Int,  y: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int, Int) -> Void
 */
static Val   do__fixed_move   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);

    gtk_fixed_move(  GTK_FIXED(/*layout*/w0), GTK_WIDGET(/*kid*/w1), /*x*/i2, /*y*/i3);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_adjustment
 *
 * gtk-client.api        type:    { session: Session,   value: Float,   lower: Float,   upper: Float,   step_increment: Float,   page_increment: Float,   page_size: Float }   ->   Widget
 * gtk-client-driver.api type:   (Session, Float, Float, Float, Float, Float, Float) -> Int (*new Widget*)
 */
static Val   do__make_adjustment   (Task* task, Val arg)
{

    double            f0 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 1)));
    double            f1 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 2)));
    double            f2 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 3)));
    double            f3 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 4)));
    double            f4 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 5)));
    double            f5 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 6)));

    int slot = find_free_widget_slot ();

    widget[slot] = (GtkWidget*) gtk_adjustment_new ( /*value*/f0, /*lower*/f1, /*upper*/f2, /*step_increment*/f3, /*page_increment*/f4, /*page_size*/f5 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_viewport
 *
 * gtk-client.api        type:    { session: Session, horizontal_adjustment: Null_Or(Widget), vertical_adjustment: Null_Or(Widget) } -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*)) -> Int (*new Widget*)
 */
static Val   do__make_viewport   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_viewport_new( GTK_ADJUSTMENT(/*null_or_widget_to_int horizontal_adjustment*/w0), GTK_ADJUSTMENT(/*null_or_widget_to_int vertical_adjustment*/w1) );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_scrolled_window
 *
 * gtk-client.api        type:    { session: Session, horizontal_adjustment: Null_Or(Widget), vertical_adjustment: Null_Or(Widget) } -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*)) -> Int (*new Widget*)
 */
static Val   do__make_scrolled_window   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_scrolled_window_new( GTK_ADJUSTMENT(/*null_or_widget_to_int horizontal_adjustment*/w0), GTK_ADJUSTMENT(/*null_or_widget_to_int vertical_adjustment*/w1) );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_horizontal_ruler
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_horizontal_ruler   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_hruler_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_vertical_ruler
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_vertical_ruler   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_vruler_new ();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_vertical_scrollbar
 *
 * gtk-client.api        type:   (Session, Null_Or(Widget)) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*new Widget*)
 */
static Val   do__make_vertical_scrollbar   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_vscrollbar_new( GTK_ADJUSTMENT(/*null_or_widget_to_int adjustment*/w0) );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_horizontal_scrollbar
 *
 * gtk-client.api        type:   (Session, Null_Or(Widget)) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*new Widget*)
 */
static Val   do__make_horizontal_scrollbar   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_hscrollbar_new( GTK_ADJUSTMENT(/*null_or_widget_to_int adjustment*/w0) );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_vertical_scale
 *
 * gtk-client.api        type:   (Session, Null_Or(Widget)) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*new Widget*)
 */
static Val   do__make_vertical_scale   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_vscale_new( GTK_ADJUSTMENT(/*null_or_widget_to_int adjustment*/w0) );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_horizontal_scale
 *
 * gtk-client.api        type:   (Session, Null_Or(Widget)) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*new Widget*)
 */
static Val   do__make_horizontal_scale   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_hscale_new( GTK_ADJUSTMENT(/*null_or_widget_to_int adjustment*/w0) );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_vertical_scale_with_range
 *
 * gtk-client.api        type:    { session: Session, min: Float, max: Float, step: Float } -> Widget
 * gtk-client-driver.api type:   (Session, Float, Float, Float) -> Int (*new Widget*)
 */
static Val   do__make_vertical_scale_with_range   (Task* task, Val arg)
{

    double            f0 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 1)));
    double            f1 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 2)));
    double            f2 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 3)));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_vscale_new_with_range( /*min*/f0, /*max*/f1, /*step*/f2 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_horizontal_scale_with_range
 *
 * gtk-client.api        type:    { session: Session, min: Float, max: Float, step: Float } -> Widget
 * gtk-client-driver.api type:   (Session, Float, Float, Float) -> Int (*new Widget*)
 */
static Val   do__make_horizontal_scale_with_range   (Task* task, Val arg)
{

    double            f0 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 1)));
    double            f1 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 2)));
    double            f2 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 3)));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_hscale_new_with_range( /*min*/f0, /*max*/f1, /*step*/f2 );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_drawing_area
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_drawing_area   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_drawing_area_new();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_pixmap
 *
 * gtk-client.api        type:    { session: Session, window: Widget, wide: Int, high: Int } -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Int (*new Widget*)
 */
static Val   do__make_pixmap   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    int slot = find_free_widget_slot ();

    widget[slot] = (GtkWidget*) gdk_pixmap_new( GDK_DRAWABLE(/*window*/w0), /*wide*/i1, /*high*/i2, -1);

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__make_status_bar
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
static Val   do__make_status_bar   (Task* task, Val arg)
{


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_statusbar_new();

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__push_text_on_status_bar
 *
 * gtk-client.api        type:   (Session, Widget, Int, String) -> Int
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, String) -> Int
 */
static Val   do__push_text_on_status_bar   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    char*             s2 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 3));

    int result = gtk_statusbar_push( GTK_STATUSBAR(/*status_bar*/w0), /*context*/i1, /*text*/s2);

    return TAGGED_INT_FROM_C_INT(result);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__pop_text_off_status_bar
 *
 * gtk-client.api        type:   (Session, Widget, Int) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
static Val   do__pop_text_off_status_bar   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_statusbar_pop(GTK_STATUSBAR(/*status_bar*/w0), /*context*/i1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__remove_text_from_status_bar
 *
 * gtk-client.api        type:    { session: Session,   status_bar: Widget,   context: Int,   message: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Void
 */
static Val   do__remove_text_from_status_bar   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    gtk_statusbar_remove( GTK_STATUSBAR(/*status_bar*/w0), /*context*/i1, /*message*/i2);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__pack_box
 *
 * gtk-client.api        type:    { session: Session,   box: Widget,   kid: Widget,   pack: Pack_From,   expand: Bool,   fill: Bool,   padding: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int, Bool, Bool, Int) -> Void
 */
static Val   do__pack_box   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               b3 =                            GET_TUPLE_SLOT_AS_VAL( arg, 4) == HEAP_TRUE;
    int               b4 =                            GET_TUPLE_SLOT_AS_VAL( arg, 5) == HEAP_TRUE;
    int               i5 =                            GET_TUPLE_SLOT_AS_INT( arg, 6);

    if (!/*pack_to_int pack*/i2)  gtk_box_pack_start(   GTK_BOX(/*box*/w0), GTK_WIDGET(/*kid*/w1), /*expand*/b3, /*fill*/b4, /*padding*/i5 ); else gtk_box_pack_end( GTK_BOX(/*box*/w0), GTK_WIDGET(/*kid*/w1), /*expand*/b3, /*fill*/b4, /*padding*/i5 );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__menu_shell_append
 *
 * gtk-client.api        type:    { session: Session,   menu: Widget,   kid: Widget } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*)) -> Void
 */
static Val   do__menu_shell_append   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];

    gtk_menu_shell_append( GTK_MENU_SHELL(/*menu*/w0), GTK_WIDGET(/*kid*/w1));

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__menu_bar_append
 *
 * gtk-client.api        type:    { session: Session,   menu: Widget,   kid: Widget } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*)) -> Void
 */
static Val   do__menu_bar_append   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];

    gtk_menu_bar_append( GTK_MENU_SHELL(/*menu*/w0), GTK_WIDGET(/*kid*/w1));

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__append_text_to_combo_box
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Void
 */
static Val   do__append_text_to_combo_box   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    gtk_combo_box_append_text( GTK_COMBO_BOX(/*combo_box*/w0), /*text*/s1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_option_menu_menu
 *
 * gtk-client.api        type:    { session: Session,   option_menu: Widget,   menu: Widget } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*)) -> Void
 */
static Val   do__set_option_menu_menu   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];

    gtk_option_menu_set_menu( GTK_OPTION_MENU(/*option_menu*/w0), GTK_WIDGET(/*menu*/w1) );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_text_tooltip_on_widget
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Void
 */
static Val   do__set_text_tooltip_on_widget   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    gtk_widget_set_tooltip_text( GTK_WIDGET(/*widget*/w0), /*text*/s1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_ruler_metric
 *
 * gtk-client.api        type:   (Session, Widget, Metric) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
static Val   do__set_ruler_metric   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_ruler_set_metric( GTK_RULER(/*ruler*/w0), int_to_metric(/*metric_to_int metric*/i1));

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_ruler_range
 *
 * gtk-client.api        type:    { session: Session,   ruler: Widget,   lower: Float,   upper: Float,   position: Float,   max_size: Float } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Float, Float, Float, Float) -> Void
 */
static Val   do__set_ruler_range   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    double            f1 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 2)));
    double            f2 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 3)));
    double            f3 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 4)));
    double            f4 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 5)));

    gtk_ruler_set_range( GTK_RULER(/*ruler*/w0), /*lower*/f1, /*upper*/f2, /*position*/f3, /*max_size*/f4);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_scrollbar_policy
 *
 * gtk-client.api        type:    { session: Session,   window: Widget,   horizontal_scrollbar: Scrollbar_Policy,   vertical_scrollbar: Scrollbar_Policy } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Void
 */
static Val   do__set_scrollbar_policy   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(/*window*/w0), int_to_policy(/*scrollbar_policy_to_int horizontal_scrollbar*/i1), int_to_policy(/*scrollbar_policy_to_int vertical_scrollbar*/i2) );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__draw_rectangle
 *
 * gtk-client.api        type:    { session: Session,   drawable: Widget,   gcontext: Widget,   filled:	Bool,   x: Int,   y: Int,   wide: Int,   high: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Bool, Int, Int, Int, Int) -> Void
 */
static Val   do__draw_rectangle   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               b2 =                            GET_TUPLE_SLOT_AS_VAL( arg, 3) == HEAP_TRUE;
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);
    int               i4 =                            GET_TUPLE_SLOT_AS_INT( arg, 5);
    int               i5 =                            GET_TUPLE_SLOT_AS_INT( arg, 6);
    int               i6 =                            GET_TUPLE_SLOT_AS_INT( arg, 7);

    gdk_draw_rectangle(   GDK_DRAWABLE(/*drawable*/w0), GDK_GC(/*gcontext*/w1), /*filled*/b2, /*x*/i3, /*y*/i4, /*wide*/i5, /*high*/i6);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__draw_drawable
 *
 * gtk-client.api        type:    { session: Session,   drawable: Widget,   gcontext: Widget,   from: Widget,   from_x:	Int,   from_y: Int,   to_x: Int,   to_y: Int,   wide: Int,   high: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int(*Widget*), Int, Int, Int, Int, Int, Int) -> Void
 */
static Val   do__draw_drawable   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    GtkWidget*        w2  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 3) ];
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);
    int               i4 =                            GET_TUPLE_SLOT_AS_INT( arg, 5);
    int               i5 =                            GET_TUPLE_SLOT_AS_INT( arg, 6);
    int               i6 =                            GET_TUPLE_SLOT_AS_INT( arg, 7);
    int               i7 =                            GET_TUPLE_SLOT_AS_INT( arg, 8);
    int               i8 =                            GET_TUPLE_SLOT_AS_INT( arg, 9);

    gdk_draw_drawable(   GDK_DRAWABLE(/*drawable*/w0), GDK_GC(/*gcontext*/w1), GDK_DRAWABLE(/*from*/w2), /*from_x*/i3, /*from_y*/i4, /*to_x*/i5, /*to_y*/i6, /*wide*/i7, /*high*/i8);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__queue_redraw
 *
 * gtk-client.api        type:    { session: Session,   widget:	Widget,   x: Int,   y: Int,   wide: Int,   high: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int, Int, Int) -> Void
 */
static Val   do__queue_redraw   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);
    int               i4 =                            GET_TUPLE_SLOT_AS_INT( arg, 5);

    gtk_widget_queue_draw_area( GTK_WIDGET(/*widget*/w0), /*x*/i1, /*y*/i2, /*wide*/i3, /*high*/i4);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__press_button
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
static Val   do__press_button   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_button_pressed(  GTK_BUTTON(/*widget*/w0) );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__release_button
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
static Val   do__release_button   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_button_released( GTK_BUTTON(/*widget*/w0) );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__click_button
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
static Val   do__click_button   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_button_clicked(  GTK_BUTTON(/*widget*/w0) );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__enter_button
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
static Val   do__enter_button   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_button_enter(    GTK_BUTTON(/*widget*/w0) );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__leave_button
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
static Val   do__leave_button   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_button_leave(    GTK_BUTTON(/*widget*/w0) );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__show_widget
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
static Val   do__show_widget   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_widget_show( /*widget*/w0 );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__show_widget_tree
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
static Val   do__show_widget_tree   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_widget_show_all( /*widget*/w0 );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__destroy_widget
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
static Val   do__destroy_widget   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_widget_destroy( /*widget*/w0 );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__emit_changed_signal
 *
 * gtk-client.api        type:   (Session, Widget)   -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
static Val   do__emit_changed_signal   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    g_signal_emit_by_name( GTK_OBJECT(/*widget*/w0), "changed");

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__pop_up_combo_box
 *
 * gtk-client.api        type:   (Session, Widget)   -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
static Val   do__pop_up_combo_box   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_combo_box_popup(   GTK_COMBO_BOX(/*widget*/w0));

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__pop_down_combo_box
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
static Val   do__pop_down_combo_box   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_combo_box_popdown( GTK_COMBO_BOX(/*widget*/w0));

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_combo_box_title
 *
 * gtk-client.api        type:   (Session, Widget, String)   -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Void
 */
static Val   do__set_combo_box_title   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    gtk_combo_box_set_title( GTK_COMBO_BOX(/*widget*/w0), /*title*/s1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_window_title
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Void
 */
static Val   do__set_window_title   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    gtk_window_set_title( GTK_WINDOW(/*window*/w0), /*title*/s1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_window_default_size
 *
 * gtk-client.api        type:   (Session, Widget, (Int,Int)) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Void
 */
static Val   do__set_window_default_size   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    gtk_window_set_default_size( GTK_WINDOW(/*widget*/w0), /*wide*/i1, /*high*/i2);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_minimum_widget_size
 *
 * gtk-client.api        type:   (Session, Widget, (Int,Int)) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Void
 */
static Val   do__set_minimum_widget_size   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    gtk_widget_set_size_request( GTK_WIDGET(/*widget*/w0), /*wide*/i1, /*high*/i2);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_border_width
 *
 * gtk-client.api        type:   (Session, Widget, Int) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
static Val   do__set_border_width   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_container_set_border_width(GTK_CONTAINER(/*widget*/w0), /*width*/i1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_event_box_visibility
 *
 * gtk-client.api        type:   (Session, Widget, Bool) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Bool) -> Void
 */
static Val   do__set_event_box_visibility   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               b1 =                            GET_TUPLE_SLOT_AS_VAL( arg, 2) == HEAP_TRUE;

    gtk_event_box_set_visible_window(GTK_EVENT_BOX(/*event_box*/w0),/*visibility*/b1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_widget_alignment
 *
 * gtk-client.api        type:    { session: Session, widget: Widget, x: Float, y: Float } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Float, Float) -> Void
 */
static Val   do__set_widget_alignment   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    double            f1 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 2)));
    double            f2 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 3)));

    gtk_misc_set_alignment(GTK_MISC(/*widget*/w0), /*x*/f1, /*y*/f2);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_widget_events
 *
 * gtk-client.api        type:   (Session, Widget, List( Event_Mask )) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
static Val   do__set_widget_events   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_widget_set_events( GTK_WIDGET(/*widget*/w0), int_to_event_mask(/*events_to_int events*/i1));

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_widget_name
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Void
 */
static Val   do__set_widget_name   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    gtk_widget_set_name( GTK_WIDGET(/*widget*/w0), /*name*/s1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_label_justification
 *
 * gtk-client.api        type:   (Session, Widget, Justification) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
static Val   do__set_label_justification   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_label_set_justify( GTK_LABEL(/*label*/w0), int_to_justification(/*justification_to_int justification*/i1));

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_label_line_wrapping
 *
 * gtk-client.api        type:   (Session, Widget, Bool) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Bool) -> Void
 */
static Val   do__set_label_line_wrapping   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               b1 =                            GET_TUPLE_SLOT_AS_VAL( arg, 2) == HEAP_TRUE;

    gtk_label_set_line_wrap( GTK_LABEL(/*label*/w0), /*wrap_lines*/b1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_label_underlines
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Void
 */
static Val   do__set_label_underlines   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    gtk_label_set_pattern( GTK_LABEL(/*label*/w0), /*underlines*/s1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_scale_value_position
 *
 * gtk-client.api        type:   (Session, Widget, Position_Type) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
static Val   do__set_scale_value_position   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_scale_set_value_pos( GTK_SCALE(/*scale*/w0), int_to_position(/*position_to_int position*/i1));

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_draw_scale_value
 *
 * gtk-client.api        type:   (Session, Widget, Bool) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Bool) -> Void
 */
static Val   do__set_draw_scale_value   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               b1 =                            GET_TUPLE_SLOT_AS_VAL( arg, 2) == HEAP_TRUE;

    gtk_scale_set_draw_value( GTK_SCALE(/*scale*/w0), /*draw_value*/b1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__get_scale_value_digits_shown
 *
 * gtk-client.api        type:   (Session, Widget) -> Int
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int
 */
static Val   do__get_scale_value_digits_shown   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    int result = gtk_scale_get_digits( GTK_SCALE(/*scale*/w0) );

    return TAGGED_INT_FROM_C_INT(result);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_scale_value_digits_shown
 *
 * gtk-client.api        type:   (Session, Widget, Int)  -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
static Val   do__set_scale_value_digits_shown   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_scale_set_digits( GTK_SCALE(/*scale*/w0), /*digits*/i1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_range_update_policy
 *
 * gtk-client.api        type:   (Session, Widget, Update_Policy) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
static Val   do__set_range_update_policy   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_range_set_update_policy( GTK_RANGE(/*scale*/w0), /*policy*/int_to_range_update_policy(/*update_policy_to_int policy*/i1));

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__get_toggle_button_state
 *
 * gtk-client.api        type:   (Session, Widget) -> Bool
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Bool
 */
static Val   do__get_toggle_button_state   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    int result = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(/*toggle_button*/w0) );

    return  result ? HEAP_TRUE : HEAP_FALSE;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_toggle_button_state
 *
 * gtk-client.api        type:   (Session, Widget, Bool) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Bool) -> Void
 */
static Val   do__set_toggle_button_state   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               b1 =                            GET_TUPLE_SLOT_AS_VAL( arg, 2) == HEAP_TRUE;

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(/*toggle_button*/w0), /*state*/b1 != 0 );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__get_adjustment_value
 *
 * gtk-client.api        type:   (Session, Widget) -> Float
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Float
 */
static Val   do__get_adjustment_value   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    double d = gtk_adjustment_get_value( GTK_ADJUSTMENT(/*adjustment*/w0) );

    return  make_float64(task, d );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_adjustment_value
 *
 * gtk-client.api        type:   (Session, Widget, Float) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Float) -> Void
 */
static Val   do__set_adjustment_value   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    double            f1 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 2)));

    gtk_adjustment_set_value( GTK_ADJUSTMENT(/*adjustment*/w0), /*value*/f1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__get_white_graphics_context
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*Widget*)
 */
static Val   do__get_white_graphics_context   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    GtkWidget* widget = (GtkWidget*) /*widget*/w0->style->white_gc;

    int slot = get_widget_id( widget );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__get_black_graphics_context
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*Widget*)
 */
static Val   do__get_black_graphics_context   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    GtkWidget* widget = (GtkWidget*) /*widget*/w0->style->black_gc;

    int slot = get_widget_id( widget );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__get_current_foreground_graphics_context
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*Widget*)
 */
static Val   do__get_current_foreground_graphics_context   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    GtkWidget* widget = (GtkWidget*) w0->style->fg_gc[ GTK_WIDGET_STATE(/*widget*/w0) ];

    int slot = get_widget_id( widget );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__get_current_background_graphics_context
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*Widget*)
 */
static Val   do__get_current_background_graphics_context   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    GtkWidget* widget = (GtkWidget*) w0->style->bg_gc[ GTK_WIDGET_STATE(/*widget*/w0) ];

    int slot = get_widget_id( widget );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__get_widget_window
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*Widget*)
 */
static Val   do__get_widget_window   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    GtkWidget* widget = (GtkWidget*) /*widget*/w0->window;

    int slot = get_widget_id( widget );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__add_kid
 *
 * gtk-client.api        type:    { session: Session,   mom: Widget,   kid: Widget } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*)) -> Void
 */
static Val   do__add_kid   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];

    gtk_container_add( GTK_CONTAINER(/*mom*/w0), GTK_WIDGET(/*kid*/w1));

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__add_scrolled_window_kid
 *
 * gtk-client.api        type:    { session: Session,   window: Widget,   kid: Widget } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*)) -> Void
 */
static Val   do__add_scrolled_window_kid   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];

    gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW(/*window*/w0), GTK_WIDGET(/*kid*/w1) );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__add_table_kid
 *
 * gtk-client.api        type:    { session: Session,   table: Widget,   kid: Widget,   left: Int,   right: Int,   top: Int,   bottom: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int, Int, Int, Int) -> Void
 */
static Val   do__add_table_kid   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);
    int               i4 =                            GET_TUPLE_SLOT_AS_INT( arg, 5);
    int               i5 =                            GET_TUPLE_SLOT_AS_INT( arg, 6);

    gtk_table_attach_defaults( GTK_TABLE(/*table*/w0), GTK_WIDGET(/*kid*/w1), /*left*/i2, /*right*/i3, /*top*/i4, /*bottom*/i5 );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__add_table_kid2
 *
 * gtk-client.api        type:    { session: Session,   table: Widget,   kid: Widget,   left: Int,   right: Int,   top: Int,   bottom: Int,   xoptions: List( Table_Attach_Option ),   yoptions: List( Table_Attach_Option ),   xpadding: Int,   ypadding: Int }   ->   Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int, Int, Int, Int, Int, Int, Int, Int) -> Void
 */
static Val   do__add_table_kid2   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);
    int               i4 =                            GET_TUPLE_SLOT_AS_INT( arg, 5);
    int               i5 =                            GET_TUPLE_SLOT_AS_INT( arg, 6);
    int               i6 =                            GET_TUPLE_SLOT_AS_INT( arg, 7);
    int               i7 =                            GET_TUPLE_SLOT_AS_INT( arg, 8);
    int               i8 =                            GET_TUPLE_SLOT_AS_INT( arg, 9);
    int               i9 =                            GET_TUPLE_SLOT_AS_INT( arg, 10);

    gtk_table_attach( GTK_TABLE(/*table*/w0), GTK_WIDGET(/*kid*/w1), /*left*/i2, /*right*/i3, /*top*/i4, /*bottom*/i5, /*sum_table_attach_options xoptions*/i6, /*sum_table_attach_options yoptions*/i7, /*xpadding*/i8, /*ypadding*/i9 );

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__get_viewport_vertical_adjustment
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*Widget*)
 */
static Val   do__get_viewport_vertical_adjustment   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    GtkWidget* widget = (GtkWidget*) gtk_viewport_get_vadjustment( GTK_VIEWPORT(/*viewport*/w0) );

    int slot = get_widget_id( widget );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__get_viewport_horizontal_adjustment
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*Widget*)
 */
static Val   do__get_viewport_horizontal_adjustment   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    GtkWidget* widget = (GtkWidget*) gtk_viewport_get_hadjustment( GTK_VIEWPORT(/*viewport*/w0) );

    int slot = get_widget_id( widget );

    return TAGGED_INT_FROM_C_INT(slot);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_table_row_spacing
 *
 * gtk-client.api        type:    { session: Session, table: Widget, row: Int, spacing: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Void
 */
static Val   do__set_table_row_spacing   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    gtk_table_set_row_spacing( GTK_TABLE(/*table*/w0), /*row*/i1, /*spacing*/i2);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_table_col_spacing
 *
 * gtk-client.api        type:    { session: Session, table: Widget, col: Int, spacing: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Void
 */
static Val   do__set_table_col_spacing   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    gtk_table_set_col_spacing( GTK_TABLE(/*table*/w0), /*col*/i1, /*spacing*/i2);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_table_row_spacings
 *
 * gtk-client.api        type:   (Session, Widget, Int) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
static Val   do__set_table_row_spacings   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_table_set_row_spacings( GTK_TABLE(/*table*/w0), /*spacing*/i1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */


/* do__set_table_col_spacings
 *
 * gtk-client.api        type:   (Session, Widget, Int) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
static Val   do__set_table_col_spacings   (Task* task, Val arg)
{

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_table_set_col_spacings( GTK_TABLE(/*table*/w0), /*spacing*/i1);

    return HEAP_VOID;
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  write_libmythryl_xxx_c_plain_fun  per  src/glu/gtk/etc/library-glue.plan. */




/*  do__set_clicked_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_clicked_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "clicked", G_CALLBACK( run_clicked_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_pressed_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_pressed_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "pressed", G_CALLBACK( run_pressed_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_release_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_release_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "release", G_CALLBACK( run_release_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_enter_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_enter_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "enter", G_CALLBACK( run_enter_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_leave_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_leave_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "leave", G_CALLBACK( run_leave_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_activate_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_activate_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( GTK_MENU_ITEM(w0), "activate", G_CALLBACK( run_activate_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_destroy_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_destroy_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "destroy", G_CALLBACK( run_destroy_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_realize_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_realize_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "realize", G_CALLBACK( run_realize_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_button_press_event_callback : Session -> Widget -> Button_Event_Callback -> Void
 */
static Val   do__set_button_press_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "button_press_event", G_CALLBACK( run_button_press_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_button_release_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_button_release_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "button_release_event", G_CALLBACK( run_button_release_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_scroll_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_scroll_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "scroll_event", G_CALLBACK( run_scroll_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_motion_notify_event_callback : Session -> Widget -> Motion_Event_Callback -> Void
 */
static Val   do__set_motion_notify_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "motion_notify_event", G_CALLBACK( run_motion_notify_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_delete_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_delete_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "delete_event", G_CALLBACK( run_delete_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_expose_event_callback : Session -> Widget -> Expose_Event_Callback -> Void
 */
static Val   do__set_expose_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "expose_event", G_CALLBACK( run_expose_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_key_press_event_callback : Session -> Widget -> Key_Event_Callback -> Void
 */
static Val   do__set_key_press_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "key_press_event", G_CALLBACK( run_key_press_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_key_release_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_key_release_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "key_release_event", G_CALLBACK( run_key_release_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_enter_notify_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_enter_notify_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "enter_notify_event", G_CALLBACK( run_enter_notify_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_leave_notify_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_leave_notify_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "leave_notify_event", G_CALLBACK( run_leave_notify_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_configure_event_callback : Session -> Widget -> Configure_Event_Callback -> Void
 */
static Val   do__set_configure_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "configure_event", G_CALLBACK( run_configure_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_focus_in_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_focus_in_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "focus_in_event", G_CALLBACK( run_focus_in_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_focus_out_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_focus_out_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "focus_out_event", G_CALLBACK( run_focus_out_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_map_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_map_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "map_event", G_CALLBACK( run_map_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_unmap_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_unmap_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "unmap_event", G_CALLBACK( run_unmap_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_property_notify_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_property_notify_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "property_notify_event", G_CALLBACK( run_property_notify_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_selection_clear_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_selection_clear_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "selection_clear_event", G_CALLBACK( run_selection_clear_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_selection_request_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_selection_request_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "selection_request_event", G_CALLBACK( run_selection_request_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_selection_notify_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_selection_notify_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "selection_notify_event", G_CALLBACK( run_selection_notify_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_proximity_in_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_proximity_in_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "proximity_in_event", G_CALLBACK( run_proximity_in_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_proximity_out_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_proximity_out_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "proximity_out_event", G_CALLBACK( run_proximity_out_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_client_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_client_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "client_event", G_CALLBACK( run_client_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_no_expose_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_no_expose_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "no_expose_event", G_CALLBACK( run_no_expose_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_window_state_event_callback : Session -> Widget -> Void_Callback -> Void
 */
static Val   do__set_window_state_event_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "window_state_event", G_CALLBACK( run_window_state_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_toggled_callback : Session -> Widget -> Bool_Callback -> Void
 */
static Val   do__set_toggled_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "toggled", G_CALLBACK( run_toggled_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/




/*  do__set_value_changed_callback : Session -> Widget -> Float_Callback -> Void
 */
static Val   do__set_value_changed_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "value_changed", G_CALLBACK( run_value_changed_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/library-glue.plan.*/


/* Do not edit this or preceding lines -- they are autobuilt. */
/////////////////////////////////////////////////////////////////////////////////////

/////////////// old libmythryl-gtk.c contents follow //////////////////////////////////

#include "../../../../c/mythryl-config.h"

#include "runtime-base.h"
#include "mythryl-callable-c-libraries.h"

#include "raise-error.h"


// This section lists the directory library of C functions that are callable from Mythryl.

// This table ultimately gets searched by
//
//     src/glu/gtk/src/gtk-client-driver-for-library-in-main-process.pkg
// via
//     get_mythryl_callable_c_function() 	in   src/c/lib/mythryl-callable-c-libraries.c


#ifndef CLIB_NAME
#define CLIB_NAME	"gtk"
#define CLIB_VERSION	"1.0"
#define CLIB_DATE	"February 13, 2008"
#endif

// The table of C functions and their Mythryl names:
//
#define CFUNC(NAME, NAME2, FUNC, LIB7TYPE)	CFUNC_BIND(NAME, NAME2, FUNC, LIB7TYPE)
static Mythryl_Name_With_C_Function CFunTable[] = {

CFUNC("init","init",	_lib7_Gtk_gtk_init,		"Void -> Void")

CFUNC("callback_queue_is_empty","callback_queue_is_empty",	   _lib7_Gtk_callback_queue_is_empty,		"Void -> Bool")
CFUNC("number_of_queued_callbacks","number_of_queued_callbacks",	   _lib7_Gtk_number_of_queued_callbacks,	"Void -> Int")
CFUNC("type_of_next_queued_callback","type_of_next_queued_callback",	   _lib7_Gtk_type_of_next_queued_callback,	"Void -> Int")
CFUNC("get_queued_void_callback","get_queued_void_callback",	   _lib7_Gtk_get_queued_void_callback,		"Void -> Int")
CFUNC("get_queued_bool_callback","get_queued_bool_callback",	   _lib7_Gtk_get_queued_bool_callback,		"Void -> (Int, Bool)")
CFUNC("get_queued_float_callback","get_queued_float_callback",	   _lib7_Gtk_get_queued_float_callback,		"Void -> (Int, Float)")
CFUNC("get_queued_button_press_callback","get_queued_button_press_callback",  _lib7_Gtk_get_queued_button_press_callback,	"Void -> (Int, Int, Int, Float, Float, Int, Int)")  // Void -> (callback_number, widget_id, button, x, y, time, modifiers)
CFUNC("get_queued_key_press_callback","get_queued_key_press_callback",     _lib7_Gtk_get_queued_key_press_callback,	"Void -> (Int, Int, Int, Int, Int)")                // Void -> (callback_number, key, keycode, time, modifiers)
CFUNC("get_queued_motion_notify_callback","get_queued_motion_notify_callback", _lib7_Gtk_get_queued_motion_notify_callback,	"Void -> (Int, Int, Int, Float, Float, Int, Bool)") // Void -> (callback_number, widget_id, time, x, y, modifiers, is_hint)
CFUNC("get_queued_expose_callback","get_queued_expose_callback",        _lib7_Gtk_get_queued_expose_callback,	"Void -> (Int, Int, Int, Int, Int, Int, Int)")      // Void -> (callback_number, widget, count, area_x, area_y, area_wide, area_high)
CFUNC("get_queued_configure_callback","get_queued_configure_callback",     _lib7_Gtk_get_queued_configure_callback,	"Void -> (Int, Int, Int, Int, Int, Int)")           // Void -> (callback_number, widget, x,    y,    wide, high)

CFUNC("get_widget_allocation","get_widget_allocation",             _lib7_Gtk_get_widget_allocation,		"Int -> (Int, Int, Int, Int)")                      // Widget -> (x,    y,    wide, high)
CFUNC("get_window_pointer","get_window_pointer",                _lib7_Gtk_get_window_pointer,		"Int -> (Int, Int, Int)")                           // Widget -> (x,    y,    modifiers)
CFUNC("make_dialog","make_dialog",                       _lib7_Gtk_make_dialog,			"Void -> (Int, Int, Int)")                          // Void   -> (dialog, vbox, action-area)
CFUNC("unref_object","unref_object",                      _lib7_Gtk_unref_object,			"Int -> Void")                                      // Widget -> Void
CFUNC("run_eventloop_once","run_eventloop_once",                _lib7_Gtk_run_eventloop_once,		"Bool -> Bool")                                     // Bool -> Bool



/////////////////////////////////////////////////////////////////////////////////////
// The following stuff gets built from paragraphs in
//     src/glu/gtk/etc/library-glue.plan
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
//   mlb::BUILD_A ("callback-fn", build_callback_function)				# In src/glu/gtk/sh/make-gtk-glue
//   ->  build_callback_function							# In src/glu/gtk/sh/make-gtk-glue
//       ->  r.build_table_entry_for_'libmythryl_xxx_c' (c_fn_name, fn_type);		# In src/lib/make-library-glue/make-library-glue.pkg
//
/* Do not edit this or following lines -- they are autobuilt. */
CFUNC("make_window",                              "make_window",                              do__make_window,                                       "Session -> Widget")
CFUNC("make_label",                               "make_label",                               do__make_label,                                       "(Session, String) -> Widget")
CFUNC("make_status_bar_context_id",               "make_status_bar_context_id",               do__make_status_bar_context_id,                       "(Session, Widget, String) -> Int")
CFUNC("make_menu",                                "make_menu",                                do__make_menu,                                         "Session -> Widget")
CFUNC("make_option_menu",                         "make_option_menu",                         do__make_option_menu,                                  "Session -> Widget")
CFUNC("make_menu_bar",                            "make_menu_bar",                            do__make_menu_bar,                                     "Session -> Widget")
CFUNC("make_combo_box",                           "make_combo_box",                           do__make_combo_box,                                    "Session -> Widget")
CFUNC("make_text_combo_box",                      "make_text_combo_box",                      do__make_text_combo_box,                               "Session -> Widget")
CFUNC("make_frame",                               "make_frame",                               do__make_frame,                                       "(Session, String) -> Widget")
CFUNC("make_button",                              "make_button",                              do__make_button,                                       "Session -> Widget")
CFUNC("make_button_with_label",                   "make_button_with_label",                   do__make_button_with_label,                           "(Session, String) -> Widget")
CFUNC("make_button_with_mnemonic",                "make_button_with_mnemonic",                do__make_button_with_mnemonic,                        "(Session, String) -> Widget")
CFUNC("make_toggle_button",                       "make_toggle_button",                       do__make_toggle_button,                                "Session -> Widget")
CFUNC("make_toggle_button_with_label",            "make_toggle_button_with_label",            do__make_toggle_button_with_label,                    "(Session, String) -> Widget")
CFUNC("make_toggle_button_with_mnemonic",         "make_toggle_button_with_mnemonic",         do__make_toggle_button_with_mnemonic,                 "(Session, String) -> Widget")
CFUNC("make_check_button",                        "make_check_button",                        do__make_check_button,                                 "Session -> Widget")
CFUNC("make_check_button_with_label",             "make_check_button_with_label",             do__make_check_button_with_label,                     "(Session, String) -> Widget")
CFUNC("make_check_button_with_mnemonic",          "make_check_button_with_mnemonic",          do__make_check_button_with_mnemonic,                  "(Session, String) -> Widget")
CFUNC("make_menu_item",                           "make_menu_item",                           do__make_menu_item,                                    "Session -> Widget")
CFUNC("make_menu_item_with_label",                "make_menu_item_with_label",                do__make_menu_item_with_label,                        "(Session, String) -> Widget")
CFUNC("make_menu_item_with_mnemonic",             "make_menu_item_with_mnemonic",             do__make_menu_item_with_mnemonic,                     "(Session, String) -> Widget")
CFUNC("make_first_radio_button",                  "make_first_radio_button",                  do__make_first_radio_button,                           "Session -> Widget")
CFUNC("make_next_radio_button",                   "make_next_radio_button",                   do__make_next_radio_button,                           "(Session, Widget) -> Widget")
CFUNC("make_first_radio_button_with_label",       "make_first_radio_button_with_label",       do__make_first_radio_button_with_label,               "(Session, String) -> Widget")
CFUNC("make_next_radio_button_with_label",        "make_next_radio_button_with_label",        do__make_next_radio_button_with_label,                "(Session, Widget, String) -> Widget")
CFUNC("make_first_radio_button_with_mnemonic",    "make_first_radio_button_with_mnemonic",    do__make_first_radio_button_with_mnemonic,            "(Session, String) -> Widget")
CFUNC("make_next_radio_button_with_mnemonic",     "make_next_radio_button_with_mnemonic",     do__make_next_radio_button_with_mnemonic,             "(Session, Widget, String) -> Widget")
CFUNC("make_arrow",                               "make_arrow",                               do__make_arrow,                                       "(Session, Arrow_Direction, Shadow_Style) -> Widget")
CFUNC("set_arrow",                                "set_arrow",                                do__set_arrow,                                        "(Session, Widget, Arrow_Direction, Shadow_Style) -> Void")
CFUNC("make_horizontal_box",                      "make_horizontal_box",                      do__make_horizontal_box,                              "(Session, Bool, Int)   ->   Widget")
CFUNC("make_vertical_box",                        "make_vertical_box",                        do__make_vertical_box,                                "(Session, Bool, Int)   ->   Widget")
CFUNC("make_horizontal_button_box",               "make_horizontal_button_box",               do__make_horizontal_button_box,                        "Session -> Widget")
CFUNC("make_vertical_button_box",                 "make_vertical_button_box",                 do__make_vertical_button_box,                          "Session -> Widget")
CFUNC("make_table",                               "make_table",                               do__make_table,                                        "{ session: Session,   rows: Int,   cols: Int,   homogeneous: Bool }   ->   Widget")
CFUNC("make_event_box",                           "make_event_box",                           do__make_event_box,                                    "Session -> Widget")
CFUNC("make_image_from_file",                     "make_image_from_file",                     do__make_image_from_file,                             "(Session, String) -> Widget")
CFUNC("make_horizontal_separator",                "make_horizontal_separator",                do__make_horizontal_separator,                         "Session -> Widget")
CFUNC("make_vertical_separator",                  "make_vertical_separator",                  do__make_vertical_separator,                           "Session -> Widget")
CFUNC("make_layout_container",                    "make_layout_container",                    do__make_layout_container,                             "Session -> Widget")
CFUNC("layout_put",                               "layout_put",                               do__layout_put,                                        "{ session: Session,  layout: Widget,  kid: Widget,  x: Int,  y: Int } -> Void")
CFUNC("layout_move",                              "layout_move",                              do__layout_move,                                       "{ session: Session, layout: Widget,  kid: Widget,  x: Int,  y: Int } -> Void")
CFUNC("make_fixed_container",                     "make_fixed_container",                     do__make_fixed_container,                              "Session -> Widget")
CFUNC("fixed_put",                                "fixed_put",                                do__fixed_put,                                         "{ session: Session, layout: Widget,  kid: Widget,  x: Int,  y: Int } -> Void")
CFUNC("fixed_move",                               "fixed_move",                               do__fixed_move,                                        "{ session: Session, layout: Widget,  kid: Widget,  x: Int,  y: Int } -> Void")
CFUNC("make_adjustment",                          "make_adjustment",                          do__make_adjustment,                                   "{ session: Session,   value: Float,   lower: Float,   upper: Float,   step_increment: Float,   page_increment: Float,   page_size: Float }   ->   Widget")
CFUNC("make_viewport",                            "make_viewport",                            do__make_viewport,                                     "{ session: Session, horizontal_adjustment: Null_Or(Widget), vertical_adjustment: Null_Or(Widget) } -> Widget")
CFUNC("make_scrolled_window",                     "make_scrolled_window",                     do__make_scrolled_window,                              "{ session: Session, horizontal_adjustment: Null_Or(Widget), vertical_adjustment: Null_Or(Widget) } -> Widget")
CFUNC("make_horizontal_ruler",                    "make_horizontal_ruler",                    do__make_horizontal_ruler,                             "Session -> Widget")
CFUNC("make_vertical_ruler",                      "make_vertical_ruler",                      do__make_vertical_ruler,                               "Session -> Widget")
CFUNC("make_vertical_scrollbar",                  "make_vertical_scrollbar",                  do__make_vertical_scrollbar,                          "(Session, Null_Or(Widget)) -> Widget")
CFUNC("make_horizontal_scrollbar",                "make_horizontal_scrollbar",                do__make_horizontal_scrollbar,                        "(Session, Null_Or(Widget)) -> Widget")
CFUNC("make_vertical_scale",                      "make_vertical_scale",                      do__make_vertical_scale,                              "(Session, Null_Or(Widget)) -> Widget")
CFUNC("make_horizontal_scale",                    "make_horizontal_scale",                    do__make_horizontal_scale,                            "(Session, Null_Or(Widget)) -> Widget")
CFUNC("make_vertical_scale_with_range",           "make_vertical_scale_with_range",           do__make_vertical_scale_with_range,                    "{ session: Session, min: Float, max: Float, step: Float } -> Widget")
CFUNC("make_horizontal_scale_with_range",         "make_horizontal_scale_with_range",         do__make_horizontal_scale_with_range,                  "{ session: Session, min: Float, max: Float, step: Float } -> Widget")
CFUNC("make_drawing_area",                        "make_drawing_area",                        do__make_drawing_area,                                 "Session -> Widget")
CFUNC("make_pixmap",                              "make_pixmap",                              do__make_pixmap,                                       "{ session: Session, window: Widget, wide: Int, high: Int } -> Widget")
CFUNC("make_status_bar",                          "make_status_bar",                          do__make_status_bar,                                   "Session -> Widget")
CFUNC("push_text_on_status_bar",                  "push_text_on_status_bar",                  do__push_text_on_status_bar,                          "(Session, Widget, Int, String) -> Int")
CFUNC("pop_text_off_status_bar",                  "pop_text_off_status_bar",                  do__pop_text_off_status_bar,                          "(Session, Widget, Int) -> Void")
CFUNC("remove_text_from_status_bar",              "remove_text_from_status_bar",              do__remove_text_from_status_bar,                       "{ session: Session,   status_bar: Widget,   context: Int,   message: Int } -> Void")
CFUNC("pack_box",                                 "pack_box",                                 do__pack_box,                                          "{ session: Session,   box: Widget,   kid: Widget,   pack: Pack_From,   expand: Bool,   fill: Bool,   padding: Int } -> Void")
CFUNC("menu_shell_append",                        "menu_shell_append",                        do__menu_shell_append,                                 "{ session: Session,   menu: Widget,   kid: Widget } -> Void")
CFUNC("menu_bar_append",                          "menu_bar_append",                          do__menu_bar_append,                                   "{ session: Session,   menu: Widget,   kid: Widget } -> Void")
CFUNC("append_text_to_combo_box",                 "append_text_to_combo_box",                 do__append_text_to_combo_box,                         "(Session, Widget, String) -> Void")
CFUNC("set_option_menu_menu",                     "set_option_menu_menu",                     do__set_option_menu_menu,                              "{ session: Session,   option_menu: Widget,   menu: Widget } -> Void")
CFUNC("set_text_tooltip_on_widget",               "set_text_tooltip_on_widget",               do__set_text_tooltip_on_widget,                       "(Session, Widget, String) -> Void")
CFUNC("set_ruler_metric",                         "set_ruler_metric",                         do__set_ruler_metric,                                 "(Session, Widget, Metric) -> Void")
CFUNC("set_ruler_range",                          "set_ruler_range",                          do__set_ruler_range,                                   "{ session: Session,   ruler: Widget,   lower: Float,   upper: Float,   position: Float,   max_size: Float } -> Void")
CFUNC("set_scrollbar_policy",                     "set_scrollbar_policy",                     do__set_scrollbar_policy,                              "{ session: Session,   window: Widget,   horizontal_scrollbar: Scrollbar_Policy,   vertical_scrollbar: Scrollbar_Policy } -> Void")
CFUNC("draw_rectangle",                           "draw_rectangle",                           do__draw_rectangle,                                    "{ session: Session,   drawable: Widget,   gcontext: Widget,   filled:	Bool,   x: Int,   y: Int,   wide: Int,   high: Int } -> Void")
CFUNC("draw_drawable",                            "draw_drawable",                            do__draw_drawable,                                     "{ session: Session,   drawable: Widget,   gcontext: Widget,   from: Widget,   from_x:	Int,   from_y: Int,   to_x: Int,   to_y: Int,   wide: Int,   high: Int } -> Void")
CFUNC("queue_redraw",                             "queue_redraw",                             do__queue_redraw,                                      "{ session: Session,   widget:	Widget,   x: Int,   y: Int,   wide: Int,   high: Int } -> Void")
CFUNC("press_button",                             "press_button",                             do__press_button,                                     "(Session, Widget) -> Void")
CFUNC("release_button",                           "release_button",                           do__release_button,                                   "(Session, Widget) -> Void")
CFUNC("click_button",                             "click_button",                             do__click_button,                                     "(Session, Widget) -> Void")
CFUNC("enter_button",                             "enter_button",                             do__enter_button,                                     "(Session, Widget) -> Void")
CFUNC("leave_button",                             "leave_button",                             do__leave_button,                                     "(Session, Widget) -> Void")
CFUNC("show_widget",                              "show_widget",                              do__show_widget,                                      "(Session, Widget) -> Void")
CFUNC("show_widget_tree",                         "show_widget_tree",                         do__show_widget_tree,                                 "(Session, Widget) -> Void")
CFUNC("destroy_widget",                           "destroy_widget",                           do__destroy_widget,                                   "(Session, Widget) -> Void")
CFUNC("emit_changed_signal",                      "emit_changed_signal",                      do__emit_changed_signal,                              "(Session, Widget)   -> Void")
CFUNC("pop_up_combo_box",                         "pop_up_combo_box",                         do__pop_up_combo_box,                                 "(Session, Widget)   -> Void")
CFUNC("pop_down_combo_box",                       "pop_down_combo_box",                       do__pop_down_combo_box,                               "(Session, Widget) -> Void")
CFUNC("set_combo_box_title",                      "set_combo_box_title",                      do__set_combo_box_title,                              "(Session, Widget, String)   -> Void")
CFUNC("set_window_title",                         "set_window_title",                         do__set_window_title,                                 "(Session, Widget, String) -> Void")
CFUNC("set_window_default_size",                  "set_window_default_size",                  do__set_window_default_size,                          "(Session, Widget, (Int,Int)) -> Void")
CFUNC("set_minimum_widget_size",                  "set_minimum_widget_size",                  do__set_minimum_widget_size,                          "(Session, Widget, (Int,Int)) -> Void")
CFUNC("set_border_width",                         "set_border_width",                         do__set_border_width,                                 "(Session, Widget, Int) -> Void")
CFUNC("set_event_box_visibility",                 "set_event_box_visibility",                 do__set_event_box_visibility,                         "(Session, Widget, Bool) -> Void")
CFUNC("set_widget_alignment",                     "set_widget_alignment",                     do__set_widget_alignment,                              "{ session: Session, widget: Widget, x: Float, y: Float } -> Void")
CFUNC("set_widget_events",                        "set_widget_events",                        do__set_widget_events,                                "(Session, Widget, List( Event_Mask )) -> Void")
CFUNC("set_widget_name",                          "set_widget_name",                          do__set_widget_name,                                  "(Session, Widget, String) -> Void")
CFUNC("set_label_justification",                  "set_label_justification",                  do__set_label_justification,                          "(Session, Widget, Justification) -> Void")
CFUNC("set_label_line_wrapping",                  "set_label_line_wrapping",                  do__set_label_line_wrapping,                          "(Session, Widget, Bool) -> Void")
CFUNC("set_label_underlines",                     "set_label_underlines",                     do__set_label_underlines,                             "(Session, Widget, String) -> Void")
CFUNC("set_scale_value_position",                 "set_scale_value_position",                 do__set_scale_value_position,                         "(Session, Widget, Position_Type) -> Void")
CFUNC("set_draw_scale_value",                     "set_draw_scale_value",                     do__set_draw_scale_value,                             "(Session, Widget, Bool) -> Void")
CFUNC("get_scale_value_digits_shown",             "get_scale_value_digits_shown",             do__get_scale_value_digits_shown,                     "(Session, Widget) -> Int")
CFUNC("set_scale_value_digits_shown",             "set_scale_value_digits_shown",             do__set_scale_value_digits_shown,                     "(Session, Widget, Int)  -> Void")
CFUNC("set_range_update_policy",                  "set_range_update_policy",                  do__set_range_update_policy,                          "(Session, Widget, Update_Policy) -> Void")
CFUNC("get_toggle_button_state",                  "get_toggle_button_state",                  do__get_toggle_button_state,                          "(Session, Widget) -> Bool")
CFUNC("set_toggle_button_state",                  "set_toggle_button_state",                  do__set_toggle_button_state,                          "(Session, Widget, Bool) -> Void")
CFUNC("get_adjustment_value",                     "get_adjustment_value",                     do__get_adjustment_value,                             "(Session, Widget) -> Float")
CFUNC("set_adjustment_value",                     "set_adjustment_value",                     do__set_adjustment_value,                             "(Session, Widget, Float) -> Void")
CFUNC("get_white_graphics_context",               "get_white_graphics_context",               do__get_white_graphics_context,                       "(Session, Widget) -> Widget")
CFUNC("get_black_graphics_context",               "get_black_graphics_context",               do__get_black_graphics_context,                       "(Session, Widget) -> Widget")
CFUNC("get_current_foreground_graphics_context",  "get_current_foreground_graphics_context",  do__get_current_foreground_graphics_context,          "(Session, Widget) -> Widget")
CFUNC("get_current_background_graphics_context",  "get_current_background_graphics_context",  do__get_current_background_graphics_context,          "(Session, Widget) -> Widget")
CFUNC("get_widget_window",                        "get_widget_window",                        do__get_widget_window,                                "(Session, Widget) -> Widget")
CFUNC("add_kid",                                  "add_kid",                                  do__add_kid,                                           "{ session: Session,   mom: Widget,   kid: Widget } -> Void")
CFUNC("add_scrolled_window_kid",                  "add_scrolled_window_kid",                  do__add_scrolled_window_kid,                           "{ session: Session,   window: Widget,   kid: Widget } -> Void")
CFUNC("add_table_kid",                            "add_table_kid",                            do__add_table_kid,                                     "{ session: Session,   table: Widget,   kid: Widget,   left: Int,   right: Int,   top: Int,   bottom: Int } -> Void")
CFUNC("add_table_kid2",                           "add_table_kid2",                           do__add_table_kid2,                                    "{ session: Session,   table: Widget,   kid: Widget,   left: Int,   right: Int,   top: Int,   bottom: Int,   xoptions: List( Table_Attach_Option ),   yoptions: List( Table_Attach_Option ),   xpadding: Int,   ypadding: Int }   ->   Void")
CFUNC("get_viewport_vertical_adjustment",         "get_viewport_vertical_adjustment",         do__get_viewport_vertical_adjustment,                 "(Session, Widget) -> Widget")
CFUNC("get_viewport_horizontal_adjustment",       "get_viewport_horizontal_adjustment",       do__get_viewport_horizontal_adjustment,               "(Session, Widget) -> Widget")
CFUNC("set_table_row_spacing",                    "set_table_row_spacing",                    do__set_table_row_spacing,                             "{ session: Session, table: Widget, row: Int, spacing: Int } -> Void")
CFUNC("set_table_col_spacing",                    "set_table_col_spacing",                    do__set_table_col_spacing,                             "{ session: Session, table: Widget, col: Int, spacing: Int } -> Void")
CFUNC("set_table_row_spacings",                   "set_table_row_spacings",                   do__set_table_row_spacings,                           "(Session, Widget, Int) -> Void")
CFUNC("set_table_col_spacings",                   "set_table_col_spacings",                   do__set_table_col_spacings,                           "(Session, Widget, Int) -> Void")
CFUNC("set_clicked_callback",                     "set_clicked_callback",                     do__set_clicked_callback,                              "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_pressed_callback",                     "set_pressed_callback",                     do__set_pressed_callback,                              "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_release_callback",                     "set_release_callback",                     do__set_release_callback,                              "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_enter_callback",                       "set_enter_callback",                       do__set_enter_callback,                                "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_leave_callback",                       "set_leave_callback",                       do__set_leave_callback,                                "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_activate_callback",                    "set_activate_callback",                    do__set_activate_callback,                             "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_destroy_callback",                     "set_destroy_callback",                     do__set_destroy_callback,                              "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_realize_callback",                     "set_realize_callback",                     do__set_realize_callback,                              "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_button_press_event_callback",          "set_button_press_event_callback",          do__set_button_press_event_callback,                   "Session -> Widget -> Button_Event_Callback -> Void")
CFUNC("set_button_release_event_callback",        "set_button_release_event_callback",        do__set_button_release_event_callback,                 "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_scroll_event_callback",                "set_scroll_event_callback",                do__set_scroll_event_callback,                         "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_motion_notify_event_callback",         "set_motion_notify_event_callback",         do__set_motion_notify_event_callback,                  "Session -> Widget -> Motion_Event_Callback -> Void")
CFUNC("set_delete_event_callback",                "set_delete_event_callback",                do__set_delete_event_callback,                         "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_expose_event_callback",                "set_expose_event_callback",                do__set_expose_event_callback,                         "Session -> Widget -> Expose_Event_Callback -> Void")
CFUNC("set_key_press_event_callback",             "set_key_press_event_callback",             do__set_key_press_event_callback,                      "Session -> Widget -> Key_Event_Callback -> Void")
CFUNC("set_key_release_event_callback",           "set_key_release_event_callback",           do__set_key_release_event_callback,                    "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_enter_notify_event_callback",          "set_enter_notify_event_callback",          do__set_enter_notify_event_callback,                   "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_leave_notify_event_callback",          "set_leave_notify_event_callback",          do__set_leave_notify_event_callback,                   "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_configure_event_callback",             "set_configure_event_callback",             do__set_configure_event_callback,                      "Session -> Widget -> Configure_Event_Callback -> Void")
CFUNC("set_focus_in_event_callback",              "set_focus_in_event_callback",              do__set_focus_in_event_callback,                       "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_focus_out_event_callback",             "set_focus_out_event_callback",             do__set_focus_out_event_callback,                      "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_map_event_callback",                   "set_map_event_callback",                   do__set_map_event_callback,                            "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_unmap_event_callback",                 "set_unmap_event_callback",                 do__set_unmap_event_callback,                          "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_property_notify_event_callback",       "set_property_notify_event_callback",       do__set_property_notify_event_callback,                "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_selection_clear_event_callback",       "set_selection_clear_event_callback",       do__set_selection_clear_event_callback,                "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_selection_request_event_callback",     "set_selection_request_event_callback",     do__set_selection_request_event_callback,              "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_selection_notify_event_callback",      "set_selection_notify_event_callback",      do__set_selection_notify_event_callback,               "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_proximity_in_event_callback",          "set_proximity_in_event_callback",          do__set_proximity_in_event_callback,                   "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_proximity_out_event_callback",         "set_proximity_out_event_callback",         do__set_proximity_out_event_callback,                  "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_client_event_callback",                "set_client_event_callback",                do__set_client_event_callback,                         "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_no_expose_event_callback",             "set_no_expose_event_callback",             do__set_no_expose_event_callback,                      "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_window_state_event_callback",          "set_window_state_event_callback",          do__set_window_state_event_callback,                   "Session -> Widget -> Void_Callback -> Void")
CFUNC("set_toggled_callback",                     "set_toggled_callback",                     do__set_toggled_callback,                              "Session -> Widget -> Bool_Callback -> Void")
CFUNC("set_value_changed_callback",               "set_value_changed_callback",               do__set_value_changed_callback,                        "Session -> Widget -> Float_Callback -> Void")
/* Do not edit this or preceding lines -- they are autobuilt. */
/////////////////////////////////////////////////////////////////////////////////////

	CFUNC_NULL_BIND
    };
#undef CFUNC



// The Gtk library:
//
// Our record                Libmythryl_Gtk
// gets compiled into        src/c/lib/mythryl-callable-c-libraries-list.h
// and thus ultimately       mythryl_callable_c_libraries__local []
// in                        src/c/lib/mythryl-callable-c-libraries.c
//
Mythryl_Callable_C_Library	    Libmythryl_Gtk = {						// Mythryl_Callable_C_Library		def in    src/c/h/mythryl-callable-c-libraries.h
    //                              ================ 
    CLIB_NAME,
    CLIB_VERSION,
    CLIB_DATE,
    NULL,
    CFunTable
};


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

