VERSION = 0.03

PACKAGE = gpe-go
PREFIX = /usr/local
DEBUG = no
LINGUAS = 

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
SRCS = $(patsubst %,%.c,$(MEMBERS))

ifdef DESKTOP #environement variable to set if you want extra code included.
CPPFLAGS += -DDESKTOP
endif

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^  $(GTKLDFLAGS) $(LDFLAGS)

install-program: all 
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
	rm -f $(PACKAGE) $(OBJS) $(DEPS)
	find . -name "*~" -exec rm -f {} \;

include ../../base/build/Makefile.dpkg_ipkg
-include $(DEPS)
