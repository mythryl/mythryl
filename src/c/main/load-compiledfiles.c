// load-compiledfiles.c
//
// This file implements the core Mythryl linker functionality
// which combines .compiled files to produce a Mythryl "executable"
// -- actually a heap image with a
//
//     #!/usr/bin/mythryl-runtime-intel32
//
// "shebang" line at the top).
//
//
// Our primary entrypoint is
//
//      load_compiled_files__may_heapclean (),
//
// which is invoked from
//
//     src/c/main/runtime-main.c:main()
//
// when mythryl-runtime-intel32 is given the "--runtime-compiledfiles-to-load=filename"
// commandline switch, where the "filename"
// parameter is the name of a file containing a
// list of filenames of the .compiled files to load, one per line. 
// (Some "compiled_files" may in fact be at some given offset inside
// a library archive file, in which case the syntax gets a bit involved.)
//


/*
###           "One of the main causes of the fall
###            of the Roman Empire was that, lacking zero,
###            they had no way to indicate successful
###            termination of their C programs."
###
###                               -- Robert Firth
*/   


#include "../mythryl-config.h"

#include "system-dependent-stuff.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "runtime-base.h"
#include "runtime-configuration.h"
#include "flush-instruction-cache-system-dependent.h"
#include "compiledfile.h"
#include "make-strings-and-vectors-etc.h"
#include "heapcleaner.h"
#include "runtime-globals.h"
#include "mythryl-callable-cfun-hashtable.h"
#include "heap.h"

#ifndef SEEK_SET
#  define SEEK_SET	0
#endif

// The
//
//     PERVASIVE_PACKAGE_PICKLE_LIST
//
// is stored in the
//
//    PERVASIVE_PACKAGE_PICKLE_LIST_REFCELL__GLOBAL
//
// refcell.
// It is a singly-linked list of (key,val) pairs -- specifically,
// (picklehash,heapblock) pairs -- with the following Mythryl type
//
//      Pervasive_Package_Pickle_List
//	  #
//        = NIL						# NIL and CONS are traditional LISP terms for final and nonfinal (respectively) linklist nodes.
//	  | CONS  ( vector_of_one_byte_unts::Vector,		# 16-byte hash of chunk -- "picklehash".
//                  unsafe_chunk::Chunk,		# Arbitrary ram-chunk on Mythryl heap -- a "pickle" to be exported.
//                  Pervasive_Package_Pickle_List	# Linklist 'next' pointer.
//                )
//        ;
//
// declared in
//
//     src/lib/std/src/unsafe/unsafe.api
//
// where 'vector_of_one_byte_unts::Vector' is the 16-byte hash of 'unsafe_chunk::Chunk'.
//
#define PERVASIVE_PACKAGE_PICKLE_LIST__GLOBAL	(*PTR_CAST( Val*, PERVASIVE_PACKAGE_PICKLE_LIST_REFCELL__GLOBAL ))



// Forward declarations for private local functions:
//
 static FILE* open_file				(const char* filename,   Bool isBinary);
//
 static Val   read_in_compiled_file_list__may_heapclean 	( Task* task, const char* compiled_files_to_load_filename, int* max_boot_path_len_ptr, Roots* extra_roots );
 static void  load_compiled_file__may_heapclean			( Task* task, char* filename, Roots* extra_roots);
 static void  register_compiled_file_exports__may_heapclean	( Task* task, Picklehash* picklehash, Val chunk, Roots*);

 static Val   picklehash_to_exports_tree	(Picklehash* picklehash);

 static void  picklehash_to_hex_string		(char *buf, int buflen, Picklehash* picklehash);


static FILE*       log_fd = NULL;

// Compute integer value of an ascii hex digit:
//
static int   hex   (int c)   {
    //
    if (isdigit(c))           return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    else                      return c - 'A' + 10;
}


static void   open_logfile   () {
    //        ============
    //
    char* filename = "load-compiledfiles.c.log";

    log_fd = fopen( filename, "w" );

    fprintf(stderr, "\n                    load-compiledfiles.c:   Writing load log to     %s\n\n", filename);    fflush(stderr);
}



