## Process this file with automake to produce Makefile.in

SUBDIRS = src

EXTRA_DIST = \
	autogen.sh \
	gpe-config-sleep.glade

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	fi
	if test -f $(srcdir)/$(PACKAGE).menu; then \
	  $(INSTALL_DATA) -D $(PACKAGE).menu $(DESTDIR)$(PREFIX)/lib/menu/$(PACKAGE); \
	fi
	if test -f $(srcdir)/$(PACKAGE).png; then \
	  $(INSTALL_DATA) -D $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png; \
	fi

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


