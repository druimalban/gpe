#CC=gcc
CC=arm-linux-gcc
CFLAGS=-g -I/usr/local/arm/arm-linux/include `gtk-config --cflags` -I/usr/include -Wall `gdk-pixbuf-config --cflags` -I../libgpewidget -I/usr/local/arm/arm-linux/include/X11/extensions #-Os -fomit-frame-pointer
#LDFLAGS=-L../libgpewidget -lgpewidget `gtk-config --libs` `gdk-pixbuf-config --libs`
LDFLAGS=-L../libgpewidget -lgpewidget -L/usr/local/arm/arm-linux/lib -L/usr/local/arm/arm-linux/lib/X11 -lgtk -lgdk -rdynamic -lgmodule -lglib -ldl -lXext -lX11 -lm -lgdk_pixbuf
STRIP=arm-linux-strip
INSTALL=install

DESTDIR=
PREFIX=/usr
BINDIR=$(PREFIX)/bin

OBJS=main.o xsi.o popupmenu.o package.o cfg.o properties.o gnome-exec.o plugin.o

all: gpe-appmgr gpe-appmgr-setup

gpe-appmgr: $(OBJS)
	$(CC) -o gpe-appmgr $(OBJS) $(LDFLAGS)

gpe-appmgr-setup: appmgr_setup.c package.o cfg.o
	$(CC) -o gpe-appmgr-setup appmgr_setup.c package.o cfg.o $(LDFLAGS) $(CFLAGS)

install: gpe-appmgr gpe-appmgr-setup
	$(INSTALL) gpe-appmgr $(DESTDIR)/$(BINDIR)/gpe-appmgr
	$(STRIP) $(DESTDIR)/$(BINDIR)/gpe-appmgr
	$(INSTALL) gpe-appmgr-setup $(DESTDIR)/$(BINDIR)/gpe-appmgr-setup
	$(STRIP) $(DESTDIR)/$(BINDIR)/gpe-appmgr-setup

ipk: all
	make DESTDIR=`pwd`/dist install
	ipkg-build dist

clean:
	rm -f *.o gpe-appmgr gpe-appmgr-setup

distclean: clean
	rm -f *.ipk
	rm -f dist/usr/bin/*