void   load_compiled_files__may_heapclean   (
    // ==================================
    //
    const char*         compiled_files_to_load_filename,
    Heapcleaner_Args*   heap_parameters,				// See   struct cleaner_args   in   src/c/h/heap.h
    Roots*              extra_roots
){
    // Load into the runtime heap all the .compiled files
    // listed one per line in given compiled_files_to_load file:
    //
    // This function is called from exactly one spot, in
    //
    //     src/c/main/runtime-main.c

    int    max_boot_path_len;
    char*  filename_buf;

    int    seen_runtime_package_picklehash = FALSE;			// FALSE until we see the picklehash naming our runtime.

    open_logfile();

    Task* task
	=
	make_task(							// make_task					def in   src/c/main/runtime-state.c
	    TRUE,							// is_boot
	    heap_parameters
	);


    // Set up handlers for ^C, divide-by-zero,
    // integer overflow etc:
    //
    set_up_fault_handlers ();						// set_up_fault_handlers			def in   src/c/machine-dependent/posix-arithmetic-trap-handlers.c
									// set_up_fault_handlers			def in   src/c/machine-dependent/win32-fault.c
									// set_up_fault_handlers			def in   src/c/machine-dependent/cygwin-fault.c	

    // Set up RunVec in CStruct in
    //
    //     runtime_package__global.
    //
    // This constitutes an ersatz exports list implementing
    //
    //     src/lib/core/init/runtime.api
    // 
    // which we will later substitute for the (useless) code from
    //
    //     src/lib/core/init/runtime.pkg
    //
    // thus providing access to critical assembly fns including
    //
    //     find_cfun
    //
    // implemented in one of
    //
    //     src/c/machine-dependent/prim.sparc32.asm
    //     src/c/machine-dependent/prim.pwrpc32.asm
    //     src/c/machine-dependent/prim.intel32.asm
    //     src/c/machine-dependent/prim.intel32.masm
    //
    construct_runtime_package__global( task );				// construct_runtime_package__global	def in   src/c/main/construct-runtime-package.c

    // Construct the list of files to be loaded:
    //
    Val compiled_file_list
	=
	read_in_compiled_file_list__may_heapclean (
	    task,
            compiled_files_to_load_filename,
            &max_boot_path_len,
	    extra_roots
	);

    Roots roots1 = { &compiled_file_list, extra_roots };

    if (! (filename_buf = MALLOC( max_boot_path_len ))) {		// This space is ultimately wasted.	XXX SUCKO FIXME
	//
	die ("unable to allot space for boot file names");
    }

    // Load all requested compiled_files into the heap:
    //
    while (compiled_file_list != LIST_NIL) {
	//
        char* filename =  filename_buf;

        // Need to make a copy of the filename because
        // load_compiled_file__may_heapclean is going to scribble into it:
        //
	strcpy( filename_buf, HEAP_STRING_AS_C_STRING( LIST_HEAD( compiled_file_list )));
       
	compiled_file_list = LIST_TAIL( compiled_file_list );		// Remove above filename from list of files to process.

	// If 'filename' does not begin with "RUNTIME_PACKAGE_PICKLEHASH=" ...
	//
	if (strstr(filename,"RUNTIME_PACKAGE_PICKLEHASH=") != filename) {
	    //
	    // ... then we can load it normally:
	    //
	    load_compiled_file__may_heapclean( task, filename, &roots1 );

	} else {

	    // We're processing the
            //
            //     RUNTIME_PACKAGE_PICKLEHASH=...
            //
            // set up for us by
            //
            //     src/app/makelib/mythryl-compiler-compiler/find-set-of-compiledfiles-for-executable.pkg

	    while (*filename++ != '=');   		// Step over "RUNTIME_PACKAGE_PICKLEHASH=" prefix.

	    if (seen_runtime_package_picklehash) {
		//
                if (log_fd) fclose( log_fd );

		die ("Runtime system picklehash registered more than once!\n");
		exit(1);								// Just for gcc's sake -- cannot exectute.

	    }

	    // Most parts of the Mythryl implementation treat the C-coded
	    // runtime functions as being just like library functions
	    // coded in Mythryl -- to avoid special cases, we go to great
	    // lengths to hide the differences.
	    //
	    // But this is one of the places where the charade breaks
	    // down -- there isn't actually any (useful) .compiled file
	    // corresponding to the runtime picklehash:  Instead, we
	    // must link runtime calls directly down into our C code.
	    //
	    // For more info, see the comments in
	    //     src/lib/core/init/runtime.pkg
	    //
	    // So here we implement some of that special handling:

	    // Register the runtime system under the given picklehash:
	    //
	    Picklehash picklehash;

	    int  l = strlen( filename );

	    for (int i = 0;   i < PICKLEHASH_BYTES;   i++) {
	        //
		int i2 = 2 * i;
		if (i2 + 1 < l) {
		    int c1 = filename[i2+0];
		    int c2 = filename[i2+1];
		    picklehash.bytes[i] = (hex(c1) << 4) + hex(c2);
		}
	    }
	    {   FILE* fd = log_fd ? log_fd : stderr;
	        //
		fprintf( fd,
		    "\n                    load-compiledfiles.c:   Runtime system picklehash is      %s\n\n",
		    filename
		);
		fflush(fd);
	    }
	    register_compiled_file_exports__may_heapclean( task, &picklehash, runtime_package__global, &roots1 );

	    seen_runtime_package_picklehash = TRUE;							// Make sure that we register the runtime system picklehash only once.
	}
    }

    if (log_fd)   fclose( log_fd );
}													// load_compiled_files__may_heapclean


