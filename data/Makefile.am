#
# data/Makefile.am for qalculate
#

SUBDIRS = icons

noinst_DATA = argumentrules.ui buttonsedit.ui calendarconversion.ui csvexport.ui csvimport.ui datasetedit.ui datasets.ui \
	decimals.ui floatingpoint.ui functionedit.ui functions.ui main.ui matrix.ui matrixedit.ui namesedit.ui \
	nbases.ui shortcuts.ui setbase.ui percentage.ui periodictable.ui plot.ui precision.ui preferences.ui \
	unitedit.ui units.ui unknownedit.ui variableedit.ui variables.ui \
	flags.gresource.xml ui.gresource.xml $(top_srcdir)/data/flags/*.png \
	org.gnome.ShellSearchProvider2.xml io.github.Qalculate.search-provider.ini

# this is for gnome

desktop_IN_FILES = qalculate-gtk.desktop.in
desktop_DATA = $(desktop_IN_FILES:.desktop.in=.desktop)
desktopdir = $(datadir)/applications
@INTLTOOL_DESKTOP_RULE@

appdata_IN_FILES = qalculate-gtk.appdata.xml.in
appdata_DATA = $(appdata_IN_FILES:.xml.in=.xml)
appdatadir = $(datadir)/metainfo
@INTLTOOL_XML_RULE@

if ENABLE_SEARCH_PROVIDER
searchproviderdata_DATA = io.github.Qalculate.search-provider.ini
searchproviderdatadir = $(datadir)/gnome-shell/search-providers
endif

dist_man_MANS = qalculate-gtk.1

EXTRA_DIST = \
	$(desktop_IN_FILES)	\
	$(appdata_IN_FILES)	\
	$(noinst_DATA)

