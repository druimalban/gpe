PREFIX = /usr/local
PACKAGE = teleport
DEBUG = yes
CVSBUILD = yes
VERSION = 0.33

LINGUAS = 

CPPFLAGS += -D_GNU_SOURCE

CFLAGS = $(GPECFLAGS) -Wall
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD
LDFLAGS += $(GPELIBS)

MEMBERS = teleport crypt displays keygen tp-keygen

ifeq ($(CVSBUILD),yes)
# temporary hack to work around lack of pkg-config for libdisplaymigration
CFLAGS += -I../libdisplaymigration
LDFLAGS += -L../libdisplaymigration
endif

CFLAGS += `libgcrypt-config --cflags`
LDFLAGS += -ldisplaymigration `libgcrypt-config --libs`

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

teleport: teleport.o crypt.o displays.o keygen.o
	$(CC) -o $@ $^ $(LDFLAGS) -lsqlite

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
