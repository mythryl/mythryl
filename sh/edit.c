// edit.c -- apply edits from foo.EDITS edits to a file foo
// 2007-04-07 CrT: Created.
//
// gcc -std=c99 edit.c -o edit
//
// Invocation is usually via "sh/do-edits" at the toplevel.

// Input to this program is two files, one a regular
// source file (SML/NJ / Mythryl, not that it matters
// to this program) and the other an edit file looking
// like
//
//    ...
//    3947: `let` -> `{`
//    4127: `` -> `;`
//    5867: `` -> `;`
//    6116: `` -> `;`
//    6479: `` -> `;`
//    6724: `` -> `;`
//    6755: `` -> `;`
//    6759: `in` -> ``
//    6794: `` -> `;`
//    6798: `end` -> `}`
//    ...
//
// The salient features here are that:
//
//  o  There is one edit command per line.
//
//  o  The first field is the character offset
//     in the input file at which to apply the
//     edit.  We assume they are sorted into
//     ascending order.
//
//  o  Our job is to replace the first string
//     given by the second.  This could be a
//     replacement, insertion or deletion,
//     depending on which of the given strings
//     is empty, if any.


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

char* program_name = "";
char* file_name = "";
FILE* text;
FILE* edits;
FILE* edited;

void usage(void) {

    fprintf (stderr,"usage:  %s foo.bar\n");
    exit(1);
}

FILE* file_open( char* file_name, char* mode ) {
    FILE*  result = fopen( file_name, mode );
    if (!result) {
        fprintf(
            stderr,
            "%s: Couldn't open %s: %s\n",
            program_name,
            file_name,
            strerror(errno)
        );
    }
    return result;
}

void init(int argc, char** argv) {

    char buf[ 1024 ];

    if (argc != 2)   usage();

    program_name = argv[0];
    file_name    = argv[1];

    text = file_open( argv[1], "r" );

    sprintf( buf, "%s.EDITS", argv[1] );

    edits = file_open( buf, "r" );

    sprintf( buf, "%s.EDITED", argv[1] );

    edited = file_open( buf, "w" );
}

int  edit_offset;
char from_string[ 128 ];
char to_string[ 128 ];

void require_char( int c0, int c1 ) {
    if (c1 == EOF) {
        fprintf(stderr,"Unexpected EOF in edits file\n");
        exit(1);
    }
    if (c0 != c1) {
        fprintf (stderr,"Got %c when %c was expected while reading edits file\n", c0, c1 );
        exit(1);
    }
}

void require_string( char* string ) {
    int c;
    while (*string) {
        c = fgetc( edits );
        require_char( *string, c );
        ++string;
    }
}

int read_edit_spec(void) {
    //
    // I wanted to do just
    //
    //     int count = fscanf( edits, "%d: `%[^`]` -> `%[^`]`\n", &edit_offset, from_string, to_string );
    //
    // but fscanf has some weird prohibition against accepting
    // zero-length fields :( so we just do it by hand:
    ///
    char buf[ 4096 ];
    char* p = buf;
    int   i = fgetc( edits );
    if (i == EOF)   return FALSE;
    do {
	*p++ = i; 
    } while (isdigit( i = fgetc( edits )));
    *p = '\0';
    edit_offset = atoi( buf );
    require_char( ':', i );
    require_string( " `" );

    p = from_string;
    while (( i = fgetc( edits )) != '`') {
        require_char(i,i);
        *p++ = i;
    }
    *p = '\0';

    require_string( " -> `" );

    p = to_string;
    while (( i = fgetc( edits )) != '`') {
        require_char(i,i);
        *p++ = i;
    }
    *p = '\0';

    require_string( "\n" );

    return TRUE;
}


int file_getc( FILE* fd ) {
    int result = fgetc( text );
    if (result == EOF) {
	fprintf(stderr,"%s: Unexpected EOF on %s\n",program_name, file_name);
	exit(1);
    }
    return result;
}

int edit(char* filename) {
    int offset_in_text = 0;
    int c;
    int edits_done = 0;

    // Over all edits to be done:
    //
    while (read_edit_spec()) {

        // Copy forward through file to read edit point:
        //
        while (offset_in_text < edit_offset) {
            fputc( file_getc( text ), edited );
            ++offset_in_text;
        }

        // Verify that expected text is found at that point:
	//
        {   char* to_match = from_string;
            while (*to_match) {
                c = file_getc( text );
                if (c != *to_match) {
		    fprintf(
                        stderr,
                        "%s: at offset %d in %s read '%c'x=%02x (expected '%c'x=%02x) while trying to match %s\n",
                        program_name,
                        offset_in_text,
                        filename,
                        c,
                        *to_match,
                        from_string   
                    );
                    exit(1);
                }
                ++offset_in_text;
                ++to_match; 
            }
        }

        // Replace it per edit specification:
        //
        {   char* to_write = to_string;
            while (*to_write) {
	      fputc( *to_write, edited);
                ++to_write; 
            }
        }

        ++edits_done;
    }

    // Copy remaining input file to edited:
    //
    while ((c = fgetc( text )) != EOF) {
        fputc( c, edited );
    }

    fclose( edited );
    fclose( text );
    fclose( edits );

    fprintf(stderr, "%s: %d edits done.\n", program_name, edits_done); 
}

main( int argc, char** argv ) {

    init( argc, argv );

    edit(argv[1]);

    exit(0);
}

