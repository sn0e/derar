CC        = gcc
CFLAGS    = -W -Wall -O3 -D_FILE_OFFSET_BITS=64
LDFLAGS   = 
RM        = rm -f
PREFIX    = /usr/local

target    = derar
objects   = fuse.o derar.o filename.o filetree.o io.o
libraries = -lfuse

all: $(target)

$(target): $(objects)
	$(CC) -o $@ $(objects) $(LDFLAGS) $(libraries)

clean:
	$(RM) $(objects) $(target)

install: $(target)
	install -m755 $(target) $(PREFIX)/bin

# Dependencies
derar_types.h: Makefile derar.h
derar.c:       Makefile derar.h filename.h filetree.h
io.c:          Makefile derar.h derar_types.h
filename.c:    Makefile
filetree.c:    Makefile derar.h derar_types.h
fuse.c:        Makefile derar.h

