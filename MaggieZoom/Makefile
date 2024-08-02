TARGET=MaggieZoom

CXX=m68k-amigaos-g++
CC=m68k-amigaos-gcc

SOURCES=main.c \
		owl.c

CFLAGS=-std=c99 -Ofast -fomit-frame-pointer -m68080 -m68881 -mhard-float -Wall

LFLAGS=-L. -noixemul
LDLIBS=-lamiga -lm

OBJS=$(SOURCES:.c=.o)

all: $(SOURCES) $(TARGET) Makefile

$(TARGET): $(OBJS) Makefile
	$(CC) $(LFLAGS) $(OBJS) $(LDLIBS) -o $(TARGET)

purge: clean
	rm -f $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
