PREFIX = /usr/local
PACKAGE = gpe-dataconf
pkgdatadir = $(PREFIX)/share/$(PACKAGE)
CVSBUILD = yes
GTK2 = yes

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all:
	make -C src all PREFIX=$(PREFIX) DESTDIR=$(DESTDIR)

clean:
	make -C src clean

install-program:
	make -C src install PREFIX=$(PREFIX) DESTDIR=$(DESTDIR)
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 gpe-dataconf.desktop $(DESTDIR)$(PREFIX)/share/applications/
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 icons/gpe-dataconf.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/gpe-dataconf.png
	mkdir -p $(DESTDIR)$(pkgdatadir)
	mkdir -p $(DESTDIR)$(pkgdatadir)/pixmaps
	install -m 644 icons/frame.png $(DESTDIR)$(pkgdatadir)/pixmaps/
	install -m 644 icons/entry.png $(DESTDIR)$(pkgdatadir)/pixmaps/
	install -m 644 icons/notebook.png $(DESTDIR)$(pkgdatadir)/pixmaps/

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
