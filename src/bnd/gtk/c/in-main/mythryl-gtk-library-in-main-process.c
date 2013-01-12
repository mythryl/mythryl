// mythryl-gtk-library-in-main-process.c
//
// This file handles the C side
// of the Mythryl <-> C interface
// layer for the Mythryl in-process
// Gtk binding.  The Mythryl side
// is implemented by
//
//     src/bnd/gtk/src/gtk-client-driver-for-library-in-main-process.pkg
//

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
#include "cfun-proto-list.h"

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
//     src/bnd/gtk/src/gtk-client-driver-for-library-in-main-process.pkg 
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





/* Do not edit this or following lines -- they are autogenerated by make-gtk-glue. */
/* _lib7_Gtk_make_window
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_window   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_window_new( GTK_WINDOW_TOPLEVEL );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_label
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_label   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_label_new( /*label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_status_bar_context_id
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Int
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Int
 */
Val   _lib7_Gtk_make_status_bar_context_id   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    int result = gtk_statusbar_get_context_id( GTK_STATUSBAR(/*status_bar*/w0), /*description*/s1);

    return TAGGED_INT_FROM_C_INT(result);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_menu
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_menu   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_menu_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_option_menu
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_option_menu   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_option_menu_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_menu_bar
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_menu_bar   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_menu_bar_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_combo_box
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_combo_box   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_combo_box_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_text_combo_box
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_text_combo_box   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_combo_box_new_text ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_frame
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_frame   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_frame_new (*/*label*/s0 ? /*label*/s0 : NULL);

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_button
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_button   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_button_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_button_with_label
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_button_with_label   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_button_new_with_label( /*label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_button_with_mnemonic
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_button_with_mnemonic   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_button_new_with_mnemonic( /*mnemonic_label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_toggle_button
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_toggle_button   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_toggle_button_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_toggle_button_with_label
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_toggle_button_with_label   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_toggle_button_new_with_label( /*label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_toggle_button_with_mnemonic
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_toggle_button_with_mnemonic   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_toggle_button_new_with_mnemonic( /*mnemonic_label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_check_button
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_check_button   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_check_button_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_check_button_with_label
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_check_button_with_label   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_check_button_new_with_label ( /*label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_check_button_with_mnemonic
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_check_button_with_mnemonic   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_check_button_new_with_mnemonic( /*mnemonic_label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_menu_item
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_menu_item   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_menu_item_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_menu_item_with_label
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_menu_item_with_label   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_menu_item_new_with_label( /*label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_menu_item_with_mnemonic
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_menu_item_with_mnemonic   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_menu_item_new_with_mnemonic( /*mnemonic_label*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_first_radio_button
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_first_radio_button   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_radio_button_new (NULL);

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_next_radio_button
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_next_radio_button   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON(/*sib*/w0));

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_first_radio_button_with_label
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_first_radio_button_with_label   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_radio_button_new_with_label(NULL,/*label*/s0);

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_next_radio_button_with_label
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_next_radio_button_with_label   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_radio_button_new_with_label_from_widget ( GTK_RADIO_BUTTON(/*sib*/w0), /*label*/s1 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_first_radio_button_with_mnemonic
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_first_radio_button_with_mnemonic   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_radio_button_new_with_mnemonic(NULL,/*label*/s0);

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_next_radio_button_with_mnemonic
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_next_radio_button_with_mnemonic   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_radio_button_new_with_mnemonic_from_widget ( GTK_RADIO_BUTTON(/*sib*/w0), /*label*/s1 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_arrow
 *
 * gtk-client.api        type:   (Session, Arrow_Direction, Shadow_Style) -> Widget
 * gtk-client-driver.api type:   (Session, Int, Int) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_arrow   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    int               i0 =                            GET_TUPLE_SLOT_AS_INT( arg, 1);
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_arrow_new( int_to_arrow_direction(/*arrow_direction_to_int arrow_direction*/i0), int_to_shadow_style(/*shadow_style_to_int shadow_style*/i1) );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_arrow
 *
 * gtk-client.api        type:   (Session, Widget, Arrow_Direction, Shadow_Style) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Void
 */
Val   _lib7_Gtk_set_arrow   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    gtk_arrow_set( GTK_ARROW(/*arrow*/w0), int_to_arrow_direction(/*arrow_direction_to_int arrow_direction*/i1), int_to_shadow_style(/*shadow_style_to_int shadow_style*/i2) );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_horizontal_box
 *
 * gtk-client.api        type:   (Session, Bool, Int)   ->   Widget
 * gtk-client-driver.api type:   (Session, Bool, Int) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_horizontal_box   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    int               b0 =                            GET_TUPLE_SLOT_AS_VAL( arg, 1) == HEAP_TRUE;
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_hbox_new ( /*homogeneous*/b0, /*spacing*/i1 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_vertical_box
 *
 * gtk-client.api        type:   (Session, Bool, Int)   ->   Widget
 * gtk-client-driver.api type:   (Session, Bool, Int) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_vertical_box   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    int               b0 =                            GET_TUPLE_SLOT_AS_VAL( arg, 1) == HEAP_TRUE;
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_vbox_new ( /*homogeneous*/b0, /*spacing*/i1 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_horizontal_button_box
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_horizontal_button_box   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_hbutton_box_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_vertical_button_box
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_vertical_button_box   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_vbutton_box_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_table
 *
 * gtk-client.api        type:    { session: Session,   rows: Int,   cols: Int,   homogeneous: Bool }   ->   Widget
 * gtk-client-driver.api type:   (Session, Int, Int, Bool) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_table   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    int               i0 =                            GET_TUPLE_SLOT_AS_INT( arg, 1);
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               b2 =                            GET_TUPLE_SLOT_AS_VAL( arg, 3) == HEAP_TRUE;

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_table_new ( /*rows*/i0, /*cols*/i1, /*homogeneous*/b2 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_event_box
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_event_box   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_event_box_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_image_from_file
 *
 * gtk-client.api        type:   (Session, String) -> Widget
 * gtk-client-driver.api type:   (Session, String) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_image_from_file   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    char*             s0 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 1));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_image_new_from_file( /*filename*/s0 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_horizontal_separator
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_horizontal_separator   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_hseparator_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_vertical_separator
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_vertical_separator   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_vseparator_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_layout_container
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_layout_container   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_layout_new (NULL, NULL);

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_layout_put
 *
 * gtk-client.api        type:    { session: Session,  layout: Widget,  kid: Widget,  x: Int,  y: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int, Int) -> Void
 */
Val   _lib7_Gtk_layout_put   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);

    gtk_layout_put( GTK_LAYOUT(/*layout*/w0), GTK_WIDGET(/*kid*/w1), /*x*/i2, /*y*/i3);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_layout_move
 *
 * gtk-client.api        type:    { session: Session, layout: Widget,  kid: Widget,  x: Int,  y: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int, Int) -> Void
 */
Val   _lib7_Gtk_layout_move   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);

    gtk_layout_move( GTK_LAYOUT(/*layout*/w0), GTK_WIDGET(/*kid*/w1), /*x*/i2, /*y*/i3);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_fixed_container
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_fixed_container   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_fixed_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_fixed_put
 *
 * gtk-client.api        type:    { session: Session, layout: Widget,  kid: Widget,  x: Int,  y: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int, Int) -> Void
 */
Val   _lib7_Gtk_fixed_put   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);

    gtk_fixed_put(   GTK_FIXED(/*layout*/w0), GTK_WIDGET(/*kid*/w1), /*x*/i2, /*y*/i3);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_fixed_move
 *
 * gtk-client.api        type:    { session: Session, layout: Widget,  kid: Widget,  x: Int,  y: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int, Int) -> Void
 */
Val   _lib7_Gtk_fixed_move   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);

    gtk_fixed_move(  GTK_FIXED(/*layout*/w0), GTK_WIDGET(/*kid*/w1), /*x*/i2, /*y*/i3);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_adjustment
 *
 * gtk-client.api        type:    { session: Session,   value: Float,   lower: Float,   upper: Float,   step_increment: Float,   page_increment: Float,   page_size: Float }   ->   Widget
 * gtk-client-driver.api type:   (Session, Float, Float, Float, Float, Float, Float) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_adjustment   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    double            f0 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 1)));
    double            f1 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 2)));
    double            f2 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 3)));
    double            f3 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 4)));
    double            f4 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 5)));
    double            f5 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 6)));

    int slot = find_free_widget_slot ();

    widget[slot] = (GtkWidget*) gtk_adjustment_new ( /*value*/f0, /*lower*/f1, /*upper*/f2, /*step_increment*/f3, /*page_increment*/f4, /*page_size*/f5 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_viewport
 *
 * gtk-client.api        type:    { session: Session, horizontal_adjustment: Null_Or(Widget), vertical_adjustment: Null_Or(Widget) } -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*)) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_viewport   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_viewport_new( GTK_ADJUSTMENT(/*null_or_widget_to_int horizontal_adjustment*/w0), GTK_ADJUSTMENT(/*null_or_widget_to_int vertical_adjustment*/w1) );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_scrolled_window
 *
 * gtk-client.api        type:    { session: Session, horizontal_adjustment: Null_Or(Widget), vertical_adjustment: Null_Or(Widget) } -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*)) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_scrolled_window   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_scrolled_window_new( GTK_ADJUSTMENT(/*null_or_widget_to_int horizontal_adjustment*/w0), GTK_ADJUSTMENT(/*null_or_widget_to_int vertical_adjustment*/w1) );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_horizontal_ruler
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_horizontal_ruler   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_hruler_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_vertical_ruler
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_vertical_ruler   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_vruler_new ();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_vertical_scrollbar
 *
 * gtk-client.api        type:   (Session, Null_Or(Widget)) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_vertical_scrollbar   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_vscrollbar_new( GTK_ADJUSTMENT(/*null_or_widget_to_int adjustment*/w0) );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_horizontal_scrollbar
 *
 * gtk-client.api        type:   (Session, Null_Or(Widget)) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_horizontal_scrollbar   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_hscrollbar_new( GTK_ADJUSTMENT(/*null_or_widget_to_int adjustment*/w0) );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_vertical_scale
 *
 * gtk-client.api        type:   (Session, Null_Or(Widget)) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_vertical_scale   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_vscale_new( GTK_ADJUSTMENT(/*null_or_widget_to_int adjustment*/w0) );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_horizontal_scale
 *
 * gtk-client.api        type:   (Session, Null_Or(Widget)) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_horizontal_scale   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_hscale_new( GTK_ADJUSTMENT(/*null_or_widget_to_int adjustment*/w0) );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_vertical_scale_with_range
 *
 * gtk-client.api        type:    { session: Session, min: Float, max: Float, step: Float } -> Widget
 * gtk-client-driver.api type:   (Session, Float, Float, Float) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_vertical_scale_with_range   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    double            f0 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 1)));
    double            f1 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 2)));
    double            f2 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 3)));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_vscale_new_with_range( /*min*/f0, /*max*/f1, /*step*/f2 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_horizontal_scale_with_range
 *
 * gtk-client.api        type:    { session: Session, min: Float, max: Float, step: Float } -> Widget
 * gtk-client-driver.api type:   (Session, Float, Float, Float) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_horizontal_scale_with_range   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    double            f0 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 1)));
    double            f1 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 2)));
    double            f2 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 3)));

    int slot = find_free_widget_slot ();

    widget[slot] = gtk_hscale_new_with_range( /*min*/f0, /*max*/f1, /*step*/f2 );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_drawing_area
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_drawing_area   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_drawing_area_new();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_pixmap
 *
 * gtk-client.api        type:    { session: Session, window: Widget, wide: Int, high: Int } -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_pixmap   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    int slot = find_free_widget_slot ();

    widget[slot] = (GtkWidget*) gdk_pixmap_new( GDK_DRAWABLE(/*window*/w0), /*wide*/i1, /*high*/i2, -1);

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_make_status_bar
 *
 * gtk-client.api        type:    Session -> Widget
 * gtk-client-driver.api type:   (Session) -> Int (*new Widget*)
 */
