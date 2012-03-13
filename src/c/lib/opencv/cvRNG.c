// cvRNG.c

#include "../../mythryl-config.h"

#if HAVE_OPENCV_CV_H
#include <opencv/cv.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"
#include "../raise-error.h"

#include <string.h>	// XXX BUGGO DELETEME

/* _lib7_OpenCV_cvRNG : Int -> Random_Number_Generator
 *
 */
Val

_lib7_OpenCV_cvRNG (Task *task, Val arg)
{

#if HAVE_OPENCV_CV_H && HAVE_LIBCV
    //
    int init  = TAGGED_INT_TO_C_INT( arg );								// Last use of 'arg'.
    //
    CvRNG rng = cvRNG( (unsigned) init);

    Val data  =  make_biwordslots_vector_sized_in_bytes__may_heapclean( task, &rng, sizeof(rng), NULL );

    return  make_vector_header(task,  UNT8_RO_VECTOR_TAGWORD, data, sizeof(rng));

#else

    extern char* no_opencv_support_in_runtime;

    return RAISE_ERROR__MAY_HEAPCLEAN(task, no_opencv_support_in_runtime, NULL);

#endif
}

// Code by Jeff Prothero: Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

