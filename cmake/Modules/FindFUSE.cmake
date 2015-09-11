# Find the FUSE includes and library
#
#  FUSE_INCLUDE_DIR - where to find fuse.h, etc.
#  FUSE_LIBRARIES   - List of libraries when using FUSE.
#  FUSE_FOUND       - True if FUSE lib is found.

# check if already in cache, be silent
IF (FUSE_INCLUDE_DIR)
        SET (FUSE_FIND_QUIETLY TRUE)
ENDIF (FUSE_INCLUDE_DIR)

# find includes
FIND_PATH (FUSE_INCLUDE_DIR fuse.h
        /usr/local/include/osxfuse
        /usr/local/include
        /usr/include
)

# find lib
if (APPLE)
    SET(FUSE_NAMES libosxfuse.dylib fuse)
else (APPLE)
    SET(FUSE_NAMES fuse)
endif (APPLE)
FIND_LIBRARY(FUSE_LIBRARIES
        NAMES ${FUSE_NAMES}
        PATHS /lib64 /lib /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib
)

include ("FindPackageHandleStandardArgs")
find_package_handle_standard_args ("FUSE" DEFAULT_MSG
    FUSE_INCLUDE_DIR FUSE_LIBRARIES)

if (FUSE_FOUND)
    if (CMAKE_SYSTEM_PROCESSOR MATCHES ia64)
        set(FUSE_DEFINITIONS "-D_REENTRANT -D_FILE_OFFSET_BITS=64")
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES amd64)
        set(FUSE_DEFINITIONS "-D_REENTRANT -D_FILE_OFFSET_BITS=64")
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES x86_64)
        set(FUSE_DEFINITIONS "-D_REENTRANT -D_FILE_OFFSET_BITS=64")
    else (CMAKE_SYSTEM_PROCESSOR MATCHES ia64)
        set(FUSE_DEFINITIONS "-D_REENTRANT")
    endif (CMAKE_SYSTEM_PROCESSOR MATCHES ia64)

endif (FUSE_FOUND)

mark_as_advanced (FUSE_INCLUDE_DIR FUSE_LIBRARIES)
