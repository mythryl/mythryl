/* cvCreateImage.c
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

/* _lib7_OpenCV_cvCreateImage : String -> Image
 *
 */
lib7_val_t

_lib7_OpenCV_cvCreateImage (lib7_state_t *lib7_state, lib7_val_t arg)
{
#if HAVE_OPENCV_CV_H && HAVE_LIBCV
  /*
    int height =  700;
    int width  = 1000;
    cvCreateImage( cvSize(1000,700),8,3);
    return LIB7_void;
  */

    IplImage img;

    lib7_val_t data   =  LIB7_CData(  lib7_state, &img, sizeof(img));

    lib7_val_t result;    SEQHDR_ALLOC(lib7_state, result, DESC_word8vec, data, sizeof(img));

    return result;

#else

    extern char* no_opencv_support_in_runtime;
    return RAISE_ERROR(lib7_state, no_opencv_support_in_runtime);

#endif
}



/* Notes:

   IplImage      is defined in		cxcore/include/cxtypes.h

   cvCreateImage is defined in		cxcore/src/cxarray.cpp



 */

/* Code by Jeff Prothero: Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

