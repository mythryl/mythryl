// osval.c

#include "../../config.h"

#include "system-dependent-unix-stuff.h"

#if HAVE_TERMIOS_H
    #include <termios.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "heap-tags.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "../posix-error/posix-name-val.h"

static name_val_t values [] = {
  {"B0", B0},
  {"B110", B110},
  {"B1200", B1200},
  {"B134", B134},
  {"B150", B150},
  {"B1800", B1800},
  {"B19200", B19200},
  {"B200", B200},
  {"B2400", B2400},
  {"B300", B300},
  {"B38400", B38400},
  {"B4800", B4800},
  {"B50", B50},
  {"B600", B600},
  {"B75", B75},
  {"B9600", B9600},
  {"BRKINT", BRKINT},
  {"CLOCAL", CLOCAL},
  {"CREAD", CREAD},
  {"CS5", CS5},
  {"CS6", CS6},
  {"CS7", CS7},
  {"CS8", CS8},
  {"CSIZE", CSIZE},
  {"CSTOPB", CSTOPB},
  {"ECHO", ECHO},
  {"ECHOE", ECHOE},
  {"ECHOK", ECHOK},
  {"ECHONL", ECHONL},
  {"EOF", VEOF},
  {"EOL", VEOL},
  {"ERASE", VERASE},
  {"HUPCL", HUPCL},
  {"ICANON", ICANON},
  {"ICRNL", ICRNL},
  {"IEXTEN", IEXTEN},
  {"IGNBRK", IGNBRK},
  {"IGNCR", IGNCR},
  {"IGNPAR", IGNPAR},
  {"INLCR", INLCR},
  {"INPCK", INPCK},
  {"INTR", VINTR},
  {"ISIG", ISIG},
  {"ISTRIP", ISTRIP},
  {"IXOFF", IXOFF},
  {"IXON", IXON},
  {"KILL", VKILL},
  {"MIN", VMIN},
  {"NCCS", NCCS},
  {"NOFLSH", NOFLSH},
  {"OPOST", OPOST},
  {"PARENB", PARENB}, 
  {"PARMRK", PARMRK},
  {"PARODD", PARODD},
  {"QUIT", VQUIT},
  {"START", VSTART},
  {"STOP", VSTOP},
  {"SUSP", VSUSP},
  {"TCIFLUSH", TCIFLUSH},
  {"TCIOFF", TCIOFF},
  {"TCIOFLUSH", TCIOFLUSH},
  {"TCION", TCION},
  {"TCOFLUSH", TCOFLUSH},
  {"TCOOFF", TCOOFF},
  {"TCOON", TCOON},
  {"TCSADRAIN", TCSADRAIN},
  {"TCSAFLUSH", TCSAFLUSH},
  {"TCSANOW", TCSANOW},
  {"TIME", VTIME},
  {"TOSTOP", TOSTOP},
};

#define NUMELMS ((sizeof values)/(sizeof (name_val_t)))


Val   _lib7_P_TTY_osval   (Task* task,  Val arg)   {
    //=================
    //
    // Mythryl type:   String -> Unt
    //
    // Return the OS-dependent, compile-time constant specified by the string.

    name_val_t* result = _lib7_posix_nv_lookup (HEAP_STRING_AS_C_STRING(arg), values, NUMELMS);

    if (result)   return  TAGGED_INT_FROM_C_INT( result->val );
    else          return  RAISE_ERROR(task, "system constant not defined");
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

