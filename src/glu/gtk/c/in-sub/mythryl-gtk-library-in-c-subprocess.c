// mythryl-gtk-library-in-c-subprocess.c -- a simple Gtk subprocess server for Mythryl.

// MOTIVATION
//
// Linking gtk directly into the main Mythryl executable
// is problematic for a whole spectrum of reasons:
//
//   o Gtk wants to own the program counter --
//     it expects us to permanently turn
//     control over to gtk_main for the duration
//     of the application run.
//
//     This is st00pid (no two such greedy libraries
//     can ever be linked into the same program) and
//     extremely inconvenient for nontrivial apps.
//
//     Putting gtk in a separate process lets the
//     main process go on its merry way, treating
//     the gui as just another sub-facilitie.
//
//   o Gtk is GPL, while the rest of Mythryl is
//     Berkley-style license.  Mixing the two in
//     a single process gives the worst of both.
//
//   o Gtk is in C, hence inherently prone to
//     memory leaks -- the de facto standard C
//     way to do garbage collection is to use
//     small processes which run briefly, recovering
//     all their leaked memory at process exit.
//
//     Putting Gtk in a subprocess lets us kill
//     and respawn it as necessary to deal with
//     memory leakage.
//
//   o Gtk is in C, hence inherently prone to
//     regular coredumps.  Keeping this kind of
//     code walled off from our Mythryl heap is
//     just sane software engineering.  As long
//     as we keep all critical state in Mythryl,
//     we can just respawn the Gtk subprocess
//     each time it coredumps, within reason.
//
//   o Gtk wants to do everything via C callbacks,
//     which is problematic in a single-image
//     solution, but works decently in a subprocess
//     where callbacks generate text output which
//     then triggers the Mythryl callback functions
//     via the pipe interpreter.
//
//   o Mythryl uses garbage collection, which
//     means processing stopping dead every now
//     and then for a tens to hundreds of
//     milliseconds.  For some interactive applications
//     such as CNC control, games, or multimedia editing,
//     this may be unacceptable:  Putting the time-critical
//     display and response loops in mythryl-gtk-library-in-c-subprocess can
//     work around this issue.

// OVERVIEW
//
// We implement a tiny Tcl-ish language which reads
// lines one by one from stdin, breaks them up into
// blank-separated words, interprets the first word
// as the name of a function, looks up that function
// in a trie, and calls it with the rest of the words
// on the line, pre-parsed into an (argc, argv) pair.
//
// Some conventions:
//
//  o  In general, we attempt where practical to write short,
//     complete lines, so that if input is available, code
//     can assume that reading until a '\n' is encountered
//     will not block.
//
//     ("Short" here thus means "smaller than the size of
//     a unix pipe buffer", which I believe can be safely
//     taken to be a minimum of 1K -- on Intel Linux, it
//     is most likely at least a 4K page.)
//
//  o  We read commands in the general format
//
//          12\n
//          foo bar zot\n
//
//     where '12' is the length of the following line
//     (including newline), 'foo' is the command verb,
//     and 'bar', 'zot' ... are arguments to the verb.
//
//     This format is intended to be human-readable in
//     simple cases for debugging purposes, while still
//     being simple and reliable to process.
//
//     Note that in general 'bar' or 'zot' may be a
//     string containing literal newlines (or anything
//     else), so while in most cases a command is
//     two lines, this may not be counted upon. 
//
//  o  In similar fashion, we read strings in the format "7"bloated":
//     length followed by contents, separated and surrounded by doublequotes.
//     This retains human readability while avoiding the need to insert
//     and remove backslashes -- string contents are sent verbatim.
//
//  o  On input, we expect each line to begin with a lower-case word.
//
//  o  On output, we start solicited return values with a lower-case word;
//     we signal unsolicited values like callback activations by lines
//     starting with an uppercase word.  (At the moment, WIDGET lines
//     are an exception, solicited but uppercase. XXX BUGGO FIXME.)
//
//  o  We handle just about any problem by printing a
//
//         FATAL <some explanatory text>
//
//     line to stdout and then doing exit(1).
//
//  o  We do not use stderr.
//     (Libraries we call might, however.)
//
//
//
//
// The glue library for accessing us from Mythryl is implemented in:
//
//     src/glu/gtk/src/gtk-client.api
//     src/glu/gtk/src/gtk-client-for-library-in-main-process.pkg



/*
###             "You can't teach an old C dog GNU tricks."
 */


#include "../../../../c/mythryl-config.h"

#include <stdio.h>	// For printf()...
#include <stdlib.h>	// For exit(), atoi(), strtod ...

#if HAVE_STRING_H
#include <string.h>	// For strlen()...
#endif

#include <ctype.h>	// For isdigit()...

#if HAVE_SYS_TYPES_H
#include <sys/types.h>	// For getpid()
#endif