Val   _lib7_Gtk_make_status_bar   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)


    int slot = find_free_widget_slot ();

    widget[slot] = gtk_statusbar_new();

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_push_text_on_status_bar
 *
 * gtk-client.api        type:   (Session, Widget, Int, String) -> Int
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, String) -> Int
 */
Val   _lib7_Gtk_push_text_on_status_bar   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    char*             s2 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 3));

    int result = gtk_statusbar_push( GTK_STATUSBAR(/*status_bar*/w0), /*context*/i1, /*text*/s2);

    return TAGGED_INT_FROM_C_INT(result);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_pop_text_off_status_bar
 *
 * gtk-client.api        type:   (Session, Widget, Int) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
Val   _lib7_Gtk_pop_text_off_status_bar   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_statusbar_pop(GTK_STATUSBAR(/*status_bar*/w0), /*context*/i1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_remove_text_from_status_bar
 *
 * gtk-client.api        type:    { session: Session,   status_bar: Widget,   context: Int,   message: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Void
 */
Val   _lib7_Gtk_remove_text_from_status_bar   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    gtk_statusbar_remove( GTK_STATUSBAR(/*status_bar*/w0), /*context*/i1, /*message*/i2);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_pack_box
 *
 * gtk-client.api        type:    { session: Session,   box: Widget,   kid: Widget,   pack: Pack_From,   expand: Bool,   fill: Bool,   padding: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int, Bool, Bool, Int) -> Void
 */
Val   _lib7_Gtk_pack_box   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               b3 =                            GET_TUPLE_SLOT_AS_VAL( arg, 4) == HEAP_TRUE;
    int               b4 =                            GET_TUPLE_SLOT_AS_VAL( arg, 5) == HEAP_TRUE;
    int               i5 =                            GET_TUPLE_SLOT_AS_INT( arg, 6);

    if (!/*pack_to_int pack*/i2)  gtk_box_pack_start(   GTK_BOX(/*box*/w0), GTK_WIDGET(/*kid*/w1), /*expand*/b3, /*fill*/b4, /*padding*/i5 ); else gtk_box_pack_end( GTK_BOX(/*box*/w0), GTK_WIDGET(/*kid*/w1), /*expand*/b3, /*fill*/b4, /*padding*/i5 );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_menu_shell_append
 *
 * gtk-client.api        type:    { session: Session,   menu: Widget,   kid: Widget } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_menu_shell_append   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];

    gtk_menu_shell_append( GTK_MENU_SHELL(/*menu*/w0), GTK_WIDGET(/*kid*/w1));

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_menu_bar_append
 *
 * gtk-client.api        type:    { session: Session,   menu: Widget,   kid: Widget } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_menu_bar_append   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];

    gtk_menu_bar_append( GTK_MENU_SHELL(/*menu*/w0), GTK_WIDGET(/*kid*/w1));

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_append_text_to_combo_box
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Void
 */
