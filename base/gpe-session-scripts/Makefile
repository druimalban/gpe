PREFIX = /usr/local
PACKAGE = gpe-session-scripts
CVSBUILD = yes
VERSION = 0.62
DEBUG = no

LINGUAS = 

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS)
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS)

EXTRA_DESKTOPS = gpe-logout.desktop.in

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

export PKG_CONFIG_PATH = $(BUILD)

all:

install-program: gpe-logout.desktop
	for i in X11/Xsession.d X11/Xinit.d; do install -d $(DESTDIR)/etc/$$i; FILES=`echo $$i/* | sed "s:$$i/CVS::"`; install -m 755 $$FILES $(DESTDIR)/etc/$$i/; done
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 gpe-xcalibrate.sh $(DESTDIR)$(PREFIX)/bin/gpe-xcalibrate.sh
	install -d $(DESTDIR)/etc/gpe
	install -m 644 X11/xsettings.default $(DESTDIR)/etc/gpe/
	install -m 644 standard.mbdock $(DESTDIR)/etc/gpe/gpe.mbdock
	install -m 644 X11/Xdefaults $(DESTDIR)/etc/X11/
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 gpe-logout.png $(DESTDIR)$(PREFIX)/share/pixmaps
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 gpe-logout.desktop $(DESTDIR)$(PREFIX)/share/applications
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install gpe-logout.sh $(DESTDIR)$(PREFIX)/bin/gpe-logout
	install gpe-auto-bluetooth.sh $(DESTDIR)$(PREFIX)/bin/gpe-auto-bluetooth

clean: clean-po

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
