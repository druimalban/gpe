CVSBUILD = yes
DEBUG = yes
PACKAGE = gpe-appmgr
VERSION = 2.7

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) -I../libgpelaunch -I../libxsettings-client -I../libxsettings
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) -L../libgpelaunch/.libs -lgpelaunch -Wl,-rpath-link,../libxsettings -L../libxsettings-client -lXsettings-client

PACKAGE_CFLAGS += $(shell pkg-config --cflags cairo)
PACKAGE_LDFLAGS += $(shell pkg-config --libs cairo)

DESTDIR=
PREFIX=/usr
BINDIR=$(PREFIX)/bin
ETCDIR=etc
INSTALL=install
STRIP=strip

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

OBJS=main.o row_view.o tab_view.o single_view.o xsettings.o

all: gpe-appmgr

gpe-appmgr: $(OBJS)
	$(CC) -o gpe-appmgr $(OBJS) $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all
	$(INSTALL) -D gpe-appmgr $(DESTDIR)/$(BINDIR)/gpe-appmgr
	$(STRIP) $(DESTDIR)/$(BINDIR)/gpe-appmgr
	mkdir -p $(DESTDIR)/$(PREFIX)/share/pixmaps
	$(INSTALL) -D dist/usr/share/pixmaps/*.png  $(DESTDIR)/$(PREFIX)/share/pixmaps

clean:
	rm -f *.o gpe-appmgr

distclean: clean
	rm -f *.ipk
	rm -rf familiar/dist

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
