#
# src/Makefile.am for qalculate
#

AM_CPPFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_DOC_DIR=\""$(docdir)"\" \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	@WEBKITGTK_CFLAGS@ \
	@QALCULATE_CFLAGS@

if PLATFORM_WIN32
qalculate_gtk_LDFLAGS = -mwindows
bin_PROGRAMS = @QALCULATE_GTK@
EXTRA_PROGRAMS = qalculate-gtk
else
bin_PROGRAMS = @QALCULATE_GTK@ qalculate-search-provider
EXTRA_PROGRAMS = qalculate-gtk qalculate-search-provider
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
	@WEBKITGTK_LIBS@ \
	@QALCULATE_LIBS@

if PLATFORM_WIN32
else
gnome-search-provider2.h gnome-search-provider2.c: org.gnome.ShellSearchProvider2.xml
	$(AM_V_GEN)gdbus-codegen \
		--c-namespace Shell \
		--generate-c-code gnome-search-provider2 \
		--interface-prefix org.gnome.Shell. \
		$<

EXTRA_DIST = org.gnome.ShellSearchProvider2.xml

qalculate_search_provider_SOURCES = \
	searchprovider.cc	\
	support.h

nodist_qalculate_search_provider_SOURCES = gnome-search-provider2.h gnome-search-provider2.c

qalculate_search_provider_LDADD = \
	@GLIB_LIBS@ \
	@GTK_LIBS@ \
	@QALCULATE_LIBS@

dbusservicedir = $(datadir)/dbus-1/services
dbusservice_DATA = org.github.Qalculate.SearchProvider.service

org.github.Qalculate.SearchProvider.service: Makefile
	$(AM_V_GEN) (echo '[D-BUS Service]'; \
		echo 'Name=org.github.Qalculate.SearchProvider'; \
		echo 'Exec=${bindir}/qalculate-search-provider') > $@.tmp && \
		mv $@.tmp $@

endif


