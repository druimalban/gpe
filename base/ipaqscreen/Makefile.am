## Process this file with automake to produce Makefile.in

SUBDIRS = intl po src

EXTRA_DIST = \
	autogen.sh \
	ipaqscreen.glade

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
	$(INSTALL_DATA) $(srcdir)/ipaqscreen.png $(DESTDIR)$(PREFIX)/share/pixmaps
	install -D $(srcdir)/ipaqscreen.menu $(DESTDIR)$(PREFIX)/lib/menu/ipaqscreen

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

