PREFIX = /usr/local
PACKAGE = gpe-todo

CFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags` -g -I../libgpewidget -Wall
CFLAGS += -DPACKAGE=\"gpe-todo\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

MEMBERS = main todo-ui todo-sql list-ui

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-todo mo

mo:
	make -C po

install-po:
	make -C po install DESTDIR=$(DESTDIR) PREFIX=$(PREFIX)

gpe-todo: $(OBJS)
	$(CC) -o $@ $^ -L../libgpewidget -lgpewidget `gtk-config --libs` -g -lsqlite

update-po: po/$(PACKAGE).pot

po/$(PACKAGE).pot: $(SOURCES)
	xgettext -k_ -o po/$(PACKAGE).pot $^

clean:
	rm -f gpe-todo $(OBJS) $(DEPS)

all: gpe-todo

install: all install-po
	install -D gpe-todo $(DESTDIR)$(PREFIX)/bin/gpe-todo
	install -D gpe-todo.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-todo
	install -D gpe-todo.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-todo.png
	strip $(DESTDIR)$(PREFIX)/bin/gpe-todo

ipkg:
	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	cp familiar/control familiar/postinst familiar/dist/CONTROL
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	chown -R root.root familiar/dist
	ipkg-build familiar/dist

-include $(DEPS)
