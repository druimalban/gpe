PREFIX = /usr/local
PACKAGE = gpe-edit
#DEBUG = yes

MEMBERS = main

CFLAGS = `gtk-config --cflags` -D_GNU_SOURCE -Wall -I../libgpewidget
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif

OBJS = $(patsubst %,%.o,$(MEMBERS))

all: gpe-edit

gpe-edit: $(OBJS)
	$(CC) -o $@ $^ `gtk-config --libs` -L../libgpewidget -lgpewidget $(LDFLAGS)

install: all
	install -D gpe-edit $(DESTDIR)$(PREFIX)/bin/gpe-edit
	install -D -m 644 gpe-edit.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-edit.png
	install -D -m 644 gpe-edit.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-edit
	strip $(DESTDIR)$(PREFIX)/bin/gpe-edit

ipkg:
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/control postinst familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist

clean:
	rm -f gpe-edit $(OBJS)
