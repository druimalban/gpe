PACKAGE = gpe-calendar
PREFIX = /usr/local
DEBUG = no
GTK2 = yes
CVSBUILD = yes
VERSION = 0.43

LINGUAS = id pt de fr zh_TW ro wa

MEMBERS = main day_view week_view future_view event-ui month_view day_popup xsettings gtkdatesel

ifeq ($(GTK2),yes)
GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`
else
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
GTKLDFLAGS = `gtk-config --libs`
endif

CPPFLAGS = -D_GNU_SOURCE -I../libgpewidget -I../libeventdb -I../libschedule
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g 
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -Wall $(GTKCFLAGS) -I../libxsettings -I../libxsettings-client -I../libdm
LDFLAGS += $(GTKLDFLAGS) -L../libxsettings -L../libxsettings-client -lXsettings-client -L../libdm -ldm -Wl,-rpath-link,../libxsettings
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

.SUFFIXES: .d

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ -L../libgpewidget -L../libeventdb -lgpewidget -L../libeventdb -leventdb -L../libschedule -lschedule -lsqlite $(LDFLAGS)

install-program: all $(PACKAGE).desktop
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	mkdir -p $(DESTDIR)$(PREFIX)/lib/menu
	install -m 644 $(PACKAGE).menu $(DESTDIR)$(PREFIX)/lib/menu/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -D $(PACKAGE).session $(DESTDIR)/etc/X11/Xsession.d/99$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS) $(PACKAGE).desktop

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
