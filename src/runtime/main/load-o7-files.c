/* load-o7-files.c
 *
 * This file implements the core Lib7 linker functionality
 * which combines oh7_files to produce a Lib7 "executable"
 * (actually a heap image which can be run via bin/runtime7).
 *
 * Our primary entrypoint is
 *      load_oh7_files(),
 * which is invoked from src/runtime/main/main.c:main()
 * when runtime7 is given the "--runtime-o7-files-to-load=filename"
 * commandline switch, where the "filename"
 * parameter is the name of a file containing a
 * list of filenames of the .o7 files to load, one per line. 
 * (Some "oh7_files" may in fact be at some given offset inside
 * a library archive file, in which case the syntax gets a bit involved.)
 *
 * Bootstrapping a fresh Lib7 install is accomplished by
 * including in the source distribution a set of libraries
 * (in "seed-libraries.x86-unix.tgz" or equivalent)
 * sufficient to build the Lib7 compiler proper plus
 * essential related tools such as the parser generator.
 * (Also included is the required OH7_FILES_TO_LOAD file.)
 *
 * We create the seed-libraries.x86-unix.tgz tarball
 * and then a full source distribution tarball by doing
 *     make self
 *     make fixpoint
 *     make seed
 *     make tar 
 * in a fully installed system, thus closing the circle.
 */

/* ###           "One of the main causes of the fall
   ###            of the Roman Empire was that, lacking zero,
   ###            they had no way to indicate successful
   ###            termination of their C programs."
   ###
   ###                               -- Robert Firth
 */   


#include "../config.h"

#include "runtime-osdep.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "runtime-base.h"
#include "runtime-limits.h"
#include "cache-flush.h"
#include "o7-file.h"
#include "runtime-heap.h"
#include "gc.h"
#include "runtime-globals.h"

#ifndef SEEK_SET
#  define SEEK_SET	0
#endif

/* The picklehash_to_exports_tree_list is stored in the PervasiveStruct refcell.
 * It is a singly-linked list of (key,val) pairs -- specifically,
 * (picklehash,heapblock) pairs -- with the following Lib7 type:
 *
 *    enum runDynDict
 *      = NILrde
 *      | CONSrde of (unt8_vector.Vector * Chunk.chunk * runDynDict)
 *
 * where 'unt8_vector.Vector' is the 16-byte hash of 'Chunk.chunk'.
 */


#define picklehash_to_exports_tree_list	(*PTR_LIB7toC(lib7_val_t, PervasiveStruct))

static lib7_val_t	oh7_file_list = LIST_nil;	/* A list of .o7 files to load */


/* Forward declarations for private local functions: */
static FILE*       open_file                     (const char* filename,   bool_t isBinary);
static void        oh7_file_read               (FILE* file, void* buf, int nbytes, const char* filename);
static void        load_oh7_file               (lib7_state_t* lib7_state, char* filename);
static void        register_oh7_file_exports   (lib7_state_t* lib7_state, picklehash_t* picklehash, lib7_val_t chunk);
static lib7_val_t picklehash_to_exports_tree    (picklehash_t* picklehash);
static void        picklehash_to_hex_string      (char *buf, picklehash_t* picklehash);
static lib7_val_t read_in_oh7_file_list       (  lib7_state_t* lib7_state,
                                                    const char*    o7files_to_load_filename,
			                            int*           max_boot_path_len_ptr
                                                 );

static FILE*       log_fd = NULL;

/* Compute integer value of an ascii hex digit:
*/
static int hex(int c)
{
  if (isdigit(c))           return c - '0';
  if (c >= 'a' && c <= 'z') return c - 'a' + 10;
  else                      return c - 'A' + 10;
}

#undef  MAX_FILENAME
#define MAX_FILENAME 1024
#define LOGFILE_EXTENSION  ".load.log"



static void

maybe_open_logfile ( char* heap_file_to_write_filename ) {

    char filename[ MAX_FILENAME ];

    if (heap_file_to_write_filename
    &&  strlen(heap_file_to_write_filename) + strlen(LOGFILE_EXTENSION) + 10 < MAX_FILENAME
    ){
        strcpy( filename, heap_file_to_write_filename );
        strcat( filename, "-" );
        sprintf(filename + strlen(filename), "%d", getpid() );
        strcat( filename, LOGFILE_EXTENSION );

	log_fd = fopen( filename, "w" );

	fprintf(stderr, "\n        src/runtime/main/load-o7-files.c:   Writing load log to               %s\n\n", filename);
    }
}



void   

