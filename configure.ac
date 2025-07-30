dnl
dnl configure.in for qalculate-gtk
dnl

dnl  ----------------------
dnl | initialize autotools |---------------------------------------------------
dnl  ----------------------

AC_INIT([Qalculate! (GTK UI)],[5.7.0],[],[qalculate-gtk])
AC_CONFIG_SRCDIR([src/main.cc])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS(config.h)
AM_MAINTAINER_MODE
AC_CONFIG_MACRO_DIR(m4)

dnl  -------------------------------
dnl | check for neccessary programs |------------------------------------------
dnl  -------------------------------

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LN_S
AC_PROG_EGREP

IT_PROG_INTLTOOL
LT_INIT

dnl  ------------------------------------
dnl | check for compiler characteristics |-------------------------------------
dnl  ------------------------------------

dnl Use -Wall if we have gcc.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
	case " $CFLAGS " in
		*[\ \	]-Wall[\ \	]*) ;;
		*) CFLAGS="$CFLAGS -Wall" ;;
	esac
fi
changequote([,])dnl

AC_CANONICAL_HOST

case "${host_os}" in
	linux*)
		build_linux=yes
		;;
	mingw*)
		build_windows=yes
		;;
	darwin*)
		build_mac=yes
		;;
esac

AM_CONDITIONAL(PLATFORM_LINUX, test "$build_linux" = "yes")
AM_CONDITIONAL(PLATFORM_WIN32, test "$build_windows" = "yes")


dnl  --------------------------------
dnl | check for neccessary libraries |-----------------------------------------
dnl  --------------------------------

AC_CHECK_LIB(pthread, pthread_create, [LIBS="$LIBS -lpthread"])

AC_LANG([C++])

AC_CHECK_HEADERS([unordered_map])

PKG_CHECK_MODULES(GLIB, [
	glib-2.0 >= 2.4
	libxml-2.0
	])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

QALCULATE_GTK="qalculate-gtk${EXEEXT}"

PKG_CHECK_MODULES(GTK, [
	gtk+-3.0 >= 3.10
	gdk-pixbuf-2.0
	])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

GTK_LIBS="-export-dynamic $GTK_LIBS"

AC_ARG_WITH([webkitgtk], AS_HELP_STRING([--with-webkitgtk], [use WebKitGTK for display of the user manual]))
AS_IF([test "x$with_webkitgtk" = "xyes"], [
	PKG_CHECK_MODULES([WEBKITGTK], [webkit2gtk-4.0])
	AC_DEFINE([USE_WEBKITGTK], [1], [Use WebKitGTK])
	AC_SUBST(WEBKITGTK_CFLAGS)
	AC_SUBST(WEBKITGTK_LIBS)
])

AS_IF([test "x$with_webkitgtk" = "x4.0"], [
	PKG_CHECK_MODULES([WEBKITGTK], [webkit2gtk-4.0])
	AC_DEFINE([USE_WEBKITGTK], [1], [Use WebKitGTK])
	AC_SUBST(WEBKITGTK_CFLAGS)
	AC_SUBST(WEBKITGTK_LIBS)
])

AS_IF([test "x$with_webkitgtk" = "x4.1"], [
	PKG_CHECK_MODULES([WEBKITGTK], [webkit2gtk-4.1])
	AC_DEFINE([USE_WEBKITGTK], [1], [Use WebKitGTK])
	AC_SUBST(WEBKITGTK_CFLAGS)
	AC_SUBST(WEBKITGTK_LIBS)
])

AC_ARG_ENABLE([gnome_search], AS_HELP_STRING([--disable-gnome-search], [disable Gnome shell search provider]))
if test -z $enable_gnome_search && (test "$build_windows" = "yes" || test "$build_mac" = "yes"); then
	enable_gnome_search=no
fi
AS_IF([test "x$enable_gnome_search" != "xno"], [
	PKG_CHECK_MODULES([GIOUNIX], [gio-unix-2.0])
	AC_SUBST(GIOUNIX_CFLAGS)
	AC_SUBST(GIOUNIX_LIBS)
])
AM_CONDITIONAL(ENABLE_SEARCH_PROVIDER, [test "x$enable_gnome_search" != "xno"])

AC_ARG_WITH([local_help], AS_HELP_STRING([--without-local-help], [use online manual instead of local help files]))
AS_IF([test "x$with_local_help" != "xno"], [
	AC_DEFINE([LOCAL_HELP], [1], [Use local help files])
])
AM_CONDITIONAL(ENABLE_LOCAL_HELP, [test "x$with_local_help" != "xno"])

PKG_CHECK_MODULES(QALCULATE, [
	libqalculate >= 5.6.0
	])
AC_SUBST(QALCULATE_CFLAGS)
AC_SUBST(QALCULATE_LIBS)

AC_SUBST(QALCULATE_GTK)

dnl  -------------------------------------
dnl | internationalization (i18n) support |------------------------------------
dnl  -------------------------------------

GETTEXT_PACKAGE=qalculate-gtk
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[The package name for gettext])

dnl Please keep this in alphabetical order
ALL_LINGUAS="ca de es fr hu nl pl pt_BR pt_PT ru sl sv zh_CN zh_TW"
AM_GNU_GETTEXT(external)
AM_GNU_GETTEXT_VERSION([0.21])

if test "$USE_NLS" = yes; then
	AC_DEFINE([ENABLE_NLS], [1], [Define to 1 if translation of program messages to the user's native language is requested.])
fi

dnl AM_GLIB_GNU_GETTEXT sets $DATADIRNAME
AC_MSG_CHECKING(locale directory)
if test "x$prefix" = "xNONE"; then
	PACKAGE_LOCALE_DIR=$ac_default_prefix/share/locale
else
	PACKAGE_LOCALE_DIR=$prefix/share/locale
fi

AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "$PACKAGE_LOCALE_DIR",
	[The directory in which qalculate's locale data will be stored])
	AC_MSG_RESULT("$PACKAGE_LOCALE_DIR")


dnl  --------
dnl | output |-----------------------------------------------------------------
dnl  --------

AC_CONFIG_FILES([
	Makefile
	data/Makefile
	data/icons/Makefile
	data/icons/16x16/Makefile
	data/icons/22x22/Makefile
	data/icons/24x24/Makefile
	data/icons/32x32/Makefile
	data/icons/48x48/Makefile
	data/icons/64x64/Makefile
	data/icons/128x128/Makefile
	data/icons/256x256/Makefile
	data/icons/scalable/Makefile
	doc/Makefile
	src/Makefile
	po/Makefile.in
])
AC_OUTPUT

