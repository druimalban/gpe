PREFIX = /usr/local

install:
	install -D gpe-terminal.png $(DESTDIR)/usr/share/pixmaps/gpe-terminal.png
	install -D gpe-terminal.menu $(DESTDIR)/usr/lib/menu/gpe-terminal

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control postinst dist/CONTROL
	make DESTDIR=`pwd`/dist install
	chown -R root.root dist
	ipkg-build dist