load_oh7_files   (

    const char*      o7files_to_load_filename,
    const char*      heap_file_to_write_filename,		/* Only used here to build the logfile name.		*/
    heap_params_t*   heap_parameters
)
{
    /* Load into the runtime heap all the .o7 files
     * listed one per line in given o7files_to_load file:
     */
    int              max_boot_path_len;
    char*	     filename_buf;

    int         seen_runtime_system_picklehash = FALSE;       /* FALSE until we see the picklehash naming our runtime. */

    maybe_open_logfile( heap_file_to_write_filename );

    lib7_state_t*   lib7_state = AllocLib7state (TRUE, heap_parameters);

#ifdef HEAP_MONITOR
    if (set_up_heap_monitor(commandline_arguments, lib7_state->lib7_heap) == FAILURE) {
	Die("unable to start heap monitor");
    }
#endif

    set_up_fault_handlers ();
    allocate_globals (lib7_state);

    /* Construct the list of files to be loaded: */
    oh7_file_list = read_in_oh7_file_list (lib7_state, o7files_to_load_filename, &max_boot_path_len);

    /* This space is ultimately wasted:           XXX BUGGO FIXME */
    if (!(filename_buf = MALLOC (max_boot_path_len))) {
	Die ("unable to allocate space for boot file names");
    }

    /* Load all requested oh7_files into the heap: */
    while (oh7_file_list != LIST_nil) {

        char* filename = filename_buf;

        /* Need to make a copy of the filename because
         * load_oh7_file is going to scribble into it:
         */
	strcpy(filename_buf, STR_LIB7toC( LIST_hd( oh7_file_list )));
       
	oh7_file_list = LIST_tl( oh7_file_list );

	if (strstr(filename,"RUNTIME_SYSTEM_PICKLEHASH=") != filename) {

	    load_oh7_file (lib7_state, filename);

	} else {

	    while (*filename++ != '=');   /* Step over "RUNTIME_SYSTEM_PICKLEHASH=" prefix. */

	    if (seen_runtime_system_picklehash) {

                if (log_fd) fclose( log_fd );

		Die ("Runtime system picklehash registered more than once!\n");

	    } else {

	        /* Most parts of the Lib7 implementation treat the C-coded */
                /* runtime functions as being just like library functions   */
                /* coded in Lib7 -- to avoid special cases, we go to great */
                /* lengths to hide the differences.                         */
                /*                                                          */
                /* But this is one of the places where the charade breaks   */
                /* down -- there isn't actually any (useful) oh7_file         */
                /* corresponding to the runtime picklehash:  Instead, we    */
                /* must link runtime calls directly down into our C code.   */
                /*                                                          */
                /* For more info, see the comments in                       */
                /*     src/lib/core/init/runtime-system-placeholder.pkg  */
                /*                                                          */
                /* So here we implement some of that special handling:      */

	        /* Register the runtime system under the given picklehash:  */
		picklehash_t picklehash;
		int  l = strlen (filename);
		int  i;
		for (i = 0; i < PICKLEHASH_BYTES; i++) {
		    int i2 = 2 * i;
		    if (i2 + 1 < l) {
			int c1 = filename[i2+0];
			int c2 = filename[i2+1];
			picklehash.bytes[i] = (hex(c1) << 4) + hex(c2);
		    }
		}
	        fprintf(
                    log_fd ? log_fd : stderr,
                    "\n        src/runtime/main/load-o7-files.c:   Runtime system picklehash is      %s\n\n",
                    filename
                );

		register_oh7_file_exports( lib7_state, &picklehash, runtimeCompileUnit );
		seen_runtime_system_picklehash = TRUE;	/* Make sure that we register the runtime system picklehash only once. */
	    }
	}
    }
    if (log_fd) fclose( log_fd );
}


static lib7_val_t   

read_in_oh7_file_list   (  lib7_state_t*   lib7_state,
			 const char*     o7files_to_load_filename,
			 int*            return_max_boot_path_len
		      )
{
    /* Open given file and read from it the list of
     * filename of oh7_files to be later loaded.
     * Return them as a Lib7 list of Lib7 strings:
     */

# define BUF_LEN	1024	/* "This should be plenty for two numbers." "640K should be enough for anyone". */
    char        buf[ BUF_LEN ];

    int		   i, j;
    int            c;

    lib7_val_t*   fileNames = NULL;
    char*	   nameBuf   = NULL;

    int            max_num_boot_files = MAX_NUM_BOOT_FILES;
    int            max_boot_path_len  = MAX_BOOT_PATH_LEN;

    int		   numFiles = 0;

    FILE*	   listF = open_file( o7files_to_load_filename, FALSE );

    fprintf (
        stderr,
        "        src/runtime/main/load-o7-files.c:   Reading   file          %s\n",
        o7files_to_load_filename
    );
    if (log_fd) {
	fprintf (
	    log_fd,
	    "        src/runtime/main/load-o7-files.c:   Reading   file                    %s\n",
	    o7files_to_load_filename
	);
    }

    if (listF) {

        /* Read header: */
        for (;;) {

	    if (!fgets (buf, BUF_LEN, listF)) {
                Die (
                    "o7files_to_load file \"%s\" ends before end-of-header (first empty line)",
                    o7files_to_load_filename
                );
            }

	    {    /* Skip leading whitespace: */
                 char* p = buf;
                 while (*p == ' ' || *p == '\t')   ++p;

		/* Header ends at first empty line: */
		if (p[0] == '\n')   break;

		/* Ignore comment lines: */
		if (p[0] == '#')   continue;

                if (strstr(p,"FILES=") == p) {
		    max_num_boot_files = strtoul(p+6, NULL, 0);
                    continue;
                }

                if (strstr(p,"MAX_LINE_LENGTH=") == p) {
		    max_boot_path_len  = strtoul(p+16, NULL, 0) +2;
                    continue;
                }

                Die (
                    "o7files_to_load file \"%s\" contains unrecognized header line \"%s\"",
                    o7files_to_load_filename,
                    p
                );
	    }
        }
        if (max_num_boot_files < 0) {
            Die("o7files_to_load file \"%s\" contains negative files count?! (%d)",
                o7files_to_load_filename,
                max_num_boot_files
            );
        } 
        if (max_boot_path_len  < 0) {
            Die("o7_file_to_load file \"%s\" contains negative boot path len?! (%d)",
                o7files_to_load_filename,
                max_boot_path_len
            );
        }


	*return_max_boot_path_len = max_boot_path_len;    /* Tell the calling function. */

	if (!(nameBuf = MALLOC( max_boot_path_len ))) {
	    Die ("unable to allocate space for .o7 file filenames");
        }

	if (!(fileNames = MALLOC( max_num_boot_files * sizeof(char*) ))) {
	    Die ("Unable to allocate space for o7-files-to-load name table");
        }

        /* Read in the file names, converting them to Lib7 strings: */
	while (fgets( nameBuf, max_boot_path_len, listF )) {

	    /* Skip leading whitespace: */
	    char* p = nameBuf;
            while (*p == ' ' || *p == '\t')   ++p;

	    /* Ignore empty lines and comment lines: */
	    if (*p == '\n')   continue;
	    if (*p ==  '#')   continue;

	    /* Strip any trailing newline: */
	    j = strlen(p)-1;
	    if (p[j] == '\n') p[j] = '\0';

	    if (numFiles < max_num_boot_files)   fileNames[numFiles++] = LIB7_CString(lib7_state, p);
	    else                                 Die ("too many files\n");
	}
	fclose (listF);
    }

    /* Create the in-heap list: */
    {   lib7_val_t   fileList = LIST_nil;
	for (i = numFiles;  --i >= 0; ) {
	    LIST_cons(lib7_state, fileList, fileNames[i], fileList);
	}

	/* These guys are no longer needed from now on */
	if (fileNames)  FREE (fileNames);
	if (nameBuf)    FREE (nameBuf);

	return fileList;
    }
}


