#
# src/Makefile.am for qalculate
#

AM_CPPFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_DOC_DIR=\""$(docdir)"\" \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	@QALCULATE_CFLAGS@

bin_PROGRAMS = @QALCULATE_GTK@
EXTRA_PROGRAMS = qalculate-gtk

if PLATFORM_WIN32
qalculate_gtk_LDFLAGS = -mwindows
endif

ui.c : $(top_srcdir)/data/ui.gresource.xml $(top_srcdir)/data/*.ui
	glib-compile-resources --generate-source --target=$@ --sourcedir=$(top_srcdir)/data $(top_srcdir)/data/ui.gresource.xml
	
flags.c : $(top_srcdir)/data/flags.gresource.xml $(top_srcdir)/data/flags/*.png
	glib-compile-resources --generate-source --target=$@ --sourcedir=$(top_srcdir)/data/flags $(top_srcdir)/data/flags.gresource.xml

qalculate_gtk_SOURCES = \
	main.cc		main.h		\
	interface.cc	interface.h	\
	callbacks.cc	callbacks.h	\
	support.h

nodist_qalculate_gtk_SOURCES = ui.c flags.c

qalculate_gtk_LDADD = \
	@GLIB_LIBS@ \
	@GTK_LIBS@ \
	@QALCULATE_LIBS@