static Val   read_in_compiled_file_list__may_heapclean   (
    //       =========================================
    //
    Task*          task,
    const char*    compiled_files_to_load_filename,
    int*           return_max_boot_path_len,
    Roots*         extra_roots
){
    // Open given file and read from it the list of
    // filenames of compiled_files to be later loaded.
    // Return them as a Mythryl list of Mythryl strings:

    #define    BUF_LEN	1024		//  "This should be plenty for two numbers."   "640K should be enough for anyone."
    char  buf[ BUF_LEN ];

//  Val*   file_names = NULL;
    char*  name_buf   = NULL;

    int    max_num_boot_files = MAX_NUMBER_OF_BOOT_FILES;
    int    max_boot_path_len  = MAX_LENGTH_FOR_A_BOOTFILE_PATHNAME;

    int    file_count = 0;

    FILE*  list_fd =  open_file( compiled_files_to_load_filename, FALSE );

    fprintf (
        stderr,
        "                    load-compiledfiles.c:   Reading   file          %s\n",
        compiled_files_to_load_filename
    );
    fflush( stderr );

    if (log_fd) {
	//
	fprintf (
	    log_fd,
	    "                    load-compiledfiles.c:   Reading   file                    %s\n",
	    compiled_files_to_load_filename
	);
	fflush( log_fd );
    }

    Val  file_list = LIST_NIL;			Roots roots1 = { &file_list, extra_roots };

    if (list_fd) {

        // Read header:
        //
        for (;;) {
	    //
	    if (!fgets (buf, BUF_LEN, list_fd)) {
                die (
                    "compiled_files_to_load file \"%s\" ends before end-of-header (first empty line)",
                    compiled_files_to_load_filename
                );
            }

	    {    char* p = buf;
                 while (*p == ' ' || *p == '\t')   ++p;		// Skip leading whitespace.

		if (p[0] == '\n')   break;			// Header ends at first empty line.

		if (p[0] == '#')   continue;			// Ignore comment lines.

                if (strstr( p,"FILES=") == p) {
		    //
		    max_num_boot_files = strtoul(p+6, NULL, 0);
                    continue;
                }

                if (strstr(p,"MAX_LINE_LENGTH=") == p) {
		    //
		    max_boot_path_len  = strtoul(p+16, NULL, 0) +2;
                    continue;
                }

                die (
                    "compiled_files_to_load file \"%s\" contains unrecognized header line \"%s\"",
                    compiled_files_to_load_filename,
                    p
                );
	    }
        }

        if (max_num_boot_files < 0)  {
	    //
            die("compiled_files_to_load file \"%s\" contains negative files count?! (%d)",
                compiled_files_to_load_filename,
                max_num_boot_files
            );
        } 

        if (max_boot_path_len  < 0) {
	    //
            die("compiled_file_to_load file \"%s\" contains negative boot path len?! (%d)",
                compiled_files_to_load_filename,
                max_boot_path_len
            );
        }


	*return_max_boot_path_len =   max_boot_path_len;		// Tell the calling function.

	if (!(name_buf = MALLOC( max_boot_path_len ))) {
	    //
	    die ("unable to allot space for .compiled file filenames");
        }

//	if (!(file_names = MALLOC( max_num_boot_files * sizeof(char*) ))) {
//	    //
//	    die ("Unable to allot space for compiledfiles-to-load name table");
//        }

        // Read in the file names, converting them to
	// Mythryl strings and saving them in a list:
        //
	while (fgets( name_buf, max_boot_path_len, list_fd )) {

	    // Skip leading whitespace:
	    //
	    char* p = name_buf;
            while (*p == ' ' || *p == '\t')   ++p;

	    // Ignore empty lines and comment lines:
	    //
	    if (*p == '\n')   continue;
	    if (*p ==  '#')   continue;

	    // Strip any trailing newline:
	    //
	    {   int j = strlen(p)-1;
		//
	        if (p[j] == '\n') p[j] = '\0';
	    }	

	    if (file_count >= max_num_boot_files)   die ("too many files\n");

	    // If our agegroup0 buffer is more than half full,
	    // empty it by doing a heapcleaning.  This is very
	    // conservative -- which is the way I like it. *grin*
	    //
	    if (agegroup0_freespace_in_bytes( task )
	      < agegroup0_usedspace_in_bytes( task )
	    ){
		call_heapcleaner_with_extra_roots( task,  0, &roots1 );
	    }

	    Val file_name
		=
		make_ascii_string_from_c_string__may_heapclean(task, p, &roots1 );

	    file_list = LIST_CONS(task, file_name, file_list);
	}

	if (name_buf)    FREE( name_buf );

	fclose( list_fd );
    }


    // Reverse filename list (to restore
    // original order) and return it:
    //
    {   Val file_list2 = LIST_NIL;			Roots roots2 = { &file_list2, &roots1 };
	//
	for (; file_list != LIST_NIL;  file_list = LIST_TAIL(file_list)) {
	    //
	    Val file_name = LIST_HEAD(file_list);
	    //
	    file_list2 = LIST_CONS(task, file_name, file_list2);

	    // Again, if our agegroup0 buffer is more than
	    // half full, empty it by doing a heapcleaning:
	    //
	    if (agegroup0_freespace_in_bytes( task )
	      < agegroup0_usedspace_in_bytes( task )
	    ){
		call_heapcleaner_with_extra_roots( task,  0, &roots2 );
	    }
	}

	return file_list2;
    }
}


static FILE*   open_file   (
    //         =========
    //
    const char*   filename,
    Bool          is_binary
){
    // Open a file in the .compiled file directory.

    FILE*   file = fopen (filename, is_binary ? "rb" : "r");

    if (!file) 	 say_error( "Unable to open \"%s\"\n", filename );

    return file;
}


