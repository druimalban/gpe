NATIVE = no
CVSBUILD = yes
DEBUG = no

ifeq ($(CVSBUILD),yes)
LDFLAGS += -L../libgpewidget
CFLAGS += -I../libgpewidget
else
CFLAGS += -I/usr/include/gpe
endif

ifeq ($(DEBUG),yes)
CFLAGS += -g -DDEBUG
else
CFLAGS += -Os -fomit-frame-pointer
endif

ifeq ($(NATIVE),yes)
LDFLAGS += -lgpewidget `gtk-config --libs` `gdk-pixbuf-config --libs`
STRIP=strip
CFLAGS += `gtk-config --cflags` `gdk-pixbuf-config --cflags` -Wall
else
CC=arm-linux-gcc
STRIP=arm-linux-strip
LDFLAGS += -L../libgpewidget -lgpewidget -L/usr/local/arm/arm-linux/lib -L/usr/local/arm/arm-linux/lib/X11 -lgtk -lgdk -rdynamic -lgmodule -lglib -ldl -lXext -lX11 -lm -lgdk_pixbuf
CFLAGS += -I/usr/local/arm/arm-linux/include `gtk-config --cflags` -I/usr/include -Wall `gdk-pixbuf-config --cflags` -I../libgpewidget -I/usr/local/arm/arm-linux/include/X11/extensions
endif

INSTALL=install

DESTDIR=
PREFIX=/usr
BINDIR=$(PREFIX)/bin
ETCDIR=etc

CFLAGS += -D_GNU_SOURCE

OBJS=main.o xsi.o popupmenu.o package.o cfg.o properties.o gnome-exec.o plugin.o tray.o

all: gpe-appmgr gpe-appmgr-setup

gpe-appmgr: $(OBJS)
	$(CC) -o gpe-appmgr $(OBJS) $(LDFLAGS)

gpe-appmgr-setup: appmgr_setup.c package.o cfg.o
	$(CC) -o gpe-appmgr-setup appmgr_setup.c package.o cfg.o $(LDFLAGS) $(CFLAGS)

install: gpe-appmgr gpe-appmgr-setup
	$(INSTALL) -D gpe-appmgr $(DESTDIR)/$(BINDIR)/gpe-appmgr
	$(STRIP) $(DESTDIR)/$(BINDIR)/gpe-appmgr
	$(INSTALL) -D gpe-appmgr-setup $(DESTDIR)/$(BINDIR)/gpe-appmgr-setup
	$(STRIP) $(DESTDIR)/$(BINDIR)/gpe-appmgr-setup
	mkdir -p $(DESTDIR)/$(PREFIX)/share/pixmaps
	$(INSTALL) -D dist/usr/share/pixmaps/*.png  $(DESTDIR)/$(PREFIX)/share/pixmaps
	$(INSTALL) -D dist/etc/menu-methods/gpe_appmgr $(DESTDIR)/$(ETCDIR)/menu-methods/gpe_appmgr
	$(INSTALL) -D dist/usr/lib/menu/gpe-appmgr-setup $(DESTDIR)/$(PREFIX)/lib/menu/gpe-appmgr-setup
	$(INSTALL) -D dist/usr/lib/menu/terminal $(DESTDIR)/$(PREFIX)/lib/menu/terminal

ipk:
	make DESTDIR=`pwd`/build all install
	$(INSTALL) -D dist/CONTROL/control build/CONTROL/control
	ipkg-build build

ipkg: ipk

clean:
	rm -f *.o gpe-appmgr gpe-appmgr-setup

distclean: clean
	rm -f *.ipk
	rm -f dist/usr/bin/*
