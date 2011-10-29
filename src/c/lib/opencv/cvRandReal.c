// cvRandReal.c

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
#include "../lib7-c.h"

/* _lib7_OpenCV_cvRandReal : Random_Number_Generator -> Float
 *
 */
Val

_lib7_OpenCV_cvRandReal (Task *task, Val arg)
{
#if HAVE_OPENCV_CV_H && HAVE_LIBCV

    CvRNG* rng
        =
        GET_VECTOR_DATACHUNK_AS( CvRNG*, arg );

    double  random_float64
        =
        cvRandReal( rng );

    Val	result;    REAL64_ALLOC(task, result, random_float64 );

    return result;

#else

    extern char* no_opencv_support_in_runtime;
    return RAISE_ERROR(task, no_opencv_support_in_runtime);

#endif
}

// Code by Jeff Prothero: Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.
