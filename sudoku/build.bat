@echo off

REM del *.pdb

set COMMON_FLAGS=/Od /W2 /Z7 /EHsc /wd4996 /nologo /MD
set BUILD_FLAGS=%COMMON_FLAGS%  /link
cl main.cpp /Femain.exe %BUILD_FLAGS% 
REM cl gemini.cpp /Fegemini.exe %BUILD_FLAGS% 
REM cl chatgpt.cpp /Fechatgpt.exe %BUILD_FLAGS% 
cl bitpacking.cpp /Febitpacking.exe %BUILD_FLAGS% 
cl gui.cpp /Fegui.exe /Iraylib-5.0_win64_msvc16/include %BUILD_FLAGS% raylib-5.0_win64_msvc16\lib\raylib.lib Shell32.lib user32.lib gdi32.lib msvcrt.lib winmm.lib /NODEFAULTLIB:libcmt
REM cl gen.cpp /Fegen.exe %BUILD_FLAGS% 

del *.ilk
del *.obj
