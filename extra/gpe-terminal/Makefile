PREFIX=/usr/local
VERSION=0.0.2

all:

clean:

install-program:
	install -m 644 -D gpe-terminal.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-terminal.png
	install -m 644 -D gpe-terminal.desktop $(DESTDIR)$(PREFIX)/share/applications/gpe-terminal.desktop

include ../build/Makefile.dpkg_ipkg

