CVSBUILD = yes
DEBUG = yes
PACKAGE = gpe-appmgr
VERSION = 2.2

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) -I../libgpelaunch
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) -lgpelaunch

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

OBJS=main.o row_view.o tab_view.o single_view.o

all: gpe-appmgr

gpe-appmgr: $(OBJS)
	$(CC) -o gpe-appmgr $(OBJS) $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all
	$(INSTALL) -D gpe-appmgr $(DESTDIR)/$(BINDIR)/gpe-appmgr
	$(STRIP) $(DESTDIR)/$(BINDIR)/gpe-appmgr
	mkdir -p $(DESTDIR)/$(PREFIX)/share/pixmaps
	$(INSTALL) -D dist/usr/share/pixmaps/*.png  $(DESTDIR)/$(PREFIX)/share/pixmaps
	$(INSTALL) -D appmgr.session $(DESTDIR)/etc/X11/Xsession.d/90GpeAppMgr

clean:
	rm -f *.o gpe-appmgr

distclean: clean
	rm -f *.ipk
	rm -rf familiar/dist

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
