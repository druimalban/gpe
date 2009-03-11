PREFIX = /usr
PACKAGE = gpe-watch
DEBUG = no
CVSBUILD = yes
GST = yes
VERSION = 0.11

MEMBERS = gpe-watch

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_DISABLE_DEPRECATED
GTKLDFLAGS += -L/skiff/local/arm-linux/lib/X11 `pkg-config --libs gtk+-2.0` -lX11 -lXrender -lXext -lXinerama

CFLAGS = $(GTKCFLAGS) -DUSE_GPE -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
BUILD = ../build
else
CFLAGS += -I/usr/include/gpe
BUILD = build
endif
LDFLAGS += -lgpewidget $(GTKLDFLAGS)

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS) $(LIB)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all $(PACKAGE).desktop
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	#mkdir -p $(DESTDIR)$(PREFIX)/share/$(PACKAGE)
	#install -m 644 pixmaps/*.png $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	rm -f $(PACKAGE) $(OBJS) $(PACKAGE).desktop *.ipk
	rm -rf familiar/dist

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
