echo off
:: make file for DEVCPP TDM-GCC C++ compiler
if not defined DEVCPP (
  set DEVCPP=%1
  set PATH=%PATH%;%1\TDM-GCC-64\bin
)
if not defined DEVCPP (
  echo Run batch file with path to DEVCPP TDM-GCC C++ compiler installed directory
  goto :END
)
if exist .\bin rmdir /Q/S bin
if not exist .\bin mkdir bin
mingw32-make.exe
cd .\bin
if exist osndxfio_tb.exe osndxfio_tb.exe
cd ..
:END
