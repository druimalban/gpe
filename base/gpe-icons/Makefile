install:
	install -D gpe-logo.png $(DESTDIR)/usr/share/pixmaps/gpe-logo.png
	install -D error.png $(DESTDIR)/usr/share/pixmaps/error.png

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control dist/CONTROL/control
	make DESTDIR=`pwd`/dist install
	chown -R root.root dist
	ipkg-build dist
