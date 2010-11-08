@echo off
set root=%1%
set main=%2%
set heap=%3%

set smlfile=XYZ_XXX_smlfile.sml
set cmfile=XYZ_XXX_cmfile.make6
set listfile=XYZ_XXX_OH7_FILES_TO_LOAD
set linkargsfile=XYZ_XXX_LINKARGS

set rare=XYZ_XXX_0123

echo structure %rare% = pkg my _ = lib7::spawn_to_disk ("%heap%", %main%) end >"%smlfile%"

echo Group structure %rare% is $basis.make6/basis.make6 "%root%" %smlfile% >%cmfile%

%COMSPEC% /C "%LIB7_HOME%\bin\perld7.bat --build %root% %cmfile% %heap% %listfile% %linkargsfile%"
IF ERRORLEVEL 1 GOTO ERR
IF NOT EXIST %linkargsfile% GOTO END
"%LIB7_HOME%\bin\runtime7.exe" --runtime-o7-files-to-load=%listfile%
del %linkargsfile%
GOTO END

:ERR
echo Compilation failed with error.

:END
REM more cleaning up
del %smlfile%
del %cmfile%
del %listfile%
del tmp7\GUID\%smlfile%
del tmp7\SKEL\%smlfile%
del tmp7\x86-win32\%smlfile%
