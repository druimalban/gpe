PREFIX = /usr/local
DEBUG = yes

MEMBERS = main day_view week_view future_view event-ui about event-db year_view month_view day_popup

CFLAGS = `gtk-config --cflags` -D_GNU_SOURCE -Wall -I../libgpewidget
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -DPACKAGE=\"gpe-calendar\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))

PIXMAPS = close.xpm config.xpm dayview.xpm futureview.xpm monthview.xpm new.xpm save.xpm today.xpm weekview.xpm yearview.xpm

all: gpe-calendar mo

mo:
	make -C po

install-po:
	make -C po install DESTDIR=$(DESTDIR) PREFIX=$(PREFIX)

gpe-calendar: $(OBJS)
	$(CC) -o $@ $^ `gtk-config --libs` -lgdk_imlib -lsqlite -L../libgpewidget -lgpewidget $(LDFLAGS)

install: all install-po
	for i in $(PIXMAPS); do install -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/gpe-calendar/pixmaps/$$i; done
	install -D gpe-calendar $(DESTDIR)$(PREFIX)/bin/gpe-calendar
	install -D gpe-calendar.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-calendar.png
	install -D gpe-calendar.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-calendar
	strip $(DESTDIR)$(PREFIX)/bin/gpe-calendar

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control postinst dist/CONTROL
	make DESTDIR=`pwd`/dist PREFIX=/usr install
	chown -R root.root dist
	ipkg-build dist

clean:
	rm -f gpe-calendar $(OBJS)
