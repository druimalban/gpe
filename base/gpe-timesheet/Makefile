PREFIX = /usr/local
PACKAGE = gpe-timesheet
CVSBUILD = yes
VERSION = 0.11
GTK2 = yes
LINGUAS = id

ifeq ($(GTK2),yes)
GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`
else
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
GTKLDFLAGS = `gtk-config --libs`
endif

ifeq ($(CVSBUILD),yes)
LGPEWIDGET = -L../libgpewidget
endif

ifeq ($(DEBUG), yes)
CFLAGS += -g
LDFLAGS += -g
endif

LGPEWIDGET += -lgpewidget 

CFLAGS += -MD -Wall $(GTKCFLAGS)

CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

MEMBERS = main sql

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: gpe-timesheet

gpe-timesheet: $(OBJS)
	$(CC) -o $@ $^ $(LGPEWIDGET) $(GTKLDFLAGS) -lsqlite $(LDFLAGS)

install: all install-mo gpe-timesheet.desktop
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s gpe-timesheet $(DESTDIR)$(PREFIX)/bin/gpe-timesheet
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 gpe-timesheet.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-timesheet.png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 gpe-timesheet.desktop $(DESTDIR)$(PREFIX)/share/applications/gpe-timesheet

clean:
	rm -f gpe-timesheet $(OBJS) $(DEPS)

-include $(DEPS)

include ../build/Makefile.dpkg_ipkg
include ../build/Makefile.translation

