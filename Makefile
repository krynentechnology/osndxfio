CPP      = g++.exe -D__DEBUG__
CC       = gcc.exe -D__DEBUG__
WINDRES  = windres.exe
OBJ      = bin/osfio.o bin/osndxfio.o bin/osndxfio_tb.o
LINKOBJ  = bin/osfio.o bin/osndxfio.o bin/osndxfio_tb.o
LIBS     = -L"%DEVCPP%/TDM-GCC-64/x86_64-w64-mingw32/lib32" -static-libgcc -m32 -g3
INCS     = -I"%DEVCPP%/TDM-GCC-64/include" -I"%DEVCPP%/TDM-GCC-64/x86_64-w64-mingw32/include" -I"%DEVCPP%/TDM-GCC-64/lib/gcc/x86_64-w64-mingw32/9.2.0/include" -I"."
CXXINCS  = -I"%DEVCPP%/TDM-GCC-64/include" -I"%DEVCPP%/TDM-GCC-64/x86_64-w64-mingw32/include" -I"%DEVCPP%/TDM-GCC-64/lib/gcc/x86_64-w64-mingw32/9.2.0/include" -I"%DEVCPP%/TDM-GCC-64/lib/gcc/x86_64-w64-mingw32/9.2.0/include/c++" -I"."
BIN      = bin/osndxfio_tb.exe
CXXFLAGS = $(CXXINCS) -m32 -g3
CFLAGS   = $(INCS) -m32 -g3
DEL      = C:\Development\devcpp\devcpp.exe INTERNAL_DEL

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

clean: clean-custom
	${DEL} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CPP) $(LINKOBJ) -o $(BIN) $(LIBS)

bin/osfio.o: osfio.cpp
	$(CPP) -c osfio.cpp -o bin/osfio.o $(CXXFLAGS)

bin/osndxfio.o: osndxfio.cpp
	$(CPP) -c osndxfio.cpp -o bin/osndxfio.o $(CXXFLAGS)

bin/osndxfio_tb.o: osndxfio_tb.cpp
	$(CPP) -c osndxfio_tb.cpp -o bin/osndxfio_tb.o $(CXXFLAGS)
