PREFIX = /usr/local
PACKAGE = gtk2-theme-angelistic

install: 
	mkdir -p $(DESTDIR)$(PREFIX)/share/themes/Angelistic/gtk-2.0/
	install -m 644 gtkrc $(DESTDIR)$(PREFIX)/share/themes/Angelistic/gtk-2.0/

ipkg:
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/control familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	ipkg-build familiar/dist

clean:

