PREFIX = /usr/local

CFLAGS = `gtk-config --cflags` -g -I../libgpewidget -Wall
CFLAGS += -DPACKAGE=\"gpe-timesheet\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

gpe-timesheet: main.o sql.o
	$(CC) -o $@ $^ -L../libgpewidget -lgpewidget `gtk-config --libs` -lsqlite -g

install:
	install -D gpe-timesheet $(DESTDIR)$(PREFIX)/bin/gpe-timesheet
	install -D clock.png $(DESTDIR)$(PREFIX)/share/gpe-timesheet/pixmaps/clock.png
	install -D xclock.png $(DESTDIR)$(PREFIX)/share/gpe-timesheet/pixmaps/xclock.png

clean:
	rm -f gpe-timesheet main.o

