PREFIX = /usr/local
PACKAGE = gpe-bluetooth
DEBUG = no
CVSBUILD = yes
GTK2 = yes

MEMBERS = main

CFLAGS = `pkg-config --cflags gtk+-2.0`
CONTROL = control1

CFLAGS += -D_GNU_SOURCE -Wall

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget
LDFLAGS += -L../libgpewidget
else
CFLAGS += -I/usr/include/gpe
endif
LDFLAGS += -lgpewidget `pkg-config --libs gtk+-2.0` -lbluetooth -lsdp -lpthread

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE) all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: install-program install-mo

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth
	install -m 644 bt-on.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	install -m 644 bt-off.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	install -m 644 cellphone.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	install -m 644 network.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/

clean:
	rm -f $(PACKAGE) $(OBJS)

include ../build/Makefile.translation
include ../build/Makefile.dpkg_ipkg
