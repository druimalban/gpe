PREFIX = /usr/local
DEBUG = yes

MEMBERS = main day_view week_view future_view event-ui event-db year_view month_view day_popup

CPPFLAGS = `gtk-config --cflags` -D_GNU_SOURCE -I../libgpewidget
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -Wall
CPPFLAGS += -DPACKAGE=\"gpe-calendar\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

.SUFFIXES: .d

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

PIXMAPS = close.xpm config.xpm dayview.xpm futureview.xpm monthview.xpm new.xpm save.xpm today.xpm weekview.xpm yearview.xpm exit.xpm

all: gpe-calendar mo

mo:
	make -C po

install-po:
	make -C po install DESTDIR=$(DESTDIR) PREFIX=$(PREFIX)

update-po: po/gpe-calendar.pot

po/gpe-calendar.pot: $(SOURCES)
	xgettext -k_ -o po/gpe-calendar.pot $^

gpe-calendar: $(OBJS)
	$(CC) -o $@ $^ `gtk-config --libs` -lsqlite -L../libgpewidget -lgpewidget $(LDFLAGS)

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
	rm -f gpe-calendar $(OBJS) $(DEPS)

-include $(DEPS)
