/* o7-file.h
 *
 * The layout is:
 *   header
 *   import PerIDs (16 bytes each)
 *   export PerIDs (16 bytes each)
 *   CM dependency information
 *   inlinable lambda expression
 *   reserved area 1 (typically empty)
 *   reserved area 2 (typically empty)
 *   code chunks
 *     This section contains a sequence of code chunks, each of
 *     which is lead by its size.  The individual sizes must sum up to
 *     bytes_of_compiled_code.
 *   pickled static environment
 */

#ifndef _O7_FILE_
#define _O7_FILE_

#ifndef _LIB7_BASE_
#include "runtime-base.h"
#endif


/** Picklehashes.  These are 16-byte          */
/* hashes of the contents of an .o7 file,  */
/* which we use as a convenient name for that */
/* version of that file:                      */
#define PICKLEHASH_BYTES	16

typedef struct {	    /* A hash of a pickled (serialized) oh7_file. */
    Byte_t	bytes[ PICKLEHASH_BYTES ];
} picklehash_t;


typedef struct {	    /* The header of a .o7 file; note that the fields */
			    /* are in big-endian representation. XXX BUGGO FIXME */
    /* XXX BUGGO FIXME Should probably put a shebang line here leading to our odump7 utility -- once it is written. :) */
    Byte_t	magic[16];	                /* Magic number.                                       */
    Int32_t	number_of_imported_picklehashes;
    Int32_t	number_of_exported_picklehashes;
    Int32_t	bytes_of_import_tree;		/* Bytes of references to values in other oh7_files. */
    Int32_t	bytes_of_dependency_info;	/* The size of the make7 dependency information area.  */
    Int32_t	bytes_of_inlinable_code;	/* Nubmer of bytes of  inlinable intermediate code.    */
    Int32_t	reserved;			/* Reserved for future use.                            */
    Int32_t	pad;	        		/* Padding for code segment alignment.                 */
    Int32_t	bytes_of_compiled_code;
    Int32_t	bytes_of_symbol_table;		/* Holds type the information for our exported values. */
} oh7_file_hdr_t;

#endif /* !_O7_FILE_ */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

