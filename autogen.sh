#! /bin/sh
libtoolize --force --copy \
&& aclocal \
&& autoheader \
&& automake --add-missing \
&& autoconf