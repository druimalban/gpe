PREFIX = /usr/local
PACKAGE = teleport

LINGUAS = 

GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CPPFLAGS += -D_GNU_SOURCE

CFLAGS = $(GTKCFLAGS) -g -I../libgpewidget -Wall -I../libdm
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

MEMBERS = teleport

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: teleport all-mo

teleport: $(OBJS)
	$(CC) -o $@ $^ -L../libgpewidget -lgpewidget $(GTKLDFLAGS) $(LDFLAGS)

clean:
	rm -f teleport $(OBJS) $(DEPS)

install: all install-mo
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s teleport $(DESTDIR)$(PREFIX)/bin/

include ../build/Makefile.dpkg_ipkg
include ../build/Makefile.translation

-include $(DEPS)
