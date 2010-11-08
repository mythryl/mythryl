/* cfun-list.h
 *
 *
 * win32 C functions for IO
 */

#ifndef CLIB_NAME
#define CLIB_NAME	"WIN32-FILESYS"
#define CLIB_VERSION	"0.1"
#define CLIB_DATE	"October 15, 1996"
#endif

CFUNC("find_first_file",\
      _lib7_win32_FS_find_first_file,\
      "String->(word32*String option)")
CFUNC("find_next_file",\
      _lib7_win32_FS_find_next_file,\
     "word32->(String option)")
CFUNC("find_close",\
      _lib7_win32_FS_find_close,\
     "word32->Bool")
CFUNC("change_directory",\
      _lib7_win32_FS_set_current_directory,\
     "String->Bool")
CFUNC("get_current_directory",\
      _lib7_win32_FS_get_current_directory,\
     "Void->String")
CFUNC("create_directory",\
      _lib7_win32_FS_create_directory,\
     "String->Bool")
CFUNC("remove_directory",\
      _lib7_win32_FS_remove_directory,\
     "String->Bool")
CFUNC("get_file_attributes",\
      _lib7_win32_FS_get_file_attributes,\
     "String->(word32 option)")
CFUNC("get_file_attributes_by_handle",\
      _lib7_win32_FS_get_file_attributes_by_handle,\
     "word32->(word32 option)")
CFUNC("get_full_path_name",\
      _lib7_win32_FS_get_full_path_name,\
     "String->String")
CFUNC("get_file_size",\
      _lib7_win32_FS_get_file_size,\
     "word32->(word32*word32)")
CFUNC("get_low_file_size",\
      _lib7_win32_FS_get_low_file_size,\
     "word32->(word32 option)")
CFUNC("get_low_file_size_by_name",\
      _lib7_win32_FS_get_low_file_size_by_name,\
     "String->(word32 option)")
CFUNC("get_file_time",\
      _lib7_win32_FS_get_file_time,\
     "String->(int*int*int*int*int*int*int*int) option")
CFUNC("set_file_time",\
      _lib7_win32_FS_set_file_time,\
     "(String*(int*int*int*int*int*int*int*int))->Bool")
CFUNC("delete_file",\
      _lib7_win32_FS_delete_file,\
     "String->Bool")
CFUNC("move_file",\
      _lib7_win32_FS_move_file,\
     "(String * String)->Bool")
CFUNC("get_temp_file_name",\
      _lib7_win32_FS_get_temp_file_name,\
     "Void->Bool")






/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