static FILE*   open_file   (   const char*   filename,
                               bool_t        isBinary
                           )
{
    /* Open a file in the .o7 file directory. */

    FILE*   file = fopen (filename, isBinary ? "rb" : "r");

    if (!file) 	Error ("unable to open \"%s\"\n", filename);

    return file;

}

/*
 * LINK ENVIRONMENT:
 *
 * At link-time, the "known universe" consists of the set
 * of already-loaded .o7 files.

 * For our purposes, each such .o7 file is named by
 * its "picklehash", which is a 16-byte hash of its
 * "pickled" (i.e, serialized as a bytestring) form.
 *
 * After being linked in, each such oh7_file returns
 * a tree of records representing its exported, externally-visible
 * functions, global variables and so forth:  This tree
 * constitutes everything we know about that oh7_file at
 * linktime.
 *
 * It follows that any item of interest known to the linker can be
 * named by a picklehash P plus a sequence of integers I0,I1...In,
 * which correspond to the recipe:
 *
 *     Look up the export tree for the already-loaded
 *     oh7_file named by picklehash P, then take
 *     the I0-th entry from that's tree's root record,
 *     the I1-th entry from the preceding result,
 *     ...
 *     and return the In-th entry from -that- result.
 *
 * Thus, every reference by one oh7_file to an external
 * value from some other oh7_file must ultimately be
 * expressed by such a (P, I0, I1, ... In) sequence.
 *
 *
 * .O7 FILE FORMAT description:
 *
*************** The following really belongs in the header file ****************
 *  Every 4-byte integer field is stored in big-endian format.
 *
 *       Start Size Purpose
 * ----BEGIN OF HEADER----
 *            0 16  magic string
 *           16  4  number_of_imported_picklehashes
 *           20  4  number_of_exported_picklehashes (currently always 0 or 1)
 *           24  4  bytes_of_import_tree    (size of import tree area)
 *           28  4  bytes_of_dependency_info  (size of make7-specific info in bytes)
 *           32  4  bytes_of_inlinable_code   (size of pickled lambda-expression(s?))
 *           36  4  size of reserved area in bytes (reserved)
 *           40  4  size of padding area in bytes (pad)
 *           44  4  bytes_of_compiled_code    (size of code area in bytes)
 *           48  4  bytes_of_symbol_table     (size of pickled symbol table)
 *           52  i  import trees [This area contains pickled import trees --
 *                    see below.  The total number of leaves in these trees is
 *                    number_of_imported_picklehashes.  The size impSzB of this area depends on the
 *                    shape of the trees.]
 *         i+52 ex  export picklehashes [Each export picklehash occupies 16 bytes.
 *                    Thus, the size ex of this area is 16*number_of_exported_picklehashes (0 or 16).]
 *      ex+i+52 cm  make7 info [Currently a list of picklehash-pairs.] (cm = bytes_of_dependency_info)
 * ----END OF HEADER----
 *            0  h  HEADER (h = 52+cm+ex+i)
 *            h  l  pickle of exported lambda-expr. (l = bytes_of_inlinable_code)
 *          l+h  r  reserved area (r = reserved)
 *        r+l+h  p  padding (p = pad)
 *      p+r+l+h  c  code area (c = bytes_of_compiled_code) [Structured into several
 *                    segments -- see below.]
 *    c+p+r+l+h  e  pickle of symbol table (e = bytes_of_symbol_table)
 *  e+c+p+r+l+h  -  END OF .O7 FILE
 *
 * IMPORT TREE FORMAT description:
 *
 *  The import tree area contains a list of key-value pairs
 *  where the keys are picklehashes and the values are trees.
 *
 *  Each picklehash key names an external .o7 file which
 *  this oh7_file references -- calling its functions,
 *  reading its global variables, whatever.
 *
 *  The set of 'j' such values needed as imports from P by
 *  our current oh7_file will form a list looking like
 *      P, I0,0 I0,1 ...  In,
 *      P  I1,0 I1,1 ...  Im 
 *      ...
 *      P  Ij,0 Ij,1 ...  Ip
 *  All of them will begin with P and often I0 will equal
 *  I1 or such:  If we merge all such common prefixes, we
 *  convert the list into a tree.  This is the tree
 *  associated with a given picklehash in the import area.
 *
 *  Trees are constructed according to the following Lib7-enum:
 *    enum tree = NODE of (int * tree) list
 *  Leaves in this tree have the form (NODE []).
 *  Trees are written recursively -- (NODE l) is represented by n (= the
 *  length of l) followed by n (int * node) subcomponents.  Each component
 *  consists of the integer selector followed by the corresponding tree.
 *
 *  The size of the import tree area is only given implicitly. When reading
 *  this area, the reader must count the number of leaves and compare it
 *  with number_of_imported_picklehashes.
 *
 *  Integer values in the import tree area (lengths and selectors) are
 *  written in "packed" integer format. In particular, this means that
 *  Values in the range 0..127 are represented by only 1 byte.
 *  Conceptually, the following pickling routine is used:
 *
 *    void recur_write_ul (unsigned long l, FILE *file)
 *    {
 *        if (l != 0) {
 *            recur_write_ul (l >> 7, file);
 *            putc ((l & 0x7f) | 0x80, file);
 *        }
 *    }
 *
 *    void write_ul (unsigned long l, FILE *file)
 *    {
 *        recur_write_ul (l >> 7, file);
 *        putc (l & 0x7f, file);
 *    }
 *
 * CODE AREA FORMAT description:
 *
 *  The code area contains multiple code segements.  There will be at least
 *  two.  The very first segment is the "data" segment -- responsible for
 *  creating literal constants on the heap.  The idea is that code in the
 *  data segment will be executed only once at link-time. Thus, it can
 *  then be garbage-collected immediately. (In the future it is possible that
 *  the data segment will not contain executable code at all but some form
 *  of bytecode that is to be interpreted separately.)
 *
 *  In the .o7 file, each code segment is represented by its size s and its
 *  entry point offset (in bytes -- written as 4-byte big-endian integers)
 *  followed by s bytes of machine- (or byte-) code. The total length of all
 *  code segments (including the bytes spent on representing individual sizes
 *  and entry points) is bytes_of_compiled_code.  The entrypoint field for the
 *  data segment is currently ignored (and should be 0).
 *
 * LINKING CONVENTIONS:
 *
 *  Linking is achieved by executing all code segments in sequential order.
 *
 *  The first code segment (i.e., the "data" segment) receives unit as
 *  its single argument.
 *
 *  The second code segment receives a record as its single argument.
 *  This record has (number_of_imported_picklehashes+1) components.
 *  The first number_of_imported_picklehashes components correspond
 *  to the leaves of the import trees.  The final
 *  component is the result from executing the data segment.
 *
 *  All other code segments receive a single argument which is the result
 *  of the preceding segment.
 *
 *  The result of the last segment represents the exports of the compilation
 *  unit.  It is to be paired up with the export picklehash and stored in the
 *  linking dictionary.  If there is no export picklehash, then the final result
 *  will be thrown away.
 *
 *  The import trees are used for constructing the argument record for the
 *  second code segment.  The picklehash at the root of each tree is the key for
 *  looking up a value in the existing dynamic environment.  In general,
 *  that value will be a record.  The selector fields of the import tree
 *  associated with the picklehash are used to recursively fetch components
 *  of that record.
 */



