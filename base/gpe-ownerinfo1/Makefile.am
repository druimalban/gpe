## Process this file with automake to produce Makefile.in

PACKAGE = gpe-ownerinfo

SUBDIRS = intl po src

EXTRA_DIST = \
	autogen.sh \
	gpe-ownerinfo.glade

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
	$(mkinstalldirs) $(DESTDIR)/etc/gpe/
	if test ! -e $(DESTDIR)/etc/gpe/gpe-ownerinfo.data; then \
	  $(INSTALL_DATA) gpe-ownerinfo.data $(DESTDIR)/etc/gpe/gpe-ownerinfo.data; \
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
