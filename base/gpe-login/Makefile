PREFIX = /usr/local
PACKAGE = gpe-login
CVSBUILD = yes
GTK2 = yes
VERSION = 0.61
DEBUG = no

LINGUAS = de sv wa nl pt

GTKLDFLAGS = `pkg-config --libs gtk+-2.0`
GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
CONTROL_GPE_LOGIN = control1

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif

CFLAGS += $(GTKCFLAGS) -Wall
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\" -D_GNU_SOURCE -I../gpe-ownerinfo

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
endif
LDFLAGS += -lgpewidget
LDFLAGS_OWNERINFO = -L../gpe-ownerinfo -lgpe-ownerinfo

MEMBERS = gpe-login

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

EXTRA_DESKTOPS = gpe-logout.desktop.in

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

export PKG_CONFIG_PATH = $(BUILD)

all: gpe-login gpe-lock-display

gpe-login: gpe-login.o nocursor.o
	$(CC) -o $@ $^ $(LDFLAGS) $(GTKLDFLAGS) $(LDFLAGS_OWNERINFO) -lcrypt

gpe-lock-display: gpe-lock-display.o
	$(CC) -o $@ $^ `pkg-config --libs x11`

install: install-scripts

install-program: gpe-login gpe-lock-display
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s gpe-login $(DESTDIR)$(PREFIX)/bin/gpe-login
	install -s gpe-lock-display $(DESTDIR)$(PREFIX)/bin/gpe-lock-display
	install -d $(DESTDIR)/etc/X11
	install -m 755 gpe-login.setup $(DESTDIR)/etc/X11/gpe-login.setup
	install -m 644 gpe-login.keylaunchrc $(DESTDIR)/etc/X11/gpe-login.keylaunchrc
	install -m 644 X11/gpe-login.gtkrc $(DESTDIR)/etc/X11/gpe-login.gtkrc
	install -m 755 X11/gpe-login.pre-session $(DESTDIR)/etc/X11/gpe-login.pre-session
	install -d $(DESTDIR)/etc/suspend-scripts
	install -m 755 gpe-login.suspend $(DESTDIR)/etc/suspend-scripts/S98lock-display
	install -d $(DESTDIR)/etc/X11/Xsession.d
	install -m 755 gpe-login.session $(DESTDIR)/etc/X11/Xsession.d/50autolock
	install -d $(DESTDIR)/etc/X11/Xinit.d
	install -m 755 gpe-login.xinit $(DESTDIR)/etc/X11/Xinit.d/99gpe-login
	install -d $(DESTDIR)/etc/gpe
	install -m 644 locale.alias $(DESTDIR)/etc/gpe/
	install -d $(DESTDIR)/etc/sysconfig
	install -m 644 gpe-login.sysconfig $(DESTDIR)/etc/sysconfig/gpelogin

install-scripts: gpe-logout.desktop
	for i in X11/Xsession.d X11/Xinit.d; do install -d $(DESTDIR)/etc/$$i; FILES=`echo $$i/* | sed "s:$$i/CVS::"`; install -m 755 $$FILES $(DESTDIR)/etc/$$i/; done
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 gpe-xcalibrate.sh $(DESTDIR)$(PREFIX)/bin/gpe-xcalibrate.sh
	install -d $(DESTDIR)/etc/gpe
	install -m 644 X11/xsettings.default $(DESTDIR)/etc/gpe/
	install -m 644 standard.mbdock $(DESTDIR)/etc/gpe/gpe.mbdock
	install -m 644 X11/Xdefaults $(DESTDIR)/etc/X11/
	mkdir -p familiar/dist.gpe-session-scripts/usr/share/pixmaps
	install -m 644 gpe-logout.png familiar/dist.gpe-session-scripts/usr/share/pixmaps
	mkdir -p familiar/dist.gpe-session-scripts/usr/share/applications
	install -m 644 gpe-logout.desktop familiar/dist.gpe-session-scripts/usr/share/applications
	mkdir -p familiar/dist.gpe-session-scripts/usr/bin
	install gpe-logout.sh familiar/dist.gpe-session-scripts/usr/bin/gpe-logout
	install gpe-auto-bluetooth.sh familiar/dist.gpe-session-scripts/usr/bin/gpe-auto-bluetooth

scripts-ipkg:
	rm -rf familiar/dist.gpe-session-scripts
	mkdir -p familiar/dist.gpe-session-scripts/CONTROL
	cp familiar/control.gpe-session-scripts familiar/dist.gpe-session-scripts/CONTROL/control
	install familiar/postinst.gpe-session-scripts familiar/dist.gpe-session-scripts/CONTROL/postinst
	make DESTDIR=`pwd`/familiar/dist.gpe-session-scripts PREFIX=/usr install-scripts
	chown -R root.root familiar/dist.gpe-session-scripts
	ipkg-build familiar/dist.gpe-session-scripts
	if [ "x$(LINGUAS)" != "x" ]; then make translation-ipkg; fi

ipkg: scripts-ipkg

clean: clean-po
	rm -f gpe-login gpe-login.o gpe-lock-display gpe-lock-display.o nocursor.o

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
-include $(DEPS)
