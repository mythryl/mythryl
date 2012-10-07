// ramlog.c
//
// Sprintfing debug info into a circular ram buffer.
//
// See also:
//
//     SYSCALL_LOG stuff in   src/c/h/runtime-base.h

#include "../mythryl-config.h"

#include <stdio.h>
#include <stdarg.h>
#include "runtime-base.h"

void   ramlog_sprintf   (char *format, ...)   {
    // ==============
    //
    char buf[ 132 ];
    va_list	ap;
    va_start (ap, format);
    vsprintf (buf, format, ap);
    va_end(ap);
}



// Code by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.


