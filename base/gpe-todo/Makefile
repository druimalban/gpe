PREFIX = /usr/local
PACKAGE = gpe-todo
GTK2 = yes
VERSION = 0.33

LINGUAS = de

ifeq ($(GTK2),yes)
GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`
else
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
GTKLDFLAGS = `gtk-config --libs`
endif

CPPFLAGS += -D_GNU_SOURCE

CFLAGS = $(GTKCFLAGS) -g -I../libgpewidget -Wall -I../libdm -I../libtododb
CFLAGS += -DPACKAGE=\"gpe-todo\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

MEMBERS = main todo-ui list-ui configure

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-todo all-mo

gpe-todo: $(OBJS)
	$(CC) -o $@ $^ -L../libgpewidget -lgpewidget $(GTKLDFLAGS) -g -lsqlite -L../libdm -ldm -L../libtododb -ltododb

clean:
	rm -f gpe-todo $(OBJS) $(DEPS)

all: gpe-todo

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s gpe-todo $(DESTDIR)$(PREFIX)/bin/gpe-todo
	mkdir -p $(DESTDIR)$(PREFIX)/lib/menu
	install -m 644 gpe-todo.menu $(DESTDIR)$(PREFIX)/lib/menu/gpe-todo
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 gpe-todo.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-todo.png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)

include ../build/Makefile.dpkg_ipkg
include ../build/Makefile.translation

-include $(DEPS)
