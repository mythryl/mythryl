// mythryl-gtk-server.c -- a simple Gtk subprocess server for Mythryl.

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
//     display and response loops in mythryl-gtk-server can
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
//     src/lib/src/gtk-client.api
//     src/lib/src/gtk-client-for-server-in-main-process.pkg



/*
###             "You can't teach an old C dog GNU tricks."
 */


#include "../mythryl-config.h"

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
    printf( "src/c/gtk/mythryl-gtk-server.c:  Fatal error:  %s  exit(1)ing.\n", text_buf );
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

    sprintf (filename, "mythryl-gtk-server-%d.log~", getpid());

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
    {   unsigned char* id = argv[ii];

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

    struct _trie_node* child  [ 256 ];		// Hey, RAM is cheap, right? :)
    Trie_Fn            trie_fn[ 256 ];

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

    {   int i;
        for (i = 256; i --> 0; ) {
            result->child  [i] = NULL;
            result->trie_fn[i] = NULL;
        }
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

      sprintf( text_buf, "do_init: failed to initialize GUI support.");  moan_and_die();
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

/* Do not edit this or following lines -- they are autogenerated by make-gtk-glue. */

static void
do_make_window( int argc, unsigned char** argv )
{
    check_argc( "do_make_window", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_window_new( GTK_WINDOW_TOPLEVEL );

         printf(             "make_window%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_window%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_label( int argc, unsigned char** argv )
{
    check_argc( "do_make_label", 1, argc );

    {
        char*             s0 =                      string_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_label_new( /*label*/s0 );

         printf(             "make_label%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_label%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_status_bar_context_id( int argc, unsigned char** argv )
{
    check_argc( "do_make_status_bar_context_id", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        char*             s1 =                      string_arg( argc, argv, 1 );

        int result = gtk_statusbar_get_context_id( GTK_STATUSBAR(/*status_bar*/w0), /*description*/s1);

         printf(              "make_status_bar_context_id%d\n", result);      fflush( stdout );
        fprintf(log_fd, "SENT: make_status_bar_context_id%d\n", result);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_menu( int argc, unsigned char** argv )
{
    check_argc( "do_make_menu", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_menu_new ();

         printf(             "make_menu%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_menu%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_option_menu( int argc, unsigned char** argv )
{
    check_argc( "do_make_option_menu", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_option_menu_new ();

         printf(             "make_option_menu%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_option_menu%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_menu_bar( int argc, unsigned char** argv )
{
    check_argc( "do_make_menu_bar", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_menu_bar_new ();

         printf(             "make_menu_bar%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_menu_bar%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_combo_box( int argc, unsigned char** argv )
{
    check_argc( "do_make_combo_box", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_combo_box_new ();

         printf(             "make_combo_box%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_combo_box%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_text_combo_box( int argc, unsigned char** argv )
{
    check_argc( "do_make_text_combo_box", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_combo_box_new_text ();

         printf(             "make_text_combo_box%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_text_combo_box%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_frame( int argc, unsigned char** argv )
{
    check_argc( "do_make_frame", 1, argc );

    {
        char*             s0 =                      string_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_frame_new (*/*label*/s0 ? /*label*/s0 : NULL);

         printf(             "make_frame%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_frame%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_button( int argc, unsigned char** argv )
{
    check_argc( "do_make_button", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_button_new ();

         printf(             "make_button%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_button%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_button_with_label( int argc, unsigned char** argv )
{
    check_argc( "do_make_button_with_label", 1, argc );

    {
        char*             s0 =                      string_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_button_new_with_label( /*label*/s0 );

         printf(             "make_button_with_label%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_button_with_label%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_button_with_mnemonic( int argc, unsigned char** argv )
{
    check_argc( "do_make_button_with_mnemonic", 1, argc );

    {
        char*             s0 =                      string_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_button_new_with_mnemonic( /*mnemonic_label*/s0 );

         printf(             "make_button_with_mnemonic%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_button_with_mnemonic%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_toggle_button( int argc, unsigned char** argv )
{
    check_argc( "do_make_toggle_button", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_toggle_button_new ();

         printf(             "make_toggle_button%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_toggle_button%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_toggle_button_with_label( int argc, unsigned char** argv )
{
    check_argc( "do_make_toggle_button_with_label", 1, argc );

    {
        char*             s0 =                      string_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_toggle_button_new_with_label( /*label*/s0 );

         printf(             "make_toggle_button_with_label%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_toggle_button_with_label%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_toggle_button_with_mnemonic( int argc, unsigned char** argv )
{
    check_argc( "do_make_toggle_button_with_mnemonic", 1, argc );

    {
        char*             s0 =                      string_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_toggle_button_new_with_mnemonic( /*mnemonic_label*/s0 );

         printf(             "make_toggle_button_with_mnemonic%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_toggle_button_with_mnemonic%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_check_button( int argc, unsigned char** argv )
{
    check_argc( "do_make_check_button", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_check_button_new ();

         printf(             "make_check_button%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_check_button%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_check_button_with_label( int argc, unsigned char** argv )
{
    check_argc( "do_make_check_button_with_label", 1, argc );

    {
        char*             s0 =                      string_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_check_button_new_with_label ( /*label*/s0 );

         printf(             "make_check_button_with_label%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_check_button_with_label%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_check_button_with_mnemonic( int argc, unsigned char** argv )
{
    check_argc( "do_make_check_button_with_mnemonic", 1, argc );

    {
        char*             s0 =                      string_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_check_button_new_with_mnemonic( /*mnemonic_label*/s0 );

         printf(             "make_check_button_with_mnemonic%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_check_button_with_mnemonic%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_menu_item( int argc, unsigned char** argv )
{
    check_argc( "do_make_menu_item", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_menu_item_new ();

         printf(             "make_menu_item%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_menu_item%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_menu_item_with_label( int argc, unsigned char** argv )
{
    check_argc( "do_make_menu_item_with_label", 1, argc );

    {
        char*             s0 =                      string_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_menu_item_new_with_label( /*label*/s0 );

         printf(             "make_menu_item_with_label%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_menu_item_with_label%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_menu_item_with_mnemonic( int argc, unsigned char** argv )
{
    check_argc( "do_make_menu_item_with_mnemonic", 1, argc );

    {
        char*             s0 =                      string_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_menu_item_new_with_mnemonic( /*mnemonic_label*/s0 );

         printf(             "make_menu_item_with_mnemonic%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_menu_item_with_mnemonic%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_first_radio_button( int argc, unsigned char** argv )
{
    check_argc( "do_make_first_radio_button", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_radio_button_new (NULL);

         printf(             "make_first_radio_button%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_first_radio_button%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_next_radio_button( int argc, unsigned char** argv )
{
    check_argc( "do_make_next_radio_button", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON(/*sib*/w0));

         printf(             "make_next_radio_button%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_next_radio_button%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_first_radio_button_with_label( int argc, unsigned char** argv )
{
    check_argc( "do_make_first_radio_button_with_label", 1, argc );

    {
        char*             s0 =                      string_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_radio_button_new_with_label(NULL,/*label*/s0);

         printf(             "make_first_radio_button_with_label%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_first_radio_button_with_label%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_next_radio_button_with_label( int argc, unsigned char** argv )
{
    check_argc( "do_make_next_radio_button_with_label", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        char*             s1 =                      string_arg( argc, argv, 1 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_radio_button_new_with_label_from_widget ( GTK_RADIO_BUTTON(/*sib*/w0), /*label*/s1 );

         printf(             "make_next_radio_button_with_label%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_next_radio_button_with_label%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_first_radio_button_with_mnemonic( int argc, unsigned char** argv )
{
    check_argc( "do_make_first_radio_button_with_mnemonic", 1, argc );

    {
        char*             s0 =                      string_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_radio_button_new_with_mnemonic(NULL,/*label*/s0);

         printf(             "make_first_radio_button_with_mnemonic%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_first_radio_button_with_mnemonic%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_next_radio_button_with_mnemonic( int argc, unsigned char** argv )
{
    check_argc( "do_make_next_radio_button_with_mnemonic", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        char*             s1 =                      string_arg( argc, argv, 1 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_radio_button_new_with_mnemonic_from_widget ( GTK_RADIO_BUTTON(/*sib*/w0), /*label*/s1 );

         printf(             "make_next_radio_button_with_mnemonic%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_next_radio_button_with_mnemonic%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_arrow( int argc, unsigned char** argv )
{
    check_argc( "do_make_arrow", 2, argc );

    {
        int               i0 =                         int_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_arrow_new( int_to_arrow_direction(/*arrow_direction_to_int arrow_direction*/i0), int_to_shadow_style(/*shadow_style_to_int shadow_style*/i1) );

         printf(             "make_arrow%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_arrow%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_arrow( int argc, unsigned char** argv )
{
    check_argc( "do_set_arrow", 3, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );

        gtk_arrow_set( GTK_ARROW(/*arrow*/w0), int_to_arrow_direction(/*arrow_direction_to_int arrow_direction*/i1), int_to_shadow_style(/*shadow_style_to_int shadow_style*/i2) );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_horizontal_box( int argc, unsigned char** argv )
{
    check_argc( "do_make_horizontal_box", 2, argc );

    {
        int               b0 =                        bool_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_hbox_new ( /*homogeneous*/b0, /*spacing*/i1 );

         printf(             "make_horizontal_box%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_horizontal_box%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_vertical_box( int argc, unsigned char** argv )
{
    check_argc( "do_make_vertical_box", 2, argc );

    {
        int               b0 =                        bool_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_vbox_new ( /*homogeneous*/b0, /*spacing*/i1 );

         printf(             "make_vertical_box%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_vertical_box%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_horizontal_button_box( int argc, unsigned char** argv )
{
    check_argc( "do_make_horizontal_button_box", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_hbutton_box_new ();

         printf(             "make_horizontal_button_box%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_horizontal_button_box%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_vertical_button_box( int argc, unsigned char** argv )
{
    check_argc( "do_make_vertical_button_box", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_vbutton_box_new ();

         printf(             "make_vertical_button_box%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_vertical_button_box%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_table( int argc, unsigned char** argv )
{
    check_argc( "do_make_table", 3, argc );

    {
        int               i0 =                         int_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );
        int               b2 =                        bool_arg( argc, argv, 2 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_table_new ( /*rows*/i0, /*cols*/i1, /*homogeneous*/b2 );

         printf(             "make_table%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_table%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_event_box( int argc, unsigned char** argv )
{
    check_argc( "do_make_event_box", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_event_box_new ();

         printf(             "make_event_box%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_event_box%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_image_from_file( int argc, unsigned char** argv )
{
    check_argc( "do_make_image_from_file", 1, argc );

    {
        char*             s0 =                      string_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_image_new_from_file( /*filename*/s0 );

         printf(             "make_image_from_file%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_image_from_file%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_horizontal_separator( int argc, unsigned char** argv )
{
    check_argc( "do_make_horizontal_separator", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_hseparator_new ();

         printf(             "make_horizontal_separator%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_horizontal_separator%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_vertical_separator( int argc, unsigned char** argv )
{
    check_argc( "do_make_vertical_separator", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_vseparator_new ();

         printf(             "make_vertical_separator%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_vertical_separator%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_layout_container( int argc, unsigned char** argv )
{
    check_argc( "do_make_layout_container", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_layout_new (NULL, NULL);

         printf(             "make_layout_container%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_layout_container%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_layout_put( int argc, unsigned char** argv )
{
    check_argc( "do_layout_put", 4, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );
        int               i3 =                         int_arg( argc, argv, 3 );

        gtk_layout_put( GTK_LAYOUT(/*layout*/w0), GTK_WIDGET(/*kid*/w1), /*x*/i2, /*y*/i3);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_layout_move( int argc, unsigned char** argv )
{
    check_argc( "do_layout_move", 4, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );
        int               i3 =                         int_arg( argc, argv, 3 );

        gtk_layout_move( GTK_LAYOUT(/*layout*/w0), GTK_WIDGET(/*kid*/w1), /*x*/i2, /*y*/i3);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_fixed_container( int argc, unsigned char** argv )
{
    check_argc( "do_make_fixed_container", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_fixed_new ();

         printf(             "make_fixed_container%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_fixed_container%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_fixed_put( int argc, unsigned char** argv )
{
    check_argc( "do_fixed_put", 4, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );
        int               i3 =                         int_arg( argc, argv, 3 );

        gtk_fixed_put(   GTK_FIXED(/*layout*/w0), GTK_WIDGET(/*kid*/w1), /*x*/i2, /*y*/i3);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_fixed_move( int argc, unsigned char** argv )
{
    check_argc( "do_fixed_move", 4, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );
        int               i3 =                         int_arg( argc, argv, 3 );

        gtk_fixed_move(  GTK_FIXED(/*layout*/w0), GTK_WIDGET(/*kid*/w1), /*x*/i2, /*y*/i3);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_adjustment( int argc, unsigned char** argv )
{
    check_argc( "do_make_adjustment", 6, argc );

    {
        double            f0 =                      double_arg( argc, argv, 0 );
        double            f1 =                      double_arg( argc, argv, 1 );
        double            f2 =                      double_arg( argc, argv, 2 );
        double            f3 =                      double_arg( argc, argv, 3 );
        double            f4 =                      double_arg( argc, argv, 4 );
        double            f5 =                      double_arg( argc, argv, 5 );

        int slot = find_free_widget_slot ();

        widget[slot] = (GtkWidget*) gtk_adjustment_new ( /*value*/f0, /*lower*/f1, /*upper*/f2, /*step_increment*/f3, /*page_increment*/f4, /*page_size*/f5 );

         printf(             "make_adjustment%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_adjustment%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_viewport( int argc, unsigned char** argv )
{
    check_argc( "do_make_viewport", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_viewport_new( GTK_ADJUSTMENT(/*null_or_widget_to_int horizontal_adjustment*/w0), GTK_ADJUSTMENT(/*null_or_widget_to_int vertical_adjustment*/w1) );

         printf(             "make_viewport%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_viewport%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_scrolled_window( int argc, unsigned char** argv )
{
    check_argc( "do_make_scrolled_window", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_scrolled_window_new( GTK_ADJUSTMENT(/*null_or_widget_to_int horizontal_adjustment*/w0), GTK_ADJUSTMENT(/*null_or_widget_to_int vertical_adjustment*/w1) );

         printf(             "make_scrolled_window%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_scrolled_window%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_horizontal_ruler( int argc, unsigned char** argv )
{
    check_argc( "do_make_horizontal_ruler", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_hruler_new ();

         printf(             "make_horizontal_ruler%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_horizontal_ruler%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_vertical_ruler( int argc, unsigned char** argv )
{
    check_argc( "do_make_vertical_ruler", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_vruler_new ();

         printf(             "make_vertical_ruler%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_vertical_ruler%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_vertical_scrollbar( int argc, unsigned char** argv )
{
    check_argc( "do_make_vertical_scrollbar", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_vscrollbar_new( GTK_ADJUSTMENT(/*null_or_widget_to_int adjustment*/w0) );

         printf(             "make_vertical_scrollbar%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_vertical_scrollbar%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_horizontal_scrollbar( int argc, unsigned char** argv )
{
    check_argc( "do_make_horizontal_scrollbar", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_hscrollbar_new( GTK_ADJUSTMENT(/*null_or_widget_to_int adjustment*/w0) );

         printf(             "make_horizontal_scrollbar%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_horizontal_scrollbar%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_vertical_scale( int argc, unsigned char** argv )
{
    check_argc( "do_make_vertical_scale", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_vscale_new( GTK_ADJUSTMENT(/*null_or_widget_to_int adjustment*/w0) );

         printf(             "make_vertical_scale%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_vertical_scale%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_horizontal_scale( int argc, unsigned char** argv )
{
    check_argc( "do_make_horizontal_scale", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_hscale_new( GTK_ADJUSTMENT(/*null_or_widget_to_int adjustment*/w0) );

         printf(             "make_horizontal_scale%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_horizontal_scale%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_vertical_scale_with_range( int argc, unsigned char** argv )
{
    check_argc( "do_make_vertical_scale_with_range", 3, argc );

    {
        double            f0 =                      double_arg( argc, argv, 0 );
        double            f1 =                      double_arg( argc, argv, 1 );
        double            f2 =                      double_arg( argc, argv, 2 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_vscale_new_with_range( /*min*/f0, /*max*/f1, /*step*/f2 );

         printf(             "make_vertical_scale_with_range%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_vertical_scale_with_range%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_horizontal_scale_with_range( int argc, unsigned char** argv )
{
    check_argc( "do_make_horizontal_scale_with_range", 3, argc );

    {
        double            f0 =                      double_arg( argc, argv, 0 );
        double            f1 =                      double_arg( argc, argv, 1 );
        double            f2 =                      double_arg( argc, argv, 2 );

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_hscale_new_with_range( /*min*/f0, /*max*/f1, /*step*/f2 );

         printf(             "make_horizontal_scale_with_range%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_horizontal_scale_with_range%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_drawing_area( int argc, unsigned char** argv )
{
    check_argc( "do_make_drawing_area", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_drawing_area_new();

         printf(             "make_drawing_area%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_drawing_area%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_pixmap( int argc, unsigned char** argv )
{
    check_argc( "do_make_pixmap", 3, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );

        int slot = find_free_widget_slot ();

        widget[slot] = (GtkWidget*) gdk_pixmap_new( GDK_DRAWABLE(/*window*/w0), /*wide*/i1, /*high*/i2, -1);

         printf(             "make_pixmap%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_pixmap%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_make_status_bar( int argc, unsigned char** argv )
{
    check_argc( "do_make_status_bar", 0, argc );

    {

        int slot = find_free_widget_slot ();

        widget[slot] = gtk_statusbar_new();

         printf(             "make_status_bar%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT:make_status_bar%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_push_text_on_status_bar( int argc, unsigned char** argv )
{
    check_argc( "do_push_text_on_status_bar", 3, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );
        char*             s2 =                      string_arg( argc, argv, 2 );

        int result = gtk_statusbar_push( GTK_STATUSBAR(/*status_bar*/w0), /*context*/i1, /*text*/s2);

         printf(              "push_text_on_status_bar%d\n", result);      fflush( stdout );
        fprintf(log_fd, "SENT: push_text_on_status_bar%d\n", result);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_pop_text_off_status_bar( int argc, unsigned char** argv )
{
    check_argc( "do_pop_text_off_status_bar", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );

        gtk_statusbar_pop(GTK_STATUSBAR(/*status_bar*/w0), /*context*/i1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_remove_text_from_status_bar( int argc, unsigned char** argv )
{
    check_argc( "do_remove_text_from_status_bar", 3, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );

        gtk_statusbar_remove( GTK_STATUSBAR(/*status_bar*/w0), /*context*/i1, /*message*/i2);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_pack_box( int argc, unsigned char** argv )
{
    check_argc( "do_pack_box", 6, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );
        int               b3 =                        bool_arg( argc, argv, 3 );
        int               b4 =                        bool_arg( argc, argv, 4 );
        int               i5 =                         int_arg( argc, argv, 5 );

        if (!/*pack_to_int pack*/i2)  gtk_box_pack_start(   GTK_BOX(/*box*/w0), GTK_WIDGET(/*kid*/w1), /*expand*/b3, /*fill*/b4, /*padding*/i5 ); else gtk_box_pack_end( GTK_BOX(/*box*/w0), GTK_WIDGET(/*kid*/w1), /*expand*/b3, /*fill*/b4, /*padding*/i5 );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_menu_shell_append( int argc, unsigned char** argv )
{
    check_argc( "do_menu_shell_append", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );

        gtk_menu_shell_append( GTK_MENU_SHELL(/*menu*/w0), GTK_WIDGET(/*kid*/w1));
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_menu_bar_append( int argc, unsigned char** argv )
{
    check_argc( "do_menu_bar_append", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );

        gtk_menu_bar_append( GTK_MENU_SHELL(/*menu*/w0), GTK_WIDGET(/*kid*/w1));
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_append_text_to_combo_box( int argc, unsigned char** argv )
{
    check_argc( "do_append_text_to_combo_box", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        char*             s1 =                      string_arg( argc, argv, 1 );

        gtk_combo_box_append_text( GTK_COMBO_BOX(/*combo_box*/w0), /*text*/s1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_option_menu_menu( int argc, unsigned char** argv )
{
    check_argc( "do_set_option_menu_menu", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );

        gtk_option_menu_set_menu( GTK_OPTION_MENU(/*option_menu*/w0), GTK_WIDGET(/*menu*/w1) );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_text_tooltip_on_widget( int argc, unsigned char** argv )
{
    check_argc( "do_set_text_tooltip_on_widget", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        char*             s1 =                      string_arg( argc, argv, 1 );

        gtk_widget_set_tooltip_text( GTK_WIDGET(/*widget*/w0), /*text*/s1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_ruler_metric( int argc, unsigned char** argv )
{
    check_argc( "do_set_ruler_metric", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );

        gtk_ruler_set_metric( GTK_RULER(/*ruler*/w0), int_to_metric(/*metric_to_int metric*/i1));
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_ruler_range( int argc, unsigned char** argv )
{
    check_argc( "do_set_ruler_range", 5, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        double            f1 =                      double_arg( argc, argv, 1 );
        double            f2 =                      double_arg( argc, argv, 2 );
        double            f3 =                      double_arg( argc, argv, 3 );
        double            f4 =                      double_arg( argc, argv, 4 );

        gtk_ruler_set_range( GTK_RULER(/*ruler*/w0), /*lower*/f1, /*upper*/f2, /*position*/f3, /*max_size*/f4);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_scrollbar_policy( int argc, unsigned char** argv )
{
    check_argc( "do_set_scrollbar_policy", 3, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );

        gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(/*window*/w0), int_to_policy(/*scrollbar_policy_to_int horizontal_scrollbar*/i1), int_to_policy(/*scrollbar_policy_to_int vertical_scrollbar*/i2) );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_draw_rectangle( int argc, unsigned char** argv )
{
    check_argc( "do_draw_rectangle", 7, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );
        int               b2 =                        bool_arg( argc, argv, 2 );
        int               i3 =                         int_arg( argc, argv, 3 );
        int               i4 =                         int_arg( argc, argv, 4 );
        int               i5 =                         int_arg( argc, argv, 5 );
        int               i6 =                         int_arg( argc, argv, 6 );

        gdk_draw_rectangle(   GDK_DRAWABLE(/*drawable*/w0), GDK_GC(/*gcontext*/w1), /*filled*/b2, /*x*/i3, /*y*/i4, /*wide*/i5, /*high*/i6);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_draw_drawable( int argc, unsigned char** argv )
{
    check_argc( "do_draw_drawable", 9, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );
        GtkWidget*        w2 =    (GtkWidget*)      widget_arg( argc, argv, 2 );
        int               i3 =                         int_arg( argc, argv, 3 );
        int               i4 =                         int_arg( argc, argv, 4 );
        int               i5 =                         int_arg( argc, argv, 5 );
        int               i6 =                         int_arg( argc, argv, 6 );
        int               i7 =                         int_arg( argc, argv, 7 );
        int               i8 =                         int_arg( argc, argv, 8 );

        gdk_draw_drawable(   GDK_DRAWABLE(/*drawable*/w0), GDK_GC(/*gcontext*/w1), GDK_DRAWABLE(/*from*/w2), /*from_x*/i3, /*from_y*/i4, /*to_x*/i5, /*to_y*/i6, /*wide*/i7, /*high*/i8);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_queue_redraw( int argc, unsigned char** argv )
{
    check_argc( "do_queue_redraw", 5, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );
        int               i3 =                         int_arg( argc, argv, 3 );
        int               i4 =                         int_arg( argc, argv, 4 );

        gtk_widget_queue_draw_area( GTK_WIDGET(/*widget*/w0), /*x*/i1, /*y*/i2, /*wide*/i3, /*high*/i4);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_press_button( int argc, unsigned char** argv )
{
    check_argc( "do_press_button", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        gtk_button_pressed(  GTK_BUTTON(/*widget*/w0) );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_release_button( int argc, unsigned char** argv )
{
    check_argc( "do_release_button", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        gtk_button_released( GTK_BUTTON(/*widget*/w0) );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_click_button( int argc, unsigned char** argv )
{
    check_argc( "do_click_button", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        gtk_button_clicked(  GTK_BUTTON(/*widget*/w0) );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_enter_button( int argc, unsigned char** argv )
{
    check_argc( "do_enter_button", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        gtk_button_enter(    GTK_BUTTON(/*widget*/w0) );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_leave_button( int argc, unsigned char** argv )
{
    check_argc( "do_leave_button", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        gtk_button_leave(    GTK_BUTTON(/*widget*/w0) );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_show_widget( int argc, unsigned char** argv )
{
    check_argc( "do_show_widget", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        gtk_widget_show( /*widget*/w0 );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_show_widget_tree( int argc, unsigned char** argv )
{
    check_argc( "do_show_widget_tree", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        gtk_widget_show_all( /*widget*/w0 );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_destroy_widget( int argc, unsigned char** argv )
{
    check_argc( "do_destroy_widget", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        gtk_widget_destroy( /*widget*/w0 );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_emit_changed_signal( int argc, unsigned char** argv )
{
    check_argc( "do_emit_changed_signal", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        g_signal_emit_by_name( GTK_OBJECT(/*widget*/w0), "changed");
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_pop_up_combo_box( int argc, unsigned char** argv )
{
    check_argc( "do_pop_up_combo_box", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        gtk_combo_box_popup(   GTK_COMBO_BOX(/*widget*/w0));
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_pop_down_combo_box( int argc, unsigned char** argv )
{
    check_argc( "do_pop_down_combo_box", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        gtk_combo_box_popdown( GTK_COMBO_BOX(/*widget*/w0));
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_combo_box_title( int argc, unsigned char** argv )
{
    check_argc( "do_set_combo_box_title", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        char*             s1 =                      string_arg( argc, argv, 1 );

        gtk_combo_box_set_title( GTK_COMBO_BOX(/*widget*/w0), /*title*/s1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_window_title( int argc, unsigned char** argv )
{
    check_argc( "do_set_window_title", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        char*             s1 =                      string_arg( argc, argv, 1 );

        gtk_window_set_title( GTK_WINDOW(/*window*/w0), /*title*/s1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_window_default_size( int argc, unsigned char** argv )
{
    check_argc( "do_set_window_default_size", 3, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );

        gtk_window_set_default_size( GTK_WINDOW(/*widget*/w0), /*wide*/i1, /*high*/i2);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_minimum_widget_size( int argc, unsigned char** argv )
{
    check_argc( "do_set_minimum_widget_size", 3, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );

        gtk_widget_set_size_request( GTK_WIDGET(/*widget*/w0), /*wide*/i1, /*high*/i2);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_border_width( int argc, unsigned char** argv )
{
    check_argc( "do_set_border_width", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );

        gtk_container_set_border_width(GTK_CONTAINER(/*widget*/w0), /*width*/i1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_event_box_visibility( int argc, unsigned char** argv )
{
    check_argc( "do_set_event_box_visibility", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               b1 =                        bool_arg( argc, argv, 1 );

        gtk_event_box_set_visible_window(GTK_EVENT_BOX(/*event_box*/w0),/*visibility*/b1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_widget_alignment( int argc, unsigned char** argv )
{
    check_argc( "do_set_widget_alignment", 3, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        double            f1 =                      double_arg( argc, argv, 1 );
        double            f2 =                      double_arg( argc, argv, 2 );

        gtk_misc_set_alignment(GTK_MISC(/*widget*/w0), /*x*/f1, /*y*/f2);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_widget_events( int argc, unsigned char** argv )
{
    check_argc( "do_set_widget_events", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );

        gtk_widget_set_events( GTK_WIDGET(/*widget*/w0), int_to_event_mask(/*events_to_int events*/i1));
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_widget_name( int argc, unsigned char** argv )
{
    check_argc( "do_set_widget_name", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        char*             s1 =                      string_arg( argc, argv, 1 );

        gtk_widget_set_name( GTK_WIDGET(/*widget*/w0), /*name*/s1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_label_justification( int argc, unsigned char** argv )
{
    check_argc( "do_set_label_justification", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );

        gtk_label_set_justify( GTK_LABEL(/*label*/w0), int_to_justification(/*justification_to_int justification*/i1));
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_label_line_wrapping( int argc, unsigned char** argv )
{
    check_argc( "do_set_label_line_wrapping", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               b1 =                        bool_arg( argc, argv, 1 );

        gtk_label_set_line_wrap( GTK_LABEL(/*label*/w0), /*wrap_lines*/b1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_label_underlines( int argc, unsigned char** argv )
{
    check_argc( "do_set_label_underlines", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        char*             s1 =                      string_arg( argc, argv, 1 );

        gtk_label_set_pattern( GTK_LABEL(/*label*/w0), /*underlines*/s1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_scale_value_position( int argc, unsigned char** argv )
{
    check_argc( "do_set_scale_value_position", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );

        gtk_scale_set_value_pos( GTK_SCALE(/*scale*/w0), int_to_position(/*position_to_int position*/i1));
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_draw_scale_value( int argc, unsigned char** argv )
{
    check_argc( "do_set_draw_scale_value", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               b1 =                        bool_arg( argc, argv, 1 );

        gtk_scale_set_draw_value( GTK_SCALE(/*scale*/w0), /*draw_value*/b1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_get_scale_value_digits_shown( int argc, unsigned char** argv )
{
    check_argc( "do_get_scale_value_digits_shown", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        int result = gtk_scale_get_digits( GTK_SCALE(/*scale*/w0) );

         printf(              "get_scale_value_digits_shown%d\n", result);      fflush( stdout );
        fprintf(log_fd, "SENT: get_scale_value_digits_shown%d\n", result);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_scale_value_digits_shown( int argc, unsigned char** argv )
{
    check_argc( "do_set_scale_value_digits_shown", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );

        gtk_scale_set_digits( GTK_SCALE(/*scale*/w0), /*digits*/i1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_range_update_policy( int argc, unsigned char** argv )
{
    check_argc( "do_set_range_update_policy", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );

        gtk_range_set_update_policy( GTK_RANGE(/*scale*/w0), /*policy*/int_to_range_update_policy(/*update_policy_to_int policy*/i1));
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_get_toggle_button_state( int argc, unsigned char** argv )
{
    check_argc( "do_get_toggle_button_state", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        int result = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(/*toggle_button*/w0) );

         printf(              "get_toggle_button_state%d\n", result);      fflush( stdout );
        fprintf(log_fd, "SENT: get_toggle_button_state%d\n", result);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_toggle_button_state( int argc, unsigned char** argv )
{
    check_argc( "do_set_toggle_button_state", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               b1 =                        bool_arg( argc, argv, 1 );

        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(/*toggle_button*/w0), /*state*/b1 != 0 );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_get_adjustment_value( int argc, unsigned char** argv )
{
    check_argc( "do_get_adjustment_value", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        double result = gtk_adjustment_get_value( GTK_ADJUSTMENT(/*adjustment*/w0) );

         printf(              "get_adjustment_value%f\n", result);      fflush( stdout );
        fprintf(log_fd, "SENT: get_adjustment_value%f\n", result);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_adjustment_value( int argc, unsigned char** argv )
{
    check_argc( "do_set_adjustment_value", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        double            f1 =                      double_arg( argc, argv, 1 );

        gtk_adjustment_set_value( GTK_ADJUSTMENT(/*adjustment*/w0), /*value*/f1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_get_white_graphics_context( int argc, unsigned char** argv )
{
    check_argc( "do_get_white_graphics_context", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        GtkWidget* widget = (GtkWidget*) /*widget*/w0->style->white_gc;

        int slot = get_widget_id( widget );

         printf(              "get_white_graphics_context%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT: get_white_graphics_context%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_get_black_graphics_context( int argc, unsigned char** argv )
{
    check_argc( "do_get_black_graphics_context", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        GtkWidget* widget = (GtkWidget*) /*widget*/w0->style->black_gc;

        int slot = get_widget_id( widget );

         printf(              "get_black_graphics_context%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT: get_black_graphics_context%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_get_current_foreground_graphics_context( int argc, unsigned char** argv )
{
    check_argc( "do_get_current_foreground_graphics_context", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        GtkWidget* widget = (GtkWidget*) w0->style->fg_gc[ GTK_WIDGET_STATE(/*widget*/w0) ];

        int slot = get_widget_id( widget );

         printf(              "get_current_foreground_graphics_context%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT: get_current_foreground_graphics_context%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_get_current_background_graphics_context( int argc, unsigned char** argv )
{
    check_argc( "do_get_current_background_graphics_context", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        GtkWidget* widget = (GtkWidget*) w0->style->bg_gc[ GTK_WIDGET_STATE(/*widget*/w0) ];

        int slot = get_widget_id( widget );

         printf(              "get_current_background_graphics_context%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT: get_current_background_graphics_context%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_get_widget_window( int argc, unsigned char** argv )
{
    check_argc( "do_get_widget_window", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        GtkWidget* widget = (GtkWidget*) /*widget*/w0->window;

        int slot = get_widget_id( widget );

         printf(              "get_widget_window%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT: get_widget_window%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_add_kid( int argc, unsigned char** argv )
{
    check_argc( "do_add_kid", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );

        gtk_container_add( GTK_CONTAINER(/*mom*/w0), GTK_WIDGET(/*kid*/w1));
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_add_scrolled_window_kid( int argc, unsigned char** argv )
{
    check_argc( "do_add_scrolled_window_kid", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );

        gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW(/*window*/w0), GTK_WIDGET(/*kid*/w1) );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_add_table_kid( int argc, unsigned char** argv )
{
    check_argc( "do_add_table_kid", 6, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );
        int               i3 =                         int_arg( argc, argv, 3 );
        int               i4 =                         int_arg( argc, argv, 4 );
        int               i5 =                         int_arg( argc, argv, 5 );

        gtk_table_attach_defaults( GTK_TABLE(/*table*/w0), GTK_WIDGET(/*kid*/w1), /*left*/i2, /*right*/i3, /*top*/i4, /*bottom*/i5 );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_add_table_kid2( int argc, unsigned char** argv )
{
    check_argc( "do_add_table_kid2", 10, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        GtkWidget*        w1 =    (GtkWidget*)      widget_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );
        int               i3 =                         int_arg( argc, argv, 3 );
        int               i4 =                         int_arg( argc, argv, 4 );
        int               i5 =                         int_arg( argc, argv, 5 );
        int               i6 =                         int_arg( argc, argv, 6 );
        int               i7 =                         int_arg( argc, argv, 7 );
        int               i8 =                         int_arg( argc, argv, 8 );
        int               i9 =                         int_arg( argc, argv, 9 );

        gtk_table_attach( GTK_TABLE(/*table*/w0), GTK_WIDGET(/*kid*/w1), /*left*/i2, /*right*/i3, /*top*/i4, /*bottom*/i5, /*sum_table_attach_options xoptions*/i6, /*sum_table_attach_options yoptions*/i7, /*xpadding*/i8, /*ypadding*/i9 );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_get_viewport_vertical_adjustment( int argc, unsigned char** argv )
{
    check_argc( "do_get_viewport_vertical_adjustment", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        GtkWidget* widget = (GtkWidget*) gtk_viewport_get_vadjustment( GTK_VIEWPORT(/*viewport*/w0) );

        int slot = get_widget_id( widget );

         printf(              "get_viewport_vertical_adjustment%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT: get_viewport_vertical_adjustment%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_get_viewport_horizontal_adjustment( int argc, unsigned char** argv )
{
    check_argc( "do_get_viewport_horizontal_adjustment", 1, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );

        GtkWidget* widget = (GtkWidget*) gtk_viewport_get_hadjustment( GTK_VIEWPORT(/*viewport*/w0) );

        int slot = get_widget_id( widget );

         printf(              "get_viewport_horizontal_adjustment%d\n", slot);      fflush( stdout );
        fprintf(log_fd, "SENT: get_viewport_horizontal_adjustment%d\n", slot);      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_table_row_spacing( int argc, unsigned char** argv )
{
    check_argc( "do_set_table_row_spacing", 3, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );

        gtk_table_set_row_spacing( GTK_TABLE(/*table*/w0), /*row*/i1, /*spacing*/i2);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_table_col_spacing( int argc, unsigned char** argv )
{
    check_argc( "do_set_table_col_spacing", 3, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );
        int               i2 =                         int_arg( argc, argv, 2 );

        gtk_table_set_col_spacing( GTK_TABLE(/*table*/w0), /*col*/i1, /*spacing*/i2);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_table_row_spacings( int argc, unsigned char** argv )
{
    check_argc( "do_set_table_row_spacings", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );

        gtk_table_set_row_spacings( GTK_TABLE(/*table*/w0), /*spacing*/i1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_table_col_spacings( int argc, unsigned char** argv )
{
    check_argc( "do_set_table_col_spacings", 2, argc );

    {
        GtkWidget*        w0 =    (GtkWidget*)      widget_arg( argc, argv, 0 );
        int               i1 =                         int_arg( argc, argv, 1 );

        gtk_table_set_col_spacings( GTK_TABLE(/*table*/w0), /*spacing*/i1);
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_mythryl_gtk_server_c_plain_fun. */

static void
do_set_clicked_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_clicked_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "clicked", G_CALLBACK( run_clicked_callback ), (void*)id );

         printf(              "set_clicked_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_clicked_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_pressed_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_pressed_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "pressed", G_CALLBACK( run_pressed_callback ), (void*)id );

         printf(              "set_pressed_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_pressed_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_release_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_release_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "release", G_CALLBACK( run_release_callback ), (void*)id );

         printf(              "set_release_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_release_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_enter_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_enter_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "enter", G_CALLBACK( run_enter_callback ), (void*)id );

         printf(              "set_enter_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_enter_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_leave_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_leave_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "leave", G_CALLBACK( run_leave_callback ), (void*)id );

         printf(              "set_leave_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_leave_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_activate_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_activate_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( GTK_MENU_ITEM(widget), "activate", G_CALLBACK( run_activate_callback ), (void*)id );

         printf(              "set_activate_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_activate_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_destroy_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_destroy_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "destroy", G_CALLBACK( run_destroy_callback ), (void*)id );

         printf(              "set_destroy_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_destroy_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_realize_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_realize_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "realize", G_CALLBACK( run_realize_callback ), (void*)id );

         printf(              "set_realize_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_realize_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_button_press_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_button_press_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "button_press_event", G_CALLBACK( run_button_press_event_callback ), (void*)id );

         printf(              "set_button_press_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_button_press_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_button_release_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_button_release_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "button_release_event", G_CALLBACK( run_button_release_event_callback ), (void*)id );

         printf(              "set_button_release_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_button_release_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_scroll_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_scroll_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "scroll_event", G_CALLBACK( run_scroll_event_callback ), (void*)id );

         printf(              "set_scroll_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_scroll_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_motion_notify_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_motion_notify_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "motion_notify_event", G_CALLBACK( run_motion_notify_event_callback ), (void*)id );

         printf(              "set_motion_notify_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_motion_notify_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_delete_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_delete_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "delete_event", G_CALLBACK( run_delete_event_callback ), (void*)id );

         printf(              "set_delete_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_delete_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_expose_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_expose_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "expose_event", G_CALLBACK( run_expose_event_callback ), (void*)id );

         printf(              "set_expose_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_expose_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_key_press_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_key_press_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "key_press_event", G_CALLBACK( run_key_press_event_callback ), (void*)id );

         printf(              "set_key_press_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_key_press_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_key_release_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_key_release_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "key_release_event", G_CALLBACK( run_key_release_event_callback ), (void*)id );

         printf(              "set_key_release_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_key_release_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_enter_notify_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_enter_notify_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "enter_notify_event", G_CALLBACK( run_enter_notify_event_callback ), (void*)id );

         printf(              "set_enter_notify_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_enter_notify_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_leave_notify_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_leave_notify_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "leave_notify_event", G_CALLBACK( run_leave_notify_event_callback ), (void*)id );

         printf(              "set_leave_notify_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_leave_notify_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_configure_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_configure_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "configure_event", G_CALLBACK( run_configure_event_callback ), (void*)id );

         printf(              "set_configure_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_configure_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_focus_in_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_focus_in_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "focus_in_event", G_CALLBACK( run_focus_in_event_callback ), (void*)id );

         printf(              "set_focus_in_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_focus_in_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_focus_out_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_focus_out_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "focus_out_event", G_CALLBACK( run_focus_out_event_callback ), (void*)id );

         printf(              "set_focus_out_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_focus_out_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_map_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_map_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "map_event", G_CALLBACK( run_map_event_callback ), (void*)id );

         printf(              "set_map_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_map_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_unmap_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_unmap_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "unmap_event", G_CALLBACK( run_unmap_event_callback ), (void*)id );

         printf(              "set_unmap_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_unmap_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_property_notify_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_property_notify_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "property_notify_event", G_CALLBACK( run_property_notify_event_callback ), (void*)id );

         printf(              "set_property_notify_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_property_notify_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_selection_clear_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_selection_clear_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "selection_clear_event", G_CALLBACK( run_selection_clear_event_callback ), (void*)id );

         printf(              "set_selection_clear_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_selection_clear_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_selection_request_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_selection_request_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "selection_request_event", G_CALLBACK( run_selection_request_event_callback ), (void*)id );

         printf(              "set_selection_request_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_selection_request_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_selection_notify_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_selection_notify_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "selection_notify_event", G_CALLBACK( run_selection_notify_event_callback ), (void*)id );

         printf(              "set_selection_notify_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_selection_notify_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_proximity_in_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_proximity_in_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "proximity_in_event", G_CALLBACK( run_proximity_in_event_callback ), (void*)id );

         printf(              "set_proximity_in_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_proximity_in_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_proximity_out_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_proximity_out_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "proximity_out_event", G_CALLBACK( run_proximity_out_event_callback ), (void*)id );

         printf(              "set_proximity_out_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_proximity_out_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_client_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_client_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "client_event", G_CALLBACK( run_client_event_callback ), (void*)id );

         printf(              "set_client_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_client_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_no_expose_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_no_expose_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "no_expose_event", G_CALLBACK( run_no_expose_event_callback ), (void*)id );

         printf(              "set_no_expose_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_no_expose_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_window_state_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_window_state_event_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "window_state_event", G_CALLBACK( run_window_state_event_callback ), (void*)id );

         printf(              "set_window_state_event_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_window_state_event_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_toggled_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_toggled_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "toggled", G_CALLBACK( run_toggled_callback ), (void*)id );

         printf(              "set_toggled_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_toggled_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */

static void
do_set_value_changed_callback( int argc, unsigned char** argv )
{
    check_argc( "do_set_value_changed_callback", 1, argc );

    {   GtkWidget* widget    =  widget_arg( argc, argv, 0 );

        int id   =  find_free_callback_id ();

        g_signal_connect( G_OBJECT(widget), "value_changed", G_CALLBACK( run_value_changed_callback ), (void*)id );

         printf(              "set_value_changed_callback%d\n", id );      fflush( stdout );
        fprintf(log_fd, "SENT: set_value_changed_callback%d\n", id );      fflush( log_fd );
    }
}
/* Above fn generated by src/lib/src/make-gtk-glue: write_gtk_server_c_set_callback_fun. */
/* Do not edit this or preceding lines -- they are autogenerated by make-gtk-glue. */

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

/* Do not edit this or following lines -- they are autogenerated by make-gtk-glue. */
    set_trie( trie, "make_window",                                do_make_window                                );
    set_trie( trie, "make_label",                                 do_make_label                                 );
    set_trie( trie, "make_status_bar_context_id",                 do_make_status_bar_context_id                 );
    set_trie( trie, "make_menu",                                  do_make_menu                                  );
    set_trie( trie, "make_option_menu",                           do_make_option_menu                           );
    set_trie( trie, "make_menu_bar",                              do_make_menu_bar                              );
    set_trie( trie, "make_combo_box",                             do_make_combo_box                             );
    set_trie( trie, "make_text_combo_box",                        do_make_text_combo_box                        );
    set_trie( trie, "make_frame",                                 do_make_frame                                 );
    set_trie( trie, "make_button",                                do_make_button                                );
    set_trie( trie, "make_button_with_label",                     do_make_button_with_label                     );
    set_trie( trie, "make_button_with_mnemonic",                  do_make_button_with_mnemonic                  );
    set_trie( trie, "make_toggle_button",                         do_make_toggle_button                         );
    set_trie( trie, "make_toggle_button_with_label",              do_make_toggle_button_with_label              );
    set_trie( trie, "make_toggle_button_with_mnemonic",           do_make_toggle_button_with_mnemonic           );
    set_trie( trie, "make_check_button",                          do_make_check_button                          );
    set_trie( trie, "make_check_button_with_label",               do_make_check_button_with_label               );
    set_trie( trie, "make_check_button_with_mnemonic",            do_make_check_button_with_mnemonic            );
    set_trie( trie, "make_menu_item",                             do_make_menu_item                             );
    set_trie( trie, "make_menu_item_with_label",                  do_make_menu_item_with_label                  );
    set_trie( trie, "make_menu_item_with_mnemonic",               do_make_menu_item_with_mnemonic               );
    set_trie( trie, "make_first_radio_button",                    do_make_first_radio_button                    );
    set_trie( trie, "make_next_radio_button",                     do_make_next_radio_button                     );
    set_trie( trie, "make_first_radio_button_with_label",         do_make_first_radio_button_with_label         );
    set_trie( trie, "make_next_radio_button_with_label",          do_make_next_radio_button_with_label          );
    set_trie( trie, "make_first_radio_button_with_mnemonic",      do_make_first_radio_button_with_mnemonic      );
    set_trie( trie, "make_next_radio_button_with_mnemonic",       do_make_next_radio_button_with_mnemonic       );
    set_trie( trie, "make_arrow",                                 do_make_arrow                                 );
    set_trie( trie, "set_arrow",                                  do_set_arrow                                  );
    set_trie( trie, "make_horizontal_box",                        do_make_horizontal_box                        );
    set_trie( trie, "make_vertical_box",                          do_make_vertical_box                          );
    set_trie( trie, "make_horizontal_button_box",                 do_make_horizontal_button_box                 );
    set_trie( trie, "make_vertical_button_box",                   do_make_vertical_button_box                   );
    set_trie( trie, "make_table",                                 do_make_table                                 );
    set_trie( trie, "make_event_box",                             do_make_event_box                             );
    set_trie( trie, "make_image_from_file",                       do_make_image_from_file                       );
    set_trie( trie, "make_horizontal_separator",                  do_make_horizontal_separator                  );
    set_trie( trie, "make_vertical_separator",                    do_make_vertical_separator                    );
    set_trie( trie, "make_layout_container",                      do_make_layout_container                      );
    set_trie( trie, "layout_put",                                 do_layout_put                                 );
    set_trie( trie, "layout_move",                                do_layout_move                                );
    set_trie( trie, "make_fixed_container",                       do_make_fixed_container                       );
    set_trie( trie, "fixed_put",                                  do_fixed_put                                  );
    set_trie( trie, "fixed_move",                                 do_fixed_move                                 );
    set_trie( trie, "make_adjustment",                            do_make_adjustment                            );
    set_trie( trie, "make_viewport",                              do_make_viewport                              );
    set_trie( trie, "make_scrolled_window",                       do_make_scrolled_window                       );
    set_trie( trie, "make_horizontal_ruler",                      do_make_horizontal_ruler                      );
    set_trie( trie, "make_vertical_ruler",                        do_make_vertical_ruler                        );
    set_trie( trie, "make_vertical_scrollbar",                    do_make_vertical_scrollbar                    );
    set_trie( trie, "make_horizontal_scrollbar",                  do_make_horizontal_scrollbar                  );
    set_trie( trie, "make_vertical_scale",                        do_make_vertical_scale                        );
    set_trie( trie, "make_horizontal_scale",                      do_make_horizontal_scale                      );
    set_trie( trie, "make_vertical_scale_with_range",             do_make_vertical_scale_with_range             );
    set_trie( trie, "make_horizontal_scale_with_range",           do_make_horizontal_scale_with_range           );
    set_trie( trie, "make_drawing_area",                          do_make_drawing_area                          );
    set_trie( trie, "make_pixmap",                                do_make_pixmap                                );
    set_trie( trie, "make_status_bar",                            do_make_status_bar                            );
    set_trie( trie, "push_text_on_status_bar",                    do_push_text_on_status_bar                    );
    set_trie( trie, "pop_text_off_status_bar",                    do_pop_text_off_status_bar                    );
    set_trie( trie, "remove_text_from_status_bar",                do_remove_text_from_status_bar                );
    set_trie( trie, "pack_box",                                   do_pack_box                                   );
    set_trie( trie, "menu_shell_append",                          do_menu_shell_append                          );
    set_trie( trie, "menu_bar_append",                            do_menu_bar_append                            );
    set_trie( trie, "append_text_to_combo_box",                   do_append_text_to_combo_box                   );
    set_trie( trie, "set_option_menu_menu",                       do_set_option_menu_menu                       );
    set_trie( trie, "set_text_tooltip_on_widget",                 do_set_text_tooltip_on_widget                 );
    set_trie( trie, "set_ruler_metric",                           do_set_ruler_metric                           );
    set_trie( trie, "set_ruler_range",                            do_set_ruler_range                            );
    set_trie( trie, "set_scrollbar_policy",                       do_set_scrollbar_policy                       );
    set_trie( trie, "draw_rectangle",                             do_draw_rectangle                             );
    set_trie( trie, "draw_drawable",                              do_draw_drawable                              );
    set_trie( trie, "queue_redraw",                               do_queue_redraw                               );
    set_trie( trie, "press_button",                               do_press_button                               );
    set_trie( trie, "release_button",                             do_release_button                             );
    set_trie( trie, "click_button",                               do_click_button                               );
    set_trie( trie, "enter_button",                               do_enter_button                               );
    set_trie( trie, "leave_button",                               do_leave_button                               );
    set_trie( trie, "show_widget",                                do_show_widget                                );
    set_trie( trie, "show_widget_tree",                           do_show_widget_tree                           );
    set_trie( trie, "destroy_widget",                             do_destroy_widget                             );
    set_trie( trie, "emit_changed_signal",                        do_emit_changed_signal                        );
    set_trie( trie, "pop_up_combo_box",                           do_pop_up_combo_box                           );
    set_trie( trie, "pop_down_combo_box",                         do_pop_down_combo_box                         );
    set_trie( trie, "set_combo_box_title",                        do_set_combo_box_title                        );
    set_trie( trie, "set_window_title",                           do_set_window_title                           );
    set_trie( trie, "set_window_default_size",                    do_set_window_default_size                    );
    set_trie( trie, "set_minimum_widget_size",                    do_set_minimum_widget_size                    );
    set_trie( trie, "set_border_width",                           do_set_border_width                           );
    set_trie( trie, "set_event_box_visibility",                   do_set_event_box_visibility                   );
    set_trie( trie, "set_widget_alignment",                       do_set_widget_alignment                       );
    set_trie( trie, "set_widget_events",                          do_set_widget_events                          );
    set_trie( trie, "set_widget_name",                            do_set_widget_name                            );
    set_trie( trie, "set_label_justification",                    do_set_label_justification                    );
    set_trie( trie, "set_label_line_wrapping",                    do_set_label_line_wrapping                    );
    set_trie( trie, "set_label_underlines",                       do_set_label_underlines                       );
    set_trie( trie, "set_scale_value_position",                   do_set_scale_value_position                   );
    set_trie( trie, "set_draw_scale_value",                       do_set_draw_scale_value                       );
    set_trie( trie, "get_scale_value_digits_shown",               do_get_scale_value_digits_shown               );
    set_trie( trie, "set_scale_value_digits_shown",               do_set_scale_value_digits_shown               );
    set_trie( trie, "set_range_update_policy",                    do_set_range_update_policy                    );
    set_trie( trie, "get_toggle_button_state",                    do_get_toggle_button_state                    );
    set_trie( trie, "set_toggle_button_state",                    do_set_toggle_button_state                    );
    set_trie( trie, "get_adjustment_value",                       do_get_adjustment_value                       );
    set_trie( trie, "set_adjustment_value",                       do_set_adjustment_value                       );
    set_trie( trie, "get_white_graphics_context",                 do_get_white_graphics_context                 );
    set_trie( trie, "get_black_graphics_context",                 do_get_black_graphics_context                 );
    set_trie( trie, "get_current_foreground_graphics_context",    do_get_current_foreground_graphics_context    );
    set_trie( trie, "get_current_background_graphics_context",    do_get_current_background_graphics_context    );
    set_trie( trie, "get_widget_window",                          do_get_widget_window                          );
    set_trie( trie, "add_kid",                                    do_add_kid                                    );
    set_trie( trie, "add_scrolled_window_kid",                    do_add_scrolled_window_kid                    );
    set_trie( trie, "add_table_kid",                              do_add_table_kid                              );
    set_trie( trie, "add_table_kid2",                             do_add_table_kid2                             );
    set_trie( trie, "get_viewport_vertical_adjustment",           do_get_viewport_vertical_adjustment           );
    set_trie( trie, "get_viewport_horizontal_adjustment",         do_get_viewport_horizontal_adjustment         );
    set_trie( trie, "set_table_row_spacing",                      do_set_table_row_spacing                      );
    set_trie( trie, "set_table_col_spacing",                      do_set_table_col_spacing                      );
    set_trie( trie, "set_table_row_spacings",                     do_set_table_row_spacings                     );
    set_trie( trie, "set_table_col_spacings",                     do_set_table_col_spacings                     );
    set_trie( trie, "set_clicked_callback",                       do_set_clicked_callback                       );
    set_trie( trie, "set_pressed_callback",                       do_set_pressed_callback                       );
    set_trie( trie, "set_release_callback",                       do_set_release_callback                       );
    set_trie( trie, "set_enter_callback",                         do_set_enter_callback                         );
    set_trie( trie, "set_leave_callback",                         do_set_leave_callback                         );
    set_trie( trie, "set_activate_callback",                      do_set_activate_callback                      );
    set_trie( trie, "set_destroy_callback",                       do_set_destroy_callback                       );
    set_trie( trie, "set_realize_callback",                       do_set_realize_callback                       );
    set_trie( trie, "set_button_press_event_callback",            do_set_button_press_event_callback            );
    set_trie( trie, "set_button_release_event_callback",          do_set_button_release_event_callback          );
    set_trie( trie, "set_scroll_event_callback",                  do_set_scroll_event_callback                  );
    set_trie( trie, "set_motion_notify_event_callback",           do_set_motion_notify_event_callback           );
    set_trie( trie, "set_delete_event_callback",                  do_set_delete_event_callback                  );
    set_trie( trie, "set_expose_event_callback",                  do_set_expose_event_callback                  );
    set_trie( trie, "set_key_press_event_callback",               do_set_key_press_event_callback               );
    set_trie( trie, "set_key_release_event_callback",             do_set_key_release_event_callback             );
    set_trie( trie, "set_enter_notify_event_callback",            do_set_enter_notify_event_callback            );
    set_trie( trie, "set_leave_notify_event_callback",            do_set_leave_notify_event_callback            );
    set_trie( trie, "set_configure_event_callback",               do_set_configure_event_callback               );
    set_trie( trie, "set_focus_in_event_callback",                do_set_focus_in_event_callback                );
    set_trie( trie, "set_focus_out_event_callback",               do_set_focus_out_event_callback               );
    set_trie( trie, "set_map_event_callback",                     do_set_map_event_callback                     );
    set_trie( trie, "set_unmap_event_callback",                   do_set_unmap_event_callback                   );
    set_trie( trie, "set_property_notify_event_callback",         do_set_property_notify_event_callback         );
    set_trie( trie, "set_selection_clear_event_callback",         do_set_selection_clear_event_callback         );
    set_trie( trie, "set_selection_request_event_callback",       do_set_selection_request_event_callback       );
    set_trie( trie, "set_selection_notify_event_callback",        do_set_selection_notify_event_callback        );
    set_trie( trie, "set_proximity_in_event_callback",            do_set_proximity_in_event_callback            );
    set_trie( trie, "set_proximity_out_event_callback",           do_set_proximity_out_event_callback           );
    set_trie( trie, "set_client_event_callback",                  do_set_client_event_callback                  );
    set_trie( trie, "set_no_expose_event_callback",               do_set_no_expose_event_callback               );
    set_trie( trie, "set_window_state_event_callback",            do_set_window_state_event_callback            );
    set_trie( trie, "set_toggled_callback",                       do_set_toggled_callback                       );
    set_trie( trie, "set_value_changed_callback",                 do_set_value_changed_callback                 );
/* Do not edit this or preceding lines -- they are autogenerated by make-gtk-glue. */

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

