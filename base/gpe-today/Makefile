PACKAGE = gpe-today
PREFIX = /usr/local
#DEBUG = yes 
#LINGUAS = pt_PT

MEMBERS = main todo calendar date

GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CPPFLAGS = -I../libgpewidget -I../libeventdb -I../librootimage -D_GNU_SOURCE

ifeq ($(DEBUG),yes)
CFLAGS = -g
#LDFLAGS = -lefence
else
CFLAGS = -Os -fomit-frame-pointer
endif

CFLAGS += -Wall $(GTKCFLAGS)
LDFLAGS += $(GTKLDFLAGS) -L../libeventdb -leventdb -L../librootimage -lrootimage
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
#CFLAGS += -MD

.SUFFIXES: .d

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

PIXMAPS = calendar.png date.png

all: $(PACKAGE) all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ -L../libgpewidget -lgpewidget -lsqlite $(LDFLAGS)

install: all install-mo
	for i in $(PIXMAPS); do install -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/pixmaps/$$i; done
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -D $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	install -D $(PACKAGE).menu $(DESTDIR)$(PREFIX)/lib/menu/$(PACKAGE)
	install -m644 -D gtkrc $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/gtkrc
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS) *~

include ../build/Makefile.translation
include ../build/Makefile.dpkg_ipkg
-include $(DEPS)
