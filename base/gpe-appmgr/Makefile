CVSBUILD = yes
DEBUG = no

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) -I../libdotdesktop -I../libxsettings -I../libxsettings-client
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) -L../libdotdesktop -ldotdesktop -L../libxsettings -L../libxsettings-client -lXsettings-client -lXsettings

DESTDIR=
PREFIX=/usr
BINDIR=$(PREFIX)/bin
ETCDIR=etc

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

OBJS=main.o xsi.o popupmenu.o package.o properties.o gnome-exec.o tray.o xsettings.o row_view.o #tab_view.o

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
