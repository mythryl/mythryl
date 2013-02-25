// mythryl-ncurses-library-in-c-subprocess.c -- a simple Ncurses subprocess server for Mythryl.


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
//     src/glu/ncurses/src/ncurses-client.api
//     src/glu/ncurses/src/ncurses-client-for-library-in-main-process.pkg




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

#include <GL/glew.h>
#include <GL/glfw.h>

typedef struct 
{ 
  float x; 
  float y; 
  float z; 
} Vector3f; 
 
Vector3f vector3f(float _x, float _y, float _z) {
    Vector3f v;
    v.x = _x; 
    v.y = _y; 
    v.z = _z; 
    return v;
}; 


FILE* log_fd = 0;

static char text_buf[ 1024 ];

void mythryl_ncurses_server_dummy( void ) {				// This just a test to see if the appropriate libraries are linking; I don't intend to actually call this fn. It is non-static only to keep gcc from muttering about unused code.
    //
    int running = GL_TRUE;

    // Initialize GLFW
    //
    if (!glfwInit()) {
	fprintf(stderr,"glfwInit() returned FALSE (?!) -- exiting.   -- do__init() in libmythryl-ncurses.c\n");
        exit( EXIT_FAILURE );
    }

    // Open an Ncurses window
    //
    if( !glfwOpenWindow( 300,300, 0,0,0,0,0,0, GLFW_WINDOW ) )
    {
	glfwTerminate();

	exit( EXIT_FAILURE );
    }

    // Main loop

    while( running )
    {
	// Ncurses rendering goes here...

	glClear( GL_COLOR_BUFFER_BIT );

	glfwSwapBuffers();				// Swap front and back rendering buffers

	running =  !glfwGetKey( GLFW_KEY_ESC )		// Check if ESC key was pressed or window was closed
                &&  glfwGetWindowParam( GLFW_OPENED );

    }


    glfwTerminate();					// Close window and terminate GLFW

    exit( EXIT_SUCCESS );				// Exit program

}

