PACKAGE = gpe-calendar
PREFIX = /usr/local
DEBUG = no
CVSBUILD = yes
VERSION = 0.48

LINGUAS = id pt de fr zh_TW ro wa sv nl cs

MEMBERS = main day_view week_view future_view event-ui month_view day_popup xsettings gtkdatesel

CPPFLAGS = -D_GNU_SOURCE -I../libeventdb -I../libschedule

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g 
else
CFLAGS += -Os -fomit-frame-pointer
CPPFLAGS += -DG_DISABLE_CAST_CHECKS
endif
CFLAGS += -Wall $(GPECFLAGS) -I../libxsettings -I../libxsettings-client -I../libdisplaymigration
LDFLAGS += $(GPELDFLAGS) -L../libxsettings -L../libxsettings-client -lXsettings-client -L../libdisplaymigration -ldisplaymigration -Wl,-rpath-link,../libxsettings
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
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -D $(PACKAGE).session $(DESTDIR)/etc/X11/Xsession.d/99$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS) $(PACKAGE).desktop

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