Val   _lib7_Gtk_append_text_to_combo_box   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    gtk_combo_box_append_text( GTK_COMBO_BOX(/*combo_box*/w0), /*text*/s1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_option_menu_menu
 *
 * gtk-client.api        type:    { session: Session,   option_menu: Widget,   menu: Widget } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_set_option_menu_menu   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];

    gtk_option_menu_set_menu( GTK_OPTION_MENU(/*option_menu*/w0), GTK_WIDGET(/*menu*/w1) );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_text_tooltip_on_widget
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Void
 */
Val   _lib7_Gtk_set_text_tooltip_on_widget   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    gtk_widget_set_tooltip_text( GTK_WIDGET(/*widget*/w0), /*text*/s1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_ruler_metric
 *
 * gtk-client.api        type:   (Session, Widget, Metric) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
Val   _lib7_Gtk_set_ruler_metric   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_ruler_set_metric( GTK_RULER(/*ruler*/w0), int_to_metric(/*metric_to_int metric*/i1));

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_ruler_range
 *
 * gtk-client.api        type:    { session: Session,   ruler: Widget,   lower: Float,   upper: Float,   position: Float,   max_size: Float } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Float, Float, Float, Float) -> Void
 */
Val   _lib7_Gtk_set_ruler_range   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    double            f1 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 2)));
    double            f2 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 3)));
    double            f3 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 4)));
    double            f4 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 5)));

    gtk_ruler_set_range( GTK_RULER(/*ruler*/w0), /*lower*/f1, /*upper*/f2, /*position*/f3, /*max_size*/f4);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_scrollbar_policy
 *
 * gtk-client.api        type:    { session: Session,   window: Widget,   horizontal_scrollbar: Scrollbar_Policy,   vertical_scrollbar: Scrollbar_Policy } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Void
 */