static void   moan_and_die   (void)   {
    //        ============
    //
    printf( "src/c/ncurses/mythryl-ncurses-library-in-c-subprocess.c:  Fatal error:  %s  exit(1)ing.\n", text_buf );
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

    sprintf (filename, "mythryl-ncurses-library-in-c-subprocess-%d.log~", getpid());

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

										void dummy_call_to__find_free_callback_id() { find_free_callback_id (); }	// Can delete this as soon as we have some real calls -- this is just to quiet gcc.

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

static int               window_size_event_callback_number;
static void GLFWCALL run_window_size_event_callback	(int wide, int high){

    char buf[ 4096 ];

    sprintf( buf, "%d %d %d", window_size_event_callback_number, wide, high );

    printf (             "WINDOW_SIZE_CALLBACK%s\n", buf); fflush( stdout );
    fprintf(log_fd,"SENT: WINDOW_SIZE_CALLBACK%s\n", buf); fflush( log_fd );
}
										void dummy_call_to__run_window_size_event_callback() { run_window_size_event_callback (1,2); }	// Can delete this as soon as we have some real calls -- this is just to quiet gcc.




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

#ifdef SOON
static void
do_test( int argc, unsigned char** argv )
{
    printf ("TEST argc d=%d\n", argc);								fflush(stdout);

    for (int i = 0; i < argc; ++i) {
	//
	printf ("ARG argv[%d] s='%s' (len d=%d)\n", i, argv[i], strlen( (char*) (argv[i])) );	fflush(stdout);
    }
}
#endif

#ifdef OLD
static void
do_quit_eventloop( int argc, unsigned char** argv )
{
    // This call will result in
    // the (innermost call of)
    // gtk_main() returning:
    //
    gtk_main_quit();
}
#endif

static void
do_init( int argc, unsigned char** argv )
{
printf("do_init/top -- src/glu/ncurses/c/in-sub/mythryl-ncurses-library-in-c-subprocess.c\n");fflush(stdout);
    if (!glfwInit()) {
	//
        sprintf( text_buf, "do_init: failed to initialize Ncurses support.");
	moan_and_die();
} else {
puts("NB: glfwInit() returned TRUE -- mythryl-ncurses-library-in-c-subprocess.c\n"); fflush(stdout);
    }

// Apparently we should do this only AFTER creating our first window (more precisely, "valid Ncurses rendering context"):
// printf("do_init/AAA: calling glewInit -- src/glu/ncurses/c/in-sub/mythryl-ncurses-library-in-c-subprocess.c\n");fflush(stdout);
    // This paragraph is from:
    //     http://ogldev.atspace.co.uk/www/tutorial02/tutorial02.htm
    //
//    GLenum result = glewInit();
//    if (result != GLEW_OK)
//      {
//	fprintf(stderr, "Error: '%s'\n", glewGetErrorString(result));
//	moan_and_die();
//      }
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
//  -> build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'
//     -> build_fun_header_for_'mythryl_xxx_library_in_c_subprocess_c'
//      + build_fun_arg_loads_for_'mythryl_xxx_library_in_c_subprocess_c'
//       							# Optionally invokes new_widget_custom_body_plain_fun_subprocess
//                                                              # or                     widget_custom_body_plain_fun_subprocess,
//                                                              # from src/glu/ncurses/sh/make-ncurses-glue
// 
// Paragraphs like
//     build-a: callback-fn
//     fn-name:
//     fn-type:
// drive the code-build path
//   mlb::BUILD_A ("callback-fn", build_callback_function)			# In src/glu/ncurses/sh/make-ncurses-glue
//   ->  build_callback_function						# In src/glu/ncurses/sh/make-ncurses-glue
//       ->  build_set_callback_fn_for_'mythryl_xxx_library_in_c_subprocess_c'	# In src/glu/ncurses/sh/make-ncurses-glue
//           ->  r.to_mythryl_xxx_library_in_c_subprocess_c_funs		# In src/lib/make-library-glue/make-library-glue.pkg
//
/* Do not edit this or following lines -- they are autobuilt.  (patchname="functions") */

static void
do__set_window_size_event_callback( int argc, unsigned char** argv )
{
    check_argc( "do__set_window_size_event_callback", 0, argc );

    int id   =  find_free_callback_id ();
    window_size_event_callback_number =  id;

    glfwSetWindowSizeCallback( run_window_size_event_callback );

     printf(              "set_window_size_event_callback%d\n", id );      fflush( stdout );
    fprintf(log_fd, "SENT: set_window_size_event_callback%d\n", id );      fflush( log_fd );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_set_callback_fn_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan.*/

static void
do__glew_init( int argc, unsigned char** argv )
{
    check_argc( "do__glew_init", 0, argc );


    GLenum result = glewInit();;
   if (result != GLEW_OK) {
       fprintf(stderr, "Error: '%s'\n", glewGetErrorString(result));
       exit(1);
   }
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan. */

static void
do__open_window2( int argc, unsigned char** argv )
{
    check_argc( "do__open_window2", 9, argc );

    int               i0 =                         int_arg( argc, argv, 0 );
    int               i1 =                         int_arg( argc, argv, 1 );
    int               i2 =                         int_arg( argc, argv, 2 );
    int               i3 =                         int_arg( argc, argv, 3 );
    int               i4 =                         int_arg( argc, argv, 4 );
    int               i5 =                         int_arg( argc, argv, 5 );
    int               i6 =                         int_arg( argc, argv, 6 );
    int               i7 =                         int_arg( argc, argv, 7 );
    int               b8 =                        bool_arg( argc, argv, 8 );

    int result = glfwOpenWindow(   /*wide*/i0, /*high*/i1,   /*redbits*/i2, /*greenbits*/i3, /*bluebits*/i4,   /*alphabits*/i5, /*depthbits*/i6, /*stencilbits*/i7,   /*fullscreen*/b8 ? GLFW_FULLSCREEN : GLFW_WINDOW );

     printf(              "open_window2%d\n", result);      fflush( stdout );
    fprintf(log_fd, "SENT: open_window2%d\n", result);      fflush( log_fd );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan. */

static void
do__open_window( int argc, unsigned char** argv )
{
    check_argc( "do__open_window", 2, argc );

    int               i0 =                         int_arg( argc, argv, 0 );
    int               i1 =                         int_arg( argc, argv, 1 );

    int result = glfwOpenWindow(   /*wide*/i0, /*high*/i1,   /*redbits*/0, /*greenbits*/0, /*bluebits*/0,   /*alphabits*/0, /*depthbits*/0, /*stencilbits*/0,   /*fullscreen*/GLFW_WINDOW );

     printf(              "open_window%d\n", result);      fflush( stdout );
    fprintf(log_fd, "SENT: open_window%d\n", result);      fflush( log_fd );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan. */

static void
do__terminate( int argc, unsigned char** argv )
{
    check_argc( "do__terminate", 0, argc );


    glfwTerminate();
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan. */

static void
do__swap_buffers( int argc, unsigned char** argv )
{
    check_argc( "do__swap_buffers", 0, argc );


    glfwSwapBuffers();
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan. */

static void
do__get_window_param( int argc, unsigned char** argv )
{
    check_argc( "do__get_window_param", 0, argc );


    int result = glfwGetWindowParam( GLFW_OPENED );

     printf(              "get_window_param%d\n", result);      fflush( stdout );
    fprintf(log_fd, "SENT: get_window_param%d\n", result);      fflush( log_fd );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan. */

static void
do__set_window_title( int argc, unsigned char** argv )
{
    check_argc( "do__set_window_title", 1, argc );

    char*             s0 =                      string_arg( argc, argv, 0 );

    glfwSetWindowTitle( s0 );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan. */

static void
do__set_window_size( int argc, unsigned char** argv )
{
    check_argc( "do__set_window_size", 2, argc );

    int               i0 =                         int_arg( argc, argv, 0 );
    int               i1 =                         int_arg( argc, argv, 1 );

    glfwSetWindowSize( /*wide*/i0, /*high*/i1 );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan. */

static void
do__set_window_position( int argc, unsigned char** argv )
{
    check_argc( "do__set_window_position", 2, argc );

    int               i0 =                         int_arg( argc, argv, 0 );
    int               i1 =                         int_arg( argc, argv, 1 );

    glfwSetWindowPos( /*x*/i0, /*y*/i1 );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan. */

static void
do__clear( int argc, unsigned char** argv )
{
    check_argc( "do__clear", 2, argc );

    int               b0 =                        bool_arg( argc, argv, 0 );
    int               b1 =                        bool_arg( argc, argv, 1 );

    glClear(   (/*color_buffer*/b0 ? GL_COLOR_BUFFER_BIT : 0)  |  (/*depth_buffer*/b1 ? GL_DEPTH_BUFFER_BIT : 0));
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan. */

static void
do__print_hello_world( int argc, unsigned char** argv )
{
    check_argc( "do__print_hello_world", 0, argc );


    fprintf(stderr,"Hello, world!\n");
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan. */

static void
do__negate_int( int argc, unsigned char** argv )
{
    check_argc( "do__negate_int", 1, argc );

    int               i0 =                         int_arg( argc, argv, 0 );

    int result = -i0;

     printf(              "negate_int%d\n", result);      fflush( stdout );
    fprintf(log_fd, "SENT: negate_int%d\n", result);      fflush( log_fd );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan. */

static void
do__negate_float( int argc, unsigned char** argv )
{
    check_argc( "do__negate_float", 1, argc );

    double            f0 =                      double_arg( argc, argv, 0 );

    double result = -f0;

     printf(              "negate_float%f\n", result);      fflush( stdout );
    fprintf(log_fd, "SENT: negate_float%f\n", result);      fflush( log_fd );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan. */

static void
do__negate_boolean( int argc, unsigned char** argv )
{
    check_argc( "do__negate_boolean", 1, argc );

    int               b0 =                        bool_arg( argc, argv, 0 );

    int result = !b0;

     printf(              "negate_boolean%d\n", result);      fflush( stdout );
    fprintf(log_fd, "SENT: negate_boolean%d\n", result);      fflush( log_fd );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/ncurses/etc/construction.plan. */
/* Do not edit this or preceding lines -- they are autobuilt. */
/////////////////////////////////////////////////////////////////////////////////////



#ifdef OLD
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
#endif

#ifdef OLD
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
#endif

#ifdef OLD
static void
do_unref_object( int argc, unsigned char** argv )
{
    check_argc( "do_unref_object", 1, argc );

    {   GtkWidget* w0    =  widget_arg( argc, argv, 0 );

	g_object_unref( G_OBJECT( w0 ) );

        widget[ get_widget_id( w0 ) ] = 0;
    }
}
#endif

#ifdef OLD
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
#endif

#ifdef OLD
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
#endif

#ifdef OLD
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
#endif

static void
init  (void)
{
    open_logfile ();

    /* Initialize our verb trie:
    */
    trie = make_trie_node();

    /* Define the set of supported input line verbs:
    */

    set_trie( trie, "init",					do_init						);

#ifdef OLD
    set_trie( trie, "make_dialog",				do_make_dialog					);
    set_trie( trie, "unref_object",				do_unref_object					);
    set_trie( trie, "quit_eventloop",				do_quit_eventloop				);
    set_trie( trie, "run_eventloop_indefinitely",		do_run_eventloop_indefinitely			);
    set_trie( trie, "run_eventloop_once",			do_run_eventloop_once				);
    set_trie( trie, "test",					do_test						);
    set_trie( trie, "get_widget_allocation",			do_get_widget_allocation			);
#endif

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
    //     -> build_trie_entry_for_'mythryl_xxx_library_in_c_subprocess_c'
    //        -> to_mythryl_xxx_library_in_c_subprocess_c_trie
    // 
    // Paragraphs like
    //     build-a: callback-fn
    //     fn-name:
    //     fn-type:
    //     lowtype:
    // drive the code-build path
    //   mlb::BUILD_A ("callback-fn", build_callback_function)				# In src/glu/ncurses/sh/make-ncurses-glue
    //   ->  build_callback_function							# In src/glu/ncurses/sh/make-ncurses-glue
    //       ->  r.build_trie_entry_for_'mythryl_xxx_library_in_c_subprocess_c'		# In src/lib/make-library-glue/make-library-glue.pkg
    //
    /* Do not edit this or following lines -- they are autobuilt.  (patchname="table") */
    set_trie( trie, "set_window_size_event_callback",             do__set_window_size_event_callback            );
    set_trie( trie, "glew_init",                                  do__glew_init                                 );
    set_trie( trie, "open_window2",                               do__open_window2                              );
    set_trie( trie, "open_window",                                do__open_window                               );
    set_trie( trie, "terminate",                                  do__terminate                                 );
    set_trie( trie, "swap_buffers",                               do__swap_buffers                              );
    set_trie( trie, "get_window_param",                           do__get_window_param                          );
    set_trie( trie, "set_window_title",                           do__set_window_title                          );
    set_trie( trie, "set_window_size",                            do__set_window_size                           );
    set_trie( trie, "set_window_position",                        do__set_window_position                       );
    set_trie( trie, "clear",                                      do__clear                                     );
    set_trie( trie, "print_hello_world",                          do__print_hello_world                         );
    set_trie( trie, "negate_int",                                 do__negate_int                                );
    set_trie( trie, "negate_float",                               do__negate_float                              );
    set_trie( trie, "negate_boolean",                             do__negate_boolean                            );
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

