bin_PROGRAMS = gpe-buttonbox

INCLUDES = @BUTTONBOX_CFLAGS@
LDADD = @BUTTONBOX_LIBS@

install-data-local:
	mkdir -p $(datadir)/gpe/pixmaps/default/gpe-buttonbox
	install -m 0644 show-desktop.png $(datadir)/gpe/pixmaps/default/gpe-buttonbox/
	install -m 0644 other-app.png $(datadir)/gpe/pixmaps/default/gpe-buttonbox/

include gpe-dist.am
