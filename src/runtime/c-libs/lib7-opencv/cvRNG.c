/* cvRNG.c
 *
 */

#include "../../config.h"

#if HAVE_OPENCV_CV_H
#include <opencv/cv.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"
#include "../lib7-c.h"

#include <string.h>	/* XXX BUGGO DELETEME */

/* _lib7_OpenCV_cvRNG : Int -> Random_Number_Generator
 *
 */
lib7_val_t

_lib7_OpenCV_cvRNG (lib7_state_t *lib7_state, lib7_val_t arg)
{

#if HAVE_OPENCV_CV_H && HAVE_LIBCV

    int init = INT_LIB7toC( arg );
    CvRNG rng = cvRNG( (unsigned) init);

    lib7_val_t data   =  LIB7_CData(  lib7_state, &rng, sizeof(rng));

    lib7_val_t result;    SEQHDR_ALLOC(lib7_state, result, DESC_word8vec, data, sizeof(rng));

    return result;

#else

    extern char* no_opencv_support_in_runtime;

    return RAISE_ERROR(lib7_state, no_opencv_support_in_runtime);

#endif
}

/* Code by Jeff Prothero: Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

