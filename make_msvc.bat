echo off
:: make file for Microsoft Visual Studio C++ Compiler
if not defined VSINSTALLDIR (
  echo Run batch file from Developer Command Prompt for VS
  echo MSVC build tools and Windows SDK should be installed
  goto :END
)
if exist .\bin rmdir /Q/S bin
if not exist .\bin mkdir bin
cd .\bin
cl.exe /I..\ ..\osfio_tb.cpp ..\osfio.cpp
if exist osfio_tb.exe osfio_tb.exe
cl.exe /I..\ ..\osndxfio_tb.cpp ..\osfio.cpp ..\osndxfio.cpp
if exist osndxfio_tb.exe osndxfio_tb.exe
cd ..
:END
