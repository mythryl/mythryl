@echo off
set root=%1%
set main=%2%
set heap=%3%

set smlfile=XYZ_XXX_smlfile.sml
set cmfile=XYZ_XXX_cmfile.lib
set listfile=XYZ_XXX_COMPILED_FILES_TO_LOAD
set linkargsfile=XYZ_XXX_LINKARGS

set rare=XYZ_XXX_0123

echo structure %rare% = pkg my _ = lib7::spawn_to_disk ("%heap%", %main%) end >"%smlfile%"

echo Group structure %rare% is $basis.lib/basis.lib "%root%" %smlfile% >%cmfile%

%COMSPEC% /C "%LIB7_HOME%\bin\perld7.bat --build %root% %cmfile% %heap% %listfile% %linkargsfile%"
IF ERRORLEVEL 1 GOTO ERR
IF NOT EXIST %linkargsfile% GOTO END
"%LIB7_HOME%\bin\mythryl-runtime-ia32.exe" --runtime-compiledfiles-to-load=%listfile%
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
del tmp7\intel32-win32\%smlfile%
