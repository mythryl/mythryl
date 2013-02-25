// mythryl-opencv-library-in-c-subprocess.c -- a simple Opencv subprocess server for Mythryl.


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
//     src/glu/opencv/src/opencv-client.api
//     src/glu/opencv/src/opencv-client-for-library-in-main-process.pkg




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

FILE* log_fd = 0;

static char text_buf[ 1024 ];


static void   moan_and_die   (void)   {
    //        ============
    //
    printf( "src/c/opencv/mythryl-opencv-library-in-c-subprocess.c:  Fatal error:  %s  exit(1)ing.\n", text_buf );
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

    sprintf (filename, "mythryl-opencv-library-in-c-subprocess-%d.log~", getpid());

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

										void dummy_opencv_call_to__find_free_callback_id() { find_free_callback_id (); }	// Can delete this as soon as we have some real calls -- this is just to quiet gcc.

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
										void dummy_opencv_call_to__string_arg() { unsigned char*argv[10]; string_arg(1,argv,2); }	// Can delete this as soon as we have some real calls -- this is just to quiet gcc.

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
do_init( int argc, unsigned char** argv )
{
}


/////////////////////////////////////////////////////////////////////////////////////
// The following stuff gets built from paragraphs in
//     src/glu/opencv/etc/construction.plan
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
//                                                              # from src/glu/opencv/sh/make-opencv-glue
// 
// Paragraphs like
//     build-a: callback-fn
//     fn-name:
//     fn-type:
// drive the code-build path
//   mlb::BUILD_A ("callback-fn", build_callback_function)			# In src/glu/opencv/sh/make-opencv-glue
//   ->  build_callback_function						# In src/glu/opencv/sh/make-opencv-glue
//       ->  build_set_callback_fn_for_'mythryl_xxx_library_in_c_subprocess_c'	# In src/glu/opencv/sh/make-opencv-glue
//           ->  r.to_mythryl_xxx_library_in_c_subprocess_c_funs		# In src/lib/make-library-glue/make-library-glue.pkg
//
/* Do not edit this or following lines -- they are autobuilt.  (patchname="functions") */

static void
do__print_hello_world( int argc, unsigned char** argv )
{
    check_argc( "do__print_hello_world", 0, argc );


    fprintf(stderr,"Hello, world!\n");
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/opencv/etc/construction.plan. */

static void
do__negate_int( int argc, unsigned char** argv )
{
    check_argc( "do__negate_int", 1, argc );

    int               i0 =                         int_arg( argc, argv, 0 );

    int result = -i0;

     printf(              "negate_int%d\n", result);      fflush( stdout );
    fprintf(log_fd, "SENT: negate_int%d\n", result);      fflush( log_fd );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/opencv/etc/construction.plan. */

static void
do__negate_float( int argc, unsigned char** argv )
{
    check_argc( "do__negate_float", 1, argc );

    double            f0 =                      double_arg( argc, argv, 0 );

    double result = -f0;

     printf(              "negate_float%f\n", result);      fflush( stdout );
    fprintf(log_fd, "SENT: negate_float%f\n", result);      fflush( log_fd );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/opencv/etc/construction.plan. */

static void
do__negate_boolean( int argc, unsigned char** argv )
{
    check_argc( "do__negate_boolean", 1, argc );

    int               b0 =                        bool_arg( argc, argv, 0 );

    int result = !b0;

     printf(              "negate_boolean%d\n", result);      fflush( stdout );
    fprintf(log_fd, "SENT: negate_boolean%d\n", result);      fflush( log_fd );
}
/* Above fn built by src/lib/make-library-glue/make-library-glue.pkg:  build_plain_fun_for_'mythryl_xxx_library_in_c_subprocess_c'  per  src/glu/opencv/etc/construction.plan. */
/* Do not edit this or preceding lines -- they are autobuilt. */
/////////////////////////////////////////////////////////////////////////////////////





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


    /////////////////////////////////////////////////////////////////////////////////////
    // The following stuff gets built from paragraphs in
    //     src/glu/opencv/etc/construction.plan
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
    //   mlb::BUILD_A ("callback-fn", build_callback_function)				# In src/glu/opencv/sh/make-opencv-glue
    //   ->  build_callback_function							# In src/glu/opencv/sh/make-opencv-glue
    //       ->  r.build_trie_entry_for_'mythryl_xxx_library_in_c_subprocess_c'		# In src/lib/make-library-glue/make-library-glue.pkg
    //
    /* Do not edit this or following lines -- they are autobuilt.  (patchname="table") */
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

