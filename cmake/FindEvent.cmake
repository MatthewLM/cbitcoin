# Find Libevent
# http://monkey.org/~provos/libevent/
#
# Once done, this will define:
#
#  Event_FOUND - system has Event
#  Event_INCLUDE_DIRS - the Event include directories
#  Event_LIBRARIES - link these to use Event
#

if (EVENT_INCLUDE_DIR AND EVENT_LIBRARY)
    # Already in cache, be silent
    set(EVENT_FIND_QUIETLY TRUE)
endif (EVENT_INCLUDE_DIR AND EVENT_LIBRARY)

find_path(EVENT_INCLUDE_DIR event.h
    PATHS /usr/include
    PATH_SUFFIXES event
)

find_library(EVENT_LIBRARY
    NAMES event
    PATHS /usr/lib /usr/local/lib
)

find_library(EVENT_CORE_LIBRARY
    NAMES event_core
    PATHS /usr/lib /usr/local/lib
)

find_library(EVENT_EXTRA_LIBRARY
    NAMES event_extra
    PATHS /usr/lib /usr/local/lib
)

find_library(EVENT_OPENSSL_LIBRARY
    NAMES event_openssl
    PATHS /usr/lib /usr/local/lib
)

find_library(EVENT_PTHREADS_LIBRARY
    NAMES event_pthreads
    PATHS /usr/lib /usr/local/lib
)

set(EVENT_LIBRARIES
    ${EVENT_LIBRARY}
    ${EVENT_CORE_LIBRARY}
    ${EVENT_EXTRA_LIBRARY}
    ${EVENT_OPENSSL_LIBRARY}
    ${EVENT_PTHREADS_LIBRARY}
)

add_definitions(-DLIBNET_LIL_ENDIAN)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Event
    DEFAULT_MSG
    EVENT_INCLUDE_DIR
    EVENT_LIBRARIES
)

mark_as_advanced(EVENT_INCLUDE_DIR EVENT_LIBRARY)
