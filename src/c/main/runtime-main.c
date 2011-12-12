// main.c
//
// This is the main routine for the runtime
// of the interactive version of Mythryl.

#include "../mythryl-config.h"

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <ctype.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "runtime-base.h"
#include "runtime-commandline-argument-processing.h"
#include "runtime-configuration.h"
#include "runtime-globals.h"



#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "heapcleaner-statistics-2.h"


FILE* DebugF = NULL;	// Referenced only here and in   src/c/main/error-reporting.c
    //
    // debug_say writes to DebugF -- see src/c/main/error-reporting.c
    //
    // DebugF defaults to stderr;  it may be set via
    //
    //     --runtime-debug=foo.log
    //
    // commandline switch.

// Runtime globals:
//
int    verbosity = 0;
Bool   codechunk_comment_display_is_enabled__global = FALSE;
Bool   cleaner_messages_are_enabled__global = FALSE;
Bool   unlimited_heap_is_enabled__global = FALSE;
char** raw_args;
char** commandline_arguments;		// Does not include the program name (argv[0]).
char*  mythryl_program_name__global;		// The program name used to invoke the runtime.



// Local variables:
//
static Bool	is_boot = FALSE;	                 	// TRUE iff we should bootstrap a system.
static char*	heap_image_to_run_filename = DEFAULT_IMAGE;	// The path name of the image file to load.
		   
static char*	compiled_files_to_load_filename = NULL;

 #if NEED_PTHREAD_SUPPORT
    static int num_procs = 1;					// Set by --runtime-nprocs=12, not otherwise used.
 #endif

static void   process_environment_options (Heapcleaner_Args** heapcleaner_args);
static void   process_commandline_options (int argc, char** argv, Heapcleaner_Args** heapcleaner_args);

static  Heapcleaner_Args*  do_start_of_world_stuff(        int argc, char** argv);
static  void               do_end_of_world_stuff_and_exit( void                 );



int   main   (int argc, char** argv) {
    //====
    //
    Heapcleaner_Args*	heapcleaner_args
	=
	do_start_of_world_stuff( argc, argv );


    if (is_boot)         load_compiled_files(  compiled_files_to_load_filename, heapcleaner_args );			// load_compiled_files					def in   src/c/main/load-compiledfiles.c
    //
    else 	         load_and_run_heap_image( heap_image_to_run_filename,   heapcleaner_args );			// load_and_run_heap_image				def in   src/c/main/load-and-run-heap-image.c


    do_end_of_world_stuff_and_exit();
}



static Heapcleaner_Args*   do_start_of_world_stuff  (int argc,  char** argv)   {
    //
    Heapcleaner_Args*	heapcleaner_args;

    process_environment_options( &heapcleaner_args );

    if (verbosity > 0) {
	printf("\n");
	printf("--------------------------------------------------------\n");
	printf("mythryl-runtime-intel32:    src/c/main/runtime-main.c:   %d args:\n",argc);
	{   int  i;
	    for (i = 0; i < argc; ++i) {
		printf("                                                %s\n", argv[i]);
	    }
	}
	printf("\n");
    }
    DebugF = stderr;

    process_commandline_options( argc, argv, &heapcleaner_args );

    set_up_timers ();													// set_up_timers					def in    src/c/main/posix-timers.c
															// set_up_timers					def in    src/c/main/win32-timers.c
    publish_runtime_package_contents ();										// publish_runtime_package_contents			def in    src/c/main/construct-runtime-package.c

    set_up_list_of_c_functions_callable_from_mythryl ();								// set_up_list_of_c_functions_callable_from_mythryl	def in    src/c/lib/mythryl-callable-c-libraries.c

    #if NEED_PTHREAD_SUPPORT
	//
        pth__start_up();												// pth__start_up					def in    src/c/pthread/pthread-on-posix-threads.c
    #endif														// pth__start_up					def in    src/c/pthread/pthread-on-solaris.c
															// pth__start_up					def in    src/c/pthread/pthread-on-sgi.c
    return heapcleaner_args;
}

static void   do_end_of_world_stuff_and_exit   (void)  {
    //
    pth__shut_down();
    print_stats_and_exit( 0 );												// Never returns.
    exit( 0 );														// Redundant -- just to suppress gcc warning.
}

static void   process_environment_options   (Heapcleaner_Args**  cleaner_args) {
    //
    char* vebosity_string = getenv( "MYTHRYL_VERBOSITY" );
    if   (vebosity_string) {
          verbosity = atoi( vebosity_string );
    }
}



