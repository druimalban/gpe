PREFIX = /usr/local

CFLAGS = `gtk-config --cflags` -g -I../libgpewidget -Wall
CFLAGS += -DPACKAGE=\"gpe-todo\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

all: gpe-todo mo

mo:
	make -C po

install-po:
	make -C po install DESTDIR=$(DESTDIR) PREFIX=$(PREFIX)

gpe-todo: main.o todo-ui.o todo-sql.o list-ui.o
	$(CC) -o $@ $^ `gtk-config --libs` -lgdk_imlib -g -lsqlite -L../libgpewidget -lgpewidget

clean:
	rm -f gpe-todo main.o todo-ui.o todo-sql.o

all: gpe-todo

install: all install-po
	install -D gpe-todo $(DESTDIR)$(PREFIX)/bin/gpe-todo
	install -D gpe-todo.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-todo
	install -D gpe-todo.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-todo.png
	install -D new.png $(DESTDIR)$(PREFIX)/share/gpe-todo/pixmaps/new.png
	install -D trash.png $(DESTDIR)$(PREFIX)/share/gpe-todo/pixmaps/trash.png
	install -D preferences.png $(DESTDIR)$(PREFIX)/share/gpe-todo/pixmaps/preferences.png
	strip $(DESTDIR)$(PREFIX)/bin/gpe-todo

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control postinst dist/CONTROL
	make DESTDIR=`pwd`/dist PREFIX=/usr install
	chown -R root.root dist
	ipkg-build dist

