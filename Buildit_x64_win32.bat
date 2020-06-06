IF EXIST builds\bin_x64_win32 RMDIR /S /Q builds\bin_x64_win32
IF EXIST builds\objp_x64_win32 RMDIR /S /Q builds\objp_x64_win32
IF EXIST builds\objl_x64_win32 RMDIR /S /Q builds\objl_x64_win32
IF EXIST builds\lib_x64_win32 RMDIR /S /Q builds\lib_x64_win32
IF EXIST builds\inc_x64_win32 RMDIR /S /Q builds\inc_x64_win32

IF NOT EXIST builds MKDIR builds
MKDIR builds\bin_x64_win32
MKDIR builds\objp_x64_win32
MKDIR builds\objl_x64_win32
MKDIR builds\lib_x64_win32
MKDIR builds\inc_x64_win32

COPY includes_com\*.h builds\lib_x64_win32
COPY multilang\libc\*.h builds\lib_x64_win32
FOR %%v in (source_com_ml\*.gml) DO m4 multilang\preproc\c_head.m4 %%v > builds\lib_x64_win32\%%~nv.h
FOR %%v in (source_com_ml\*.gml) DO m4 multilang\preproc\c_body.m4 %%v > builds\objl_x64_win32\%%~nv.c
FOR %%v in (source_com\*.cpp) DO g++ -mthreads %%v -O2 -Wall -Ibuilds\lib_x64_win32 -c -o builds\objl_x64_win32\%%~nv.o
FOR %%v in (source_com_any_win32\*.cpp) DO g++ -mthreads %%v -O2 -Wall -Ibuilds\lib_x64_win32 -c -o builds\objl_x64_win32\%%~nv.o
FOR %%v in (source_com_x64_any\*.cpp) DO g++ -mthreads %%v -O2 -Wall -Ibuilds\lib_x64_win32 -c -o builds\objl_x64_win32\%%~nv.o
FOR %%v in (builds\objl_x64_win32\*.c) DO gcc -mthreads %%v -O2 -w -Ibuilds\lib_x64_win32 -c -o builds\objl_x64_win32\%%~nv.o
FOR %%v in (multilang\libc\*.c) DO gcc -mthreads %%v -O2 -Wall -Ibuilds\lib_x64_win32 -c -o builds\objl_x64_win32\%%~nv.o
ar rvs builds\lib_x64_win32\libwhodun.a builds\objl_x64_win32\*.o

COPY includes_pro\*.h builds\inc_x64_win32
FOR %%v in (source_pro\*.cpp) DO g++ -mthreads %%v -O2 -Wall -Ibuilds\lib_x64_win32 -Ibuilds\inc_x64_win32 -c -o builds\objp_x64_win32\%%~nv.o
g++ -mthreads -o builds\bin_x64_win32\Protengine.exe builds\objp_x64_win32\*.o -Lbuilds\lib_x64_win32 -lwhodun -lz

FOR %%v in (source_pro_uti\*.cpp) DO g++ -mthreads -O2 -Wall -Ibuilds\lib_x64_win32 -o builds\bin_x64_win32\%%~nv.exe %%v -Lbuilds\lib_x64_win32 -lwhodun -lz

COPY scripts_pro\*.py builds\bin_x64_win32
COPY scripts_pro\*.m4 builds\bin_x64_win32





