DEBUG = yes

MEMBERS = main gtkdatesel day_view week_view future_view event-ui about event-db year_view

CFLAGS = `gtk-config --cflags` -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g
else
CFLAGS += -Os -fomit-frame-pointer
endif

OBJS = $(patsubst %,%.o,$(MEMBERS))

gpe-calendar: $(OBJS)
	$(CC) -o $@ $^ `gtk-config --libs` -lgdk_imlib -lsqlite $(LDFLAGS)

install:
	install -D gpe-calendar $(DESTDIR)/usr/bin/gpe-calendar
	strip $(DESTDIR)/usr/bin/gpe-calendar

ipkg:
	make DESTDIR=`pwd`/dist install
	ipkg-build dist

clean:
	rm -f gpe-calendar $(OBJS)