// LINK ENVIRONMENT:
//
// At link-time, the "known universe" consists of the set
// of already-loaded .compiled files.
//
// For our purposes, each such .compiled file is named by
// its "picklehash", which is a 16-byte hash of its
// "pickled" (i.e, serialized as a bytestring) form.
//
// After being linked in, each such compiledfile returns
// a tree of records representing its exported, externally-visible
// functions, global variables and so forth:  This tree
// constitutes everything we know about that compiledfile at
// linktime.
//
// It follows that any item of interest known to the linker can be
// named by a picklehash P plus a sequence of integers I0,I1...In,
// which correspond to the recipe:
//
//     Look up the export tree for the already-loaded
//     compiledfile named by picklehash P, then take
//     the I0-th entry from that's tree's root record,
//     the I1-th entry from the preceding result,
//     ...
//     and return the In-th entry from -that- result.
//
// Thus, every reference by one compiledfile to an external
// value from some other compiledfile must ultimately be
// expressed by such a (P, I0, I1, ... In) sequence.
//
//
// .compiled FILE FORMAT description:
//
//////////////////// The following really belongs in the header file /////////////////
//  Every 4-byte integer field is stored in big-endian format.
//
//       Start Size Purpose
// ----BEGIN OF HEADER----
//            0 16  magic string
//           16  4  number_of_imported_picklehashes
//           20  4  number_of_exported_picklehashes (currently always 0 or 1)
//           24  4  bytes_of_import_tree    (size of import tree area)
//           28  4  bytes_of_dependency_info  (size of makelib-specific info in bytes)
//           32  4  bytes_of_inlinable_code   (size of pickled lambda-expression(s?))
//           36  4  size of reserved area in bytes (reserved)
//           40  4  size of padding area in bytes (pad)
//           44  4  bytes_of_compiled_code    (size of code area in bytes)
//           48  4  bytes_of_symbolmapstack     (size of pickled symbol table)
//           52  i  import trees [This area contains pickled import trees --
//                    see below.  The total number of leaves in these trees is
//                    number_of_imported_picklehashes.  The size impSzB of this area depends on the
//                    shape of the trees.]
//         i+52 ex  export picklehashes [Each export picklehash occupies 16 bytes.
//                    Thus, the size ex of this area is 16*number_of_exported_picklehashes (0 or 16).]
//      ex+i+52 cm  makelib info [Currently a list of picklehash-pairs.] (cm = bytes_of_dependency_info)
// ----END OF HEADER----
//            0  h  HEADER (h = 52+cm+ex+i)
//            h  l  pickle of exported lambda-expr. (l = bytes_of_inlinable_code)
//          l+h  r  reserved area (r = reserved)
//        r+l+h  p  padding (p = pad)
//      p+r+l+h  c  code area (c = bytes_of_compiled_code) [Structured into several
//                    segments -- see below.]
//    c+p+r+l+h  e  pickle of symbol table (e = bytes_of_symbolmapstack)
//  e+c+p+r+l+h  -  END OF .compiled FILE
//
// IMPORT TREE FORMAT description:
//
//  The import tree area contains a list of key-value pairs
//  where the keys are picklehashes and the values are trees.
//
//  Each picklehash key names an external .compiled file which
//  this compiledfile references -- calling its functions,
//  reading its global variables, whatever.
//
//  The set of 'j' such values needed as imports from P by
//  our current compiledfile will form a list looking like
//      P, I0,0 I0,1 ...  In,
//      P  I1,0 I1,1 ...  Im 
//      ...
//      P  Ij,0 Ij,1 ...  Ip
//  All of them will begin with P and often I0 will equal
//  I1 or such:  If we merge all such common prefixes, we
//  convert the list into a tree.  This is the tree
//  associated with a given picklehash in the import area.
//
//  Trees are constructed according to the following Lib7-enum:
//    enum tree = NODE of (int * tree) list
//  Leaves in this tree have the form (NODE []).
//  Trees are written recursively -- (NODE l) is represented by n (= the
//  length of l) followed by n (int * node) subcomponents.  Each component
//  consists of the integer selector followed by the corresponding tree.
//
//  The size of the import tree area is only given implicitly. When reading
//  this area, the reader must count the number of leaves and compare it
//  with number_of_imported_picklehashes.
//
//  Integer values in the import tree area (lengths and selectors) are
//  written in "packed" integer format. In particular, this means that
//  Values in the range 0..127 are represented by only 1 byte.
//  Conceptually, the following pickling routine is used:
//
//    void recur_write_ul (unsigned long l, FILE *file)
//    {
//        if (l != 0) {
//            recur_write_ul (l >> 7, file);
//            putc ((l & 0x7f) | 0x80, file);
//        }
//    }
//
//    void write_ul (unsigned long l, FILE *file)
//    {
//        recur_write_ul (l >> 7, file);
//        putc (l & 0x7f, file);
//    }
//
// CODE AREA FORMAT description:
//
//  The code area contains multiple code segements.  There will be at least
//  two.  The very first segment is the "data" segment -- responsible for
//  creating literal constants on the heap.  The idea is that code in the
//  data segment will be executed only once at link-time. Thus, it can
//  then be garbage-collected immediately. (In the future it is possible that
//  the data segment will not contain executable code at all but some form
//  of bytecode that is to be interpreted separately.)					// My impression is that this is now true -- see src/c/heapcleaner/make-package-literals-via-bytecode-interpreter.c
//											//     -- 2012-06-02 CrT
//  In the .compiled file, each code segment is represented by its size s and its
//  entry point offset (in bytes -- written as 4-byte big-endian integers)
//  followed by s bytes of machine- (or byte-) code. The total length of all
//  code segments (including the bytes spent on representing individual sizes
//  and entry points) is bytes_of_compiled_code.  The entrypoint field for the
//  data segment is currently ignored (and should be 0).
//
// LINKING CONVENTIONS:
//
//  Linking is achieved by executing all code segments in sequential order.
//
//  The first code segment (i.e., the "data" segment) receives Void as
//  its single argument.
//
//  The second code segment receives a record as its single argument.
//  This record has (number_of_imported_picklehashes+1) components.
//  The first number_of_imported_picklehashes components correspond
//  to the leaves of the import trees.  The final
//  component is the result from executing the data segment.
//
//  All other code segments receive a single argument which is the result
//  of the preceding segment.								// My impression is that in general there are only ever two code segments at present.	-- 2012-06-02 CrT
//
//  The result of the last segment represents the exports of the compilation
//  unit.  It is to be paired up with the export picklehash and stored in the
//  linking dictionary.  If there is no export picklehash, then the final result
//  will be thrown away.
//
//  The import trees are used for constructing the argument record for the
//  second code segment.  The picklehash at the root of each tree is the key for
//  looking up a value in the existing dynamic environment.  In general,
//  that value will be a record.  The selector fields of the import tree
//  associated with the picklehash are used to recursively fetch components
//  of that record.




static void   read_n_bytes_from_file   (FILE* file,  void* buf,  int nbytes,  const char* filename)   {
    //        ======================
    //
    if (fread(buf, nbytes, 1, file) == -1) {
	//
	die ("Cannot read file \"%s\"", filename);   	// XXX BUGGO FIXME  That's unhelpful!
    }
}




