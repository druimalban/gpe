PREFIX = /usr/local
PACKAGE = gpe-todo

ifeq ($(GTK2),yes)
GTKCFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_ENABLE_BROKEN
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`
else
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
GTKLDFLAGS = `gtk-config --libs`
endif

CFLAGS = $(GTKCFLAGS) -g -I../libgpewidget -Wall
CFLAGS += -DPACKAGE=\"gpe-todo\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

MEMBERS = main todo-ui todo-sql list-ui configure

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-todo mo

mo:
	make -C po

install-po:
	make -C po install DESTDIR=$(DESTDIR) PREFIX=$(PREFIX)

gpe-todo: $(OBJS)
	$(CC) -o $@ $^ -L../libgpewidget -lgpewidget $(GTKLDFLAGS) -g -lsqlite

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

include ../build/Makefile.dpkg_ipkg

-include $(DEPS)
