CC        = gcc
CFLAGS    = -W -Wall -O3 -D_FILE_OFFSET_BITS=64
LDFLAGS   = 
RM        = rm -f
PREFIX    = /usr/local

derar_objects     = cmdline.o derar.o filename.o filetree.o io.o
derar_libraries   =

derarfs_objects   = fuse.o derar.o filename.o filetree.o io.o
derarfs_libraries = -lfuse

all: derar derarfs

derar: $(derar_objects)
	$(CC) -o $@ $(derar_objects) $(LDFLAGS) $(derar_libraries)

derarfs: $(derarfs_objects)
	$(CC) -o $@ $(derarfs_objects) $(LDFLAGS) $(derarfs_libraries)

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
cmdline.c:     Makefile derar.h
