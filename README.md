# derar

Virtual filesystem for reading RAR archives through FUSE. Allows mounting an archive to a directory and reading its contents without unpacking.
Supports both single file and split archives (partXXX/rXX) but no compression - only Store archives work.

## Building

Requires FUSE obviously. Mac users will need to install [FUSE for OSX](https://osxfuse.github.io/).
```bash
$ cmake .
$ make
```

## Installing

```bash
$ sudo cp derar /usr/local/bin
```
