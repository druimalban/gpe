PREFIX = /usr/local
PACKAGE = libgpewidget
DEBUG = no
GTK2 = yes

GTKRC = gtkrc

LINGUAS = en_AU

MEMBERS = gtkdatecombo errorbox init pixmaps smallbox gtkminifilesel render dirbrowser picturebutton what stylus gtkdatesel screensize question gtksimplemenu spacing tray tap

ifeq ($(GTK2),yes)
MEMBERS += gpetooltips2 gtkgpepixmap2 gpe-iconlist
SONAME = libgpewidget.so.1
CONTROL = control1
else
MEMBERS += gpetooltips gtkgpepixmap about
SONAME = libgpewidget.so.0
CONTROL = control
endif

ifeq ($(GTK2),yes)
CFLAGS = `pkg-config --cflags gtk+-2.0`
LDFLAGS = `pkg-config --libs gtk+-2.0`
else
CFLAGS = `gdk-pixbuf-config --cflags` `gtk-config --cflags`
LDFLAGS =`gtk-config --libs` `gdk-pixbuf-config --libs` 
endif
CFLAGS += -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PACKAGE)\" -D_GNU_SOURCE -Wall -fPIC
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -MD

CPPFLAGS += -I./gpe

HEADERS = init.h render.h errorbox.h smallbox.h pixmaps.h gtkminifilesel.h gtkdatecombo.h gtkdatesel.h dirbrowser.h stylus.h picturebutton.h spacing.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: libgpewidget.so

libgpewidget.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS)

gpe-iconlist.c: gpeiconlist.gob
	gob2 --no-private-header gpeiconlist.gob
	mv -f gpe-iconlist.h gpe/

libgpewidget.a: $(OBJS)
	ar cq $@ $^

.c.s:;
	$(CC) $(CFLAGS) -S $<

upstream: clean
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; cd ..; cp -a libgpewidget libgpewidget-$$VERSION.orig; tar czf libgpewidget_$$VERSION.orig.tar.gz --exclude=libgpewidget-$$VERSION.orig/debian --exclude=libgpewidget-$$VERSION.orig/CVS --exclude=libgpewidget-$$VERSION.orig/familiar libgpewidget-$$VERSION.orig ; rm -rf libgpewidget-$$VERSION.orig )

dpkg: upstream
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; TMP=/var/tmp/debian/$(PACKAGE); PV=$(PACKAGE)-$$VERSION; rm -rf $$TMP; mkdir -p $$TMP; cp ../$(PACKAGE)_$$VERSION.orig.tar.gz $$TMP; (cd $$TMP; tar xzf $(PACKAGE)_$$VERSION.orig.tar.gz ; mv $$PV.orig $$PV ); mkdir -p $$TMP/$$PV/debian; cp debian/* $$TMP/$$PV/debian; cd $$TMP/$$PV; fakeroot dpkg-buildpackage -sa ; cd ..; rm -rf $$PV)

install: $(SONAME) install-mo
	install -D $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)
	strip $(DESTDIR)$(PREFIX)/lib/$(SONAME)
ifneq ($(GTK2),yes)
	install -D $(GTKRC) $(DESTDIR)$(PREFIX)/share/gpe/$(GTKRC)
endif

install-devel:
	for i in $(HEADERS); do install -m 644 -D $$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/libgpewidget.so

ipkg: clean
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/$(CONTROL) familiar/dist/CONTROL/control
	cp familiar/postinst familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr DEBUG=no install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist

clean:
	rm -f $(OBJS) $(SONAME) libgpewidget.so $(DEPS)

include ../build/Makefile.translation

-include $(DEPS)
