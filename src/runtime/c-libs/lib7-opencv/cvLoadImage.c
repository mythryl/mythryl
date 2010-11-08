/* cvLoadImage.c
 *
 */

#include "../../config.h"

#if HAVE_OPENCV_CV_H
#include <opencv/cv.h>
#endif

#if HAVE_OPENCV_HIGHGUI_H
#include <opencv/highgui.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "runtime-globals.h"
#include "cfun-proto-list.h"
#include "../lib7-c.h"

/* _lib7_OpenCV_cvLoadImage : String -> Image
 *
 */
lib7_val_t

_lib7_OpenCV_cvLoadImage (lib7_state_t *lib7_state, lib7_val_t arg)
{
#if HAVE_OPENCV_CV_H && HAVE_LIBCV

    char*      filename  =  STR_LIB7toC( arg );
    IplImage*  ipl_image =  cvLoadImage( filename, CV_LOAD_IMAGE_UNCHANGED );

    if (!ipl_image)   RAISE_ERROR(lib7_state, "cvLoadImage returned NULL");

    {   
	/* Copy image into heap, so that it can be
	 * garbage-collected when no longer needed:
	 */

	lib7_val_t header;	lib7_val_t header_data;
	lib7_val_t image;	lib7_val_t  image_data;

	lib7_val_t result;

        header_data  =  LIB7_CData(  lib7_state, image, sizeof(IplImage));
        SEQHDR_ALLOC(lib7_state, header, DESC_word8vec, header_data, sizeof(IplImage));

	CRoots[NumCRoots++] = &header;	/* Protect header from garbage collection while allocating image. */

	image_data   =  LIB7_CData(  lib7_state, ipl_image->imageData, ipl_image->imageSize);
        SEQHDR_ALLOC(lib7_state, image, DESC_word8vec, image_data, ipl_image->imageSize);
        
	--NumCRoots;
	cvReleaseImage( &image );

	REC_ALLOC2(lib7_state, result, header, image);

	return result;
    }

#else

    extern char* no_opencv_support_in_runtime;
    return RAISE_ERROR(lib7_state, no_opencv_support_in_runtime);

#endif
}



/* Notes:

   IplImage       is defined in		cxcore/include/cxtypes.h

   cvCreateImage  is defined in		cxcore/src/cxarray.cpp
   cvCreateData   is defined in		cxcore/src/cxarray.cpp
   cvReleaseImage is defined in		cxcore/src/cxarray.cpp

   cvLoadImage    is defined in		otherlibs/highgui/loadsave.cpp


 */

/* Code by Jeff Prothero: Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
