PREFIX = /usr/local
PACKAGE = teleport
DEBUG = yes

LINGUAS = 

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` -DGTK_DISABLE_DEPRECATED
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CPPFLAGS += -D_GNU_SOURCE

CFLAGS = $(GTKCFLAGS) -I../libgpewidget -Wall -I../libdm
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

MEMBERS = teleport crypt displays keygen

CFLAGS += -I../libdm `libgcrypt-config --cflags`
LDFLAGS += -L../libdm -ldm `libgcrypt-config --libs`

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

all: teleport all-mo tp-keygen

teleport: $(OBJS)
	$(CC) -o $@ $^ -L../libgpewidget -lgpewidget $(GTKLDFLAGS) $(LDFLAGS) -lsqlite

tp-keygen: tp-keygen.o keygen.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f teleport tp-keygen tp-keygen.o $(OBJS) $(DEPS)

install: all install-mo
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s teleport $(DESTDIR)$(PREFIX)/bin/
	install -s tp-keygen $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 teleport.png $(DESTDIR)$(PREFIX)/share/pixmaps
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 teleport.desktop $(DESTDIR)$(PREFIX)/share/applications

include ../build/Makefile.dpkg_ipkg
include ../build/Makefile.translation

-include $(DEPS)
