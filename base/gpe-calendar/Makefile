#DEBUG = yes

MEMBERS = main day_view week_view future_view event-ui about event-db year_view

CFLAGS = `gtk-config --cflags` -D_GNU_SOURCE -Wall -I../libgpewidget
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif

OBJS = $(patsubst %,%.o,$(MEMBERS))

all: gpe-calendar

gpe-calendar: $(OBJS)
	$(CC) -o $@ $^ `gtk-config --libs` -lgdk_imlib -lsqlite -L../libgpewidget -lgpewidget $(LDFLAGS)

install: all
	install -D gpe-calendar $(DESTDIR)/usr/bin/gpe-calendar
	install -D gpe-calendar.png $(DESTDIR)/usr/share/pixmaps/gpe-calendar.png
	install -D gpe-calendar.menu $(DESTDIR)/usr/lib/menu/gpe-calendar
	strip $(DESTDIR)/usr/bin/gpe-calendar

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control postinst dist/CONTROL
	make DESTDIR=`pwd`/dist install
	chown -R root.root dist
	ipkg-build dist

clean:
	rm -f gpe-calendar $(OBJS)
