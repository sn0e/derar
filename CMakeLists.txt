cmake_minimum_required (VERSION 2.8.11)
project (DERAR)
subdirs(src)

add_executable(derar
  derar.c
  filename.c
  filetree.c
  fuse.c
  io.c
)

target_link_libraries(derar fuse)
