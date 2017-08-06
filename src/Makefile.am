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

qalculate_gtk_SOURCES = \
	main.cc		main.h		\
	interface.cc	interface.h	\
	callbacks.cc	callbacks.h	\
	support.h

qalculate_gtk_LDADD = \
	@GLIB_LIBS@ \
	@GTK_LIBS@ \
	@QALCULATE_LIBS@

