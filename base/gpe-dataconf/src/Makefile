PREFIX = /usr/local
PACKAGE = gpe-dataconf

DIR_PO := ../po

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_ENABLE_BROKEN
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CPPFLAGS += -D_GNU_SOURCE

CFLAGS = $(GTKCFLAGS) -g -I../../libgpewidget -I../include -Wall 
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

MEMBERS = callbacks interface main support db questionfx

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-dataconf all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ -L../../libgpewidget -lgpewidget $(GTKLDFLAGS) -lsqlite

clean:
	rm -f gpe-dataconf $(OBJS) $(DEPS)

install: all install-mo
	install -D gpe-dataconf $(DESTDIR)$(PREFIX)/bin/gpe-dataconf
	strip $(DESTDIR)$(PREFIX)/bin/gpe-dataconf

include ../../build/Makefile.dpkg_ipkg
include ../../build/Makefile.translation

-include $(DEPS)