static Int1   read_packed_int1   (FILE* file,  const char* filename)   {
    //        ================
    //
    // Read an integer in "packed" format.
    // (Small numbers only require 1 byte.)

    Unt1	n = 0;

    // High bit of each byte is 'more bytes to come' flag,
    // low seven bits contain integer data.
    // Most significant byte comes first:
    //
    for (;;) {
        //
        Unt8 c;
	read_n_bytes_from_file( file, &c, sizeof(c), filename );		// XXX SUCKO FIXME  One subroutine call per byte?! Lordy, what fools these mortals be.

	n = (n << 7) | (c & 0x7f);

	if (!(c & 0x80)) break;
    }

    return ((Int1)n);

    // XXX BUGGO FIXME this whole boyz-very-own-file-compression
    // nonsense is terminally silly.  If we need file compression,
    // we should use an off-the-shelf solution like lzo.
    // That would keep the compression code factored out of the
    // compiledfile code, and do a far better job to boot. (Heh.) 
}



static int   fetch_imports   (
    //       =============
    //
    Task*	    task,
    //
    FILE*           file,
    const char*     filename,
    //
    int             next_import_record_slot_to_fill,
    Val		    tree_node
) {
    //////////////////////////////////////////////////////
    // We are traversing a Mythryl heap tree of records
    // constituting the complete set of exported values
    // (functions, variables...) from some previously
    // loaded compiledfile external to the compiledfile
    // currently loading.
    //
    // Our task is to select from that tree those
    // values which are of interest to (imported by)
    // the compiledfile currently being loaded:  We will
    // save these values in an import record being
    // constructed on the heap.
    //
    // Our guide is a list of 'kid_count' selectors
    // (slot numbers within tree_node) which we
    // read from the compiledfile being loaded.
    //
    // If this list is empty, then 'tree_node' is
    // itself one of the values we're importing,
    // and we just save a pointer to it in the
    // import record and return.
    //
    // Otherwise, We do a recursive walk of some
    // subtree of the Lib7 record tree rooted at
    // 'tree_node', saving in the import record
    // each leaf visited. We read out of the compiledfile
    // a 'kid_count' long sequence of selectors giving
    // which children of 'tree_node' to recursively
    // visit, and call ourself recursively on each of
    // the thus-indicated children of 'tree_node.
    //////////////////////////////////////////////////////

    // How many children of 'tree_node' should we visit?
    //
    Int1  kid_count =   read_packed_int1 (file, filename);

    if (!kid_count) {
        //
        // Save tree_node in the import record...
        //
	set_slot_in_nascent_heapchunk( task, next_import_record_slot_to_fill, tree_node );

	++ next_import_record_slot_to_fill;

    } else {

        // Recursively visit each of those children in turn:
        //
	while (kid_count --> 0) {

            // Which child should we visit next?
            //
	    Int1 kid_selector =  read_packed_int1( file, filename );

            // Visit it:
            //
	    next_import_record_slot_to_fill
                =
                fetch_imports   (
                    task,
		    file,
		    filename,
		    next_import_record_slot_to_fill,
		    GET_TUPLE_SLOT_AS_VAL( tree_node, kid_selector )
		);
	}
    }

    return  next_import_record_slot_to_fill;
}



