VERSION = 0.04

PACKAGE = gpe-go

ifndef PREFIX
PREFIX = /usr
endif

ifndef DEBUG
DEBUG = no
endif

LINGUAS = #cs
EXTRA_DESKTOPS = \
    gpe-go-9.desktop.in \
    gpe-go-13.desktop.in \
    gpe-go-19.desktop.in

MEMBERS = gpe-go

PIXMAPS = board white.png black.png

GTKCFLAGS   = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CPPFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE 
ifeq ($(DEBUG),yes)
CFLAGS  += -O2 -g
LDFLAGS += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -Wall -I../../base/libgpewidget
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

LDFLAGS += -L../../base/libgpewidget -lgpewidget

.SUFFIXES: .d

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))
EXTRA_DESKTOPS_RM = $(patsubst %.in,%,$(EXTRA_DESKTOPS))

ifdef DESKTOP #environement variable to set if you want extra code included.
CPPFLAGS += -DDESKTOP
endif

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^  $(GTKLDFLAGS) $(LDFLAGS)

install-program: $(EXTRA_DESKTOPS_RM) all #Later: depends only on $(PACKAGE).desktop
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE)-9.desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -m 644 $(PACKAGE)-13.desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -m 644 $(PACKAGE)-19.desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)
	for i in $(PIXMAPS); do \
	install -m 644 pixmaps/$$i   $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)/;\
	done

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS) $(PACKAGE).desktop $(EXTRA_DESKTOPS_RM)
	find . -name "*~" -exec rm -f {} \;


#--include GPE generic Makefiles
ifndef CVSBUILD
CVSBUILD = no
endif

ifeq ($(CVSBUILD),yes)
BUILD = ../../base/build
else
BUILD = build
endif

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation


-include $(DEPS)
