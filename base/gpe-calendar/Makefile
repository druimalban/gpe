PACKAGE = gpe-calendar
PREFIX = /usr/local
DEBUG = yes

LINGUAS = de pt_PT

MEMBERS = main day_view week_view future_view event-ui event-db year_view month_view day_popup old-db

CPPFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags` -D_GNU_SOURCE -I../libgpewidget
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -Wall
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

.SUFFIXES: .d

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

PIXMAPS = close.xpm config.xpm dayview.xpm futureview.xpm monthview.xpm new.xpm save.xpm today.xpm weekview.xpm yearview.xpm exit.xpm

all: $(PACKAGE) all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ -L../libgpewidget -lgpewidget `gtk-config --libs` -lsqlite $(LDFLAGS)

install: all install-mo
	for i in $(PIXMAPS); do install -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/pixmaps/$$i; done
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -D $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	install -D $(PACKAGE).menu $(DESTDIR)$(PREFIX)/lib/menu/$(PACKAGE)
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include ../build/Makefile.translation
include ../build/Makefile.dpkg_ipkg
-include $(DEPS)
