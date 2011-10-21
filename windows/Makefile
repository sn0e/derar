CC        = i486-mingw32-gcc
RES       = i486-mingw32-windres
CFLAGS    = -O3 -W -Wall -D_FILE_OFFSET_BITS=64 -D_LARGE_FILE -I.
LDFLAGS   = -L. -mwindows
RM        = rm -f

target    = derar.exe
objects   = dokan.o ui.o uires.o strtok_r.o ../derar/derar.o ../derar/filename.o ../derar/filetree.o ../derar/io.o
libraries = -ldokan

all: $(target) Makefile

$(target): $(objects)
	$(CC) -o $@ $(objects) $(LDFLAGS) $(libraries)

uires.o: uires.rc
	$(RES) uires.rc -o uires.o 

clean:
	$(RM) $(objects) $(target)

# Dependencies
dokan.c: Makefile derar.h
ui.c:    Makefile derar.h

