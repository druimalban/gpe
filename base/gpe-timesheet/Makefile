CFLAGS = `gtk-config --cflags` -g -I../libgpewidget -Wall

gpe-timesheet: main.o
	$(CC) -o $@ $^ -L../libgpewidget -lgpewidget `gtk-config --libs` -g

install:
	install -D gpe-timesheet $(DESTDIR)/usr/bin/gpe-timesheet
	install -D clock.png $(DESTDIR)/usr/share/gpe-timesheet/pixmaps/clock.png
	install -D xclock.png $(DESTDIR)/usr/share/gpe-timesheet/pixmaps/xclock.png

clean:
	rm -f gpe-timesheet main.o

