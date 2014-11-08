AUTOMAKE_OPTIONS = foreign

CFLAGS = " -Wall -Wextra -Wno-uninitialized -Wno-pointer-to-int-cast -pedantic -std=gnu99 -D_GNU_SOURCE "

ACLOCAL_AMFLAGS = ' -I m4 '

SUBDIRS = src dependencies test include examples client-server


clean:
	find . -name '*.la' -exec rm {} \; && find . -name '*.o' -exec rm {} \; && find . -name '*.lo' -exec rm {} \; && ( find . -name '.deps' | while read line; do rm -r $line; done )