#if HAVE_UNISTD_H
#include <unistd.h>	// For getpid(), STDIN_FILENO...
#endif

#include <gtk/gtk.h>

FILE* log_fd = 0;

static char text_buf[ 1024 ];

static void   moan_and_die   (void)   {
    //        ============
    //
    printf( "src/glu/gtk/c/in-sub/mythryl-gtk-library-in-c-subprocess.c:  Fatal error:  %s  exit(1)ing.\n", text_buf );
    fprintf( log_fd, "FATAL: %s   exit(1)ing.\n", text_buf );
    fclose(  log_fd );
    exit(1);
}

static void
check_argc( char* name, int need, int argc )
{
    if (need == argc) return;
    sprintf(text_buf, "%s: argc should be %d, was %d.\n", name, need, argc); moan_and_die();
}


static void
open_logfile (void)
{
    char filename[ 128 ];

    sprintf (filename, "mythryl-gtk-library-in-c-subprocess-%d.log~", getpid());

    log_fd = fopen( filename, "w" );

    if (!log_fd) {
	sprintf(text_buf, "Unable to create logfile %s?!\n", filename);
	moan_and_die();
    }
}




static int
find_free_callback_id ()
{
    static int next_callback_id = 1;
    //
    return next_callback_id++;
}




#define MAX_WIDGETS 1024
static GtkWidget* widget[ MAX_WIDGETS ];			// XXX BUGGO FIXME Should expand in size as needed.

static int
find_free_widget_slot(void)
{
    int i;
    for (i = 1; i < MAX_WIDGETS; ++i) {				// We reserve 0 as an always-invalid analog to NULL.
        if (!widget[i])  return i;
    }
    sprintf(text_buf, "find_free_widget_slot: All slots full.");
    moan_and_die();
    return 0;							// Can't happen, but keeps gcc quiet. */
}

static int
find_widget_id( GtkWidget* query_widget )
{   int  i;
    for (i = 1;   i < MAX_WIDGETS;   ++i) {

        if (widget[i] == query_widget)   return i;
    }
    return 0;
}

static int
get_widget_id( GtkWidget* query_widget )
{
    int slot = find_widget_id( query_widget );

    if (!slot) {
	 slot = find_free_widget_slot ();

	 widget[slot] = (GtkWidget*) query_widget;
    }

    return slot;
}

static GtkWidget*
widget_arg( int iargc, unsigned char** argv, int ii)
{
    unsigned int argc = (unsigned int) iargc;
    unsigned int i    = (unsigned int) ii;

    if (i >= argc) {
        sprintf (text_buf, "widget_arg: Attempted to use arg %d from length-%d argv.\n", ii, iargc); moan_and_die();
    }
    {   unsigned char*  id =  argv[ ii ];
	//
        if (id[0] != 'W'
        ||  id[1] != 'I'
        ||  id[2] != 'D'
        ||  id[3] != 'G'
        ||  id[4] != 'E'
        ||  id[5] != 'T'
        ||  !isdigit( id[6] )
	){
	  sprintf (text_buf, "FATAL widget_arg bad widget id '%s'\n", id); moan_and_die();
        }

        {   int widget_id = atoi( (char*) (id + 6) );
	    if (widget_id >= MAX_WIDGETS) {
	        sprintf (text_buf, "widget_arg: widget id '%s' out of range (max is %d).\n", id, MAX_WIDGETS); moan_and_die();
	    }
            {   GtkWidget* result = widget[ widget_id ];
	        //
	        if (!result && widget_id) {
		    sprintf (text_buf, "widget_arg: widget id '%s' is not assigned.\n", id); moan_and_die();
	        }

                return result;
            }
        }
    }
}

static char*
string_arg( int iargc, unsigned char** argv, int ii)
{
    unsigned int argc = (unsigned int) iargc;
    unsigned int i    = (unsigned int) ii;

    if (i >= argc) {
        sprintf (text_buf, "string_arg: Attempted to use arg %d from length-%d argv.\n", ii, iargc); moan_and_die();
    }
    return (char*) argv[ ii ];
}

static double
double_arg( int iargc, unsigned char** argv, int ii)
{
    unsigned int argc = (unsigned int) iargc;
    unsigned int i    = (unsigned int) ii;

    if (i >= argc) {
        sprintf (text_buf, "double_arg: Attempted to use arg %d from length-%d argv.\n", ii, iargc); moan_and_die();
    }
    {   double result = strtod( (char*) argv[ii], NULL );

        return result;
    }
}

static int
int_arg( int iargc, unsigned char** argv, int ii)
{
    unsigned int argc = (unsigned int) iargc;
    unsigned int i    = (unsigned int) ii;

    if (i >= argc) {
        sprintf (text_buf, "int_arg: Attempted to use arg %d from length-%d argv.\n", ii, iargc); moan_and_die();
    }
    {   int result = atoi( (char*) argv[ii] );

        return result;
    }
}

