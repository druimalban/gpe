PREFIX = /usr/local
PACKAGE = gpe-question
CVSBUILD = yes
VERSION = 0.02
GTK2 = yes

ifeq ($(GTK2),yes)
GTKLDFLAGS = `pkg-config --libs gtk+-2.0`
GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
else
GTKLDFLAGS = `gtk-config --libs`
GTKCFLAGS = `gtk-config --cflags` `gdk-pixbuf-config --cflags`
endif

CFLAGS = $(GTKCFLAGS) -Os -Wall -fomit-frame-pointer
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
else
CFLAGS += -I/usr/include/gpe
endif
LDFLAGS += -lgpewidget

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE)

$(PACKAGE): $(PACKAGE).o
	$(CC) -o $@ $^ $(LDFLAGS) $(GTKLDFLAGS) -lcrypt

install-program: all
	install -Ds $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE).o $(PACKAGE)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
