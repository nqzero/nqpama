AC_PREREQ(2.60)
AC_INIT([pulseaudio-mixer-applet], [0.2.2])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.11 -Wall -Werror foreign])
AM_SILENT_RULES([yes])
AC_CONFIG_MACRO_DIR([m4])

IT_PROG_INTLTOOL([0.35.0])
PKG_PROG_PKG_CONFIG([0.19])

AC_PROG_CC
# Compiling sources with per-target flags requires AM_PROG_CC_C_O
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AM_PATH_GTK_2_0([2.16.0],,AC_MSG_ERROR([Gtk+ 2.16.0 or higher required.]))
PKG_CHECK_MODULES(PULSEAUDIO_MIXER_APPLET, [glib-2.0 gobject-2.0 gtk+-2.0 libpanelapplet-2.0  libpulse >= 0.9.15 libpulse-mainloop-glib])

DATADIR=${prefix}/${DATADIRNAME}
AC_DEFINE_UNQUOTED(DATADIR, "$DATADIR", [Data directory])
AC_SUBST(DATADIR)

GETTEXT_PACKAGE=pulseaudio-mixer-applet
GNOMELOCALEDIR=${prefix}/${DATADIRNAME}/locale
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext Package])
AC_DEFINE_UNQUOTED(GNOMELOCALEDIR, "$GNOMELOCALEDIR", [Locale dir])
AC_SUBST(GETTEXT_PACKAGE)
AC_SUBST(GNOMELOCALEDIR)
AM_GLIB_GNU_GETTEXT

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
        Makefile
        src/Makefile
        po/Makefile.in
])
AC_OUTPUT
