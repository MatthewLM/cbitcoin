AC_PREREQ([2.64])
AC_INIT([cbitcoin], [2.0 (pre-alpha)], [cbitcoin@thelibertyportal.com])
AC_CONFIG_SRCDIR([dependencies/crypto/CBOpenSSLCrypto.c])

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

# Determine compile options
save_cflags="$CFLAGS"
CFLAGS="-arch i386 -arch x86_64"
AC_MSG_CHECKING([whether CC supports -arch i386 -arch x86_64])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])], 
    [AC_MSG_RESULT([yes])]
    [COMPILE_CONFIG_FLAGS="-arch i386 -arch x86_64"], 
    [AC_MSG_RESULT([no])]
	[CFLAGS=-m64]
	[AC_MSG_CHECKING([whether CC supports -m64])]
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])], 
		[AC_MSG_RESULT([yes])]
		[LINKER_CONFIG_FLAGS="-m64"]
	)
)
CFLAGS="$save_cflags"

# Determine additional compile options for libraries
save_cflags="$CFLAGS"
CFLAGS="-fPIC"
AC_MSG_CHECKING([whether CC supports -fPIC])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])], 
    [AC_MSG_RESULT([yes])]
    [LIB_COMPILE_CONFIG_FLAGS="-fPIC"], 
    [AC_MSG_RESULT([no])]
)
CFLAGS="$save_cflags"

# Add substitutions

AC_SUBST([LINKER_CONFIG_FLAGS])
AC_SUBST([COMPILE_CONFIG_FLAGS])
AC_SUBST([LIB_COMPILE_CONFIG_FLAGS])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
