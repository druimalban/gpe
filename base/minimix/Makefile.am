bin_PROGRAMS = minimix

INCLUDES = @MINIMIX_CFLAGS@
LDADD = @MINIMIX_LIBS@

EXTRA_DIST = minimix.png minimix.desktop

install-data-local:
	install -d $(DESTDIR)$(datadir)/pixmaps
	install -m 0644 minimix.png $(DESTDIR)$(datadir)/pixmaps
	install -d $(DESTDIR)$(datadir)/applications
	install -m 0644 minimix.desktop $(DESTDIR)$(datadir)/applications

include gpe-dist.am
