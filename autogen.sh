#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
DIE=0

THEDIR=`pwd`
cd $srcdir

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile GTK+."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at http://ftp.gnu.org/gnu/autoconf/"
	DIE=1
}

(libtool --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have libtool installed to compile murrine."
	echo "Get http://ftp.gnu.org/gnu/libtool/libtool-1.5.10.tar.gz"
	echo "(or a newer version if it is available)"
	DIE=1
}

(intltoolize --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have intltool installed to compile murrine."
	DIE=1
}
if test "$DIE" -eq 1; then
	exit 1
fi

if test -z "$*"; then
	echo "I am going to run ./configure with no arguments - if you wish "
	echo "to pass any to it, please specify them on the $0 command line."
fi

libtoolize --force --copy
intltoolize --force --copy --automake

aclocal $ACLOCAL_FLAGS
autoconf
autoheader
automake --add-missing
cd $THEDIR

$srcdir/configure "$@"
