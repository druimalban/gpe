PREFIX = /usr/local
PACKAGE = gpe-bluetooth
DEBUG = no
CVSBUILD = yes
GTK2 = yes
VERSION = 0.21

MEMBERS = main dun sdp pan lap rfcomm

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
BUILD = ../build
else
CFLAGS += -I/usr/include/gpe
BUILD = build
endif
LDFLAGS += -lgpewidget `pkg-config --libs gtk+-2.0` -lbluetooth -lsdp `pkg-config --libs gthread-2.0`

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE) all-mo bnep-helper rfcomm-helper

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

bnep-helper: bnep-helper.c

rfcomm-helper: rfcomm-helper.c

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/lib/gpe-bluetooth
	install -s bnep-helper $(DESTDIR)$(PREFIX)/lib/gpe-bluetooth/
	chmod u+s $(DESTDIR)$(PREFIX)/lib/gpe-bluetooth/bnep-helper
	install -s rfcomm-helper $(DESTDIR)$(PREFIX)/lib/gpe-bluetooth/
	chmod u+s $(DESTDIR)$(PREFIX)/lib/gpe-bluetooth/rfcomm-helper
	mkdir -p $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth
	install -m 644 pixmaps/bt-on.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	install -m 644 pixmaps/bt-off.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	install -m 644 pixmaps/cellphone.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	install -m 644 pixmaps/network.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	install -m 644 pixmaps/Computer.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)/etc/bluetooth/
	install hciattach.sh $(DESTDIR)/etc/bluetooth/hciattach
	install -d $(DESTDIR)/etc/ppp/peers/
	install -m 644 peers/lap $(DESTDIR)/etc/ppp/peers/lap

clean:
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
