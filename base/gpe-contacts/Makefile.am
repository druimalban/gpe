## Process this file with automake to produce Makefile.in

PREFIX = /usr/local

SUBDIRS = po src

EXTRA_DIST = \
	autogen.sh \
	adb.glade

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/icons; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/icons/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(PREFIX)/share/pixmaps; \
	    fi \
	  done \
	fi
	$(INSTALL_DATA) contacts-layout.xml $(DESTDIR)$(pkgdatadir)/default-layout.xml
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
