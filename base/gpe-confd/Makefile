PREFIX = /usr/local
PACKAGE = gpe-confd
DEBUG = no

MEMBERS = main xsettings-manager

GTKCFLAGS = `pkg-config --cflags glib-2.0`
GTKLDFLAGS += `pkg-config --libs glib-2.0`

CPPFLAGS += -D_GNU_SOURCE

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

LDFLAGS += -lsqlite $(GTKLDFLAGS) -L../libxsettings -lXsettings
CPPFLAGS += $(GTKCFLAGS) -I../libxsettings

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: $(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)/etc/X11/Xsession.d
	install $(PACKAGE).xs $(DESTDIR)/etc/X11/Xsession.d/70gpe-confd

clean:
	rm -f $(PACKAGE)
	rm -f $(OBJS) $(DEPS)

include ../build/Makefile.translation
include ../build/Makefile.dpkg_ipkg
-include $(DEPS)
