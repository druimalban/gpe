#CC=gcc
CC=arm-linux-gcc
CFLAGS=-g -I/usr/local/arm/arm-linux/include `gtk-config --cflags` `imlib-config --cflags-gdk` -I/usr/include -Wall
#LDFLAGS=`gtk-config --libs` `imlib-config --libs-gdk`
LDFLAGS=-L/usr/local/arm/arm-linux/lib -L/usr/local/arm/arm-linux/lib/X11 -lgtk -lgdk -rdynamic -lgmodule -lglib -ldl -lXi -lXext -lX11 -lm -lgdk_imlib
STRIP=arm-linux-strip
INSTALL=install

DESTDIR=
PREFIX=/usr
BINDIR=$(PREFIX)/bin

OBJS=main.o xsi.o popupmenu.o package.o cfg.o properties.o

all: gpe-appmgr gpe-appmgr-setup

gpe-appmgr: $(OBJS)
	$(CC) -o gpe-appmgr $(OBJS) $(LDFLAGS)

gpe-appmgr-setup: appmgr_setup.c package.o
	$(CC) -o gpe-appmgr-setup appmgr_setup.c package.o $(LDFLAGS) $(CFLAGS)

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
	rm -f dist/usr/bin/gpe-appmgr*