static void

oh7_file_read   (FILE *file, void *buf, int nbytes, const char *filename)
{
    if (fread(buf, nbytes, 1, file) == -1) {
	Die ("cannot read file \"%s\"", filename);   /* XXX BUGGO FIXME that's unhelpful */
    }
}




static Int32_t

read_packed_int32   (FILE *file, const char *filename)
{

    /* Read an integer in "packed" format.  */
    /* (Small numbers only require 1 byte.) */

    Unsigned32_t	n = 0;

    /* High bit of each byte is 'more bytes to come' flag, */
    /* low seven bits contain integer data.                */
    /* Most significant byte comes first:                  */ 
    for (;;) {
        Byte_t		c;

        /* XXX BUGGO FIXME one subroutine call per byte?! Lordy, what fools these mortals be. */ 
	oh7_file_read (file, &c, sizeof(c), filename);

	n = (n << 7) | (c & 0x7f);

	if (!(c & 0x80)) break;
    }

    return ((Int32_t)n);

    /* XXX BUGGO FIXME this whole boyz-very-own-file-compression   */
    /* nonsense is terminally silly.  If we need file compression, */  
    /* we should use an off-the-shelf solution like lzo.           */
    /* That would keep the compression code factored out of the    */
    /* oh7_file code, and do a far better job to boot. (Heh.)    */    
}



static int

