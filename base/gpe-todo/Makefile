CFLAGS = `gtk-config --cflags` -g -I../libgpewidget -Wall

gpe-todo: main.o todo-ui.o todo-sql.o list-ui.o
	$(CC) -o $@ $^ `gtk-config --libs` -g -lsqlite -L../libgpewidget -lgpewidget

clean:
	rm -f gpe-todo main.o todo-ui.o todo-sql.o

all: gpe-todo

install: all
	install -D gpe-todo $(DESTDIR)/usr/bin/gpe-todo
	strip $(DESTDIR)/usr/bin/gpe-todo

ipkg:
	mkdir -p dist/CONTROL
	cp control postinst dist/CONTROL
	make DESTDIR=`pwd`/dist install
	ipkg-build dist

