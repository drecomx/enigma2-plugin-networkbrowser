AM_CFLAGS = \
	-Wall \
	@LIBNFS_CFLAGS@ \
	@LIBTIRPC_CFLAGS@

AM_CPPFLAGS = \
	@PYTHON_CPPFLAGS@ \
	-include Python.h

plugindir = $(libdir)/enigma2/python/Plugins/SystemPlugins/NetworkBrowser

plugin_LTLIBRARIES = nfsutils.la rpcinfo.la

nfsutils_la_SOURCES = \
	showmount.c

nfsutils_la_LDFLAGS = \
	-avoid-version \
	-module \
	-shared

nfsutils_la_LIBADD = \
	@LIBNFS_LIBS@ \
	@PYTHON_LDFLAGS@

rpcinfo_la_SOURCES = \
	rpcinfo.c

rpcinfo_la_LIBADD = \
	@LIBTIRPC_LIBS@

rpcinfo_la_LDFLAGS = \
	-avoid-version \
	-module \
	-shared
