NATIVE = no
CVSBUILD = yes
DEBUG = no

ifeq ($(CVSBUILD),yes)
LDFLAGS += -L../libgpewidget -L../libxsettings  -L../libxsettings-client -L../libdotdesktop
CFLAGS += -I../libgpewidget -I../libxsettings-client -I../libxsettings -I../libdotdesktop
else
CFLAGS += -I/usr/include/gpe
endif

ifeq ($(DEBUG),yes)
CFLAGS += -g -DDEBUG
else
CFLAGS += -Os -fomit-frame-pointer
endif

ifeq ($(NATIVE),yes)
STRIP=strip

LDFLAGS += -lgpewidget `pkg-config --libs gtk+-2.0 gdk-pixbuf-2.0` -ldotdesktop #-lXinerama
CFLAGS += `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0` -Wall

else #cross-compile!

CC=arm-linux-gcc
STRIP=arm-linux-strip

#LDFLAGS += -lgpewidget `pkg-config --libs gtk+-2.0 gdk-pixbuf-2.0 | sed "s/-L.*lib //"`
LDFLAGS += -lgpewidget -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lpangoxft-1.0 -lpangox-1.0 -lpango-1.0 -lgdk_pixbuf-2.0 -lm -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0 -lXinerama -ldotdesktop
CFLAGS += `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0` -Wall

endif #native or cross-compile

LDFLAGS += -lXsettings -lXsettings-client


INSTALL=install

DESTDIR=
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
ETCDIR=etc

CFLAGS += -D_GNU_SOURCE

OBJS=main.o xsi.o popupmenu.o package.o properties.o gnome-exec.o plugin.o tray.o xsettings.o

all: gpe-appmgr

gpe-appmgr: $(OBJS)
	$(CC) -o gpe-appmgr $(OBJS) $(LDFLAGS)

install: gpe-appmgr
	$(INSTALL) -D gpe-appmgr $(DESTDIR)/$(BINDIR)/gpe-appmgr
	$(STRIP) $(DESTDIR)/$(BINDIR)/gpe-appmgr
	mkdir -p $(DESTDIR)/$(PREFIX)/share/pixmaps
	$(INSTALL) -D dist/usr/share/pixmaps/*.png  $(DESTDIR)/$(PREFIX)/share/pixmaps
	$(INSTALL) -D dist/etc/menu-methods/gpe_appmgr $(DESTDIR)/$(ETCDIR)/menu-methods/gpe_appmgr
	$(INSTALL) -D appmgr.session $(DESTDIR)/etc/X11/Xsession.d/90GpeAppMgr

clean:
	rm -f *.o gpe-appmgr

distclean: clean
	rm -f *.ipk
	rm -rf familiar/dist

include ../build/Makefile.translation
include ../build/Makefile.dpkg_ipkg
