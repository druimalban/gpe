PREFIX = /usr/local

install:
	install -D gpe-terminal.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-terminal.png
	install -D gpe-terminal.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-terminal

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp familiar/control familiar/postinst dist/CONTROL
	make DESTDIR=`pwd`/dist install
	chown -R root.root dist
	ipkg-build dist
