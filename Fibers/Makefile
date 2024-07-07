TARGET=FiberTest

AS=vasmm68k_mot
CXX=m68k-amigaos-g++
CC=m68k-amigaos-gcc

SOURCES=main.cpp \
		fibers.cpp \
		fiberScheduler.s

ASFLAGS=-m68080 -m68882 -quiet -Fhunk

CFLAGS=-std=c11 -O3 -fomit-frame-pointer -m68030 -m68881 -mhard-float -Wall
CXXFLAGS=-std=c++98 -O3 -fomit-frame-pointer -m68030 -m68881 -mhard-float -Wall

LFLAGS=-L. -noixemul
LDLIBS=-lamiga -lm -lstdc++

OBJSCXX=$(SOURCES:.cpp=.o)
OBJSC=$(OBJSCXX:.c=.o)
OBJS=$(OBJSC:.s=.o)

all: $(SOURCES) $(TARGET) Makefile

$(TARGET): $(OBJS) Makefile
	$(CC) $(LFLAGS) $(OBJS) $(LDLIBS) -o $(TARGET)

purge: clean
	rm -f $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

