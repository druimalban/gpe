PREFIX = /usr/local
PACKAGE = gpe-timesheet
CVSBUILD = yes

ifeq ($(GTK2),yes)
GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`
else
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
GTKLDFLAGS = `gtk-config --libs`
endif

ifeq ($(CVSBUILD),yes)
LGPEWIDGET = -L../libgpewidget
endif

ifeq ($(DEBUG), yes)
CFLAGS += -g
LDFLAGS += -g
endif

LGPEWIDGET += -lgpewidget 

CFLAGS += -MD -Wall $(GTKCFLAGS)

CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

MEMBERS = main sql

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

gpe-timesheet: $(OBJS)
	$(CC) -o $@ $^ $(LGPEWIDGET) $(GTKLDFLAGS) -lsqlite $(LDFLAGS)

install:
	strip gpe-timesheet
	install -D gpe-timesheet $(DESTDIR)$(PREFIX)/bin/gpe-timesheet
	install -D -m 644 clock.png $(DESTDIR)$(PREFIX)/share/gpe-timesheet/pixmaps/clock.png
	install -D -m 644 xclock.png $(DESTDIR)$(PREFIX)/share/gpe-timesheet/pixmaps/xclock.png

upstream: clean
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; cd ..; cp -a $(PACKAGE) $(PACKAGE)-$$VERSION.orig; tar czf $(PACKAGE)_$$VERSION.orig.tar.gz --exclude=$(PACKAGE)-$$VERSION.orig/debian --exclude=$(PACKAGE)-$$VERSION.orig/CVS --exclude=$(PACKAGE)-$$VERSION.orig/familiar $(PACKAGE)-$$VERSION.orig ; rm -rf $(PACKAGE)-$$VERSION.orig )

dpkg: upstream
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; TMP=/var/tmp/debian/$(PACKAGE); PV=$(PACKAGE)-$$VERSION; rm -rf $$TMP; mkdir -p $$TMP; cp ../$(PACKAGE)_$$VERSION.orig.tar.gz $$TMP; (cd $$TMP; tar xzf $(PACKAGE)_$$VERSION.orig.tar.gz ; mv $$PV.orig $$PV ); mkdir -p $$TMP/$$PV/debian; cp debian/* $$TMP/$$PV/debian; cd $$TMP/$$PV; fakeroot dpkg-buildpackage  ; cd ..; rm -rf $$PV)

ipkg:
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/control familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist

clean:
	rm -f gpe-timesheet $(OBJS)

-include $(DEPS)