fetch_imports     (   lib7_state_t* lib7_state,
		      FILE*          file,
		      const char*    filename,
		      int            next_import_record_slot_to_fill,
		      lib7_val_t    tree_node
		  )
{
    /****************************************************/
    /* We are traversing a Lib7 heap tree of records   */
    /* constituting the complete set of exported values */
    /* (functions, variables...) from some previously   */
    /* loaded oh7_file external to the oh7_file             */
    /* currently loading.                               */
    /*                                                  */
    /* Our task is to select from that tree those       */
    /* values which are of interest to (imported by)    */
    /* the oh7_file currently being loaded:  We will      */
    /* save these values in an import record being      */
    /* constructed on the heap.                         */
    /*                                                  */
    /* Our guide is a list of 'kid_count' selectors     */
    /* (slot numbers within tree_node) which we read    */
    /* read from the oh7_file being loaded.               */
    /*                                                  */
    /* If this list is empty, then 'tree_node' is       */
    /* itself one of the values we're importing,        */
    /* and we just save a pointer to it in the          */
    /* import record and return.                        */
    /*                                                  */
    /* Otherwise, We do a recursive walk of some        */
    /* subtree of the Lib7 record tree rooted at       */
    /* 'tree_node', saving in the import record         */
    /* each leaf visited. We read out of the oh7_file     */
    /* a 'kid_count' long sequence of selectors giving  */
    /* which children of 'tree_node' to recursively     */
    /* visit, and call ourself recursively on each of   */
    /* the thus-indicated children of 'tree_node.       */
    /****************************************************/

    /* How many children of 'tree_node' should we visit?
    */
    Int32_t  kid_count
        =
        read_packed_int32 (file, filename);

    if (!kid_count) {

        /* Save tree_node in the import record...
        */
	LIB7_AllocWrite( lib7_state, next_import_record_slot_to_fill, tree_node );

	++ next_import_record_slot_to_fill;

    } else {

        /* Recursively visit each of those children in turn:
        */
	while (kid_count --> 0) {

            /* Which child should we visit next?
            */
	    Int32_t kid_selector = read_packed_int32 (file, filename);

            /* Visit it:
            */
	    next_import_record_slot_to_fill
                =
                fetch_imports   (
                    lib7_state,
		    file,
		    filename,
		    next_import_record_slot_to_fill,
		    REC_SEL( tree_node, kid_selector )
		);
	}
    }

    return  next_import_record_slot_to_fill;
}



static void

