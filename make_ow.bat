echo off
:: make file for Open Watcom C++ compiler
if not defined WATCOM (
  set WATCOM=%1
  set ADD_WATCOM_PATH=Y
)
if not defined WATCOM (
  echo run batch file with path to Open Watcom C++ compiler installed directory
  goto :END
)
if %ADD_WATCOM_PATH%==Y set PATH=%PATH%;%WATCOM%\binnt;%WATCOM%\binw
set ADD_WATCOM_PATH=N
if not exist .\bin mkdir bin
cd .\bin
del /F /Q *.*
owcc.exe -mconsole -mtune=686 -I=%WATCOM%\h -I=..\ ..\osfio_tb.cpp ..\osfio.cpp
if exist osfio_tb.exe osfio_tb.exe
owcc.exe -mconsole -mtune=686 -I=%WATCOM%\h -I=..\ ..\osndxfio_tb.cpp ..\osfio.cpp ..\osndxfio.cpp
if exist osndxfio_tb.exe osndxfio_tb.exe
cd ..
:END