Val   _lib7_Gtk_set_scrollbar_policy   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(/*window*/w0), int_to_policy(/*scrollbar_policy_to_int horizontal_scrollbar*/i1), int_to_policy(/*scrollbar_policy_to_int vertical_scrollbar*/i2) );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_draw_rectangle
 *
 * gtk-client.api        type:    { session: Session,   drawable: Widget,   gcontext: Widget,   filled:	Bool,   x: Int,   y: Int,   wide: Int,   high: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Bool, Int, Int, Int, Int) -> Void
 */
Val   _lib7_Gtk_draw_rectangle   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               b2 =                            GET_TUPLE_SLOT_AS_VAL( arg, 3) == HEAP_TRUE;
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);
    int               i4 =                            GET_TUPLE_SLOT_AS_INT( arg, 5);
    int               i5 =                            GET_TUPLE_SLOT_AS_INT( arg, 6);
    int               i6 =                            GET_TUPLE_SLOT_AS_INT( arg, 7);

    gdk_draw_rectangle(   GDK_DRAWABLE(/*drawable*/w0), GDK_GC(/*gcontext*/w1), /*filled*/b2, /*x*/i3, /*y*/i4, /*wide*/i5, /*high*/i6);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_draw_drawable
 *
 * gtk-client.api        type:    { session: Session,   drawable: Widget,   gcontext: Widget,   from: Widget,   from_x:	Int,   from_y: Int,   to_x: Int,   to_y: Int,   wide: Int,   high: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int(*Widget*), Int, Int, Int, Int, Int, Int) -> Void
 */
Val   _lib7_Gtk_draw_drawable   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

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
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_queue_redraw
 *
 * gtk-client.api        type:    { session: Session,   widget:	Widget,   x: Int,   y: Int,   wide: Int,   high: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int, Int, Int) -> Void
 */