load_oh7_file   (   lib7_state_t* lib7_state,
		  char*          filename
	      )
{
    /*************************************************/
    /* Loading an oh7_file is a five-step process:     */
    /*                                               */
    /* 1) Read the header, which holds various       */
    /*    numbers we need such as the number of      */
    /*    code segments in the oh7_file.               */
    /*                                               */
    /* 2) Locate all the values imported by this     */
    /*    oh7_file from the export lists of            */
    /*    previously loaded oh7_files.                 */
    /*      For subsequent ease of access, we        */
    /*    construct an 'import record' (a vector)    */
    /*    holding all these values packed            */
    /*    consecutively.                             */
    /*                                               */
    /*************************************************/
    FILE*           file;
    int		    i;
    int		    bytes_of_code_remaining;
    int		    import_record_slot_count; /* Size-in-slots of import_record. */
    int             bytes_of_exports = 0;

    lib7_val_t	    codeChunk;
    lib7_val_t	    import_record;    /* Contains all the values we import from other oh7_files. */
    lib7_val_t	    closure;
    lib7_val_t	    lib7_result;

    oh7_file_hdr_t   header;

    picklehash_t	export_picklehash;

    Int32_t         segment_size_in_bytes;
    Int32_t         entrypoint_offset_in_bytes;

    size_t          archive_offset;
    char*           oh7_file_name = filename;
    

    /* If 'filename' is a "library@offset:oh7_file" triple,
     * parse it into its three parts:
     */
    {   char* at_ptr
            =
            strchr (filename, '@');

	if  (!at_ptr) {

	    archive_offset = 0; 	/* We're loading a bare oh7_file, not one packed within a library archive. */

	} else {

            char* colon_ptr = strchr (at_ptr + 1, ':');
	    if   (colon_ptr) {
		 *colon_ptr = '\0';

		 oh7_file_name = colon_ptr + 1;
	    }

	    archive_offset = strtoul (at_ptr + 1, NULL, 0);        /* XXX BUGGO FIXME Needs more sanity checking. */
	    *at_ptr = '\0';
	}
    }

    /* Log all files loaded for diagnostic/information purposes:
    */
    if (!archive_offset) {

	fprintf (
	    log_fd ? log_fd : stderr,
	    "        src/runtime/main/load-o7-files.c:   Loading   object file   %s\n",
		   filename
	);

    } else {

	fprintf (
	    log_fd ? log_fd : stderr,
	    "        src/runtime/main/load-o7-files.c:   Loading   offset        %8d in lib  %s  \tnamely object file %s\n",
	    archive_offset,
	    filename,
	    oh7_file_name
	);
    }

    /* Open the file:
    */
    file = open_file( filename, TRUE );            if (!file) Exit (1);

    /* If an offset is given (which is to say, if we are loading
     * an oh7_file packed within a library archive) then
     * then seek to the beginning of the section that contains
     * the image of our oh7_file:
     */
    if (archive_offset) {

        if (fseek (file, archive_offset, SEEK_SET) == -1) {

  	    /* XXX BUGGO FIXME should call strerror(errno)
             * here to report the specific error:
             */
	    Die ("Cannot seek on archive file \"%s@%ul\"",
		 filename, (unsigned long) archive_offset
            );
        }
    }

    /* Get the header:
    */
    oh7_file_read (file, &header, sizeof(oh7_file_hdr_t), filename);

    /* The integers in the header are kept in big-endian byte 
     * order, so convert them if we're on a little-endian box:
     */
    header.number_of_imported_picklehashes	= BIGENDIAN_TO_HOST( header.number_of_imported_picklehashes );
    header.number_of_exported_picklehashes	= BIGENDIAN_TO_HOST( header.number_of_exported_picklehashes );
    header.bytes_of_import_tree		= BIGENDIAN_TO_HOST( header.bytes_of_import_tree            );
    header.bytes_of_dependency_info	= BIGENDIAN_TO_HOST( header.bytes_of_dependency_info	 );
    header.bytes_of_inlinable_code		= BIGENDIAN_TO_HOST( header.bytes_of_inlinable_code	 );
    header.reserved			= BIGENDIAN_TO_HOST( header.reserved			 );
    header.pad             		= BIGENDIAN_TO_HOST( header.pad       			 );
    header.bytes_of_compiled_code		= BIGENDIAN_TO_HOST( header.bytes_of_compiled_code		 );
    header.bytes_of_symbol_table		= BIGENDIAN_TO_HOST( header.bytes_of_symbol_table		 );

    /* XXX BUGGO FIXME These days 99% of the market is little-endian, */
    /* so should either change to always little-endian, or else       */
    /* (better) always use host system's native byte ordering.        */
    /* Ideally, we should be able to just map the oh7_file into         */
    /* memory and be ready to go, with no bit-fiddling needed at all. */



    /* Read the 'import tree' and locate all the thus-specified
     * needed values located in the export tree of previously-
     * loaded oh7_files:
     */
    import_record_slot_count
        =
        header.number_of_imported_picklehashes + 1;

    /* Make sure we have enough free heap space to allocate 
     * our 'import record' vector of imported values:
     */
    if (need_to_collect_garbage (lib7_state, REC_SZB(import_record_slot_count))) {

	collect_garbage_with_extra_roots (lib7_state, 0, &oh7_file_list, NULL);
    }

    /* Write the header for our 'import record', which will be 
     * aLib7 record with 'import_record_slot_count' slots:
     */
    LIB7_AllocWrite (lib7_state, 0, MAKE_DESC(import_record_slot_count, DTAG_record));

    /* Locate all the required import values and
     * save them in our nascent on-heap 'import record':
     */
    {   int    next_import_record_slot_to_fill = 1;

        /* Over all previously loaded oh7_files from which we import values: */
	while (next_import_record_slot_to_fill < import_record_slot_count) {

	    picklehash_t	picklehash_naming_previously_loaded_oh7_file;

	    oh7_file_read (file, &picklehash_naming_previously_loaded_oh7_file, sizeof(picklehash_t), filename);

            /* Locate all needed imports exported by that
             * particular pre-loaded oh7_file:
             */
	    next_import_record_slot_to_fill
            =
            fetch_imports (
                lib7_state,
                file,
                filename,
                next_import_record_slot_to_fill,
		picklehash_to_exports_tree( &picklehash_naming_previously_loaded_oh7_file )
            );
	}
    }

    /* Put a dummy valid value (NIL) in the last slot, */
    /* just so the garbage collector won't go bananas  */
    /* if it looks at that slot:                       */
    LIB7_AllocWrite( lib7_state, import_record_slot_count, LIB7_nil );

    /* Complete the above by actually allocating */
    /* the 'import record' on the Lib7 heap:    */
    import_record = LIB7_Alloc( lib7_state, import_record_slot_count );

    /* Get the export picklehash for this oh7_file.    */
    /* This is the name by which other oh7_files will  */
    /* refer to us in their turn as they are loaded.     */
    /*                                                   */
    /* Some oh7_files may not have such a name, in     */
    /* which case they have no directly visible exported */   
    /* values.  (This typically means that they are a    */
    /* plug-in which installs pointers to itself in some */
    /* other module's datastructures, as a side-effect   */
    /* during loading.)                                  */
    if (header.number_of_exported_picklehashes == 1) {

	bytes_of_exports = sizeof( picklehash_t );

	oh7_file_read (file, &export_picklehash, bytes_of_exports, filename);

    } else if (header.number_of_exported_picklehashes != 0) {

	Die ("Number of exported picklehashes is %d (should be 0 or 1)",
            (int)header.number_of_exported_picklehashes
        );
    }

    /* Seek to the first "code segment" within our oh7_file image. */
    /* This contains bytecoded instructions interpretable by         */
    /* build-literals.c which construct all the needed constant      */
    /* lists &tc for this oh7_file.  (If we stored them as actual  */
    /* lists, we'd have to do relocations on all the pointers in     */
    /* those structures at this point.  The bytecode solution seems  */
    /* simpler.)                                                     */
    {
        /* XXX BUGGO FIXME A 'long' is 32 bits on 32-bit Linux,
         * but files longer than 2GB (signed long!) are often
         * supported.  We probably should use fseeko in those
         * cases and then
         *    #define _FILE_OFFSET_BITS 64
         * so as to support large (well, *huge* :) library files.
         * See the manpage for details.
         * This probably won't be a frequent problem in practice
         * for a few years yet, and by then we'll probably be
         * running 64-bit Linux anyhow, so not a high priority.
         */
	long file_offset = archive_offset
	                 + sizeof(oh7_file_hdr_t)
			 + header.bytes_of_import_tree
	                 + bytes_of_exports
	                 + header.bytes_of_dependency_info
			 + header.bytes_of_inlinable_code
			 + header.reserved
	                 + header.pad;

	if (fseek(file, file_offset, SEEK_SET) == -1) {
	    Die ("cannot seek on .o7 file \"%s\"", filename);
        }
    }

    /**********************************************************/
    /* In principle, an oh7_file can contain any number of  */
    /* code segments, so we track the number of bytes of code */
    /* left to process:  When it hits zero, we've done all    */
    /* the code segments.                                     */
    /*                                                        */
    /* In practice, we currently always have exactly two      */
    /* code segments, the first of which contains the byte-   */
    /* coded logic constructing our literals (constant        */
    /* and the second of which contains all our compiled      */
    /* native code for the oh7_file, including that which   */
    /* constructs our tree of exported (directly externally   */
    /* visible) values.                                       */
    /**********************************************************/
    bytes_of_code_remaining = header.bytes_of_compiled_code;

    /* Read the size and the dummy entry point for the       */
    /* first code segment (literal-constructing bytecodes).  */
    /* The entrypoint offset of this first segment is always */
    /* zero, which is why we ignore it here:                 */
    oh7_file_read (file, &segment_size_in_bytes, sizeof(Int32_t), filename);
    segment_size_in_bytes = BIGENDIAN_TO_HOST( segment_size_in_bytes );
    oh7_file_read (file, &entrypoint_offset_in_bytes, sizeof(Int32_t), filename);
    /* entrypoint_offset_in_bytes = BIGENDIAN_TO_HOST( entrypoint_offset_in_bytes ); */

    bytes_of_code_remaining -= segment_size_in_bytes + 2 * sizeof(Int32_t);
    if (bytes_of_code_remaining < 0) {
	Die ("format error (data size mismatch) in .o7 file \"%s\"", filename);
    }

    if (segment_size_in_bytes <= 0) {
	lib7_result = LIB7_void;
    } else {
	Byte_t		*dataChunk = NEW_VEC(Byte_t, segment_size_in_bytes);

	oh7_file_read (file, dataChunk, segment_size_in_bytes, filename);
	SaveCState (lib7_state, &oh7_file_list, &import_record, NULL);
	lib7_result = BuildLiterals (lib7_state, dataChunk, segment_size_in_bytes);
	FREE(dataChunk);
	RestoreCState (lib7_state, &oh7_file_list, &import_record, NULL);
    }

    /* Do a functional update of the last element of the import_record; */
    for (i = 0;  i < import_record_slot_count;  i++) {
	LIB7_AllocWrite(lib7_state, i, PTR_LIB7toC(lib7_val_t, import_record)[i-1]);
    }
    LIB7_AllocWrite( lib7_state, import_record_slot_count, lib7_result );
    lib7_result = LIB7_Alloc( lib7_state, import_record_slot_count );

    /* Do a garbage collection, if necessary: */
    if (need_to_collect_garbage( lib7_state, PICKLEHASH_BYTES + REC_SZB(5)) ) {

	collect_garbage_with_extra_roots (lib7_state, 0, &oh7_file_list, &lib7_result, NULL);
    }
    while (bytes_of_code_remaining > 0) {   /* In practice, we always execute this loop exactly once. */

        /* Read the size and entry point for this code chunk */
	oh7_file_read (file, &segment_size_in_bytes, sizeof(Int32_t), filename);
	segment_size_in_bytes = BIGENDIAN_TO_HOST( segment_size_in_bytes );
	oh7_file_read (file, &entrypoint_offset_in_bytes, sizeof(Int32_t), filename);
	entrypoint_offset_in_bytes = BIGENDIAN_TO_HOST( entrypoint_offset_in_bytes );

        /* How much more? */
	bytes_of_code_remaining -= segment_size_in_bytes + 2 * sizeof(Int32_t);

	if (bytes_of_code_remaining < 0) {
	    Die ("format error (code size mismatch) in .o7 file \"%s\"", filename);
        }

        /* Allocate heap space and read code chunk: */
	codeChunk = LIB7_AllocCode (lib7_state, segment_size_in_bytes);
	oh7_file_read (file, PTR_LIB7toC(char, codeChunk), segment_size_in_bytes, filename);

        /* Flush the instruction cache, so CPU will see    */
        /* our newly loaded code.  (To gain speed, and     */
        /* simplify the hardware design, most modern CPUs  */
        /*  assume that code is never modified on the fly, */
        /* or at least not without manually  flushing the  */
        /* instruction cache this way.)                    */
	FlushICache (PTR_LIB7toC(char, codeChunk), segment_size_in_bytes);
      
        /* Create closure, taking entry point into account: */
	REC_ALLOC1 (lib7_state, closure,
		    PTR_CtoLib7 (PTR_LIB7toC (char, codeChunk) + entrypoint_offset_in_bytes));

        /* Apply the closure to the import picklehash vector.           */
        /*                                                              */
        /* This actually executes all the top-level code for            */
        /* the compile unit, which is to say that if the                */
        /* source for our oh7_file looked something like              */
        /*                                                              */
        /* package myStruct = struct                                  */
        /*     val _ = file.output (file.stdout, "Hello, world!\n") */
        /* end                                                          */
        /*                                                              */
        /* then when we do the following 'apply' call, you'd see        */
        /*                                                              */
        /* Hello, world!                                                */
        /*                                                              */
        /* printed on the standard output.                              */
        /*                                                              */
        /* In addition, invisible compiler-generated code               */
        /* constructs and returns the tree of exports from              */
        /* our oh7_file.                                              */
	SaveCState                  (lib7_state, &oh7_file_list, NULL);
	lib7_result = ApplyLib7Fn (lib7_state, closure, lib7_result, TRUE);
	RestoreCState               (lib7_state, &oh7_file_list, NULL);

	if (need_to_collect_garbage (lib7_state, PICKLEHASH_BYTES+REC_SZB(5))) {
	    collect_garbage_with_extra_roots (lib7_state, 0, &oh7_file_list, &lib7_result, NULL);
        }
    }

    /* Publish this oh7_file's exported-values tree */
    /* for the benefit of oh7_files loaded later:   */
    if (bytes_of_exports) {

	register_oh7_file_exports (
            lib7_state,
            &export_picklehash,     /* key -- the 16-byte picklehash naming this oh7_file. */
            lib7_result            /* val -- the tree of exported Lib7 values.             */
        );
    }

    fclose (file);
}                                   /*load_oh7_file */



