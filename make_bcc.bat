echo off
:: make file for Borland C++ compiler
if not defined BCC55 (
  set BCC55=%1
  set ADD_BCC55_PATH=Y
)
if not defined BCC55 (
  echo run batch file with path to Borland C++ compiler installed directory
  goto :END
)
if %ADD_BCC55_PATH%==Y set PATH=%PATH%;%BCC55%\bin
set ADD_BCC55_PATH=N
if not exist .\bin mkdir bin
cd .\bin
del /F /Q *.*
bcc32.exe -6 -p -I%BCC55%\include -L%BCC55%\Lib -I..\ -tWC ..\osfio_tb.cpp ..\osfio.cpp
if exist osfio_tb.exe osfio_tb.exe
bcc32.exe -6 -p -I%BCC55%\include -L%BCC55%\Lib -I..\ -tWC ..\osndxfio_tb.cpp ..\osfio.cpp ..\osndxfio.cpp
if exist osndxfio_tb.exe osndxfio_tb.exe
cd ..
:END
