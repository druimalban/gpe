PREFIX = /usr/local
PIXMAPS = copy.png cut.png error.png exit.png new.png open.png paste.png save.png save_as.png home.png future_view.png day_view.png week_view.png month_view.png year_view.png cancel.png empty.png properties.png search.png search_and_replace.png watch.png pause_watch.png delete.png ok.png

SPIXMAPS = delrecord.xpm new.xpm tick.xpm

install:
	for i in $(PIXMAPS); do install -D default/$$i $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$$i; done
	#for i in $(SPIXMAPS); do install -D storm/$$i $(DESTDIR)/usr/share/gpe/pixmaps/$$i; done
	install -D gpe-logo.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/gpe-logo.png

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control dist/CONTROL/control
	make DESTDIR=`pwd`/dist install
	chown -R root.root dist
	ipkg-build dist