Val   _lib7_Gtk_queue_redraw   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);
    int               i4 =                            GET_TUPLE_SLOT_AS_INT( arg, 5);

    gtk_widget_queue_draw_area( GTK_WIDGET(/*widget*/w0), /*x*/i1, /*y*/i2, /*wide*/i3, /*high*/i4);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_press_button
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_press_button   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_button_pressed(  GTK_BUTTON(/*widget*/w0) );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_release_button
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_release_button   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_button_released( GTK_BUTTON(/*widget*/w0) );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_click_button
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_click_button   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_button_clicked(  GTK_BUTTON(/*widget*/w0) );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_enter_button
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_enter_button   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_button_enter(    GTK_BUTTON(/*widget*/w0) );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_leave_button
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_leave_button   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_button_leave(    GTK_BUTTON(/*widget*/w0) );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_show_widget
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_show_widget   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_widget_show( /*widget*/w0 );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_show_widget_tree
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_show_widget_tree   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_widget_show_all( /*widget*/w0 );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_destroy_widget
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_destroy_widget   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_widget_destroy( /*widget*/w0 );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_emit_changed_signal
 *
 * gtk-client.api        type:   (Session, Widget)   -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_emit_changed_signal   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    g_signal_emit_by_name( GTK_OBJECT(/*widget*/w0), "changed");

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_pop_up_combo_box
 *
 * gtk-client.api        type:   (Session, Widget)   -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_pop_up_combo_box   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_combo_box_popup(   GTK_COMBO_BOX(/*widget*/w0));

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_pop_down_combo_box
 *
 * gtk-client.api        type:   (Session, Widget) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_pop_down_combo_box   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    gtk_combo_box_popdown( GTK_COMBO_BOX(/*widget*/w0));

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_combo_box_title
 *
 * gtk-client.api        type:   (Session, Widget, String)   -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Void
 */
Val   _lib7_Gtk_set_combo_box_title   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    gtk_combo_box_set_title( GTK_COMBO_BOX(/*widget*/w0), /*title*/s1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_window_title
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Void
 */
Val   _lib7_Gtk_set_window_title   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    gtk_window_set_title( GTK_WINDOW(/*window*/w0), /*title*/s1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_window_default_size
 *
 * gtk-client.api        type:   (Session, Widget, (Int,Int)) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Void
 */
Val   _lib7_Gtk_set_window_default_size   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    gtk_window_set_default_size( GTK_WINDOW(/*widget*/w0), /*wide*/i1, /*high*/i2);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_minimum_widget_size
 *
 * gtk-client.api        type:   (Session, Widget, (Int,Int)) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Void
 */
Val   _lib7_Gtk_set_minimum_widget_size   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    gtk_widget_set_size_request( GTK_WIDGET(/*widget*/w0), /*wide*/i1, /*high*/i2);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_border_width
 *
 * gtk-client.api        type:   (Session, Widget, Int) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
Val   _lib7_Gtk_set_border_width   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_container_set_border_width(GTK_CONTAINER(/*widget*/w0), /*width*/i1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_event_box_visibility
 *
 * gtk-client.api        type:   (Session, Widget, Bool) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Bool) -> Void
 */
Val   _lib7_Gtk_set_event_box_visibility   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               b1 =                            GET_TUPLE_SLOT_AS_VAL( arg, 2) == HEAP_TRUE;

    gtk_event_box_set_visible_window(GTK_EVENT_BOX(/*event_box*/w0),/*visibility*/b1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_widget_alignment
 *
 * gtk-client.api        type:    { session: Session, widget: Widget, x: Float, y: Float } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Float, Float) -> Void
 */
Val   _lib7_Gtk_set_widget_alignment   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    double            f1 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 2)));
    double            f2 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 3)));

    gtk_misc_set_alignment(GTK_MISC(/*widget*/w0), /*x*/f1, /*y*/f2);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_widget_events
 *
 * gtk-client.api        type:   (Session, Widget, List( Event_Mask )) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
Val   _lib7_Gtk_set_widget_events   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_widget_set_events( GTK_WIDGET(/*widget*/w0), int_to_event_mask(/*events_to_int events*/i1));

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_widget_name
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Void
 */
Val   _lib7_Gtk_set_widget_name   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    gtk_widget_set_name( GTK_WIDGET(/*widget*/w0), /*name*/s1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_label_justification
 *
 * gtk-client.api        type:   (Session, Widget, Justification) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
Val   _lib7_Gtk_set_label_justification   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_label_set_justify( GTK_LABEL(/*label*/w0), int_to_justification(/*justification_to_int justification*/i1));

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_label_line_wrapping
 *
 * gtk-client.api        type:   (Session, Widget, Bool) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Bool) -> Void
 */
