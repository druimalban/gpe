PREFIX = /usr/local
DEBUG = yes

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
	install -D gpe-edit.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-edit.png
	install -D gpe-edit.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-edit
	strip $(DESTDIR)$(PREFIX)/bin/gpe-edit

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control postinst dist/CONTROL
	make DESTDIR=`pwd`/dist install
	chown -R root.root dist
	ipkg-build dist

clean:
	rm -f gpe-edit $(OBJS)
