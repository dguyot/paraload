#
#      you need to have GNU make to use this Makefile!!!
#

VERSIONDEF := -D'VERSION=1.2'
GCCDEF := -D'GCC_VERSION="$(shell gcc --version | head -n 1)"'
USERDEF := -D'USER_LOGIN="$(shell whoami)"'

#compilers to use:
CCC = gcc
CPP = g++
#the flags for compilation:
CFLAGS = -Wpadded -Wall -O3 $(VERSIONDEF) $(GCCDEF) $(USERDEF)
CPPFLAGS = -Wpadded -Wall -O3 -std=c++11 $(VERSIONDEF) $(GCCDEF) $(USERDEF)
#the flags for linking:
LDFLAGS = -Wpadded -Wall -O3 -std=c++11
#the name of the executable you want to build:
EXEC = paraload
#the name of the c sources for compiling:
C_SRC = utils.c networks.c md5.c
#the name of the c++ sources for compiling:
CPP_SRC = Cline.cpp Comm_C.cpp Comm_S.cpp Conf.cpp Fetch.cpp Index.cpp Monitor.cpp Tool.cpp
#the name of the c objects for building final executable:
C_OBJ = $(C_SRC:.c=.o)
#the name of the cpp objects for building final executable:
CPP_OBJ = $(CPP_SRC:.cpp=.o)


all: $(EXEC)

$(EXEC): $(C_OBJ) $(CPP_OBJ)
	$(CPP) -o $@ $^ paraload.cpp $(LDFLAGS)

%.o: %.c
	$(CCC) -o $@ -c $^ $(CFLAGS)

%.o: %.cpp
	$(CPP) -o $@ -c $< $(CPPFLAGS)

clean:
	rm *.o
	
cleanall:
	rm *.o $(EXEC)
