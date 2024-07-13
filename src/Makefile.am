#
# src/Makefile.am for qalculate
#

AM_CPPFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_DOC_DIR=\""$(docdir)"\" \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	@WEBKITGTK_CFLAGS@ \
	@GIOUNIX_CFLAGS@ \
	@QALCULATE_CFLAGS@

if ENABLE_SEARCH_PROVIDER
searchproviderdir=@libexecdir@
searchprovider_PROGRAMS = qalculate-search-provider
endif
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
	setbasedialog.cc setbasedialog.h \
	buttonseditdialog.cc buttonseditdialog.h \
	shortcutsdialog.cc shortcutsdialog.h \
	decimalsdialog.cc decimalsdialog.h \
	precisiondialog.cc precisiondialog.h \
	datasetsdialog.cc datasetsdialog.h \
	functionsdialog.cc functionsdialog.h \
	unitsdialog.cc unitsdialog.h \
	variablesdialog.cc variablesdialog.h \
	plotdialog.cc plotdialog.h \
	periodictabledialog.cc periodictabledialog.h \
	numberbasesdialog.cc numberbasesdialog.h \
	percentagecalculationdialog.cc percentagecalculationdialog.h \
	calendarconversiondialog.cc calendarconversiondialog.h \
	floatingpointdialog.cc floatingpointdialog.h \
	main.cc main.h \
	interface.cc interface.h \
	callbacks.cc callbacks.h \
	support.h util.h settings.h

nodist_qalculate_gtk_SOURCES = ui.c flags.c

qalculate_gtk_LDADD = \
	@GLIB_LIBS@ \
	@GTK_LIBS@ \
	@WEBKITGTK_LIBS@ \
	@QALCULATE_LIBS@

if ENABLE_SEARCH_PROVIDER
gnome-search-provider2.c: $(top_srcdir)/data/org.gnome.ShellSearchProvider2.xml
	$(AM_V_GEN)gdbus-codegen \
		--c-namespace Shell \
		--generate-c-code gnome-search-provider2 \
		--interface-prefix org.gnome.Shell. \
		$<

qalculate_search_provider_SOURCES = \
	searchprovider.cc \
	support.h

nodist_qalculate_search_provider_SOURCES = gnome-search-provider2.h gnome-search-provider2.c

qalculate_search_provider_LDADD = \
	@GLIB_LIBS@ \
	@GTK_LIBS@ \
	@GIOUNIX_LIBS@ \
	@QALCULATE_LIBS@

searchprovider.o: gnome-search-provider2.c

dbusservicedir = $(datadir)/dbus-1/services
dbusservice_DATA = io.github.Qalculate.SearchProvider.service

io.github.Qalculate.SearchProvider.service: Makefile
	$(AM_V_GEN) (echo '[D-BUS Service]'; \
		echo 'Name=io.github.Qalculate.SearchProvider'; \
		echo 'Exec=@libexecdir@/qalculate-search-provider') > $@.tmp && \
		mv $@.tmp $@

endif


