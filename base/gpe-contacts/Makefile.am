## Process this file with automake to produce Makefile.in

PREFIX = /usr/local

SUBDIRS = m4  po src

EXTRA_DIST = config.rpath  \
	autogen.sh \
	adb.glade

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/icons; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/icons/*.xpm; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	fi
	$(mkinstalldirs) $(DESTDIR)$(PREFIX)/share/pixmaps
	$(INSTALL_DATA) $(srcdir)/icons/gpe-contacts.png $(DESTDIR)$(PREFIX)/share/pixmaps
	$(INSTALL_DATA) contacts-layout.xml $(DESTDIR)$(pkgdatadir)/default-layout.xml
	$(INSTALL_DATA) well-known-tags $(DESTDIR)$(pkgdatadir)/well-known-tags
	install -D gpe-contacts.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-contacts

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

include ../build/Makefile.dpkg_ipkg

ACLOCAL_AMFLAGS = -I m4
