TARGET=TrackRacer

AS=vasmm68k_mot
CXX=m68k-amigaos-g++
CC=m68k-amigaos-gcc

SOURCES=main.c \
		ObjLoader.c \
		dynarray.c \
		trackloader.c \
		trackcurves.c \
		trackdraw.c \
		trackupdate.c \
		trackcollision.c \
		trackplayer.c \
		apolloImage.c \
		upscale.s

ASFLAGS=-m68080 -m68881 -quiet -Fhunk

# CFLAGS=-std=c11 -O0 -m68080 -mhard-float -Wall -Wno-unused-function -DAMIGA=1
CFLAGS=-std=c11 -Ofast -fomit-frame-pointer -m68080 -mhard-float -Wall -Wno-unused-function  -DAMIGA=1

LFLAGS=-L. -noixemul
LDLIBS=-lamiga -lm

OBJSC=$(SOURCES:.c=.o)
OBJS=$(OBJSC:.s=.o)

all: $(SOURCES) $(TARGET) Makefile

$(TARGET): $(OBJS) Makefile
	$(CC) $(LFLAGS) $(OBJS) $(LDLIBS) -o $(TARGET)

purge: clean
	rm -f $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