static void

register_oh7_file_exports   (   lib7_state_t*   lib7_state,
			      picklehash_t*    c_picklehash,   /* Picklehash key as a C string. */
			      lib7_val_t      exports_tree
			  )
{
    /*******************************************************/
    /* Add a picklehash/exports_tree key/val naming pair  */
    /* to our heap-allocated list of loaded oh7_files.   */
    /*******************************************************/

    lib7_val_t	    lib7_picklehash;   /* Picklehash key as a Lib7 string within the Lib7 heap. */

    /* Copy the picklehash naming this oh7_file  */
    /* into the Lib7 heap, so that we can use     */
    /* it in a Lib7 heap record:                  */
    lib7_picklehash = LIB7_AllocString( lib7_state,           PICKLEHASH_BYTES );
    memcpy( STR_LIB7toC(lib7_picklehash), (char*)c_picklehash, PICKLEHASH_BYTES );

    /* Allocate the list record and thread it onto the exports list: */
    REC_ALLOC3(
        lib7_state,
        picklehash_to_exports_tree_list,    /* Where to save the pointer to the new record.                          */
        lib7_picklehash,                   /* key naming oh7_file -- first slot in new record.                    */
        exports_tree,                       /* tree of values exported from oh7_file -- second slot in new record. */
        picklehash_to_exports_tree_list     /* pointer to next record in list -- third slot in new record.           */
    );
}



