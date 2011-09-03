/* poll.c
 *
 * crude implementation of a polling function
 */

#include "../../config.h"

#include <windows.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "lib7-c.h"

#include "win32-fault.h"


/* _lib7_win32_OS_poll : unt1 list * (int1.Int * int) option -> unt1 list
 */
Val _lib7_win32_OS_poll (Task *task, Val arg)
{
  DWORD dwMilliseconds;
  Val pollList = GET_TUPLE_SLOT_AS_VAL(arg,0);
  Val timeout = GET_TUPLE_SLOT_AS_VAL (arg,1);
  int sec,usec;
  Val l,item;
  Val resList;
  HANDLE handle,*hArray;
  int result;

  int count,index;

  /* first, convert timeout to milliseconds */
  if (timeout==OPTION_NULL)
    dwMilliseconds = INFINITE;
  else {
    timeout = OPTION_GET(timeout);
    sec = TUPLE_GET_INT1(timeout,0);
    usec = GET_TUPLE_SLOT_AS_INT(timeout,1);
    dwMilliseconds = (sec*1000)+(usec/1000);
  }

  /* count number of handles */
  for (l=pollList,count=0; l!=LIST_NIL; l=LIST_TAIL(l)) 
    count++;
  
  /* Allocate array of handles: */
  hArray = MALLOC_VEC (HANDLE,count);
  
  /* Initialize the array: */
  for (l=pollList,index=0; l!=LIST_NIL; l=LIST_TAIL(l)) {
    item = LIST_HEAD (l);
    handle = (HANDLE) WORD_LIB7toC (item);
    hArray[index++] = handle;
  }
    
  /* Generalized poll to see if anything is available: */
  result = WaitForMultipleObjects (count,hArray,FALSE,dwMilliseconds);
  if (result==WAIT_FAILED) return LIST_NIL;
  else if (result==WAIT_TIMEOUT) return LIST_NIL;

   /* At least one handle was ready. Find all that are */
  for (index = count,resList=LIST_NIL; index>0; index--) {
    handle = hArray[index-1];
    result = WaitForSingleObject (handle,0);
    if (result==WAIT_FAILED || result==WAIT_TIMEOUT) continue;
    WORD_ALLOC(task,item,(Val_Sized_Unt)handle);
    LIST_CONS (task,resList,item,resList);
  }

  FREE(hArray);
  return resList;
}    

/* end of poll.c */








/* COPYRIGHT (c) 1998 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
 * released under Gnu Public Licence version 3.
 */

