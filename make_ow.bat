echo off
:: make file for Open Watcom C++ compiler
if not defined WATCOM (
  set WATCOM=%1
  set PATH=%PATH%;%1\binnt;%1\binw
)
if not defined WATCOM (
  echo Run batch file with path to Open Watcom C++ compiler installed directory
  goto :END
)
if exist .\bin rmdir /Q/S bin
if not exist .\bin mkdir bin
cd .\bin
if "%1"=="DEBUG" (
  owcc.exe -mconsole -mtune=686 -g3 -gd -I=%WATCOM%\h -I=..\ ..\osfio_tb.cpp ..\osfio.cpp
) else (
  owcc.exe -mconsole -mtune=686 -I=%WATCOM%\h -I=..\ ..\osfio_tb.cpp ..\osfio.cpp
)
if exist osfio_tb.exe osfio_tb.exe
if "%1"=="DEBUG" (
  owcc.exe -mconsole -mtune=686 -g3 -gd -I=%WATCOM%\h -I=..\ ..\osndxfio_tb.cpp ..\osfio.cpp ..\osndxfio.cpp
) else (
  owcc.exe -mconsole -mtune=686 -I=%WATCOM%\h -I=..\ ..\osndxfio_tb.cpp ..\osfio.cpp ..\osndxfio.cpp
)
if exist osndxfio_tb.exe osndxfio_tb.exe
cd ..
:END
