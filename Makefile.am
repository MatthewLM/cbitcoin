AUTOMAKE_OPTIONS = foreign

CFLAGS = " -Wall -Wextra -Wno-uninitialized -Wno-pointer-to-int-cast -pedantic -std=gnu99 -D_GNU_SOURCE "

ACLOCAL_AMFLAGS = ' -I m4 '

SUBDIRS = src dependencies test examples client-server 


clean:
	#find . -name '*.la' -exec rm {} \; && find . -name '*.o' -exec rm {} \; && find . -name '*.lo' -exec rm {} \; &&  ( find . -name '.deps'  -exec rm -r {} \; || echo "do not worry about errors here" ) && find . -name 'Makefile' -exec rm {} \; && find . -name 'Makefile.in' -exec rm {} \; && find . -name '*.a' -exec rm {} \;
	git reset --hard && git clean -fxd
