CFLAGS = `gtk-config --cflags` -g -I../libgpewidget -Wall

gpe-todo: main.o todo-ui.o todo-sql.o list-ui.o
	$(CC) -o $@ $^ `gtk-config --libs` -lgdk_imlib -g -lsqlite -L../libgpewidget -lgpewidget

clean:
	rm -f gpe-todo main.o todo-ui.o todo-sql.o

all: gpe-todo

install: all
	install -D gpe-todo $(DESTDIR)/usr/bin/gpe-todo
	install -D gpe-todo.menu $(DESTDIR)/usr/lib/menu/gpe-todo
	install -D gpe-todo.png $(DESTDIR)/usr/share/pixmaps/gpe-todo.png
	strip $(DESTDIR)/usr/bin/gpe-todo

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control postinst dist/CONTROL
	make DESTDIR=`pwd`/dist install
	chown -R root.root dist
	ipkg-build dist

