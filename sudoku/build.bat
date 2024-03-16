@echo off

REM del *.pdb

set COMMON_FLAGS=/O2 /W3 /Z7 /DUNICODE_X /EHsc /wd4996 /nologo /MT
set BUILD_FLAGS=%COMMON_FLAGS%  /link user32.lib
cl main.cpp /Femain.exe %BUILD_FLAGS% 
REM cl main_backtrack.cpp /Femain_backtrack.exe %BUILD_FLAGS% 

del *.ilk
del *.obj
