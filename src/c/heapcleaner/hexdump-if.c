// hexdump-if.c
//
// Code to generate classic debugger style hexdumps looking like:
//
//     1267202947.150471:  sendbuf.c/top: Data to send: 
//             00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f
//     000000: 42.00 00.0b;00.00 00.12|00.10 00.00;4d.49 54.2d|4d.41 47.49;43.2d 43.4f|4f.4b 49.45;2d.31 00.00|    B...........MIT-MAGIC-COOKIE-1..
//     000020: 2c.5c 95.7f;c4.1b c3.cf|57.44 4b.46;37.fe 67.be|                                                    ,\..Ä.ÃÏWDKF7þg¾
//
// At the moment we get called from:
//     src/c/lib/socket/recv.c
//     src/c/lib/socket/sendbuf.c
//
// The "-if" on our name is because, like log_if in  src/c/main/error-reporting.c
// we generate output on
//
//     log_if_fd
//
// -- and only if it is nonzero.
// 
// Created 2010-02-26 CrT.


#include "../mythryl-config.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// #include "sockets-osdep.h"
// #include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
// #include "raise-error.h"
// #include "cfun-proto-list.h"

#include "hexdump-if.h"

// Dump 'data' as hex, with displayed addresses starting at zero:
//
void   hexdump0     (  void (*writefn)(void*, char*), void* writefn_arg,	// Continuation receiving our output. writefn is often dump_buf_to_fd() (below), in which case writefn_arg is the fd. 
                       char* message,						// Explanatory title string for human consumption.
                       unsigned char* data,					// Data to be hexdumped.
                       int data_len						// Length of preceding.
                    )
{
    char buf[ 256 ];
    int i;
    writefn(writefn_arg, message);
    if (data_len > 32) {
	strcpy(buf,"\n        00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f");
	writefn(writefn_arg, buf);
    }
    for (i = 0; i < data_len; i += 32) {
	int  j;
	if (data_len > 32) {
	    sprintf (buf, "\n%06x: ", i);
	    writefn(writefn_arg, buf);
	}

	for (j = 0; j < 32; ++j) {

	    // Place puncturation between the byes
	    // to make it easier to find the boundaries
	    // of 16-bit, 32-bit and 64-bit aligned values:
	    //
	    int c;
	    switch (j & 7) {
	    case 0:  c = '.';	break;
	    case 1:  c = ' ';	break;
	    case 2:  c = '.';	break;
	    case 3:  c = ';';	break;
	    case 4:  c = '.';	break;
	    case 5:  c = ' ';	break;
	    case 6:  c = '.';	break;
	    case 7:  c = '|';	break;
	    }

	    if (i+j < data_len) {
		sprintf (buf, "%02x%c", data[i+j], c);
		writefn(writefn_arg, buf);
	    } else {
		if (data_len > 32) {
		    strcpy (buf, "   ");
		    writefn(writefn_arg, buf);
		}
	    }
	}
	strcpy (buf, "    ");
	writefn(writefn_arg, buf);

	for (j = 0; j < 32 && i+j < data_len; ++j) {
	    unsigned char c = data[i+j];
	    if (!isprint(c)) c = '.';
	    char buf2[2];
	    buf2[0] = c;
	    buf2[1] = '\0';
	    writefn(writefn_arg, buf2);
	}
    }

    writefn(writefn_arg, "\n");
}



// Dump 'data' as hex, with displayed addresses starting at 'data':
//
void   hexdump      (  void (*writefn)(void*, char*), void* writefn_arg,	// Continuation receiving our output. writefn is often dump_buf_to_fd() (below), in which case writefn_arg is the fd. 
                       char* message,						// Explanatory title string for human consumption.
                       unsigned char* data,					// Data to be hexdumped.
                       int data_len						// Length of preceding.
                    )
{
    char buf[ 256 ];
    int i;
    writefn(writefn_arg, message);

    unsigned char* start = (unsigned char*) (((Punt)data) & ~0x1F);		// Back up to a 32-byte boundary.
    unsigned char* stop  = data + data_len;
    if (stop - start > 32) {
	strcpy(buf,"\n            00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f");
	writefn(writefn_arg, buf);
    }
    for (i = 0; start + i < stop; i += 32) {
	int  j;
	if (stop - start > 32) {
	    sprintf (buf, "\n%p: ", start+i);
	    writefn(writefn_arg, buf);
	}

	// Display the 32 bytes on the line as hex:
	//
	for (j = 0; j < 32; ++j) {

	    // Place puncturation between the byes
	    // to make it easier to find the boundaries
	    // of 16-bit, 32-bit and 64-bit aligned values:
	    //
	    int c;
	    switch (j & 7) {
	    case 0:  c = '.';	break;
	    case 1:  c = ' ';	break;
	    case 2:  c = '.';	break;
	    case 3:  c = ';';	break;
	    case 4:  c = '.';	break;
	    case 5:  c = ' ';	break;
	    case 6:  c = '.';	break;
	    case 7:  c = '|';	break;
	    }

	    if (start + i+j < data) {
		sprintf (buf, "  %c", c);
		writefn(writefn_arg, buf);
	    } else if (start + i+j < stop) {
		sprintf (buf, "%02x%c", start[i+j], c);
		writefn(writefn_arg, buf);
	    } else {
		if (stop - start > 32) {
		    strcpy (buf, "   ");
		    writefn(writefn_arg, buf);
		}
	    }
	}
	strcpy (buf, "    ");
	writefn(writefn_arg, buf);

	// Now re-display the 32 bytes on the line as ascii:
	//
	for (j = 0; j < 32 && start+i+j < stop; ++j) {
	    char buf2[2];
	    if (start + i+j < data) {
		buf2[0] = ' ';
	    } else {	
		unsigned char c = start[i+j];
		if (!isprint(c)) c = '.';
		buf2[0] = c;
	    }
	    buf2[1] = '\0';
	    writefn(writefn_arg, buf2);
	}
    }

    writefn(writefn_arg, "\n");
}



static void   dump_buf_to_fd   (void* fd_as_voidptr, char* buf) {
    //        ==============
    //
    int fd = (int) fd_as_voidptr;
    //
    write(fd, buf, strlen(buf));
}

void   hexdump_if   (char* message, unsigned char* data, int data_len)   {
    // ==========
    //
    if (log_if_fd && data_len > 0) {
	//
	hexdump0( dump_buf_to_fd, (void*)log_if_fd, message, data, data_len );
    }
}



static void   dump_buf_to_file   (void* fd_as_voidptr, char* buf) {
    //        ================
    //
    FILE* fd = (FILE*) fd_as_voidptr;
    //
    fwrite(buf, 1, strlen(buf), fd);
}

void   hexdump_to_file  (FILE* fd, char* message, unsigned char* data, int data_len)   {
    // ===============
    //
    hexdump( dump_buf_to_file, (void*)fd, message, data, data_len );
}

// COPYRIGHT (c) 2010 by Jeff Prothero,
// released under Gnu Public Licence version 3.

