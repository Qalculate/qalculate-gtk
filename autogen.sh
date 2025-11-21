#! /bin/sh
libtoolize --force --copy \
&& aclocal \
&& autoheader \
&& automake --add-missing \
&& sed '/<releases>/,/<\/releases>/d' data/qalculate-gtk.appdata.xml.in >data/qalculate-gtk.noinst.appdata.xml \
&& autoconf
