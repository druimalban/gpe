PREFIX = /usr/local
PACKAGE = teleport
DEBUG = yes
CVSBUILD = yes
VERSION = 0.31.1

LINGUAS = 

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_DISABLE_DEPRECATED
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CPPFLAGS += -D_GNU_SOURCE

CFLAGS = $(GTKCFLAGS) -I../libgpewidget -Wall
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

MEMBERS = teleport crypt displays keygen

CFLAGS += -I../libdisplaymigration `libgcrypt-config --cflags`
LDFLAGS += -L../libdisplaymigration -ldisplaymigration `libgcrypt-config --libs`

ifeq ($(DEBUG),yes)
CFLAGS += -O -g
LDFLAGS += -g
else
CFLAGS += -Os -fomit-frame-pointer
LDFLAGS += 
endif

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: teleport all-mo tp-keygen

teleport: $(OBJS)
	$(CC) -o $@ $^ -L../libgpewidget -lgpewidget $(GTKLDFLAGS) $(LDFLAGS) -lsqlite

tp-keygen: tp-keygen.o keygen.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f teleport tp-keygen tp-keygen.o $(OBJS) $(DEPS)

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s teleport $(DESTDIR)$(PREFIX)/bin/
	install -s tp-keygen $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 teleport.png $(DESTDIR)$(PREFIX)/share/pixmaps
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 teleport.desktop $(DESTDIR)$(PREFIX)/share/applications

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
