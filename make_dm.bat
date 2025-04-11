echo off
:: make file for Digital Mars C++ compiler
if not defined DM857 (
  set DM857=%1
  set PATH=%PATH%;%1\bin
)
if not defined DM857 (
  echo Run batch file with path to Digital Mars C++ compiler installed directory
  goto :END
) else (
  set LIB=%DM857%\lib
)
if exist .\bin rmdir /Q/S bin
if not exist .\bin mkdir bin
cd .\bin
dmc -6 -I%DM857%\include -I..\ ..\osfio_tb.cpp ..\osfio.cpp 
if exist osfio_tb.exe osfio_tb.exe
dmc -6 -I%DM857%\include -I..\ ..\osndxfio_tb.cpp ..\osfio.cpp ..\osndxfio.cpp
if exist osndxfio_tb.exe osndxfio_tb.exe
cd ..
:END
