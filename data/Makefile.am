#
# data/Makefile.am for qalculate
#

ui_DATA = argumentrules.ui csvexport.ui csvimport.ui datasetedit.ui datasets.ui decimals.ui \
	functionedit.ui functions.ui main.ui matrix.ui matrixedit.ui namesedit.ui nbases.ui \
	setbase.ui periodictable.ui plot.ui precision.ui preferences.ui unitedit.ui \
	units.ui unknownedit.ui variableedit.ui variables.ui
uidir = $(datadir)/$(PACKAGE)/ui

# this is for gnome

desktop_IN_FILES = qalculate-gtk.desktop.in
desktop_DATA = $(desktop_IN_FILES:.desktop.in=.desktop)
desktopdir = $(datadir)/applications
@INTLTOOL_DESKTOP_RULE@

appdata_IN_FILES = qalculate-gtk.appdata.xml.in
appdata_DATA = $(appdata_IN_FILES:.xml.in=.xml)
appdatadir = $(datadir)/appdata
@INTLTOOL_XML_RULE@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = qalculate.png

EXTRA_DIST = \
	$(desktop_IN_FILES)	\
	$(appdata_IN_FILES)	\
	$(ui_DATA)	\
	$(pixmaps_DATA)