static void   process_commandline_options   (
    //
    int                 argc,
    char**              argv,
    Heapcleaner_Args**  cleaner_args
) {
    char	option[ MAX_COMMANDLINE_ARGUMENT_PART_LENGTH ];
    char*	option_arg;
    char**	next_arg;

    Bool	seen_error = FALSE;

    // Special-case handling for heap images that invoke us
    // via the shebang mechanism of having
    //    #/usr/bin/mythryl-runtime-intel32
    // at the top of the file.
    // We presume this to be the case if arg0
    // ends in "/mythryl-runtime-intel32"":
    // and arg1 starts with "--shebang":
    //
    if (argc > 1) {
        char* arg0 = argv[0];
        if (*arg0) {
	    int   arg0len = 0;    
	    char* arg0end = arg0;   do {   ++arg0len; } while (*++arg0end);
	   if (								// Should maybe use some string lib fn here instead!

	    // This is deprecated logic.  It may be
	    // best to keep it around for awhile to
	    // support people upgrading via older binaries...? -- 2010-12-11 CrT
	    //
            ((  arg0end[-1] == '7'					// Match   /runtime7   suffix, end-first.
            &&  arg0end[-2] == 'e' 
            &&  arg0end[-3] == 'm' 
            &&  arg0end[-4] == 'i' 
            &&  arg0end[-5] == 't'
            &&  arg0end[-6] == 'n' 
            &&  arg0end[-7] == 'u' 
            &&  arg0end[-8] == 'r' 
            &&  arg0end[-9] == '/' 
            )||(

            // This is the current logic:
	    //
               (arg0end[ -1] == '2' || arg0end[ -1] == '4')		// Match   /mythryl-runtime-intel32   or   /mythryl-runtime-ia64   suffix, end-first.
            && (arg0end[ -2] == '3' || arg0end[ -2] == '6')
            &&  arg0end[ -3] == 'a' 
            &&  arg0end[ -4] == 'i' 
            &&  arg0end[ -5] == '-'    // Should maybe use some string lib fn here instead.
            &&  arg0end[ -6] == 'e' 
            &&  arg0end[ -7] == 'm' 
            &&  arg0end[ -8] == 'i' 
            &&  arg0end[ -9] == 't' 
            &&  arg0end[-10] == 'n' 
            &&  arg0end[-11] == 'u' 
            &&  arg0end[-12] == 'r' 
            &&  arg0end[-13] == '-' 
            &&  arg0end[-14] == 'l' 
            &&  arg0end[-15] == 'y' 
            &&  arg0end[-16] == 'r' 
            &&  arg0end[-17] == 'h' 
            &&  arg0end[-18] == 't' 
            &&  arg0end[-19] == 'y' 
            &&  arg0end[-20] == 'm' 
            &&  arg0end[-21] == '/' 

            )||(

            // This is future logic:
	    //
               (arg0end[ -1] == '2' || arg0end[ -1] == '4')		// Match   /mythryl-runtime-intel32   or   /mythryl-runtime-intel64   suffix, end-first.
            && (arg0end[ -2] == '3' || arg0end[ -2] == '6')
            &&  arg0end[ -3] == 'l' 
            &&  arg0end[ -4] == 'e' 
            &&  arg0end[ -5] == 't' 
            &&  arg0end[ -6] == 'n' 
            &&  arg0end[ -7] == 'i' 
            &&  arg0end[ -8] == '-'    // Should maybe use some string lib fn here instead.
            &&  arg0end[ -9] == 'e' 
            &&  arg0end[-10] == 'm' 
            &&  arg0end[-11] == 'i' 
            &&  arg0end[-12] == 't' 
            &&  arg0end[-13] == 'n' 
            &&  arg0end[-14] == 'u' 
            &&  arg0end[-15] == 'r' 
            &&  arg0end[-16] == '-' 
            &&  arg0end[-17] == 'l' 
            &&  arg0end[-18] == 'y' 
            &&  arg0end[-19] == 'r' 
            &&  arg0end[-20] == 'h' 
            &&  arg0end[-21] == 't' 
            &&  arg0end[-22] == 'y' 
            &&  arg0end[-23] == 'm' 
            &&  arg0end[-24] == '/' 
            )
            )

            &&  argv[1][0] == '-' 
            &&  argv[1][1] == '-' 
            &&  argv[1][2] == 's' 
            &&  argv[1][3] == 'h' 
            &&  argv[1][4] == 'e' 
            &&  argv[1][5] == 'b' 
            &&  argv[1][6] == 'a' 
            &&  argv[1][7] == 'n' 
            &&  argv[1][8] == 'g' 
	    ){
		//////////////////////////////////////////////////////
		// Any switches on the shebang line will be either
		// collected in arg1 or scattered through arg1-argN,
		// depending on the operating system, according to
		//
		// http://homepages.cwi.nl/~aeb/std/hashexclam.html
		// See also:
		// http://en.wikipedia.org/wiki/Shebang_(Unix)
		//
		// For now, we require that the shebang line have
		// the switch "--shebang" immediately following the
		// executable path.  This is clumsy, but we don't
		// anticipate end-user use of the facility, and it
		// does minimize the possibility of confusion
		// between shebang and non-shebang invocations.
		//
		// For now, we silently ignore any other shebang-line switches:
		//////////////////////////////////////////////////////

	        ++argv; --argc;                           // Skip the mythryl-runtime-intel32 path.
                while (**argv == '-') { ++argv; --argc; } // Ignore all shebang line switches.
		heap_image_to_run_filename = *argv;       // Remember heap file to load.
//		++argv; --argc;                           // Hide heapfile from subquent logic.

                if (verbosity > 0) {
		    fprintf(
			stderr,
			"Looks like a shebang invocation -- heap_image_to_run_filename set to '%s'\n",
			heap_image_to_run_filename
		    );
                }
	    }
        }
    }

    // Scan for any heap/cleaner parameters:
    //
    if (!(*cleaner_args = handle_heapcleaner_commandline_arguments(argv))) {		// handle_heapcleaner_commandline_arguments		is from   src/c/heapcleaner/heapcleaner-initialization.c
	seen_error = TRUE;
    }

    raw_args = argv;

    if (verbosity > 0) {
        printf("             src/c/main/runtime-main.c:   Constructing a %d-slot commandline_arguments perlvector...\n",argc);
    }

    commandline_arguments = MALLOC_VEC(char*, argc);

    if (verbosity > 0) {
        printf("             src/c/main/runtime-main.c:   Setting mythryl_program_name__global to '%s'...\n",*argv);
    }

    mythryl_program_name__global = *argv++;
    next_arg = commandline_arguments;
    while (--argc > 0) {
	char	*arg = *argv++;

	#define MATCH(opt)	(strcmp(opt, option) == 0)

	#define CHECK(opt)	{									\
				    if (option_arg[0] == '\0') {					\
					seen_error = TRUE;						\
					say_error( "Missing argument for \"%s\" option\n", opt );		\
					continue;							\
				    }									\
				}


        // Process only commandline arguments starting with
        //
        //     --runtime-
        //
	if (is_runtime_option(arg, option, &option_arg)) {	// is_runtime_option	is from   src/c/main/runtime-options.c
	    //
	    if (MATCH("compiledfiles-to-load")) {
		CHECK("compiledfiles-to-load");
		is_boot = TRUE;
		compiled_files_to_load_filename = option_arg;

                if (verbosity > 0) {
                    printf("             src/c/main/runtime-main.c:   --runtime-compiledfiles-to-load setting compiled_files_to_load_filename to '%s'...\n",compiled_files_to_load_filename);
                }

	    } else if (MATCH("heap-image-to-run")) {

		CHECK("heap-image-to-run");
		heap_image_to_run_filename = option_arg;
                if (verbosity > 0) {
                    printf("             src/c/main/runtime-main.c:   --runtime-heap-image-to-run setting heap_image_to_run_filename to '%s'...\n", heap_image_to_run_filename);
                }

	    } else if (MATCH("cmdname")) {

		CHECK("cmdname");
		mythryl_program_name__global = option_arg;
                if (verbosity > 0) {
                    printf("             src/c/main/runtime-main.c:   --runtime-cmdname setting mythryl_program_name__global to '%s'...\n", mythryl_program_name__global);
                }
#if NEED_PTHREAD_SUPPORT
	    } else if (MATCH("nprocs")) {

		CHECK("nprocs");

		num_procs =  atoi( option_arg );

                // Clamp NumProcs to a sane range:
		//
		if      (num_procs < 0)
		         num_procs = 0;
		else if (num_procs > MAX_PTHREADS)
		         num_procs = MAX_PTHREADS;
#endif
	    } else if (MATCH("verbosity")) {

		CHECK("verbosity");
		verbosity = atoi(option_arg);

	    } else if (MATCH("show-code-chunk-comments")) {

	        codechunk_comment_display_is_enabled__global = TRUE;

	    } else if (MATCH("debug")) {                          	// "debug" is a terrible name for this switch XXX BUGGO FIXME

		CHECK("debug");

		if ((DebugF = fopen(option_arg, "w")) == NULL) {
		    //
		    DebugF = stderr;					// Restore the file pointer.
		    seen_error = TRUE;
		    say_error( "Unable to open debug output file \"%s\"\n", *(argv[-1]) );
		    continue;
		}

	    } else if (MATCH("stats")) {

		CHECK("stats");

		heapcleaner_statistics_fd__global = open (option_arg, O_WRONLY|O_TRUNC|O_CREAT, 0666);

		if (heapcleaner_statistics_fd__global == -1) {
		    //
		    seen_error = TRUE;
		    say_error( "Unable to open statistics file \"%s\"\n", *(argv[-1]) );
		    continue;
		}
            }
	} else {
            if (verbosity > 0) {
                printf("             src/c/main/runtime-main.c:   Setting commandline_arguments[%d] to '%s'...\n", next_arg-commandline_arguments,arg);
            } 
	    *next_arg++ = arg;
	}
    }

    if (verbosity > 0)   printf("\n");
    *next_arg = NULL;

    if (seen_error)   print_stats_and_exit( 1 );

}						// process_commandline_options


void   print_stats_and_exit   (int code)   {
    // ====================
    //
    // Exit from the Mythryl system.
    //
    #if COUNT_REG_MASKS
	dump_masks();
    #endif

    if (heapcleaner_statistics_fd__global >= 0) {
	STATS_FLUSH_BUF();
	close (heapcleaner_statistics_fd__global);
    }

    exit( code );
}


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