static void   load_compiled_file__may_heapclean   (
    //        =================================
    //
    Task*  task,
    char*  filename,
    Roots* extra_roots
){
    ///////////////////////////////////////////////////////
    // Loading an compiledfile is a five-step process:
    //
    // 1. Read the header, which holds various
    //    numbers we need such as the number of
    //    code segments in the compiledfile.
    //
    // 2. Locate all the values imported by this
    //    compiledfile from the export lists of
    //    previously loaded compiled_files.
    //      For subsequent ease of access, we
    //    construct an 'import record' (a vector)
    //    holding all these values packed
    //    consecutively.
    //
    ///////////////////////////////////////////////////////

    FILE* file;
    int   i;
    int   bytes_of_code_remaining;
    int   bytes_of_exports = 0;

    Compiledfile_Header   header;

    Picklehash	export_picklehash;

    Int1         segment_bytesize;
    Int1         entrypoint_offset_in_bytes;

    size_t          archive_offset;
    char*           compiledfile_filename = filename;
    

    // If 'filename' is a "library@offset:compiledfile" triple,
    // parse it into its three parts:
    //
    {   char* at_ptr
            =
            strchr (filename, '@');

	if  (!at_ptr) {

	    archive_offset = 0; 	// We're loading a bare .compiled, not one packed within a library archive.

	} else {

            char* colon_ptr = strchr (at_ptr + 1, ':');
	    if   (colon_ptr) {
		 *colon_ptr = '\0';

		 compiledfile_filename = colon_ptr + 1;
	    }

	    archive_offset = strtoul (at_ptr + 1, NULL, 0);        // XXX SUCKO FIXME Needs more sanity checking.
	    *at_ptr = '\0';
	}
    }

    // Log all files loaded, for diagnostic/information purposes:
    //
    {	FILE* fd =  log_fd ? log_fd : stderr;
	//
	if (!archive_offset) {
	    //
	    fprintf( fd,
		"                    load-compiledfiles.c:   Loading   object file   %s\n",
		       filename
	    );

	} else {

	    fprintf( fd,
		"                    load-compiledfiles.c:   Loading   offset        %8d in lib  %s  \tnamely object file %s\n",
		archive_offset,
		filename,
		compiledfile_filename
	    );
	}

	fflush( fd );
    }

    // Open the file:
    //
    file = open_file( filename, TRUE );            if (!file)   print_stats_and_exit( 1 );

    // If an offset is given (which is to say, if we are loading
    // a compiledfile packed within a library archive) then
    // seek to the beginning of the section that contains the
    // image of our compiledfile:
    //
    if (archive_offset) {
        //
        if (fseek (file, archive_offset, SEEK_SET) == -1) {
	    //
	    die ("Cannot seek on archive file \"%s@%ul\": %s", filename, (unsigned long) archive_offset, strerror(errno) );
        }
    }

    // Get the header:
    //
    read_n_bytes_from_file( file, &header, sizeof(Compiledfile_Header), filename );

    // The integers in the header are kept in big-endian byte 
    // order, so convert them if we're on a little-endian box:
    //
    header.number_of_imported_picklehashes	= BIGENDIAN_TO_HOST( header.number_of_imported_picklehashes	);
    header.number_of_exported_picklehashes	= BIGENDIAN_TO_HOST( header.number_of_exported_picklehashes	);
    header.bytes_of_import_tree			= BIGENDIAN_TO_HOST( header.bytes_of_import_tree            	);
    header.bytes_of_dependency_info		= BIGENDIAN_TO_HOST( header.bytes_of_dependency_info	 	);
    header.bytes_of_inlinable_code		= BIGENDIAN_TO_HOST( header.bytes_of_inlinable_code	 	);
    header.reserved				= BIGENDIAN_TO_HOST( header.reserved			 	);
    header.pad             			= BIGENDIAN_TO_HOST( header.pad       			 	);
    header.bytes_of_compiled_code		= BIGENDIAN_TO_HOST( header.bytes_of_compiled_code		);
    header.bytes_of_symbolmapstack		= BIGENDIAN_TO_HOST( header.bytes_of_symbolmapstack		);

															// XXX SUCKO FIXME These days 99% of the market is little-endian,
															// so should either change to always little-endian, or else
															// (better) always use host system's native byte ordering.
															// Ideally we should be able to just mmap the .compiledfile into
															// memory and be ready to go, with no bit-fiddling needed at all.


    // Read the 'import tree' and locate all the thus-specified
    // needed values located in the export tree of previously-
    // loaded compiled_files:
    //
    int imports_record_slot_count
        =
        header.number_of_imported_picklehashes + 1;

    // Make sure we have enough free heap space to allot 
    // our 'import record' vector of imported values:
    //
    if (need_to_call_heapcleaner (task, REC_BYTESIZE(imports_record_slot_count))) {
        //
	call_heapcleaner_with_extra_roots (task, 0, extra_roots );
    }

    // Write the header for our 'import record', which will be 
    // a Mythryl record with 'imports_record_slot_count' slots:
    //
    set_slot_in_nascent_heapchunk(											// set_slot_in_nascent_heapchunk	is from   src/c/h/make-strings-and-vectors-etc.h
	task,
	0,
	MAKE_TAGWORD(imports_record_slot_count, PAIRS_AND_RECORDS_BTAG)
    );

    // Locate all the required import values and
    // save them in our nascent on-heap 'import record':
    //
    {   int    next_imports_record_slot_to_fill = 1;

        // Over all previously loaded .compiled files
        // from which we import values:
        //
	while (next_imports_record_slot_to_fill < imports_record_slot_count) {
	    //
	    Picklehash	picklehash_naming_previously_loaded_compiled_file;

	    read_n_bytes_from_file( file, &picklehash_naming_previously_loaded_compiled_file, sizeof(Picklehash), filename );

            // Locate all needed imports exported by that
            // particular pre-loaded compiledfile:
            //
	    next_imports_record_slot_to_fill
		=
		fetch_imports (
		    task,
		    file,
		    filename,
		    next_imports_record_slot_to_fill,
		    picklehash_to_exports_tree( &picklehash_naming_previously_loaded_compiled_file )
		);
	}
    }

    // Put a dummy valid value (NIL) in the last slot,
    // just so the cleaner won't go bananas if it
    // looks at that slot:
    //
    set_slot_in_nascent_heapchunk( task, imports_record_slot_count, HEAP_NIL );

    // Complete the above by actually allocating
    // the 'import record' on the Mythryl heap:
    //
    Val import_record									// Contains all the values we import from other compiled_files.
	=
	commit_nascent_heapchunk( task, imports_record_slot_count );			// commit_nascent_heapchunk	is from   src/c/h/make-strings-and-vectors-etc.h

    Roots roots1 = { &import_record, extra_roots };

    // Get the export picklehash for this compiledfile.
    // This is the name by which other compiled_files will
    // refer to us in their turn as they are loaded.
    //
    // Some compiled_files may not have such a name, in
    // which case they have no directly visible exported
    // values.  (This typically means that they are a
    // plug-in which installs pointers to itself in some
    // other module's datastructures, as a side-effect
    // during loading.)
    //
    if (header.number_of_exported_picklehashes == 1) {
        //
	bytes_of_exports = sizeof( Picklehash );

	read_n_bytes_from_file( file, &export_picklehash, bytes_of_exports, filename );

    } else if (header.number_of_exported_picklehashes != 0) {

	die ("Number of exported picklehashes is %d (should be 0 or 1)",
	    //
            (int)header.number_of_exported_picklehashes
        );
    }

    // Seek to the first "code segment" within our compiledfile image.
    // This contains bytecoded instructions interpretable by
    // make-package-literals-via-bytecode-interpreter.c which construct all the needed constant
    // lists etc for this compiledfile.  (If we stored them as actual
    // lists, we'd have to do relocations on all the pointers in
    // those structures at this point.  The bytecode solution seems
    // simpler.)
    {
        // XXX BUGGO FIXME A 'long' is 32 bits on 32-bit Linux,
        // but files longer than 2GB (signed long!) are often
        // supported.  We probably should use fseeko in those
        // cases and then
        //    #define _FILE_OFFSET_BITS 64
        // so as to support large (well, *huge* :) library files.
        // See the manpage for details.
        // This probably won't be a frequent problem in practice
        // for a few years yet, and by then we'll probably be
        // running 64-bit Linux anyhow, so not a high priority.
        //
	long file_offset = archive_offset
	                 + sizeof(Compiledfile_Header)
			 + header.bytes_of_import_tree
	                 + bytes_of_exports
	                 + header.bytes_of_dependency_info
			 + header.bytes_of_inlinable_code
			 + header.reserved
	                 + header.pad;

	if (fseek(file, file_offset, SEEK_SET) == -1) {
	    //
	    die ("cannot seek on .compiled file \"%s\": %s", filename, strerror(errno) );
        }
    }

    ////////////////////////////////////////////////////////////////
    // In principle, a .compiled file can contain any number of
    // code segments, so we track the number of bytes of code
    // left to process:  When it hits zero, we've done all
    // the code segments.
    //
    // In practice, we currently always have exactly two
    // code segments, the first of which contains the
    // byte-coded logic constructing our literals (constants
    // -- see src/c/heapcleaner/make-package-literals-via-bytecode-interpreter.c)
    // and the second of which contains all our compiled
    // native code for the compiledfile, including that
    // which constructs our tree of exported (directly externally
    // visible) values.
    ////////////////////////////////////////////////////////////////

    bytes_of_code_remaining
	=
	header.bytes_of_compiled_code;

    // Read the size and the dummy entry point for the
    // first code segment (literal-constructing bytecodes).
    // The entrypoint offset of this first segment is always
    // zero, which is why we ignore it here:
    //
    read_n_bytes_from_file( file, &segment_bytesize, sizeof(Int1), filename );
    //
    segment_bytesize = BIGENDIAN_TO_HOST( segment_bytesize );
    //
    read_n_bytes_from_file( file, &entrypoint_offset_in_bytes, sizeof(Int1), filename );
    //	
    // entrypoint_offset_in_bytes = BIGENDIAN_TO_HOST( entrypoint_offset_in_bytes );

    bytes_of_code_remaining -=  segment_bytesize + 2 * sizeof(Int1);
    //
    if (bytes_of_code_remaining < 0) {
	//
	die ("format error (data size mismatch) in .compiled file \"%s\"", filename);
    }


    Val   mythryl_result =   HEAP_VOID;


    if (segment_bytesize > 0) {
	//
	Unt8* data_chunk =  MALLOC_VEC( Unt8, segment_bytesize );

	read_n_bytes_from_file( file, data_chunk, segment_bytesize, filename );

	mythryl_result = make_package_literals_via_bytecode_interpreter__may_heapclean (task, data_chunk, segment_bytesize, &roots1);

	FREE(data_chunk);
    }

    // Do a functional update of the last element of the import_record:
    //
    for (i = 0;  i < imports_record_slot_count;  i++) {
	//
	set_slot_in_nascent_heapchunk(task, i, PTR_CAST(Val*, import_record)[i-1]);	// <============ last use of import_record
    }
    set_slot_in_nascent_heapchunk( task, imports_record_slot_count, mythryl_result );
    mythryl_result = commit_nascent_heapchunk( task, imports_record_slot_count );

    Roots roots2 = { &mythryl_result, extra_roots };					// 'extra_roots' not '&roots1' because import_record is dead here.

    // Do a garbage collection, if necessary:
    //
    if (need_to_call_heapcleaner( task, PICKLEHASH_BYTES + REC_BYTESIZE(5)) ) {
        //
	call_heapcleaner_with_extra_roots (task, 0, &roots2 );
    }

    while (bytes_of_code_remaining > 0) {						// In practice, we always execute this loop exactly once.
	//
        // Read the size and entry point for this code chunk:

	read_n_bytes_from_file( file, &segment_bytesize, sizeof(Int1), filename );
      
	segment_bytesize =  BIGENDIAN_TO_HOST( segment_bytesize );

	read_n_bytes_from_file( file, &entrypoint_offset_in_bytes, sizeof(Int1), filename );

	entrypoint_offset_in_bytes =  BIGENDIAN_TO_HOST( entrypoint_offset_in_bytes );

        // How much more?
        //
	bytes_of_code_remaining -=  segment_bytesize + 2 * sizeof(Int1);
	//
	if (bytes_of_code_remaining < 0)   die ("format error (code size mismatch) in .compiled file \"%s\"", filename);

        // Allocate heap space and read code chunk:
	//
	Val code_chunk = allocate_nonempty_code_chunk (task, segment_bytesize);
	//
	read_n_bytes_from_file( file, PTR_CAST(char*, code_chunk), segment_bytesize, filename );

        // Flush the instruction cache, so CPU will see
        // our newly loaded code.  (To gain speed, and
        // simplify the hardware design, most modern CPUs
        //  assume that code is never modified on the fly,
        // or at least not without manually  flushing the
        // instruction cache this way.)
	//
	flush_instruction_cache (PTR_CAST(char*, code_chunk), segment_bytesize);					// flush_instruction_cache is a no-op on intel32
															// flush_instruction_cache	is from   src/c/h/flush-instruction-cache-system-dependent.h 
        // Create closure, taking entry point into account:
	//
	{   Val closure
		=
		make_one_slot_record(  task,  PTR_CAST( Val, PTR_CAST (char*, code_chunk) + entrypoint_offset_in_bytes)  );

	    // Apply the closure to the import picklehash vector.
	    //
	    // This actually executes all the top-level code for
	    // the compile unit, which is to say that if the
	    // source for our compiledfile looked something like
	    //
	    // package my_pkg {
	    //     my _ = file::print "Hello, world!\n";
	    // };
	    //
	    // then when we do the following 'apply' call, you'd see
	    //
	    // Hello, world!
	    //
	    // printed on the standard output.
	    //
	    // In addition, invisible compiler-generated code
	    // constructs and returns the tree of exports from
	    // our compiledfile.
	    //
	    save_c_state                                          (task, extra_roots);				// We do NOT want mythryl_result on the extra_roots list here.
	    mythryl_result =  run_mythryl_function__may_heapclean (task, closure, mythryl_result, TRUE, NULL); 	// run_mythryl_function__may_heapclean		def in   src/c/main/run-mythryl-code-and-runtime-eventloop.c
	    restore_c_state					  (task, extra_roots);
	}

	if (need_to_call_heapcleaner (task, PICKLEHASH_BYTES+REC_BYTESIZE(5))) {
	    //
	    call_heapcleaner_with_extra_roots (task, 0, &roots2 );
        }
    }

    // Publish this compiled_file's exported-values tree
    // for the benefit of compiled_files loaded later:
    //
    if (bytes_of_exports) {
	//
	register_compiled_file_exports__may_heapclean (
            task,
            &export_picklehash,     // key -- the 16-byte picklehash naming this compiledfile.
            mythryl_result,         // val -- the tree of exported Mythryl values.
	    extra_roots
        );
    }

    fclose( file );
}                                   // load_compiled_file__may_heapclean



