// set-heapdump-shebang.c

// Created 2007-03-15 CrT.

// "Executables" produced by the mythryl compiler are actually
// binary heap images with a shebang line at the top to run
// them via the mythryl-runtime-intel32 C executable.
//
// This internal shebang line needs to be different for
// test versions, when it will be something like
//     /pub/home/joe/src/mythryl/bin/runtime --shebang
// and for installed versions, where it will be
//     /usr/bin/runtime --shebang
//
// Our job in this program is to update such a shebang line.


#include "../mythryl-config.h"

#include <stdio.h>
#include <stdlib.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <string.h>
#include <errno.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "../heapcleaner/runtime-heap-image.h"


char* our_name = "set-heapdump-shebang.c";


void   usage   (void) {

    fprintf(stderr,"Usage: %s heapimage  '#!/usr/bin/mythryl-runtime-intel32 --shebang'\n", our_name );
    exit(1);
}






void   validate_shebang_line   ( char* shebang ) {

    if (shebang[0] != '#'
    ||  shebang[1] != '!'
    ){
	fprintf(stderr,"%s: shebang doesn't start with '#!': %s\n", our_name, shebang );
	exit(1);
    }

    if (strlen( shebang )+1 >= SHEBANG_SIZE) {
        fprintf(stderr,"%s: shebang is longer than %d limit: %s\n", our_name, SHEBANG_SIZE, shebang );
	exit(1);
    }
}



void   install_shebang   ( char* filename, char* shebang ) {

    Heapfile_Header header;

    int fd = open( filename, O_RDONLY );

    if (fd == -1) {
      fprintf(stderr,"%s: Couldn't open heap image %s for reading: %s\n", our_name, filename, strerror(errno));
      exit(1);
    }

    {   int bytes_read = read( fd, &header, sizeof( header ) );

	if (bytes_read == -1) {
	    fprintf(stderr,"%s: Couldn't read %d byte header from heap image %s for reading: %s\n", our_name, sizeof( header ), filename, strerror( errno));
	    exit(1);
	}

	if (bytes_read != sizeof( header )) {
	    fprintf(stderr,"%s: Failed to read header from heap image %s (got %d instead of %d bytes)\n", our_name, filename, bytes_read, sizeof( header ) );
	    exit(1);
	}

	if (header.magic != IMAGE_MAGIC      &&
	    header.magic != PICKLE_MAGIC
	){
	    fprintf(stderr,"%s: Don't recognize magic number in heap image %s\n", our_name, filename );
	    exit(1);
        }
    } 

    close( fd );

    {   int  i;
        for (i = SHEBANG_SIZE;  i --> 0; )  header.shebang[i] = 0;
        strcpy(
            header.shebang,
            shebang
        );
    }

    fd = open( filename, O_WRONLY );

    if (fd == -1) {
        fprintf(stderr,"%s: Couldn't open heap image %s for writing: %s\n", our_name, filename, strerror(errno));
        exit(1);
    }


    {   int bytes_written = write( fd, &header, sizeof( header ) );

	if (bytes_written == -1) {
	    fprintf(stderr,"%s: Couldn't write %d byte header to heap image %s: %s\n", our_name, sizeof( header ), filename, strerror( errno));
	    exit(1);
	}

	if (bytes_written != sizeof( header )) {
	    fprintf(stderr,"%s: Failed to write header to heap image %s (wrote %d instead of %d bytes)\n", our_name, filename, bytes_written, sizeof( header ) );
	    exit(1);
	}
    } 

    if (close( fd )) {
	fprintf(stderr,"%s: Failed to close %s after updating header: %s\n", our_name, filename, strerror( errno));
        exit(1);
    }
}



int   main   ( int argc, char** argv ) {

    our_name = argv[0];

    if (argc != 3)   usage();

    validate_shebang_line( argv[2] );
    install_shebang( argv[1], argv[2] );

    exit(0);
}


// Code by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.
