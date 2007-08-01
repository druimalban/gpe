SUBDIRS = mimedir bin po docs

EXTRA_DIST = \
	COPYING.LIB		\
	mimedir-1.0.pc.in	\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in

distclean-local:
	$(RM) -f intltool-extract intltool-merge intltool-update

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mimedir-1.0.pc

include gpe-dist.am