static void   register_compiled_file_exports__may_heapclean   (
    //        =============================================
    //
    Task*       task,
    Picklehash* c_picklehash,       // Picklehash key as a C string.
    Val         exports_tree,
    Roots*      extra_roots 
){
    Roots roots1 = { &exports_tree, extra_roots };

    ///////////////////////////////////////////////////////////
    // Add a picklehash/exports_tree key/val pair to our
    // heap-allocated list of loaded compiled_files.
    ///////////////////////////////////////////////////////////

    // Copy the picklehash naming this compiledfile
    // into the Mythryl heap, so that we can use
    // it in a Mythryl-heap record:
    //
    Val heap_picklehash
	=
	allocate_nonempty_ascii_string__may_heapclean( task,  PICKLEHASH_BYTES, &roots1 );	// allocate_nonempty_ascii_string__may_heapclean	def in   src/c/heapcleaner/make-strings-and-vectors-etc.c
												
    memcpy( HEAP_STRING_AS_C_STRING(heap_picklehash), (char*)c_picklehash, PICKLEHASH_BYTES );

    // Allocate the list record and thread it onto the exports list:
    //
    PERVASIVE_PACKAGE_PICKLE_LIST__GLOBAL
	=
        make_three_slot_record( task,
	    //
	    heap_picklehash,					// Key naming compiledfile -- first slot in new record.
	    exports_tree,					// Tree of values exported from compiledfile -- second slot in new record.
	    PERVASIVE_PACKAGE_PICKLE_LIST__GLOBAL		// Pointer to next record in list -- third slot in new record.
	);
}