static int
bool_arg( int iargc, unsigned char** argv, int ii)
{
    unsigned int argc = (unsigned int) iargc;
    unsigned int i    = (unsigned int) ii;

    if (i >= argc) {
        sprintf (text_buf, "bool_arg: Attempted to use arg %d from length-%d argv.\n", ii, iargc); moan_and_die();
    }
    {   char* arg = (char*) argv[ii];
	//
        if (!strcmp( arg, "TRUE"     ))  return TRUE;
        if (!strcmp( arg, "FALSE"    ))  return FALSE;

        sprintf (text_buf, "bool_arg: boolean value '%s' is not supported.\n", arg);
        moan_and_die();
    }
    return 0;								// Cannot execute; keeps gcc quiet.
}


// These are operationally identical, but for debugging
// it is convenient to log them separately:
//
static void   run_clicked_callback( GtkWidget* widget, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (click)\n",   (int) user_data); fflush( log_fd ); }
static void   run_pressed_callback( GtkWidget* widget, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (press)\n",   (int) user_data); fflush( log_fd ); }
static void   run_enter_callback  ( GtkWidget* widget, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (enter)\n",   (int) user_data); fflush( log_fd ); }
static void   run_leave_callback  ( GtkWidget* widget, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (leave)\n",   (int) user_data); fflush( log_fd ); }
static void run_release_callback  ( GtkWidget* widget, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (release)\n", (int) user_data); fflush( log_fd ); }

static void run_activate_callback(GtkWidget* widget, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (activate)\n",(int) user_data); fflush( log_fd ); }

static gboolean run_destroy_callback			( GtkObject* object, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (destroy)\n", (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_realize_callback			( GtkWidget* widget, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (realize)\n", (int) user_data); fflush( log_fd ); return TRUE; }

static gboolean run_button_press_event_callback		( GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
    char buf[ 4096 ];

    int type = 0;
    switch (event->type) {
    case GDK_BUTTON_PRESS:  type = 1; break;
    case GDK_2BUTTON_PRESS: type = 2; break;
    case GDK_3BUTTON_PRESS: type = 3; break;
    default:
	sprintf (text_buf, "run_button_press_event: type value '%d' is not supported.\n", event->type); moan_and_die();
    }

    {   int    widget_id  = get_widget_id( (GtkWidget*) event->window );
	int    button     =                             event->button;
	double x          =                             event->x;
	double y          =                             event->y;
        int    time       =                             event->time;
        int    modifiers  =                             event->state;

	sprintf(buf, "%d %d %d %f %f %d %d", (int) user_data, widget_id, button, x, y, time, modifiers);

	printf (             "BUTTON_CALLBACK%s\n", buf); fflush( stdout );
	fprintf(log_fd,"SENT: BUTTON_CALLBACK%s\n", buf); fflush( log_fd );

	return TRUE;
    }
}

static gboolean run_motion_notify_event_callback	( GtkWidget* widget, GdkEventMotion* event, gpointer user_data)
{
    char buf[ 4096 ];

    int    slot      =  get_widget_id ((GtkWidget*) (event->window));

    int    time      =  event->time;
    double x         =  event->x;
    double y         =  event->y;
    int    modifiers =  event->state; 
    int    is_hint   =  event->is_hint;

    sprintf( buf, "%d %d %d %f %f %d %d", (int) user_data, slot, time, x, y, modifiers, is_hint);

    printf (             "MOTION_CALLBACK%s\n",                       buf); fflush( stdout );
    fprintf(log_fd,"SENT: MOTION_CALLBACK%s (motion_notify_event)\n", buf); fflush( log_fd );

    return TRUE;
}

static gboolean run_expose_event_callback		( GtkWidget* w, GdkEventExpose* event, gpointer user_data)
{
    char buf[ 4096 ];

    int    count     =  event->count;
    int    area_x    =  event->area.x;
    int    area_y    =  event->area.y;
    int    area_wide =  event->area.width;
    int    area_high =  event->area.height;

    int    slot      =  find_widget_id( w );

    sprintf( buf, "%d %d %d %d %d %d %d", (int) user_data, slot, count, area_x, area_y, area_wide, area_high);

    printf (             "EXPOSE_CALLBACK%s\n",                buf); fflush( stdout );
    fprintf(log_fd,"SENT: EXPOSE_CALLBACK%s (expose_event)\n", buf); fflush( log_fd );

    return TRUE;
}

static gboolean run_configure_event_callback		( GtkWidget* widget, GdkEventConfigure* event, gpointer user_data)
{
    char buf[ 4096 ];

    int    x    =  event->x;
    int    y    =  event->y;
    int    wide =  event->width;
    int    high =  event->height;

    int    slot      =  find_widget_id( widget );

    sprintf( buf, "%d %d %d %d %d %d", (int) user_data, slot, x, y, wide, high);

    printf (             "CONFIGURE_CALLBACK%s\n",                    buf); fflush( stdout );
    fprintf(log_fd,"SENT: CONFIGURE_CALLBACK%s (configure_event)\n",  buf); fflush( log_fd );
    return TRUE;
}

static gboolean run_key_press_event_callback		( GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
    char buf[ 4096 ];

    int  key       =  event->keyval;
    int  keycode   =  event->hardware_keycode;
    int  time      =  event->time;
    int  modifiers =  event->state;

    sprintf( buf, "%d %d %d %d %d", (int) user_data, key, keycode, time, modifiers);

                 printf ("CALLBACK%s\n",                   buf); fflush( stdout );
    fprintf(log_fd,"SENT: CALLBACK%s (key_press_event)\n", buf); fflush( log_fd );
    return TRUE;
}

static gboolean run_button_release_event_callback	(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (button_release_event)\n",    (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_scroll_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (scroll_event)\n",            (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_delete_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (delete_event)\n",            (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_key_release_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (key_release_event)\n",       (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_enter_notify_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (enter_notify_event)\n",      (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_leave_notify_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (leave_notify_event)\n",      (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_focus_in_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (focus_in_event)\n",          (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_focus_out_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (focus_out_event)\n",         (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_map_event_callback			(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (map_event)\n",               (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_unmap_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (unmap_event)\n",             (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_property_notify_event_callback	(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (property_notify_event)\n",   (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_selection_clear_event_callback	(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (selection_clear_event)\n",   (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_selection_request_event_callback	(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (selection_request_event)\n", (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_selection_notify_event_callback	(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (selection_notify_event)\n",  (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_proximity_in_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (proximity_in_event)\n",      (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_proximity_out_event_callback	(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (proximity_out_event)\n",     (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_client_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (client_event)\n",            (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_no_expose_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (no_expose_event)\n",         (int) user_data); fflush( log_fd ); return TRUE; }
static gboolean run_window_state_event_callback		(GtkWidget* widget, GdkEvent* event, gpointer user_data) {  printf ("CALLBACK%d\n", (int) user_data); fflush( stdout ); fprintf(log_fd,"SENT: CALLBACK%d (window_state_event)\n",      (int) user_data); fflush( log_fd ); return TRUE; }

// This one returns a boolean value:
//
static void
run_toggled_callback ( GtkToggleButton* widget, gpointer user_data)
{
    gboolean is_set = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );

    printf (             "BOOL_CALLBACK%d %s\n",           (int) user_data, is_set ? "TRUE" : "FALSE");    fflush( stdout );
    fprintf(log_fd,"SENT: BOOL_CALLBACK%d %s (toggled)\n", (int) user_data, is_set ? "TRUE" : "FALSE");    fflush( log_fd );
}

// This one returns a double value:
//
static void
run_value_changed_callback ( GtkAdjustment* adjustment, gpointer user_data)
{
    double value = gtk_adjustment_get_value( GTK_ADJUSTMENT(adjustment) );

    printf (             "FLOAT_CALLBACK%d %f\n",                (int) user_data, value);    fflush( stdout );
    fprintf(log_fd,"SENT: FLOAT_CALLBACK%d %f (value_change)\n", (int) user_data, value);    fflush( log_fd );
}



static int    main_argc;
static char** main_argv;

// We interpret input lines from our superprocess
// by taking the first blank-delimited word on the
// line as naming a function, and calling that function
// with the rest of the words on the line as arguments
// (in the usual (argc, argv) format).
//
// We use a trie to map words to functions.
// A hashtable would be more conventional, but
// a trie is simpler and faster, and the wasted
// space is inconsequential.
//
// Here we implement the trie in question.

/*
###          ``As defined by me, nearly 50 years ago, it is properly pronounced
###           "tree" as in the word "retrieval". At least that was my intent when
###            I gave it the name "Trie". The idea behind the name was to combine
###            reference to both the structure (a tree structure) and a major
###            purpose (data storage and retrieval).''
###
###                                           --  Ed Fredkin, 31 July 2008
*/

typedef void (*Trie_Fn) ( int argc, unsigned char** argv );

typedef struct _trie_node {
    //
    struct _trie_node* child  [ 256 ];		// Hey, RAM is cheap, right? :)
    Trie_Fn            trie_fn[ 256 ];
    //
} Trie_Node;    

Trie_Node* trie = NULL;

static Trie_Node*
make_trie_node (void)
{
    Trie_Node* result
        =
        (Trie_Node*)
        malloc(   sizeof( Trie_Node )   );

    if (!result) {  sprintf( text_buf, "make_trie_node: Couln't allocate node." ); moan_and_die(); }

    for (int i = 256; i --> 0; ) {
	//
	result->child  [i] =  NULL;
	result->trie_fn[i] =  NULL;
    }

    return result;
}

static Trie_Fn
get_trie( Trie_Node*trie, unsigned char* name )
{
    if (!trie)  return NULL;
    if (!name[1]) return trie->trie_fn[name[0]];
    return get_trie( trie->child[ name[0] ], name+1 );
}

static void
set_trie_u( Trie_Node*trie, unsigned char* name, Trie_Fn trie_fn )
{
    if (!name[1]) {
        trie->trie_fn[name[0]] = trie_fn;
        return;
    }

    if (!trie->child[ name[0] ]) {
         trie->child[ name[0] ] = make_trie_node();
    }

    set_trie_u( trie->child[ name[0] ], name+1, trie_fn );
}

static int duplicate_trie_entries = 0;

static void
set_trie( Trie_Node*trie, char* name, Trie_Fn trie_fn )
{
    // Check for duplicates:
    //
    if (get_trie( trie, (unsigned char*)name )) {
	//
        fprintf(stderr, "Bug: Duplicate trie entry for '%s'\n", name );    fflush( stderr );
        fprintf(log_fd, "Bug: Duplicate trie entry for '%s'\n", name );    fflush( log_fd );
	++duplicate_trie_entries;
	return;
    }

    set_trie_u( trie, (unsigned char*) name, trie_fn);
}

static void
do_test( int argc, unsigned char** argv )
{
    printf ("TEST argc d=%d\n", argc);								fflush(stdout);

    for (int i = 0; i < argc; ++i) {
	//
	printf ("ARG argv[%d] s='%s' (len d=%d)\n", i, argv[i], strlen( (char*) (argv[i])) );	fflush(stdout);
    }
}

static void
do_quit_eventloop( int argc, unsigned char** argv )
{
    // This call will result in
    // the (innermost call of)
    // gtk_main() returning:
    //
    gtk_main_quit();
}

static void
do_init( int argc, unsigned char** argv )
{
    check_argc( "do_init", 0, argc );

    if (!gtk_init_check( &main_argc, &main_argv )) {
	//
	sprintf( text_buf, "do_init: failed to initialize GUI support.");
	moan_and_die();
    } else {
puts("NB: gtk_init_check returned true\n"); fflush(stdout);
    }
}

static int
int_to_arrow_direction (int arrow_direction)
{
    switch (arrow_direction) {
    case 0:  return GTK_ARROW_UP;    
    case 1:  return GTK_ARROW_DOWN;    
    case 2:  return GTK_ARROW_LEFT;    
    case 3:  return GTK_ARROW_RIGHT;
    default:
        sprintf( text_buf, "int_to_arrow_direction: bad arg %d.", arrow_direction );
        moan_and_die();
    }
    return 0;			// Just to quiet compilers.
}

static int
int_to_shadow_style (int shadow_style)
{
    switch (shadow_style) {
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

static int
int_to_policy (int policy)
{
    switch (policy) {
    case 0:  return GTK_POLICY_AUTOMATIC;
    case 1:  return GTK_POLICY_ALWAYS;
    default:
      sprintf( text_buf, "int_to_policy: bad arg %d.", policy );
      moan_and_die();
    }
    return 0;			// Just to quiet compilers.
}

static int int_to_event_mask( int i1 ) {
    //
    int mask =  0;
    //
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
    //
    return i1;
}

static int   int_to_position   (int i1)   {
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

    return i1;
}

static int   int_to_metric   (int i1)   {
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

    return i1;
}

static int int_to_range_update_policy( int i1 ) {
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
//  -> build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'
//     -> build_fun_header_for_'mythryl_xxx_library_in_c_subprocess_c'
//      + build_fun_arg_loads_for_'mythryl_xxx_library_in_c_subprocess_c'
//       							# Optionally invokes new_widget_custom_body_plain_fun_subprocess
//                                                              # or                     widget_custom_body_plain_fun_subprocess,
//                                                              # from src/glu/gtk/sh/make-gtk-glue
// 
// Paragraphs like
//     build-a: callback-fn
//     fn-name:
//     fn-type:
//     lowtype:
// drive the code-build path
//   mlb::BUILD_A ("callback-fn", build_callback_function)			# In src/glu/gtk/sh/make-gtk-glue
//   ->  build_callback_function						# In src/glu/gtk/sh/make-gtk-glue
//       ->  build_set_callback_fn_for_'mythryl_xxx_library_in_c_subprocess_c'	# In src/glu/gtk/sh/make-gtk-glue
//           ->  r.to_mythryl_xxx_library_in_c_subprocess_c_funs		# In src/lib/make-library-glue/make-library-glue.pkg
//
/* Do not edit this or following lines -- they are autobuilt.  (patchname="functions") */

static void
do__set_value_changed_callback( int argc, unsigned char** argv )
{
    check_argc( "do__set_value_changed_callback", 1, argc );

    GtkWidget* widget    =  widget_arg( argc, argv, 0 );

    int id   =  find_free_callback_id ();

    g_signal_connect( G_OBJECT(widget), "value_changed", G_CALLBACK( run_value_changed_callback ), (void*)id );

     printf(              "set_value_changed_callback%d\n", id );      fflush( stdout );
    fprintf(log_fd, "SENT: set_value_changed_callback%d\n", id );      fflush( log_fd );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/gtk/etc/construction.plan.*/
/* Do not edit this or preceding lines -- they are autobuilt. */
/////////////////////////////////////////////////////////////////////////////////////



static void
do_make_dialog( int argc, unsigned char** argv )
{
    check_argc( "do_make_dialog", 0, argc );

    /* Anyone creating a dialog is almost certainly
     * going to immediately need the dialog's vbox
     * and action area, so we save round trips by
     * returning them here in the initial make call:
     */
    {   int dialog;
        int vbox;
	int action_area;

	dialog      = find_free_widget_slot ();   widget[dialog]      = gtk_dialog_new();
	vbox        = find_free_widget_slot ();   widget[vbox]        = GTK_DIALOG( widget[dialog] )->vbox;
	action_area = find_free_widget_slot ();   widget[action_area] = GTK_DIALOG( widget[dialog] )->action_area;

	 printf(              "make_dialogWIDGET%d WIDGET%d WIDGET%d\n", dialog, vbox, action_area);	fflush( stdout );
	fprintf(log_fd, "SENT: make_dialogWIDGET%d WIDGET%d WIDGET%d\n", dialog, vbox, action_area);	fflush( log_fd );
    }
}

static void
do_get_window_pointer( int argc, unsigned char** argv )
{
    check_argc( "do_get_viewport_vertical_adjustment", 1, argc );

    {	GtkWidget* w0 =  widget_arg( argc, argv, 0 );	/* window */

        int             x;
        int             y;
        GdkModifierType modifiers;

/*        GdkWindow* result_window = */  gdk_window_get_pointer (GDK_WINDOW(w0), &x, &y, &modifiers); 

	printf(               "get_window_pointer%d %d %d\n", x, y, modifiers);	fflush( stdout );
	fprintf(log_fd, "SENT: get_window_pointer%d %d %d\n", x, y, modifiers);	fflush( log_fd );
    }
}



static void
do_get_widget_allocation( int argc, unsigned char** argv )
{
    char buf[ 4096 ];

    check_argc( "do_get_widget_allocation", 1, argc );

    {   GtkWidget* w0  =  widget_arg( argc, argv, 0 );

        w0 = GTK_WIDGET( w0 );		/* Verify user gave us something appropriate. */

        int x    =  w0->allocation.x;
        int y    =  w0->allocation.y;
        int wide =  w0->allocation.width;
        int high =  w0->allocation.height;

        sprintf( buf, "%d %d %d %d", x, y, wide, high);

	printf(               "get_widget_allocation%s\n", buf);	fflush( stdout );
	fprintf(log_fd, "SENT: get_widget_allocation%s\n", buf);	fflush( log_fd );
    }
}

static void
do_unref_object( int argc, unsigned char** argv )
{
    check_argc( "do_unref_object", 1, argc );

    {   GtkWidget* w0    =  widget_arg( argc, argv, 0 );

	g_object_unref( G_OBJECT( w0 ) );

        widget[ get_widget_id( w0 ) ] = 0;
    }
}

/* Callback which GTK+ will call whenever there is
 * input available on stdin, so that we can continue
 * being responsive to commands from our superprocess
 * even after calling gtk_main(), which hogs control
 * of the program counter forever. :(
 *
 * We don't need or want any of the arguments to this
 * function, but they are forced on us by gtk_input_add(),
 * and they do no harm:
 */
static void  read_eval_print_one_stdin_command  (void);	/* Forward declaration. */
static void
stdin_callback (
    gpointer          data,		/* Always NULL,		  always ignored.	*/
    gint              source,		/* Always STDIN_FILENO,   always ignored.	*/
    GdkInputCondition condition		/* Always GDK_INPUT_READ, always ignored.	*/
) {
    
    fprintf(log_fd, "stdin_callback/TOP...\n");    fflush( log_fd );

    read_eval_print_one_stdin_command ();

    fprintf(log_fd, "stdin_callback/DONE.\n");     fflush( log_fd );
}

static void
do_run_eventloop_once( int argc, unsigned char** argv )
{
    check_argc( "do_run_eventloop_once", 0, argc );

    {   int block_until_event = bool_arg( argc, argv, 0 );

        int quit_called = gtk_main_iteration_do( block_until_event );	/* http://www.gtk.org/api/2.6/gtk/gtk-General.html#gtk-main-iteration-do
                                                                         */
	printf(                "run_eventloop_once%d\n",quit_called);    fflush( stdout );
	fprintf( log_fd, "SENT: run_eventloop_once%d\n",quit_called);    fflush( log_fd );
    }
}

static void
do_run_eventloop_indefinitely( int argc, unsigned char** argv )
{
fprintf(log_fd, "do_run_eventloop_indefinitely/TOP...\n");    fflush( log_fd );
    check_argc( "do_run_eventloop_indefinitely", 0, argc );

    /* Before we call gtk_main() -- which won't return
     * until gtk_main_quit() is called, if ever -- we
     * have to set up logic so that we can continue
     * to read and respond to commands from our
     * super-process:
     */
fprintf(log_fd, "do_run_eventloop_indefinitely/II...\n");    fflush( log_fd );
    gdk_input_add(			/* Ask GTK+ to watch ...					*/
        STDIN_FILENO,			/* ... the stdin file descriptor, and... 			*/
	GDK_INPUT_READ, 		/* ... whenever there is input available to be read on it...	*/
	stdin_callback,			/* ... to call this function ...				*/
	NULL				/* ... with this (ignored) argument.				*/
    );

fprintf(log_fd, "do_run_eventloop_indefinitely/III...\n");    fflush( log_fd );
    gtk_main ();
fprintf(log_fd, "do_run_eventloop_indefinitely/DONE.\n");    fflush( log_fd );

    printf(                "EVENTLOOP_DONE\n");    fclose( stdout );
    fprintf( log_fd, "SENT: EVENTLOOP_DONE\n");    fclose( log_fd );

    exit(0);
}

static void
init  (void)
{
    open_logfile ();

    /* Initialize our widget array:
    */
    {   int i;
        for (i = MAX_WIDGETS;  i --> 0;  )  widget[i] = NULL;
    }

    /* Initialize our verb trie:
    */
    trie = make_trie_node();

    /* Define the set of supported input line verbs:
    */

    set_trie( trie, "make_dialog",				do_make_dialog					);
    set_trie( trie, "get_window_pointer",			do_get_window_pointer				);
    set_trie( trie, "unref_object",				do_unref_object					);
    set_trie( trie, "quit_eventloop",				do_quit_eventloop				);
    set_trie( trie, "init",					do_init						);
    set_trie( trie, "run_eventloop_indefinitely",		do_run_eventloop_indefinitely			);
    set_trie( trie, "run_eventloop_once",			do_run_eventloop_once				);
    set_trie( trie, "test",					do_test						);
    set_trie( trie, "get_widget_allocation",			do_get_widget_allocation			);

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
    //     -> build_trie_entry_for_'mythryl_xxx_library_in_c_subprocess_c'
    //        -> to_mythryl_xxx_library_in_c_subprocess_c_trie
    // 
    // Paragraphs like
    //     build-a: callback-fn
    //     fn-name:
    //     fn-type:
    //     lowtype:
    // drive the code-build path
    //   mlb::BUILD_A ("callback-fn", build_callback_function)				# In src/glu/gtk/sh/make-gtk-glue
    //   ->  build_callback_function							# In src/glu/gtk/sh/make-gtk-glue
    //       ->  r.build_trie_entry_for_'mythryl_xxx_library_in_c_subprocess_c'		# In src/lib/make-library-glue/make-library-glue.pkg
    //
    /* Do not edit this or following lines -- they are autobuilt.  (patchname="table") */
    set_trie( trie, "set_value_changed_callback",                 do__set_value_changed_callback                );
    /* Do not edit this or preceding lines -- they are autobuilt. */
    /////////////////////////////////////////////////////////////////////////////////////

    if (duplicate_trie_entries) {
        fprintf(stderr, "%d duplicate trie entries.\n", duplicate_trie_entries );    fflush( stderr );
        fprintf(log_fd, "%d duplicate trie entries.\n", duplicate_trie_entries );    fflush( log_fd );
        exit(1);
    }
}  

#define MAX_LINE_LENGTH 8192
static unsigned char  line_buf[ MAX_LINE_LENGTH + 4 ];		/* + 4 is cheap insurance. :) */
static unsigned char* line_end;

static void
read_one_line (void)
{
    int c;
    line_end = line_buf;
    while ((c = getchar()) != '\n') {

        if (c == EOF)   { sprintf( text_buf, "read_one_line: EOF on input." ); moan_and_die(); }

        if (line_end - line_buf >= MAX_LINE_LENGTH) {
	    sprintf( text_buf, "read_one_line: Line greater than MAX_LINE_LENGTH." );  moan_and_die();
        }

	*line_end++ = c;
    }

    *line_end = '\0'; 

    if (!isdigit( line_buf[0] )) {

      fprintf( log_fd, "read: %s\n", line_buf );	fflush( log_fd );

    } else {
	/* For the moment, ignore lines starting with a digit:
	*/ 

        int line_length
            =
	    atoi( (char*) line_buf );
  
        if (line_length > MAX_LINE_LENGTH) {
	    sprintf( text_buf, "read_one_line: Announced line length greater than MAX_LINE_LENGTH: %d", line_length);
            moan_and_die();
        }

        {   int  i;
	    for (i = 0;  i < line_length;  ++i) {

		line_buf[i] = getchar();
	    }
	}

        if (line_buf[ line_length - 1 ]  !=  '\n') {
	    sprintf( text_buf, "read_one_line: Command ends with %c instead of newline?!", line_buf[ line_length - 1 ]);
            moan_and_die();
        }
        fputs( "read: ", log_fd );				fflush( log_fd );
        fwrite( line_buf, sizeof(char), line_length, log_fd );	fflush( log_fd );

        /* Drop the terminal newline: */
        --line_length;

        line_end =  &line_buf[ line_length ];
       *line_end = '\0'; 
    }
}

#define MAX_ARGC (MAX_LINE_LENGTH >> 3)
static int argc;
static unsigned char* argv[ MAX_ARGC ];

/* Skip over sequence of blanks between commandline words.
 * We should probably just require them to be a single
 * blank, and simplify this logic:
 */
static unsigned char*
eat_blanks( unsigned char** pp )
{
    unsigned char* start = *pp;
    unsigned char* p     = start;

    for (;;) {
        if (p >= line_end)   return NULL;

        if (*p == ' ') ++p;
        else           break;
    }

    *pp = p;
    return start;
}

/* Scan one quoted string from superprocess command line.
 * It will have a format like  "7"bloated":
 * length followed by contents, delimited by doublequotes.
 */
static unsigned char*
eat_doublequote( unsigned char** pp )
{
    unsigned char* start = *pp;
    unsigned char* p     = start;

    if (*p++ != '"') {
        sprintf( text_buf, "eat_doublequote: Initial character was '%c'?!", *p );
	moan_and_die();
    }

    if (!isdigit(*p)) {
        sprintf( text_buf, "eat_doublequote: Second character was '%c'?!", *p );
	moan_and_die();
    }

    {   int quote_len = atoi( (char*) p );

        while (isdigit(*p)) ++p;

	if (*p++ != '"') {
	    sprintf( text_buf, "eat_doublequote: Length field ended by '%c'?!", *p );
	    moan_and_die();
	}

        start = p;
        p    += quote_len;

        if (p >= line_end) {
	    sprintf( text_buf, "eat_doublequote: Bogus quote length: %d", quote_len );
	    moan_and_die();
        }

	if (*p != '"') {
	    sprintf( text_buf, "eat_doublequote: Final character was '%c'?!", *p );
	    moan_and_die();
	}

	*p++ = '\0';
    }

    *pp = p;
    return start;
}

/* Scan one blank-delimited field from line_buf,
 * null-terminate it, and save it in argv[]:
 */
static unsigned char*
eat_nonblanks( unsigned char** pp )
{
    unsigned char* start = *pp;
    unsigned char* p     = start;

    switch (*p) {

    case '"':
        return eat_doublequote(pp) ;

    case ' ':
	sprintf( text_buf, "eat_nonblanks: Initial character was a blank!" );
        moan_and_die();

    default:
        ;
    }

    for (;;) {

        if (p == line_end)  break;

        if (*p != ' ') ++p;
        else           break;
    }

    *p  = '\0';
    *pp = p+1;
    return start;
}

/* Break line_buf commandline read from superprocess
 * into blank-separated fields stored in argv[]:
 */
static void
parse_line (void)
{
    unsigned char* p =  line_buf;
    argc    =  0;

    for (;;) {
        if (!eat_blanks(&p))  return;
        argv[ argc++ ] = eat_nonblanks(&p);	/* Updates 'p' via side-effect. */

        if (argc >= MAX_ARGC)  { sprintf(text_buf, "parse_line: More than MAX_ARGC words on input line."); moan_and_die(); }
    }
}

/* Dispatch command stored in argv[]
 * to appropriate function for execution:
 */
static void
execute_line (void)
{
    Trie_Fn trie_fn;
    if (argc == 0) return;
    trie_fn = get_trie( trie, argv[0] );
    if (!trie_fn) { sprintf( text_buf, "FATAL no such function as '%s'.\n", argv[0]); moan_and_die(); }
    trie_fn( argc-1, argv+1 );
}

static void
read_eval_print_one_stdin_command (void)
{
    read_one_line ();		/* Read a complete command line from superprocess.			*/
    parse_line    ();		/* Break it into blank-separated fields.				*/
    execute_line  ();		/* Dispatch to the appropriate function, writing results to stdout.	*/
}
int
main (int argc, char **argv)
{
    main_argc = argc;
    main_argv = argv;

    init();

    for (;;)   read_eval_print_one_stdin_command ();

    exit(0);		/* Cannot execute, but might re-assure compiler. */
}


// Code by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