Val   _lib7_Gtk_set_label_line_wrapping   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               b1 =                            GET_TUPLE_SLOT_AS_VAL( arg, 2) == HEAP_TRUE;

    gtk_label_set_line_wrap( GTK_LABEL(/*label*/w0), /*wrap_lines*/b1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_label_underlines
 *
 * gtk-client.api        type:   (Session, Widget, String) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), String) -> Void
 */
Val   _lib7_Gtk_set_label_underlines   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    char*             s1 =   HEAP_STRING_AS_C_STRING (GET_TUPLE_SLOT_AS_VAL( arg, 2));

    gtk_label_set_pattern( GTK_LABEL(/*label*/w0), /*underlines*/s1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_scale_value_position
 *
 * gtk-client.api        type:   (Session, Widget, Position_Type) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
Val   _lib7_Gtk_set_scale_value_position   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_scale_set_value_pos( GTK_SCALE(/*scale*/w0), int_to_position(/*position_to_int position*/i1));

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_draw_scale_value
 *
 * gtk-client.api        type:   (Session, Widget, Bool) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Bool) -> Void
 */
Val   _lib7_Gtk_set_draw_scale_value   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               b1 =                            GET_TUPLE_SLOT_AS_VAL( arg, 2) == HEAP_TRUE;

    gtk_scale_set_draw_value( GTK_SCALE(/*scale*/w0), /*draw_value*/b1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_get_scale_value_digits_shown
 *
 * gtk-client.api        type:   (Session, Widget) -> Int
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int
 */
Val   _lib7_Gtk_get_scale_value_digits_shown   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    int result = gtk_scale_get_digits( GTK_SCALE(/*scale*/w0) );

    return TAGGED_INT_FROM_C_INT(result);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_scale_value_digits_shown
 *
 * gtk-client.api        type:   (Session, Widget, Int)  -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
Val   _lib7_Gtk_set_scale_value_digits_shown   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_scale_set_digits( GTK_SCALE(/*scale*/w0), /*digits*/i1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_range_update_policy
 *
 * gtk-client.api        type:   (Session, Widget, Update_Policy) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
Val   _lib7_Gtk_set_range_update_policy   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_range_set_update_policy( GTK_RANGE(/*scale*/w0), /*policy*/int_to_range_update_policy(/*update_policy_to_int policy*/i1));

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_get_toggle_button_state
 *
 * gtk-client.api        type:   (Session, Widget) -> Bool
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Bool
 */
Val   _lib7_Gtk_get_toggle_button_state   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    int result = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(/*toggle_button*/w0) );

    return  result ? HEAP_TRUE : HEAP_FALSE;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_toggle_button_state
 *
 * gtk-client.api        type:   (Session, Widget, Bool) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Bool) -> Void
 */
Val   _lib7_Gtk_set_toggle_button_state   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               b1 =                            GET_TUPLE_SLOT_AS_VAL( arg, 2) == HEAP_TRUE;

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(/*toggle_button*/w0), /*state*/b1 != 0 );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_get_adjustment_value
 *
 * gtk-client.api        type:   (Session, Widget) -> Float
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Float
 */
Val   _lib7_Gtk_get_adjustment_value   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    double d = gtk_adjustment_get_value( GTK_ADJUSTMENT(/*adjustment*/w0) );

    return  make_float64(task, d );
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_adjustment_value
 *
 * gtk-client.api        type:   (Session, Widget, Float) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Float) -> Void
 */
Val   _lib7_Gtk_set_adjustment_value   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    double            f1 =        *(PTR_CAST(double*, GET_TUPLE_SLOT_AS_VAL( arg, 2)));

    gtk_adjustment_set_value( GTK_ADJUSTMENT(/*adjustment*/w0), /*value*/f1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_get_white_graphics_context
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*Widget*)
 */
Val   _lib7_Gtk_get_white_graphics_context   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    GtkWidget* widget = (GtkWidget*) /*widget*/w0->style->white_gc;

    int slot = get_widget_id( widget );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_get_black_graphics_context
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*Widget*)
 */
Val   _lib7_Gtk_get_black_graphics_context   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    GtkWidget* widget = (GtkWidget*) /*widget*/w0->style->black_gc;

    int slot = get_widget_id( widget );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_get_current_foreground_graphics_context
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*Widget*)
 */
Val   _lib7_Gtk_get_current_foreground_graphics_context   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    GtkWidget* widget = (GtkWidget*) w0->style->fg_gc[ GTK_WIDGET_STATE(/*widget*/w0) ];

    int slot = get_widget_id( widget );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_get_current_background_graphics_context
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*Widget*)
 */
Val   _lib7_Gtk_get_current_background_graphics_context   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    GtkWidget* widget = (GtkWidget*) w0->style->bg_gc[ GTK_WIDGET_STATE(/*widget*/w0) ];

    int slot = get_widget_id( widget );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_get_widget_window
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*Widget*)
 */
