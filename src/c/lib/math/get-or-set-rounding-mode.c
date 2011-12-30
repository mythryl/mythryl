// get-or-set-rounding-mode.c


#include "../../mythryl-config.h"

#include "runtime-base.h"
#include "fp-dep.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"
#include "lib7-c.h"

#ifndef NO_ROUNDING_MODE_CTL
    // 
    // Mapping between the Mythryl and C representations of rounding modes.
    //	
    #if defined(RMODE_C_EQ_LIB7)
        #define RMODE_CtoLib7(m)	TAGGED_INT_FROM_C_INT(m)
        #define RMODE_LIB7toC(m)	TAGGED_INT_TO_C_INT(m)
    #else
        #define RMODE_CtoLib7(m)						\
	  (RMODE_EQ(m, FE_TONEAREST) ? TAGGED_INT_FROM_C_INT(0)				\
	    : (RMODE_EQ(m, FE_TOWARDZERO) ? TAGGED_INT_FROM_C_INT(1)			\
	      : (RMODE_EQ(m, FE_UPWARD) ? TAGGED_INT_FROM_C_INT(2) : TAGGED_INT_FROM_C_INT(3))))
	static fe_rnd_mode_t ModeMap[4] = {
		FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD, FE_DOWNWARD
	    };
        #define RMODE_LIB7toC(m)	ModeMap[TAGGED_INT_TO_C_INT(m)]
    #endif
#endif /* !NO_ROUNDING_MODE_CTL */



// Get/set the rounding mode; the values are interpreted as follows:
//
//	0	To nearest
//	1	To zero
//	2	To +Inf
//	3	To -Inf
//
Val   _lib7_Math_get_or_set_rounding_mode   (Task* task,  Val arg)   {
    //===================================
    //
    // Mythryl type:   Null_Or(Int) -> Int
    //
    // This fn gets bound as   get_or_set_rounding_mode   in:
    //
    //     src/lib/std/src/ieee-float.pkg
    //

									    ENTER_MYTHRYL_CALLABLE_C_FN("_lib7_Math_get_or_set_rounding_mode");

    #ifdef NO_ROUNDING_MODE_CTL
	//
        return RAISE_ERROR(task, "Rounding mode control not supported");
	//
    #else
	//
	if (arg == OPTION_NULL) {
	    //
	    fe_rnd_mode_t	 result = fegetround();
	    return RMODE_CtoLib7(result);

	} else {

	    fe_rnd_mode_t	  mode   =  RMODE_LIB7toC( OPTION_GET( arg ));
	    fe_rnd_mode_t	  result =  fesetround( mode );				// fesetround	def in    src/c/machine-dependent/prim.intel32.asm
	    return RMODE_CtoLib7( result );
	}
    #endif
}



// COPYRIGHT (c) 1996 AT&T Research.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.

