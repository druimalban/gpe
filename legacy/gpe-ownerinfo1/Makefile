PACKAGE = gpe-ownerinfo
PREFIX = /usr/local
DEBUG = no
CVSBUILD = yes
LINGUAS = de pt_PT

MEMBERS = src/callbacks  src/interface  src/main  src/support

CFLAGS += -I../libgpewidget -I../librootimage
LDFLAGS += -L../libgpewidget -L../librootimage
LDFLAGS += -lgpewidget -lrootimage

ifeq ($(GTK2),yes)
GTKCFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_ENABLE_BROKEN
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

include ../../base/build/Makefile.dpkg_ipkg
-include $(DEPS)
