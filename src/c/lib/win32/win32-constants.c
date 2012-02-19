/* win32-constants.c
 *
 * interface to win32 constants
 */

#include "../../mythryl-config.h"

#include <windows.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "raise-error.h"

#include "name-val.h"

#define TAB_SZ(t)  ((sizeof t)/(sizeof(name_val_t)))

typedef struct {
  name_val_t *ptab;
  int size;
} tab_desc_t;

/* general table */
static name_val_t general_tab[] = {
  {"INVALID_HANDLE_VALUE",  (Val_Sized_Unt)INVALID_HANDLE_VALUE}
};

/* FILE_ table */
static name_val_t file_tab[] = {
  {"BEGIN",  FILE_BEGIN},
  {"CURRENT",  FILE_CURRENT},
  {"END",  FILE_END},
};

/* FILE_ATTRIBUTE_ table */
static name_val_t file_attribute_tab[] = {
  {"ARCHIVE",  FILE_ATTRIBUTE_ARCHIVE},
/** future win32 use
  {"ATOMIC_WRITE", FILE_ATTRIBUTE_ATOMIC_WRITE},
**/
  {"DIRECTORY", FILE_ATTRIBUTE_DIRECTORY},
  {"HIDDEN", FILE_ATTRIBUTE_HIDDEN},
  {"NORMAL", FILE_ATTRIBUTE_NORMAL},
  {"READONLY", FILE_ATTRIBUTE_READONLY},
  {"SYSTEM", FILE_ATTRIBUTE_SYSTEM},
  {"TEMPORARY", FILE_ATTRIBUTE_TEMPORARY},
/** future win32 use
  {"XACTION_WRITE", FILE_ATTRIBUTE_XACTION_WRITE},
**/
};

/* FILE_FLAG__ table */
static name_val_t file_flag_tab[] = {
  {"BACKUP_SEMANTICS", FILE_FLAG_BACKUP_SEMANTICS},
  {"DELETE_ON_CLOSE", FILE_FLAG_DELETE_ON_CLOSE},
  {"NO_BUFFERING", FILE_FLAG_NO_BUFFERING},
  {"OVERLAPPED", FILE_FLAG_OVERLAPPED},
  {"POSIX_SEMANTICS", FILE_FLAG_POSIX_SEMANTICS},
  {"RANDOM_ACCESS", FILE_FLAG_RANDOM_ACCESS},
  {"SEQUENTIAL_SCAN", FILE_FLAG_SEQUENTIAL_SCAN},
  {"WRITE_THROUGH", FILE_FLAG_WRITE_THROUGH},
};

/* FILE_MODE__ table */
static name_val_t file_mode_tab[] = {
  {"CREATE_ALWAYS", CREATE_ALWAYS},
  {"CREATE_NEW", CREATE_NEW},
  {"OPEN_ALWAYS", OPEN_ALWAYS},
  {"OPEN_EXISTING", OPEN_EXISTING},
  {"TRUNCATE_EXISTING", TRUNCATE_EXISTING},
};

/* FILE_SHARE_ table */
static name_val_t file_share_tab[] = {
  {"READ",  FILE_SHARE_READ},
  {"WRITE",  FILE_SHARE_WRITE},
};

/* GENERIC__ table */
static name_val_t generic_tab[] = {
  {"READ",  GENERIC_READ},
  {"WRITE",  GENERIC_WRITE},
};

/* STD_HANDLE table */
static name_val_t std_handle_tab[] = {
  {"ERROR",  STD_ERROR_HANDLE},
  {"INPUT",  STD_INPUT_HANDLE},
  {"OUTPUT",  STD_OUTPUT_HANDLE},
};

/* every constant table must have an entry in the descriptor table */
static tab_desc_t table[] = {
  {file_tab, TAB_SZ(file_tab)},
  {file_attribute_tab, TAB_SZ(file_attribute_tab)},
  {file_flag_tab, TAB_SZ(file_flag_tab)},
  {file_mode_tab, TAB_SZ(file_mode_tab)},
  {file_share_tab, TAB_SZ(file_share_tab)},
  {general_tab, TAB_SZ(general_tab)},
  {generic_tab, TAB_SZ(generic_tab)},
  {std_handle_tab, TAB_SZ(std_handle_tab)},
};

/* Constant ilks */
static name_val_t ilk[] = {
  {"FILE", 0},
  {"FILE_ATTRIBUTE", 1},
  {"FILE_FLAG", 2},
  {"FILE_MODE", 3},
  {"FILE_SHARE", 4},
  {"GENERAL", 5},
  {"GENERIC", 6},
  {"STD_HANDLE",7},
};
#define N_ILKES TAB_SZ(ilk)


/* _lib7_win32_get_const: (String * String) -> word
 * lookup (ilk,constant) pair
 */
Val _lib7_win32_get_const(Task *task, Val arg)
{
  char *s1 = HEAP_STRING_AS_C_STRING(GET_TUPLE_SLOT_AS_VAL(arg,0));
  char *s2 = HEAP_STRING_AS_C_STRING(GET_TUPLE_SLOT_AS_VAL(arg,1));
  name_val_t *ptab, *res;
  int index;
  Val v;

  ptab = nv_lookup(s1, ilk, N_ILKES);
  if (ptab) {
    index = ptab->data;
    ASSERT(index < TAB_SZ(table));
    if (res = nv_lookup(s2, table[index].ptab, table[index].size)) {
      return  make_one_word_unt(task, res->data );
    }
    return RAISE_ERROR__MAY_HEAPCLEAN(task,"win32_cconst: unknown constant", NULL);
  }
  return RAISE_ERROR__MAY_HEAPCLEAN(task,"win32_cconst: unknown constant ilk", NULL);
}

/* end of win32-constants.c */


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
 * released under Gnu Public Licence version 3.
 */

