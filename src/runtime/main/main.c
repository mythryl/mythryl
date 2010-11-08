/* main.c
 *
 * This is the main routine for the interactive version of lib7.
 */

#include "../config.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "runtime-base.h"
#include "runtime-options.h"
#include "runtime-limits.h"
#include "runtime-globals.h"

#ifdef COLLECT_STATS

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "stats-data.h"
#endif

FILE* DebugF = NULL;
    /*
     * SayDebug writes to DebugF -- see src/runtime/main/error.c
     *
     * DebugF defaults to stderr;  it may be set via
     *
     *     --runtime-debug=foo.log
     *
     * commandline switch.
     */

#ifdef TARGET_BYTECODE
FILE		*BC_stdout = NULL;
#endif

/* Runtime globals:
*/
int		verbosity = 0;
bool_t          show_code_chunk_comments = FALSE;
bool_t		GCMessages = FALSE;
bool_t		UnlimitedHeap = FALSE;
char		**raw_args;
char		**commandline_arguments;	/* does not include the command name (argv[0]) */
char		*Lib7CommandName;	/* the command name used to invoke the runtime */

/* Local variables:
*/
static bool_t	is_boot = FALSE;	                 /* TRUE iff we should bootstrap a system.    */
static char*	heap_image_to_run_filename = DEFAULT_IMAGE; /* The path name of the image file to load.  */
static char*	heap_image_to_write_filename = NULL;     /* The path name of the image file to write. */
		   
static char	*o7files_to_load_filename = NULL;
# ifdef MP_SUPPORT
static int		NumProcs = 1;	/* not used */
# endif

static void process_environment_options (heap_params_t** heapParams);
static void process_commandline_options (int argc, char** argv, heap_params_t** heapParams);


int
main (int argc, char **argv)
{
    heap_params_t	*heapParams;

    process_environment_options   (   &heapParams   );

    if (verbosity > 0) {
	printf("\n");
	printf("--------------------------------------------------------\n");
	printf("runtime7:            src/runtime/main/main.c:   %d args:\n",argc);
	{   int  i;
	    for (i = 0; i < argc; ++i) {
		printf("                                                %s\n", argv[i]);
	    }
	}
	printf("\n");
    }
    DebugF = stderr;

    process_commandline_options (argc, argv, &heapParams);

#ifdef TARGET_BYTECODE
    if (BC_stdout == NULL) {
	BC_stdout = fdopen(dup(1), "w");
    }
#endif

    set_up_timers ();
    record_globals ();
    InitCFunList ();

#ifdef MP_SUPPORT
    MP_Init();
#endif

    /* Start Lib7d: */
    if (is_boot) {
        load_oh7_files (o7files_to_load_filename, heap_image_to_write_filename, heapParams);
    } else {
	load_and_run_heap_image( heap_image_to_run_filename, heapParams );
    }

    Exit (0);
}



static void   process_environment_options   (   heap_params_t**  heapParams   )
{
    char* vebosity_string = getenv( "LIB7_VERBOSITY" );
    if   (vebosity_string) {
          verbosity = atoi( vebosity_string );
    }
}