Val   _lib7_Gtk_get_widget_window   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    GtkWidget* widget = (GtkWidget*) /*widget*/w0->window;

    int slot = get_widget_id( widget );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_add_kid
 *
 * gtk-client.api        type:    { session: Session,   mom: Widget,   kid: Widget } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_add_kid   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];

    gtk_container_add( GTK_CONTAINER(/*mom*/w0), GTK_WIDGET(/*kid*/w1));

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_add_scrolled_window_kid
 *
 * gtk-client.api        type:    { session: Session,   window: Widget,   kid: Widget } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*)) -> Void
 */
Val   _lib7_Gtk_add_scrolled_window_kid   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];

    gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW(/*window*/w0), GTK_WIDGET(/*kid*/w1) );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_add_table_kid
 *
 * gtk-client.api        type:    { session: Session,   table: Widget,   kid: Widget,   left: Int,   right: Int,   top: Int,   bottom: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int, Int, Int, Int) -> Void
 */
Val   _lib7_Gtk_add_table_kid   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    GtkWidget*        w1  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 2) ];
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);
    int               i3 =                            GET_TUPLE_SLOT_AS_INT( arg, 4);
    int               i4 =                            GET_TUPLE_SLOT_AS_INT( arg, 5);
    int               i5 =                            GET_TUPLE_SLOT_AS_INT( arg, 6);

    gtk_table_attach_defaults( GTK_TABLE(/*table*/w0), GTK_WIDGET(/*kid*/w1), /*left*/i2, /*right*/i3, /*top*/i4, /*bottom*/i5 );

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_add_table_kid2
 *
 * gtk-client.api        type:    { session: Session,   table: Widget,   kid: Widget,   left: Int,   right: Int,   top: Int,   bottom: Int,   xoptions: List( Table_Attach_Option ),   yoptions: List( Table_Attach_Option ),   xpadding: Int,   ypadding: Int }   ->   Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int(*Widget*), Int, Int, Int, Int, Int, Int, Int, Int) -> Void
 */
Val   _lib7_Gtk_add_table_kid2   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

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
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_get_viewport_vertical_adjustment
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*Widget*)
 */
Val   _lib7_Gtk_get_viewport_vertical_adjustment   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    GtkWidget* widget = (GtkWidget*) gtk_viewport_get_vadjustment( GTK_VIEWPORT(/*viewport*/w0) );

    int slot = get_widget_id( widget );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_get_viewport_horizontal_adjustment
 *
 * gtk-client.api        type:   (Session, Widget) -> Widget
 * gtk-client-driver.api type:   (Session, Int(*Widget*)) -> Int (*Widget*)
 */
Val   _lib7_Gtk_get_viewport_horizontal_adjustment   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];

    GtkWidget* widget = (GtkWidget*) gtk_viewport_get_hadjustment( GTK_VIEWPORT(/*viewport*/w0) );

    int slot = get_widget_id( widget );

    return TAGGED_INT_FROM_C_INT(slot);
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_table_row_spacing
 *
 * gtk-client.api        type:    { session: Session, table: Widget, row: Int, spacing: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Void
 */
Val   _lib7_Gtk_set_table_row_spacing   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    gtk_table_set_row_spacing( GTK_TABLE(/*table*/w0), /*row*/i1, /*spacing*/i2);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_table_col_spacing
 *
 * gtk-client.api        type:    { session: Session, table: Widget, col: Int, spacing: Int } -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int, Int) -> Void
 */
Val   _lib7_Gtk_set_table_col_spacing   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);
    int               i2 =                            GET_TUPLE_SLOT_AS_INT( arg, 3);

    gtk_table_set_col_spacing( GTK_TABLE(/*table*/w0), /*col*/i1, /*spacing*/i2);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_table_row_spacings
 *
 * gtk-client.api        type:   (Session, Widget, Int) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
