PREFIX = /usr/local
PACKAGE = gpe-dm
CVSBUILD = yes
GTK2 = yes
VERSION = 0.21

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

gpe-dm: gpe-dm.o
	$(CC) -o $@ $^ -L/usr/X11R6/lib -lX11

install: gpe-dm
	install -D gpe-dm $(DESTDIR)$(PREFIX)/bin/gpe-dm
	strip $(DESTDIR)$(PREFIX)/bin/gpe-dm
	mkdir -p $(DESTDIR)/etc/init.d
	mkdir -p $(DESTDIR)/etc/rc2.d
	install gpe-dm.init $(DESTDIR)/etc/init.d/gpe-dm
	ln -sf /etc/init.d/gpe-dm $(DESTDIR)/etc/rc2.d/S99gpe-dm
	install -D X11/Xsession $(DESTDIR)/etc/X11/Xsession
	install -D X11/Xinit $(DESTDIR)/etc/X11/Xinit
	install -D X11/Xserver $(DESTDIR)/etc/X11/Xserver

clean: clean-po
	rm -f gpe-login gpe-login.o gpe-dm gpe-dm.o gpe-lock-display gpe-lock-display.o nocursor.o

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
-include $(DEPS)
