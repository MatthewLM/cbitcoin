AC_PREREQ([2.64])
AC_INIT([cbitcoin], [2.0 (pre-alpha)], [cbitcoin@thelibertyportal.com])
AC_CONFIG_SRCDIR([library/dependencies/crypto/CBOpenSSLCrypto.c])

# Set CONFIGFLAGS

CONFIGFLAGS="-Wall -Wno-uninitialized -Wno-pointer-to-int-cast -pedantic -std=gnu99 -D_GNU_SOURCE -I/opt/local/ssl/include -I/opt/local/include -I/usr/local/ssl/include -Werror"
CONFIGFLAGS+=" $CFLAGS"
CONFIGLFLAGS=" $LFLAGS"

# Search for OpenSSL and LibEvent.
AC_SEARCH_LIBS([SHA1], [crypto], [], [echo Default dependencies cannot be installed.])
AC_SEARCH_LIBS([event_new], [event], [], [echo Default dependencies cannot be installed.])

# Checks for standard header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h unistd.h stdbool.h], [], [AC_MSG_ERROR([Cannot configure core library])])

# Checks for POSIX header files.
AC_CHECK_HEADERS([fcntl.h netdb.h netinet/in.h sys/socket.h], [], [echo Default dependencies cannot be installed.])

# Checks for library functions.
AC_CHECK_FUNCS([malloc realloc memmove memset strchr strtol strtoul], [], [AC_MSG_ERROR([Cannot configure core library])])
AC_CHECK_FUNCS([socket], [], [echo Default dependencies cannot be installed.])

# Check for asprintf

AC_CHECK_FUNC([asprintf],
	[CONFIGFLAGS+=" -DCB_HAVE_ASPRINTF"],
	[]
)

# Determine additional compile options for libraries
save_cflags="$CFLAGS"
CFLAGS="-fPIC"
AC_MSG_CHECKING([whether CC supports -fPIC])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])], 
    [AC_MSG_RESULT([yes])]
    [CONFIGFLAGS+=" -fPIC"], 
    [AC_MSG_RESULT([no])]
)
CFLAGS="$save_cflags"

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
	[CONFIGFLAGS+=" -g"],
	[CONFIGFLAGS+=" -O3"]
)

AC_SUBST([CONFIGFLAGS])
AC_SUBST([CONFIGLFLAGS])
AC_SUBST([NOEC])

AC_CONFIG_FILES([Makefile])
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
hasFeature "Error correction: " "$enable_ec" "yes"
hasFeature "Debug build: " "$enable_debug" "no"
echo
