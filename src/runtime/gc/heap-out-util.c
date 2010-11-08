/* heap-out-util.c
 *
 * Utility routines to export (or blast) an Lib7 heap image.
 */

#include "../config.h"

#include <string.h>
#include "runtime-base.h"
#include "heap.h"
#include "runtime-values.h"
#include "runtime-heap-image.h"
#include "c-globals-table.h"
#include "heap-output.h"

#include "shebang-line.h"

/* HeapIO_WriteImageHeader:
 *
 *  Blast out the lib7_image_hdr_t.
 */
status_t HeapIO_WriteImageHeader (writer_t *wr, int kind)
{
    lib7_image_hdr_t	header;

    {   int i;
        for (i = SHEBANG_SIZE;  i --> 0; )  header.shebang[i] = 0;
        strcpy(
            header.shebang,
            SHEBANG_LINE
        );
    }

    header.byteOrder = ORDER;
    header.magic	  = ((kind == EXPORT_HEAP_IMAGE) || (kind == EXPORT_FN_IMAGE))
			? IMAGE_MAGIC : BLAST_MAGIC;
    header.kind	  = kind;
    /* header.arch[] */
    /* header.opsys[] */

    WR_Write(wr, &header, sizeof(header));
    if (WR_Error(wr))
	return FAILURE;
    else
	return SUCCESS;

} /* end of HeapIO_WriteImageHeader */


/* HeapIO_WriteExterns:
 *
 * Write out the external symbol table, returning the number of bytes
 * written (-1 on error).
 */
Addr_t HeapIO_WriteExterns (writer_t *wr, export_table_t *table)
{
    int			i, numExterns;
    export_item_t	*externs;
    extern_table_hdr_t	header;
    Addr_t		strSize, nbytes = sizeof(extern_table_hdr_t), padSzB;

    ExportedSymbols (table, &numExterns, &externs);

    for (strSize = 0, i = 0;  i < numExterns;  i++)
	strSize += (strlen(externs[i]) + 1);

    /* Include padding to WORD_SZB bytes: */
    padSzB = ROUNDUP(strSize, WORD_SZB) - strSize;
    strSize += padSzB;
    nbytes += strSize;

    /* Write out the header: */
    header.numExterns = numExterns;
    header.externSzB = strSize;
    WR_Write(wr, &header, sizeof(header));

    /* Write out the external symbols: */
    for (i = 0;  i < numExterns;  i++) {
	WR_Write (wr, externs[i], strlen(externs[i])+1);
    }

    /* Write the padding: */
    {
	char	pad[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	if (padSzB != 0) {
	    WR_Write (wr, pad, padSzB);
	}
    }

    /*
  done:;
    */
    FREE (externs);

    if (WR_Error(wr))
	return -1;
    else
	return nbytes;

} /* end of HeapIO_WriteExterns */


/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

