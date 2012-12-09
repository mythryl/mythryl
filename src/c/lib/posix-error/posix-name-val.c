// posix-name-val.c
//
// Support for string to int lookup.


#include "../../mythryl-config.h"

#include <stdlib.h>
#include <string.h>
#include "posix-name-val.h"

static int   cmp   (const void *key, const void *item) {
    //       ===
    //
    return strcmp(((name_val_t*)key)->name, ((name_val_t*)item)->name);
}



name_val_t*   _lib7_posix_nv_binary_search   (char* key,  name_val_t* array,  int numelms) {
    //        =====================
    //
    // Mythryl type:
    //
    // Given a string key, an array of name/value pairs and the size of the
    // array, find element in the array with matching key and return a pointer
    // to it. If not found, return NULL. We use binary search, so we assume
    // the array is sorted.
    //
    name_val_t   k;
    //
    k.name = key;
    //
    return ((name_val_t *)bsearch(&k,array,numelms,sizeof (name_val_t),cmp));
}


// COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
//
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2012,
// released under Gnu Public Licence version 3.

