VERSION = 0.06pre2

PACKAGE = gpe-go

ifndef PREFIX
PREFIX = /usr/local
endif

ifndef DEBUG
DEBUG = no
endif

ifndef CVSBUILD
CVSBUILD = yes
endif


LINGUAS = cs de pl ro sv nl fr pt ru sr

MEMBERS = gpe-go sgf sgf-handler model board

PIXMAPS = board white.png black.png

GTKCFLAGS  += `pkg-config --cflags gtk+-2.0` 
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CPPFLAGS += $(GTKCFLAGS)
CPPFLAGS += -DGTK_DISABLE_DEPRECATED
CPPFLAGS += -D_GNU_SOURCE 

ifeq ($(DEBUG),yes)
CFLAGS  += -O2 -g -DDEBUG=yes
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

ifdef DESKTOP #environement variable to set if you want extra code included.
CPPFLAGS += -DDESKTOP
endif

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(GTKLDFLAGS)

install-program: $(PACKAGE).desktop all 
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)
	for i in $(PIXMAPS); do \
	install -m 644 pixmaps/$$i   $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)/;\
	done

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS) $(PACKAGE).desktop
	find . -name "*~" -exec rm -f {} \;


#--include GPE generic Makefiles
ifeq ($(CVSBUILD),yes)
BUILD = ../../base/build
else
BUILD = build
endif

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation


-include $(DEPS)