static void   process_commandline_options   (   int              argc,
						char**           argv,
						heap_params_t**  heapParams
) {
    char	option[ MAX_OPT_LEN ];
    char*	option_arg;
    char**	next_arg;

    bool_t	seen_error = FALSE;

    /* Special-case handling for heap images that invoke us
     * via the shebang mechanism of having
     *    #/usr/bin/runtime7
     * at the top of the file.
     * We presume this to be the case if arg0
     * ends in "/runtime7"":
     * and arg1 starts with "--shebang":
     */
    if (argc > 1) {
        char* arg0 = argv[0];
        if (*arg0) {
	    char* arg0end = arg0;    while (*++arg0end);
            if (arg0end[-1] == '7' 
            &&  arg0end[-2] == 'e' 
            &&  arg0end[-3] == 'm' 
            &&  arg0end[-4] == 'i' 
            &&  arg0end[-5] == 't'    /* Should maybe use some string lib fn here instead. */
            &&  arg0end[-6] == 'n' 
            &&  arg0end[-7] == 'u' 
            &&  arg0end[-8] == 'r' 
            &&  arg0end[-9] == '/' 

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
		/****************************************************/
		/* Any switches on the shebang line will be either  */
		/* collected in arg1 or scattered through arg1-argN,*/
		/* depending on the operating system, according to  */
		/*                                                  */ 
		/* http://homepages.cwi.nl/~aeb/std/hashexclam.html */
		/* See also:                                        */ 
		/* http://en.wikipedia.org/wiki/Shebang_(Unix)      */ 
		/*                                                  */ 
		/* For now, we require that the shebang line have   */
		/* the switch "--shebang" immediately following the */
		/* executable path.  This is clumsy, but we don't   */
		/* anticipate end-user use of the facility, and it  */
		/* does minimize the possibility of confusion       */
		/* between shebang and non-shebang invocations.     */
		/*                                                  */ 
		/* For now, we silently ignore any other shebang-   */ 
		/* line switches:                                   */ 
		/****************************************************/
	        ++argv; --argc;                           /* Skip the runtime7 path.            */
                while (**argv == '-') { ++argv; --argc; } /* Ignore all shebang line switches.  */
		heap_image_to_run_filename = *argv;       /* Remember heap file to load.        */
/*		++argv; --argc;                           /  Hide heapfile from subquent logic. */

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

    /* Scan for any heap/GC parameters: */
    if (!(*heapParams = ParseHeapParams(argv))) {
	seen_error = TRUE;
    }

    raw_args = argv;

    if (verbosity > 0) {
        printf("                     src/runtime/main/main.c:   Constructing a %d-slot commandline_arguments perlvector...\n",argc);
    }

    commandline_arguments = NEW_VEC(char*, argc);

    if (verbosity > 0) {
        printf("                     src/runtime/main/main.c:   Setting Lib7CommandName to '%s'...\n",*argv);
    }

    Lib7CommandName = *argv++;
    next_arg = commandline_arguments;
    while (--argc > 0) {
	char	*arg = *argv++;

#define MATCH(opt)	(strcmp(opt, option) == 0)

#define CHECK(opt)	{						\
			    if (option_arg[0] == '\0') {					\
				seen_error = TRUE;						\
				Error("missing argument for \"%s\" option\n", opt);		\
				continue;							\
			    }								\
                        } /* CHECK */


        /* Process only commandline arguments starting with
         *
         *     --runtime-
         */
	if (is_runtime_option(arg, option, &option_arg)) {	/* is_runtime_option	is from   src/runtime/main/runtime-options.c	*/

	    if (MATCH("o7-files-to-load")) {
		CHECK("o7-files-to-load");
		is_boot = TRUE;
		o7files_to_load_filename = option_arg;

                if (verbosity > 0) {
                    printf("                     src/runtime/main/main.c:   --runtime-o7-files-to-load setting o7files_to_load_filename to '%s'...\n",o7files_to_load_filename);
                }

	    } else if (MATCH("heap-image-to-run")) {

		CHECK("heap-image-to-run");
		heap_image_to_run_filename = option_arg;
                if (verbosity > 0) {
                    printf("                     src/runtime/main/main.c:   --runtime-heap-image-to-run setting heap_image_to_run_filename to '%s'...\n", heap_image_to_run_filename);
                }

	    } else if (MATCH("heap")) {

		CHECK("heap");
		heap_image_to_write_filename = option_arg;
 	        *next_arg++ = arg;			/* Copy this one through -- it gets read and used in src/lib/core/internal/boot-dictionary.pkg. */

	    } else if (MATCH("cmdname")) {

		CHECK("cmdname");
		Lib7CommandName = option_arg;
                if (verbosity > 0) {
                    printf("                     src/runtime/main/main.c:   --runtime-cmdname setting Lib7CommandName to '%s'...\n", Lib7CommandName);
                }
#ifdef MP_SUPPORT
	    } else if (MATCH("nprocs")) {

		CHECK("nprocs");
		NumProcs = atoi(option_arg);

                /* Clamp NumProcs to a sane range: */
		if      (NumProcs < 0)
		         NumProcs = 0;
		else if (NumProcs > MAX_NUM_PROCS)
		         NumProcs = MAX_NUM_PROCS;
#endif
	    } else if (MATCH("verbosity")) {

		CHECK("verbosity");
		verbosity = atoi(option_arg);

	    } else if (MATCH("show-code-chunk-comments")) {

	        show_code_chunk_comments = TRUE;

	    } else if (MATCH("debug")) {                          /* "debug" is a terrible name for this switch XXX BUGGO FIXME */

		CHECK("debug");

		if ((DebugF = fopen(option_arg, "w")) == NULL) {

		    DebugF = stderr;				/* Restore the file pointer.	*/
		    seen_error = TRUE;
		    Error("unable to open debug output file \"%s\"\n", *(argv[-1]));
		    continue;
		}
#ifdef COLLECT_STATS
	    } else if (MATCH("stats")) {

		CHECK("stats");

		StatsFD = open (option_arg, O_WRONLY|O_TRUNC|O_CREAT, 0666);

		if (StatsFD == -1) {
		    seen_error = TRUE;
		    Error("unable to open statistics file \"%s\"\n", *(argv[-1]));
		    continue;
		}
#endif

#ifdef TARGET_BYTECODE
	    } else if (MATCH("dump")) {

		CHECK("dump");
		BC_stdout = fopen (option_arg, "w");

#ifdef INSTR_TRACE
	    } else if (MATCH("trace")) {

		extern bool_t	traceOn;
		traceOn = TRUE;
#endif
#endif
            }
	} else {
            if (verbosity > 0) {
                printf("                     src/runtime/main/main.c:   Setting commandline_arguments[%d] to '%s'...\n", next_arg-commandline_arguments,arg);
            } 
	    *next_arg++ = arg;
	}
    }

    if (verbosity > 0)   printf("\n");
    *next_arg = NULL;

    if (seen_error)   Exit (1);

}                            /* process_commandline_options */


void   Exit   (int code)
{
    /* Exit from the Lib7 system. */
#if COUNT_REG_MASKS
    dump_masks();
#endif
#ifdef COLLECT_STATS
    if (StatsFD >= 0) {
	STATS_FLUSH_BUF();
	close (StatsFD);
    }
#endif
#if (defined(TARGET_BYTECODE) && defined(INSTR_COUNT))
    {
	extern void PrintInstrCount (FILE *f);
	PrintInstrCount (DebugF);
    }
#endif

    exit (code);
}


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

