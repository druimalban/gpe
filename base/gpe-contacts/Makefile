PREFIX = /usr/local
PACKAGE = gpe-contacts

ifeq ($(GTK2),yes)
GTKCFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_ENABLE_BROKEN
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`
else
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
GTKLDFLAGS = `gtk-config --libs`
endif

CPPFLAGS += -D_GNU_SOURCE

CFLAGS = $(GTKCFLAGS) -g -I../../libgpewidget -Wall `pkg-config --cflags xml2`
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

MEMBERS = callbacks db editing interface main structure support

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-contacts all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ -L../../libgpewidget -lgpewidget $(GTKLDFLAGS) `pkg-config --libs xml2` -g -lsqlite

clean:
	rm -f gpe-contacts $(OBJS) $(DEPS)

install: all install-mo
	install -D gpe-contacts $(DESTDIR)$(PREFIX)/bin/gpe-contacts
	strip $(DESTDIR)$(PREFIX)/bin/gpe-contacts

include ../../build/Makefile.dpkg_ipkg
include ../../build/Makefile.translation

-include $(DEPS)
