PREFIX = /usr/local
PACKAGE = gpe-login
CVSBUILD = yes
GTK2 = yes
VERSION = 0.43

ifeq ($(GTK2),yes)
GTKLDFLAGS = `pkg-config --libs gtk+-2.0`
GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
CONTROL_GPE_LOGIN = control.gpe-login1
else
GTKLDFLAGS = `gtk-config --libs`
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
CONTROL_GPE_LOGIN = control.gpe-login
endif

CFLAGS = $(GTKCFLAGS) -Os -Wall -fomit-frame-pointer -I../librootimage
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget -I../librootimage
LDFLAGS += -L../libgpewidget -L../librootimage
else
CFLAGS += -I/usr/include/gpe
endif
LDFLAGS += -lgpewidget
LDFLAGS_ROOT = -lrootimage

MEMBERS = gpe-login

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

EXTRA_DESKTOPS = gpe-logout.desktop.in

all: gpe-login gpe-lock-display mo

mo:
	make -C po

install-po: mo
	make -C po install DESTDIR=$(DESTDIR) PREFIX=$(PREFIX)

update-po: po/$(PACKAGE).pot

po/$(PACKAGE).pot: $(SOURCES)
	xgettext -k_ -o po/$(PACKAGE).pot $^

gpe-login: gpe-login.o nocursor.o
	$(CC) -o $@ $^ $(LDFLAGS) $(GTKLDFLAGS) $(LDFLAGS_ROOT) -lcrypt

gpe-lock-display: gpe-lock-display.o
	$(CC) -o $@ $^ -L/usr/X11R6/lib -lX11

install: install-login

install-login: gpe-login install-po
	install -D gpe-login $(DESTDIR)$(PREFIX)/bin/gpe-login
	install -D gpe-lock-display $(DESTDIR)$(PREFIX)/bin/gpe-lock-display
	strip $(DESTDIR)$(PREFIX)/bin/gpe-login
	strip $(DESTDIR)$(PREFIX)/bin/gpe-lock-display
	install -d $(DESTDIR)/etc/X11
	install -m 755 gpe-login.setup $(DESTDIR)/etc/X11/gpe-login.setup
	install -m 644 gpe-login.keylaunchrc $(DESTDIR)/etc/X11/gpe-login.keylaunchrc
	install -m 644 X11/gpe-login.gtkrc $(DESTDIR)/etc/X11/gpe-login.gtkrc
	install -m 755 X11/gpe-login.pre-session $(DESTDIR)/etc/X11/gpe-login.pre-session
	install -d $(DESTDIR)/etc/suspend-scripts
	install -m 755 gpe-login.suspend $(DESTDIR)/etc/suspend-scripts/S98lock-display
	install -d $(DESTDIR)/etc/X11/Xsession.d
	install -m 755 gpe-login.session $(DESTDIR)/etc/X11/Xsession.d/50autolock
	install -m 755 X11/10unsetgtkrc $(DESTDIR)/etc/X11/Xsession.d/10unsetgtkrc
	install -d $(DESTDIR)/etc/X11/Xinit.d
	install -m 755 gpe-login.xinit $(DESTDIR)/etc/X11/Xinit.d/99gpe-login

install-scripts: gpe-logout.desktop
	for i in X11/Xsession-gpe.d X11/Xinit.d; do install -d $(DESTDIR)/etc/$$i; FILES=`echo $$i/* | sed "s:$$i/CVS::"`; install -m 755 $$FILES $(DESTDIR)/etc/$$i/; done
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 gpe-xcalibrate.sh $(DESTDIR)$(PREFIX)/bin/gpe-xcalibrate.sh
	install -d $(DESTDIR)/etc/gpe
	install -m 644 X11/xsettings.default $(DESTDIR)/etc/gpe/
	install -m 644 X11/Xdefaults $(DESTDIR)/etc/X11/
	mkdir -p familiar/dist.gpe-session-scripts/usr/share/pixmaps
	install -m 644 gpe-logout.png familiar/dist.gpe-session-scripts/usr/share/pixmaps
	mkdir -p familiar/dist.gpe-session-scripts/usr/share/applications
	install -m 644 gpe-logout.desktop familiar/dist.gpe-session-scripts/usr/share/applications
	mkdir -p familiar/dist.gpe-session-scripts/usr/bin
	install gpe-logout.sh familiar/dist.gpe-session-scripts/usr/bin/gpe-logout

ipkg:
	make PREFIX=/usr clean all
	rm -rf familiar/dist.gpe-login
	mkdir -p familiar/dist.gpe-login/CONTROL
	cp familiar/$(CONTROL_GPE_LOGIN) familiar/dist.gpe-login/CONTROL/control
	cp familiar/postinst.gpe-login familiar/dist.gpe-login/CONTROL/postinst
	cp familiar/postrm.gpe-login familiar/dist.gpe-login/CONTROL/postrm
	make DESTDIR=`pwd`/familiar/dist.gpe-login PREFIX=/usr install-login
	chown -R root.root familiar/dist.gpe-login
	ipkg-build familiar/dist.gpe-login
	#
	rm -rf familiar/dist.gpe-session-scripts
	mkdir -p familiar/dist.gpe-session-scripts/CONTROL
	cp familiar/control.gpe-session-scripts familiar/dist.gpe-session-scripts/CONTROL/control
	make DESTDIR=`pwd`/familiar/dist.gpe-session-scripts PREFIX=/usr install-scripts
	chown -R root.root familiar/dist.gpe-session-scripts
	ipkg-build familiar/dist.gpe-session-scripts

clean: clean-po
	rm -f gpe-login gpe-login.o gpe-lock-display gpe-lock-display.o nocursor.o

include ../build/Makefile.translation
-include $(DEPS)
