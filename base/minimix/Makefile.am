bin_PROGRAMS = minimix

INCLUDES = @MINIMIX_CFLAGS@
LDADD = @MINIMIX_LIBS@

EXTRA_DIST = minimix.png

install-data-local:
	install -d $(DESTDIR)$(datadir)/pixmaps
	install -m 0644 minimix.png $(DESTDIR)$(datadir)/pixmaps

include gpe-dist.am
