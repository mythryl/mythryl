// generate-regmask-h.c
//
// This file generates default definitions of some compiler flags and
// various register masks.  The masks define the registers that are
// live in the following situations:
//
//   FUN_MASK	-- typeagnostic (wrapped) function entry.
//
//   RET_MASK	-- return fate mask
//
//   CONT_MASK	-- wrapped callcc fate entry.
//
//   EXN_MASK	-- exception handler entry
//
// The defined constants are:
//
//   CALLEE_SAVED_REGISTERS_COUNT
//   CALLEE_SAVED_FLOAT_REGISTERS_COUNT

#include "../mythryl-config.h"

#include "header-file-autogeneration-stuff.h"

#ifndef DST_FILE
    #define DST_FILE "reg-mask.h"
#endif

#ifndef CALLEE_SAVED_REGISTERS_COUNT
#  define GEN_CALLEESAVE
#  if defined(TARGET_M68)
#    define CALLEE_SAVED_REGISTERS_COUNT	0
#  else
#    define CALLEE_SAVED_REGISTERS_COUNT	3
#  endif
#endif
#ifndef CALLEE_SAVED_FLOAT_REGISTERS_COUNT
#  define GEN_FLOAT_CALLEESAVE
#  define CALLEE_SAVED_FLOAT_REGISTERS_COUNT 0
#endif

#if (CALLEE_SAVED_REGISTERS_COUNT > 0)
#  define FUN_MASK ((1 << (CALLEE_SAVED_REGISTERS_COUNT + 4)) - 1)
#  define RET_MASK ((1 << (CALLEE_SAVED_REGISTERS_COUNT + 4)) - 0x10 + 0xc)
#  define CONT_MASK FUN_MASK
#  define EXN_MASK FUN_MASK
#else
#  define FUN_MASK ((1 << (CALLEE_SAVED_REGISTERS_COUNT + 4)) - 1)
#  define RET_MASK (0xd)
#  define CONT_MASK FUN_MASK
#  define EXN_MASK CONT_MASK
#endif

main ()
{
    char*       filename      = DST_FILE;
    char*       unique_string = "REG_MASK_H";
    char*       progname      = "src/c/config/generate-regmask-h.c";

    FILE	    *f;

    f = start_generating_header_file( filename, unique_string, progname );

    fprintf (f, "\n");
#ifdef GEN_CALLEESAVE
    fprintf (f, "#define CALLEE_SAVED_REGISTERS_COUNT       %d\n", CALLEE_SAVED_REGISTERS_COUNT);
#endif
#ifdef GEN_FLOAT_CALLEESAVE
    fprintf (f, "#define CALLEE_SAVED_FLOAT_REGISTERS_COUNT %d\n", CALLEE_SAVED_FLOAT_REGISTERS_COUNT);
#endif
    fprintf (f, "\n");
    fprintf (f, "#define FUN_MASK\t\t%d\t/*\t%#010x\t*/\n", 
	     FUN_MASK, FUN_MASK);
    fprintf (f, "#define RET_MASK\t\t%d\t/*\t%#010x\t*/\n", 
	     RET_MASK, RET_MASK);
    fprintf (f, "#define CONT_MASK\t\t%d\t/*\t%#010x\t*/\n",
	     CONT_MASK, CONT_MASK);
    fprintf (f, "#define EXN_MASK\t\t%d\t/*\t%#010x\t*/\n", 
	     EXN_MASK, EXN_MASK);
    fprintf (f, "\n");

    finish_generating_header_file( f, unique_string );

    exit (0);

}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2013,
// released per terms of SMLNJ-COPYRIGHT.

