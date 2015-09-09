cmake_minimum_required(VERSION 2.8.11)
project(DERAR)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(FUSE REQUIRED)
include_directories(${FUSE_INCLUDE_DIR})
add_definitions(${FUSE_DEFINITIONS})

set(LIBS ${FUSE_LIBRARIES})

add_executable(derar
  derar.c
  filename.c
  filetree.c
  fuse.c
  io.c
)

target_link_libraries(derar ${LIBS})
