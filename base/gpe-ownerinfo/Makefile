PACKAGE = gpe-ownerinfo2
VERSION = 0.07

PREFIX = /usr/local
DEBUG = no
CVSBUILD = yes
GTK2=yes
LINGUAS = de

MEMBERS = gpe-ownerinfo2
CONTROL = control1

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget -I../librootimage
LDFLAGS += -L../libgpewidget -L../librootimage
BUILD = ../build
else
CFLAGS += -I/usr/include/gpe
BUILD = build
endif
LDFLAGS += -lgpewidget -lrootimage

ifeq ($(GTK2),yes)
GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`
CONTROL = control1
else
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
GTKLDFLAGS = `gtk-config --libs`
CONTROL = control
endif

CPPFLAGS = $(GTKCFLAGS) -D_GNU_SOURCE 
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

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^  $(GTKLDFLAGS) $(LDFLAGS)

install: all 
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	strip $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -d $(DESTDIR)$(PREFIX)/lib/menu
	install -d $(DESTDIR)/etc/gpe/

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

distclean: clean
	rm -rf familiar/dist/

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg

-include $(DEPS)
