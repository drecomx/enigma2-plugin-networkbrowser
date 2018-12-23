AC_INIT([enigma2-plugin-systemplugins-networkbrowser],[4.3.0],[https://github.com/opendreambox/enigma2-plugin-networkbrowser/issues])
AM_INIT_AUTOMAKE([foreign])

AM_SILENT_RULES([yes])

AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE

AC_PROG_CC
LT_INIT
AC_CONFIG_MACRO_DIRS([m4])

AM_PATH_PYTHON
AX_PYTHON_DEVEL

PKG_CHECK_MODULES(LIBNFS, libnfs)
PKG_CHECK_MODULES(LIBTIRPC, libtirpc)

AC_CONFIG_FILES([
Makefile
meta/Makefile
src/Makefile
src/icons/Makefile
src/lib/Makefile
])
AC_OUTPUT
