VERSION = 1.0.4

PREFIX = /usr/local
PACKAGE = gpe-soundbite
DEBUG = yes
CVSBUILD = yes
ESD = yes

MEMBERS = main gsm-codec
ifeq ($(ESD),yes)
MEMBERS += esd
else
MEMBERS += oss
endif

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -DENABLE_NLS
PACKAGE_CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) `pkg-config --cflags libglade-2.0`
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) -lgsm `pkg-config --libs libglade-2.0`

ifeq ($(ESD),yes)
PACKAGE_LDFLAGS += -lesd
endif

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all $(PACKAGE)-play.desktop  $(PACKAGE)-record.desktop
	install -Ds $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -D $(PACKAGE)-play $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)-play
	install -D $(PACKAGE)-record $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)-record
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE)-play.desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)-play.desktop
	install -m 644 $(PACKAGE)-record.desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)-record.desktop
	install -D -m 644 gpe-soundbite-play.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-soundbite-play.png
	install -D -m 644 gpe-soundbite-record.png $(DESTDIR)$(PREFIX)/share/pixmaps/gpe-soundbite-record.png
	install -m 644 $(PACKAGE).glade $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/gpe-soundbite.glade

clean:
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
