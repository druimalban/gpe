## Process this file with automake to produce Makefile.in

tchi:
	cd src && make tchi
arm: 
	cd src && make arm
strip:
	cd src && make strip
ipkg:
	cd src && make ipkg

SUBDIRS = src

EXTRA_DIST = \
	gpe-sketchbook.glade \
	gpe-sketchbook.menu \
	gpe-sketchbook.png \
	TODO \
	familiar/control \
        familiar/postinst \
        ipkg-build

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

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

