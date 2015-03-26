                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([cbitcoin], [0.01], [cbitcoin@thelibertyportal.com])
AC_CONFIG_SRCDIR([lib/CBAddress.c])
AM_MAINTAINER_MODE



ACX_PTHREAD

# set flags
CONFIGFLAGS=" -Wall -Wextra -Wno-uninitialized -Wno-pointer-to-int-cast -pedantic -std=gnu99 -D_GNU_SOURCE  "
CONFIGFLAGS+=" $CFLAGS "
CONFIGLFLAGS=" $LFLAGS "

LIBS=" $PTHREAD_LIBS $LIBS "
CFLAGS=" $CFLAGS $PTHREAD_CFLAGS "

LT_PREREQ([2.2])
LT_INIT()




# Determine if no error correction wanted.

AC_ARG_ENABLE([ec],
    AS_HELP_STRING([--disable-ec], [Disable Error Correction]))

AS_IF([test "x$enable_ec" = "xno"],
        [NOEC="no-"],
        [NOEC=""]
)
# Determine if debug build is enabled

AC_ARG_ENABLE([debug],
    AS_HELP_STRING([--enable-debug], [Enable debug symbols]))

AS_IF([test "x$enable_debug" = "xyes"],
        [CONFIGFLAGS+=" -g -DCBDEBUG "],
        [CONFIGFLAGS+=" -O3 -DNDEBUG "]
)

# Determine if stack protection is disabled

AC_ARG_ENABLE([stack-protector],
    AS_HELP_STRING([--disable-stack-protector], [Disable the use of -fstack-protector-strong]))

AS_IF([test "x$enable_stack_protector" = "xno"],
        [stackProtector=0],
        [stackProtector=1]
)

# Determine if Werror should be disabled

AC_ARG_ENABLE([werror],
    AS_HELP_STRING([--disable-werror], [Disable Error on Warnings]))

AS_IF([test "x$enable_werror" = "xno"],
        [],
        [CONFIGFLAGS+=" -Werror "]
)

# Check features

save_cflags=" $CFLAGS "
save_lflags=" $LFLAGS "

CFLAGS+=" $CONFIGFLAGS "
LFLAGS+=" $CONFIGLFLAGS "
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],[],[])
AC_LINK_IFELSE([AC_LANG_PROGRAM([])],[],[])

# If using stack protection, try -fstack-protector-strong, if not try to fallback to -fstack-protector-all

AC_MSG_CHECKING([that stack protection works])
AS_IF(
        [test $stackProtector = 1],
        [CFLAGS+=" -fstack-protector-strong"]
        [AC_COMPILE_IFELSE(
                [AC_LANG_PROGRAM([])],
                [CONFIGFLAGS+=" -fstack-protector-strong"],
                [CFLAGS="$CONFIGFLAGS -fstack-protector-all"]
                [AC_COMPILE_IFELSE(
                        [AC_LANG_PROGRAM([])],
                        [CONFIGFLAGS+=" -fstack-protector-all"],
                        [AC_MSG_RESULT([no])]
                        [AC_MSG_ERROR([Cannot configure with stack protection])]
                )]
        )],
        []
)
AC_MSG_RESULT([yes])

CFLAGS=" $save_cflags "
LFLAGS=" $save_lflags "

# Search for OpenSSL and LibEvent.
AC_CHECK_LIB(crypto, MD5_Init, CRYPTO_LIBS=-lcrypto,
  [AC_MSG_ERROR([Missing required libcrypto])])
AC_CHECK_LIB(event_core, event_base_new, EVENT_LIBS=" -levent_core -levent -levent_extra -levent_openssl -levent_pthreads ",
  [AC_MSG_ERROR([Missing required libevent])])
AC_CHECK_LIB(rt, mq_send, REALTIME_LIBS=" -lrt ",
  [AC_MSG_ERROR([Missing required librtai-dev])])

AC_SUBST(CRYPTO_LIBS)
AC_SUBST(EVENT_LIBS)
AC_SUBST(REALTIME_LIBS)

AC_SEARCH_LIBS([SHA1], [crypto], [OPENSSL=1], [OPENSSL=0])
AC_SEARCH_LIBS([event_new], [event], [LIBEVENT=1], [LIBEVENT=0])




# Checks for standard header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h unistd.h stdbool.h], [], [AC_MSG_ERROR([Cannot configure core library])])

# Checks for POSIX header files.
AC_CHECK_HEADERS([fcntl.h netdb.h netinet/in.h sys/socket.h arpa/inet.h sys/time.h mqueue.h], [], [echo Default dependencies cannot be installed.])

# Checks for library functions.
AC_CHECK_FUNCS([malloc realloc memmove memset strchr strtol strtoul mkdir rmdir strerror strstr ], [], [AC_MSG_ERROR([Cannot configure core library])])
AC_CHECK_FUNCS([socket], [], [echo Default dependencies cannot be installed.])

# Check for asprintf

AC_CHECK_FUNC([asprintf],
        [CONFIGFLAGS+=" -DCB_HAVE_ASPRINTF"],
        []
)


# Determine additional compile options for libraries

CFLAGS+=" -fPIC -pthread "
AC_MSG_CHECKING([whether CC supports -fPIC])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
    [AC_MSG_RESULT([yes])]
    [CONFIGFLAGS+=" -fPIC"],
    [AC_MSG_RESULT([no])]
)

AC_SUBST([CONFIGFLAGS])
AC_SUBST([CONFIGLFLAGS])
AC_SUBST([CC])
AC_SUBST([NOEC])


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday memmove memset mkdir rmdir socket strchr strerror strstr strtol strtoul])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_FILES([ Makefile include/Makefile dependencies/Makefile dependencies/crypto/Makefile dependencies/logging/Makefile dependencies/random/Makefile  dependencies/sockets/Makefile   dependencies/storage/Makefile dependencies/threads/Makefile lib/Makefile src/Makefile test/Makefile ])

AC_PROG_CC
AC_PROG_CC_STDC
AM_INIT_AUTOMAKE
AC_OUTPUT


echo
echo "CONFIGURE SUMMARY:"
echo

hasFeature(){
        printf "$1"
        [[ "$3" = "no" ]] && o="yes" || o="no"
        [[ "$2" = "$o" ]] && echo $o || echo "$3"
}

echo Configure compile flags: $CONFIGFLAGS
echo Configure link flags: $CONFIGLFLAGS
echo Compiler: $CC
hasFeature "Error correction: " "$enable_ec" "yes"
hasFeature "Debug build: " "$enable_debug" "no"
hasFeature "Stack protection: " "$enable_stack_protector" "yes"
hasFeature "Error on compilation warnings: " "$enable_werror" "yes"
echo

LIBRARY_NOTICE=" You will not be able to build all of the program. You can still build the \"core\" target and targets which do not use the library. Otherwise please install the library."

if [[ "$OPENSSL" -eq 0 ]]; then
        echo "OpenSSL not available.$LIBRARY_NOTICE"
fi
if [[ "$LIBEVENT" -eq 0 ]]; then
        echo "Libevent not available.$LIBRARY_NOTICE"
fi
echo 

