PREFIX = /usr/local

install:
	install -D gpe-edit.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-terminal.png
	install -D gpe-edit.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-terminal

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control postinst dist/CONTROL
	make DESTDIR=`pwd`/dist install
	chown -R root.root dist
	ipkg-build dist

clean:
	rm -f gpe-edit $(OBJS)
