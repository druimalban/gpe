MEMBERS = main gtkdatesel day_view week_view future_view event-ui about
CFLAGS = `gtk-config --cflags` -Os -Wall -fomit-frame-pointer

OBJS = $(patsubst %,%.o,$(MEMBERS))

gpe-calendar: $(OBJS)
	$(CC) -o $@ $^ `gtk-config --libs` -lgdk_imlib

install:
	install -D gpe-calendar $(DESTDIR)/usr/bin/gpe-calendar
	strip $(DESTDIR)/usr/bin/gpe-calendar

ipkg:
	make DESTDIR=`pwd`/dist install
	ipkg-build dist

clean:
	rm -f gpe-calendar $(OBJS)
