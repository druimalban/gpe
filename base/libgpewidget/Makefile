PREFIX = /usr/local
PACKAGE = libgpewidget
DEBUG = yes

GTKRC = gtkrc

MEMBERS = gtkdatecombo errorbox init pixmaps smallbox gtkminifilesel render dirbrowser picturebutton what stylus gtkdatesel screensize question about

ifeq ($(GTK2),yes)
MEMBERS += gpetooltips2 gtkgpepixmap2
SONAME = libgpewidget.so.1
CONTROL = control1
else
MEMBERS += gpetooltips gtkgpepixmap
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

HEADERS = init.h render.h errorbox.h smallbox.h pixmaps.h gtkminifilesel.h gtkdatecombo.h gtkdatesel.h dirbrowser.h stylus.h picturebutton.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: libgpewidget.so

update-po: po/$(PACKAGE).pot

po/$(PACKAGE).pot: $(SOURCES)
	xgettext -k_ -o $@ $^

libgpewidget.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS)

libgpewidget.a: $(OBJS)
	ar cq $@ $^

.c.s:;
	$(CC) $(CFLAGS) -S $<

upstream: clean
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; cd ..; cp -a libgpewidget libgpewidget-$$VERSION.orig; tar czf libgpewidget_$$VERSION.orig.tar.gz --exclude=libgpewidget-$$VERSION.orig/debian --exclude=libgpewidget-$$VERSION.orig/CVS --exclude=libgpewidget-$$VERSION.orig/familiar libgpewidget-$$VERSION.orig ; rm -rf libgpewidget-$$VERSION.orig )

dpkg: upstream
	( VERSION=`dpkg-parsechangelog | grep ^Version | cut -d\  -f2 | cut -d- -f1`; TMP=/var/tmp/debian/$(PACKAGE); PV=$(PACKAGE)-$$VERSION; rm -rf $$TMP; mkdir -p $$TMP; cp ../$(PACKAGE)_$$VERSION.orig.tar.gz $$TMP; (cd $$TMP; tar xzf $(PACKAGE)_$$VERSION.orig.tar.gz ; mv $$PV.orig $$PV ); mkdir -p $$TMP/$$PV/debian; cp debian/* $$TMP/$$PV/debian; cd $$TMP/$$PV; fakeroot dpkg-buildpackage -sa ; cd ..; rm -rf $$PV)

install: $(SONAME)
	install -D $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)
	strip $(DESTDIR)$(PREFIX)/lib/$(SONAME)
	install -D $(GTKRC) $(DESTDIR)$(PREFIX)/share/gpe/$(GTKRC)

install-devel:
	for i in $(HEADERS); do install -m 644 -D $$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/libgpewidget.so

ipkg:
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/$(CONTROL) familiar/dist/CONTROL/control
	cp familiar/postinst familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist

clean:
	rm -f $(OBJS) $(SONAME) libgpewidget.so $(DEPS)

-include $(DEPS)