Val   _lib7_Gtk_set_table_row_spacings   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_table_set_row_spacings( GTK_TABLE(/*table*/w0), /*spacing*/i1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */


/* _lib7_Gtk_set_table_col_spacings
 *
 * gtk-client.api        type:   (Session, Widget, Int) -> Void
 * gtk-client-driver.api type:   (Session, Int(*Widget*), Int) -> Void
 */
Val   _lib7_Gtk_set_table_col_spacings   (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0  =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];
    int               i1 =                            GET_TUPLE_SLOT_AS_INT( arg, 2);

    gtk_table_set_col_spacings( GTK_TABLE(/*table*/w0), /*spacing*/i1);

    return HEAP_VOID;
#else
    extern char* no_gtk_support_in_runtime;
    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_plain_fun. */




/*  _lib7_Gtk_set_clicked_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_clicked_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "clicked", G_CALLBACK( run_clicked_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_pressed_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_pressed_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "pressed", G_CALLBACK( run_pressed_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_release_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_release_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "release", G_CALLBACK( run_release_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_enter_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_enter_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "enter", G_CALLBACK( run_enter_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_leave_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_leave_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "leave", G_CALLBACK( run_leave_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_activate_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_activate_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( GTK_MENU_ITEM(w0), "activate", G_CALLBACK( run_activate_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_destroy_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_destroy_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "destroy", G_CALLBACK( run_destroy_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_realize_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_realize_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "realize", G_CALLBACK( run_realize_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_button_press_event_callback : Session -> Widget -> Button_Event_Callback -> Void
 */
Val   _lib7_Gtk_set_button_press_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "button_press_event", G_CALLBACK( run_button_press_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_button_release_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_button_release_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "button_release_event", G_CALLBACK( run_button_release_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_scroll_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_scroll_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "scroll_event", G_CALLBACK( run_scroll_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_motion_notify_event_callback : Session -> Widget -> Motion_Event_Callback -> Void
 */
Val   _lib7_Gtk_set_motion_notify_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "motion_notify_event", G_CALLBACK( run_motion_notify_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_delete_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_delete_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "delete_event", G_CALLBACK( run_delete_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_expose_event_callback : Session -> Widget -> Expose_Event_Callback -> Void
 */
Val   _lib7_Gtk_set_expose_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "expose_event", G_CALLBACK( run_expose_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_key_press_event_callback : Session -> Widget -> Key_Event_Callback -> Void
 */
Val   _lib7_Gtk_set_key_press_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "key_press_event", G_CALLBACK( run_key_press_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_key_release_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_key_release_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "key_release_event", G_CALLBACK( run_key_release_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_enter_notify_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_enter_notify_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "enter_notify_event", G_CALLBACK( run_enter_notify_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_leave_notify_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_leave_notify_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "leave_notify_event", G_CALLBACK( run_leave_notify_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_configure_event_callback : Session -> Widget -> Configure_Event_Callback -> Void
 */
Val   _lib7_Gtk_set_configure_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "configure_event", G_CALLBACK( run_configure_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_focus_in_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_focus_in_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "focus_in_event", G_CALLBACK( run_focus_in_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_focus_out_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_focus_out_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "focus_out_event", G_CALLBACK( run_focus_out_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_map_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_map_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "map_event", G_CALLBACK( run_map_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_unmap_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_unmap_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "unmap_event", G_CALLBACK( run_unmap_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_property_notify_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_property_notify_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "property_notify_event", G_CALLBACK( run_property_notify_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_selection_clear_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_selection_clear_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "selection_clear_event", G_CALLBACK( run_selection_clear_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_selection_request_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_selection_request_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "selection_request_event", G_CALLBACK( run_selection_request_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_selection_notify_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_selection_notify_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "selection_notify_event", G_CALLBACK( run_selection_notify_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_proximity_in_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_proximity_in_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "proximity_in_event", G_CALLBACK( run_proximity_in_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_proximity_out_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_proximity_out_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "proximity_out_event", G_CALLBACK( run_proximity_out_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_client_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_client_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "client_event", G_CALLBACK( run_client_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_no_expose_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_no_expose_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "no_expose_event", G_CALLBACK( run_no_expose_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_window_state_event_callback : Session -> Widget -> Void_Callback -> Void
 */
Val   _lib7_Gtk_set_window_state_event_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "window_state_event", G_CALLBACK( run_window_state_event_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_toggled_callback : Session -> Widget -> Bool_Callback -> Void
 */
Val   _lib7_Gtk_set_toggled_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "toggled", G_CALLBACK( run_toggled_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */




/*  _lib7_Gtk_set_value_changed_callback : Session -> Widget -> Float_Callback -> Void
 */
Val   _lib7_Gtk_set_value_changed_callback (Task* task, Val arg)
{
#if (HAVE_GTK_2_0_GTK_GTK_H || HAVE_GTK_GTK_H)

    GtkWidget*        w0 __attribute__((unused)) =    (GtkWidget*)      widget[ GET_TUPLE_SLOT_AS_INT( arg, 1) ];        // '1' because 'arg' is a duple (session, widget).

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(w0), "value_changed", G_CALLBACK( run_value_changed_callback ), (void*)id );

    return TAGGED_INT_FROM_C_INT(id);
#else
    extern char* no_gtk_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_gtk_support_in_runtime, NULL);
#endif
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_driver_c_set_callback_fun. */


/* Do not edit this or preceding lines -- they are autogenerated by make-gtk-glue. */




// Code by Jeff Prothero: Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