static lib7_val_t

picklehash_to_exports_tree   (picklehash_t* picklehash)
{
    lib7_val_t        p;

    /*************************************************************/
    /* We identify a (particular version of) an oh7_file using */
    /* a 16-byte hash of its serialized ("pickled") form.        */
    /*                                                           */
    /* Our global picklehash_to_exports_tree_list is a singly    */
    /* linked list with one entry for each oh7_file which      */
    /* we have loaded into memory.                               */
    /*                                                           */
    /* Each entry in the list maps the picklehash naming that    */
    /* oh7_file to the tree of values (functions &tc)          */
    /* exported by the oh7_file for use by other oh7_files.  */
    /*                                                           */
    /* Here we look up the export tree associated with a given   */
    /* picklehash by doing an O(N) scan down the linklist.       */
    /*                                                           */
    /* XXX BUGGO FIXME  It is criminally st00pid to be using an  */
    /* O(N) lookup algorithm for a linklist which will often be  */
    /* hundreds or even thousands of entries long, on which we   */
    /* may be doing up to a million lookups.  Especially when    */
    /* the key comparisons are expensive.  Can't we arrange to   */
    /* use our standard redblack tree implementation here?       */
    /*************************************************************/

    /* For all oh7_files loaded into memory:
     */
    for (p = picklehash_to_exports_tree_list;  p != LIB7_void;  p = REC_SEL(p, 2)) {

        /* If the picklehash on this record     */
        /* matches our search key picklehash... */
        lib7_val_t id = REC_SEL(p, 0);
	if (memcmp( (char*) picklehash, STR_LIB7toC(id), PICKLEHASH_BYTES) == 0) {

	    /* ... then return its matching export tree: */
	    return REC_SEL(p, 1);
        }
    }

    /* If we get here, something is badly broken:
     * We should never be asked to find a picklehash
     * which isn't in the list -- all OH7_FILES_TO_LOAD
     * lists are supposed to be topologically sorted by
     * dependencies, so we never load an oh7_file until
     * we've loaded every oh7_file it depends upon.
     */
    {   char	buf[ PICKLEHASH_BYTES * 4 ];
	picklehash_to_hex_string( buf, picklehash );
	Die ("unable to find picklehash (oh7_file identifier) '%s'", buf);
    }
}



static void

picklehash_to_hex_string   (   char* buf,
                               picklehash_t* picklehash
                           )
{
    char*   cp = buf;    /* XXX BUGGO FIXME no buffer overrun check */
    int	    i;

    *cp++ = '[';
    for (i = 0;  i < PICKLEHASH_BYTES;  i++) {
	sprintf (cp, "%02x", picklehash->bytes[i]);
	cp += 2;
    }
    *cp++ = ']';
    *cp++ = '\0';

}


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

