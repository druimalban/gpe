## Process this file with automake to produce Makefile.in

prefix=@prefix@

PREFIX=/usr/local

SUBDIRS = intl po src

AM_INSTALL_PROGRAM_FLAGS=-s install

EXTRA_DIST = \
	autogen.sh \
	ipaqscreen.glade

install-data-local:
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(PREFIX)/share/pixmaps
	$(INSTALL_DATA) $(srcdir)/ipaqscreen.png $(DESTDIR)$(PREFIX)/share/pixmaps
	$(mkinstalldirs) $(DESTDIR)$(PREFIX)/lib/menu
	$(INSTALL_DATA) $(srcdir)/ipaqscreen.menu $(DESTDIR)$(PREFIX)/lib/menu/ipaqscreen

include ../build/Makefile.dpkg_ipkg
