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
//     src/glu/gtk/etc/construction.plan
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
/* Do not edit this or following lines -- they are autobuilt.  (patchname="functions") */


/*  do__set_value_changed_callback : Session -> Widget -> Float_Callback -> Void
 */
static Val   do__set_value_changed_callback (Task* task, Val arg)
{

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "value_changed", G_CALLBACK( run_value_changed_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'libmythryl_xxx_c'  per  src/glu/gtk/etc/construction.plan.*/


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
//     src/glu/gtk/etc/construction.plan
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
/* Do not edit this or following lines -- they are autobuilt.  (patchname="table") */
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

