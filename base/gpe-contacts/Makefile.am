## Process this file with automake to produce Makefile.in

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
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	fi
	$(INSTALL_DATA) contacts-layout.xml $(DESTDIR)$(pkgdatadir)/default-layout.xml

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

