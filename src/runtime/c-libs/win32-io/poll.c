/* poll.c
 *
 * crude implementation of a polling function
 */

#include "../../config.h"

#include <windows.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"

#include "win32-fault.h"


/* _lib7_win32_OS_poll : word32 list * (int32.Int * int) option -> word32 list
 */
lib7_val_t _lib7_win32_OS_poll (lib7_state_t *lib7_state, lib7_val_t arg)
{
  DWORD dwMilliseconds;
  lib7_val_t pollList = REC_SEL(arg,0);
  lib7_val_t timeout = REC_SEL (arg,1);
  int sec,usec;
  lib7_val_t l,item;
  lib7_val_t resList;
  HANDLE handle,*hArray;
  int result;

  int count,index;

  /* first, convert timeout to milliseconds */
  if (timeout==OPTION_NONE)
    dwMilliseconds = INFINITE;
  else {
    timeout = OPTION_get(timeout);
    sec = REC_SELINT32(timeout,0);
    usec = REC_SELINT(timeout,1);
    dwMilliseconds = (sec*1000)+(usec/1000);
  }

  /* count number of handles */
  for (l=pollList,count=0; l!=LIST_nil; l=LIST_tl(l)) 
    count++;
  
  /* Allocate array of handles: */
  hArray = NEW_VEC (HANDLE,count);
  
  /* Initialize the array: */
  for (l=pollList,index=0; l!=LIST_nil; l=LIST_tl(l)) {
    item = LIST_hd (l);
    handle = (HANDLE) WORD_LIB7toC (item);
    hArray[index++] = handle;
  }
    
  /* Generalized poll to see if anything is available: */
  result = WaitForMultipleObjects (count,hArray,FALSE,dwMilliseconds);
  if (result==WAIT_FAILED) return LIST_nil;
  else if (result==WAIT_TIMEOUT) return LIST_nil;

   /* At least one handle was ready. Find all that are */
  for (index = count,resList=LIST_nil; index>0; index--) {
    handle = hArray[index-1];
    result = WaitForSingleObject (handle,0);
    if (result==WAIT_FAILED || result==WAIT_TIMEOUT) continue;
    WORD_ALLOC(lib7_state,item,(Word_t)handle);
    LIST_cons (lib7_state,resList,item,resList);
  }

  FREE(hArray);
  return resList;
}    

/* end of poll.c */








/* COPYRIGHT (c) 1998 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

