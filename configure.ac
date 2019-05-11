dnl
dnl configure.in for qalculate-gtk
dnl

dnl  ----------------------
dnl | initialize autotools |---------------------------------------------------
dnl  ----------------------

AC_INIT([Qalculate! (GTK+ UI)], [3.2.0],[],[qalculate-gtk])
AC_CONFIG_SRCDIR([src/callbacks.cc])
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE


dnl  -------------------------------
dnl | check for neccessary programs |------------------------------------------
dnl  -------------------------------

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_STDC
AC_PROG_LN_S
AC_HEADER_STDC
AC_PROG_INTLTOOL
AM_PROG_LIBTOOL

dnl  ------------------------------------
dnl | check for compiler characteristics |-------------------------------------
dnl  ------------------------------------

dnl Replace -Os with -O2 to stop segfault on startup
if test "x$GCC" = "xyes"; then
	case $CFLAGS in
	*-Os*)
		CFLAGS="$CFLAGS -O2"
		;;
	esac
	case $CXXFLAGS in
	*-Os*)
		CXXFLAGS="$CXXFLAGS -O2"
		;;
	esac
fi


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

AM_CONDITIONAL(PLATFORM_LINUX, test "$host_os" = "linux")
AM_CONDITIONAL(PLATFORM_WIN32, test "$host_os" = "mingw32")


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
	gtk+-3.0 >= 3.12
	gdk-pixbuf-2.0
	])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

GTK_LIBS="-export-dynamic $GTK_LIBS"

PKG_CHECK_MODULES(QALCULATE, [
	libqalculate >= 3.2.0
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
ALL_LINGUAS="nl sv"
AM_GLIB_GNU_GETTEXT

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


	 
INTLTOOL_QALCULATE_DEFINITIONS_RULE='%.xml:       %.xml.in       $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po-defs/*.po) ; LC_ALL=C $(INTLTOOL_MERGE) -x -u -c $(top_builddir)/po-defs/.intltool-merge-cache $(top_srcdir)/po-defs $< [$]@' 
AC_SUBST(INTLTOOL_QALCULATE_DEFINITIONS_RULE)	 

dnl  --------
dnl | output |-----------------------------------------------------------------
dnl  --------

AC_OUTPUT([
  Makefile
  data/Makefile
  doc/Makefile
  src/Makefile
  po/Makefile.in
])

