PIXMAPS = close.xpm config.xpm new.xpm save.xpm

install:
	for i in $(PIXMAPS); do install -D $$i $(DESTDIR)/usr/share/gpe/pixmaps/$$i; done
	install -D gpe-logo.png $(DESTDIR)/usr/share/gpe/pixmaps/gpe-logo.png
	install -D error.png $(DESTDIR)/usr/share/gpe/pixmaps/error.png

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control dist/CONTROL/control
	make DESTDIR=`pwd`/dist install
	chown -R root.root dist
	ipkg-build dist
