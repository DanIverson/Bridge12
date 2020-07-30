# This Makefile builds spinapi as a static or dynamic library on Linux.

# Name of the library
SPINAPI = spinapi2
ARCH    = x64

FTD2XXDIR = ./FTD2XX/Linux
SRC_FILES = stubs.c
CC        = gcc  
CFLAGS    = -std=gnu99 -fPIC -c -Wall
DEFINES   = -D__LINUX__
INC       = -I/user/include
LIB       = 
SRC       = $(SRC_FILES)
COMPILE   = $(CC) $(SRC) $(CFLAGS) $(DEFINES) $(INC) $(LIB)



default: dynamic



compile:

ifeq ($(ARCH), $(filter $(ARCH), x86 x32 i386))
	#Compile SpinAPI into 32-bit object files
	$(COMPILE) -m32 $<
	
	#Extract objects from 32-bit FTD2XX library into temp folder
	cd         .temp &&\
	ar    -x   ../$(FTD2XXDIR)/x86/libftd2xx.a
else
	#Compile SpinAPI into 64-bit object files
	$(COMPILE) -m64 $<
				
	#Extract objects from 64-bit FTD2XX library into temp folder
endif

	#Store built objects into temp folder



dynamic: compile
	#Combine all objects in temp into a shared object,
	# this will be the dynamic SpinAPI library 	

ifeq ($(ARCH), $(filter $(ARCH), x86 x32 i386))
	#Build 32-bit library if specified
	gcc    -shared -m32 -o   libspinapi.so .temp/*.o
else
	#Build 64-bit library by default
	gcc    -shared -m64 -o   libspinapi2.so stubs.o
endif



install:
	-mkdir -p   /usr/include/spinapi
	-cp    -f   ../inc/*.h             /usr/include/spinapi
	-cp    -f   ../lib/libspinapi.so   /usr/lib
	-cp    -f   ../lib/libspinapi.a    /usr/lib



uninstall:
	-rm        /usr/include/spinapi.h
	-rm        /usr/lib/libspinapi.so
	-rm        /usr/lib/libspinapi.a



clean:
	-rm        *.o
	-rm        libspinapi.so
	-rm        libspinapi.a
	-rm   -rf  ./.temp
