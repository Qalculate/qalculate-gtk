#! /bin/sh
libtoolize --force --copy \
&& autopoint --force \
&& aclocal \
&& autoheader \
&& automake --add-missing \
&& autoconf