static Val   picklehash_to_exports_tree   (Picklehash* picklehash)   {
    //       ==========================
    //
    Val        p;

    //////////////////////////////////////////////////////////////////////////////
    // We identify a (particular version of) a compiledfile using
    // a 16-byte hash of its serialized ("pickled") form.
    //
    // Our global PERVASIVE_PACKAGE_PICKLE_LIST__GLOBAL is a singly
    // linked list with one entry for each compiledfile which
    // we have loaded into memory.
    //
    // Each entry in the list maps the picklehash naming that
    // compiledfile to the tree of values (functions etc)
    // exported by the compiledfile for use by other compiled_files.
    //
    // Here we look up the export tree associated with a given
    // picklehash by doing an O(N) scan down the linklist.
    //
    // XXX BUGGO FIXME  It is criminally st00pid to be using an
    // O(N) lookup algorithm for a linklist which will often be
    // hundreds or even thousands of entries long, on which we
    // may be doing up to a million lookups.  Especially when
    // the key comparisons are expensive.  Can't we arrange to
    // use our standard redblack tree implementation here?
    //////////////////////////////////////////////////////////////////////////////



    // For all compiled_files loaded into memory:
    //
    for (p = PERVASIVE_PACKAGE_PICKLE_LIST__GLOBAL;  p != HEAP_VOID;  p = GET_TUPLE_SLOT_AS_VAL(p, 2)) {

        // If the picklehash on this record
        // matches our search key picklehash...
        //
        Val id = GET_TUPLE_SLOT_AS_VAL(p, 0);
	if (memcmp( (char*) picklehash, HEAP_STRING_AS_C_STRING(id), PICKLEHASH_BYTES) == 0) {

	    // ... then return its matching export tree:
	    //
	    return  GET_TUPLE_SLOT_AS_VAL(p, 1);
        }
    }

    // If we get here, something is badly broken:
    // We should never be asked to find a picklehash
    // which isn't in the list -- all COMPILED_FILES_TO_LOAD
    // lists are supposed to be topologically sorted by
    // dependencies, so we never load an compiledfile until
    // we've loaded every compiledfile it depends upon.
    //
    {   char	buf[ PICKLEHASH_BYTES * 4 ];
        //
	picklehash_to_hex_string( buf, PICKLEHASH_BYTES * 4, picklehash );
	//
	die ("unable to find picklehash (compiledfile identifier) '%s'", buf);	// Doesn't return.
    }
    exit(1);										// Redundant -- just to suppress gcc warning.
}



static void   picklehash_to_hex_string   (char* buf,  int buflen,  Picklehash* picklehash)   {
    //        ========================
    //
    // Convert picklehash to a string like "[a10f37c690a348700382e0fe1176a109]"
    //
    char*   cp = buf;    // XXX BUGGO FIXME no buffer overrun check.   Then again, it is only used when we're already going down. :-)
    
    *cp++ = '[';

    {   int  i;
	for (i = 0;  i < PICKLEHASH_BYTES;  i++) {
	    //
	    sprintf (cp, "%02x", picklehash->bytes[i]);
	    cp += 2;
	}
    }

    *cp++ = ']';
    *cp++ = '\0';
}


// COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2014,
// released per terms of SMLNJ-COPYRIGHT.





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
