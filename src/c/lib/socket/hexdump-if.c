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
// The "-if" on our name is because, like print-if.c,
// we generate output on
//
//     print_if_fd
//
// -- and only if it is nonzero.
// 
// Created 2010-02-26 CrT.


#include "../../config.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#include "print-if.h"
#include "hexdump-if.h"

void   hexdump_if   (char* message, unsigned char* data, int data_len)   {
    // ==========
    //
    if (print_if_fd && data_len > 0) {

        char buf[ 256 ];
	int i;
        print_if( message );
	if (data_len > 32) {
            strcpy(buf,"\n        00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f");
	    write(print_if_fd, buf, strlen(buf));
        }
	for (i = 0; i < data_len; i += 32) {
	    int  j;
	    if (data_len > 32) {
		sprintf (buf, "\n%06x: ", i);
		write(print_if_fd, buf, strlen(buf));
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
	            write(print_if_fd, buf, 3);
		} else {
		    if (data_len > 32) {
			strcpy (buf, "   ");
			write(print_if_fd, buf, strlen(buf));
		    }
		}
	    }
	    strcpy (buf, "    ");
	    write(print_if_fd, buf, strlen(buf));

	    for (j = 0; j < 32 && i+j < data_len; ++j) {
		unsigned char c = data[i+j];
		if (!isprint(c)) c = '.';
		write(print_if_fd, &c, 1);
	    }
	}
	write(print_if_fd, "\n", 1);
    }
}



// COPYRIGHT (c) 2010 by Jeff Prothero,
// released under Gnu Public Licence version 3.

