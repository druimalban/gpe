PACKAGE = gpe-calendar
PREFIX = /usr/local
DEBUG = no
CVSBUILD = yes
VERSION = 0.55

LINGUAS = id pt de fr zh_TW ro wa sv nl cs es sk sr ru hu


ifeq ($(CVSBUILD),yes)
LIBGPEVTYPE_PC = libgpevtype-uninstalled
LIBGPEVTYPE_PC_EXTRA=PKG_CONFIG_PATH=../../base/libgpevtype
PACKAGE_CPPFLAGS += -I../libeventdb -I../libschedule -I../libxsettings -I../libxsettings-client -I../libdisplaymigration -I../libgpepimc
PACKAGE_LDFLAGS += -L../libxsettings -L../libxsettings-client -Wl,-rpath-link,../libxsettings -L../libdisplaymigration -L../libeventdb -L../libschedule -L../libgpepimc
else
LIBGPEVTYPE_PC = libgpevtype
endif
PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) `pkg-config --cflags mimedir-1.0` `$(LIBGPEVTYPE_PC_EXTRA) pkg-config --cflags $(LIBGPEVTYPE_PC)`
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) -lXsettings-client -ldisplaymigration -leventdb -lschedule -lgpepimc `pkg-config --libs mimedir-1.0` `$(LIBGPEVTYPE_PC_EXTRA) pkg-config --libs $(LIBGPEVTYPE_PC)`

MEMBERS = main day_view week_view future_view event-ui month_view day_popup xsettings gtkdatesel import-vcal

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
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

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
