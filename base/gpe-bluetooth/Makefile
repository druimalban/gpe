PREFIX = /usr/local
PACKAGE = gpe-bluetooth
DEBUG = no
CVSBUILD = yes
GTK2 = yes
VERSION = 0.25

MEMBERS = main dun sdp pan lap rfcomm pin-ui dbus pin-dbus

CFLAGS = `pkg-config --cflags gtk+-2.0` `pkg-config --cflags dbus-glib-1` -Wall

CPPFLAGS += -D_GNU_SOURCE -DDBUS_API_SUBJECT_TO_CHANGE -DGPE_BLUETOOTH

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
LDFLAGS += -lgpewidget `pkg-config --libs gtk+-2.0` -lbluetooth -lsdp `pkg-config --libs gthread-2.0` `pkg-config --libs dbus-glib-1`

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE) all-mo bnep-helper rfcomm-helper

pin-ui.c: ../bluez-pin/pin-ui.c
	cp $< $@
	chmod a-w $@

pin-dbus.c: ../bluez-pin/pin-dbus.c
	cp $< $@
	chmod a-w $@

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

bnep-helper: bnep-helper.c

rfcomm-helper: rfcomm-helper.c

install-program: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/lib/gpe-bluetooth
	install -s bnep-helper $(DESTDIR)$(PREFIX)/lib/gpe-bluetooth/
	chmod u+s $(DESTDIR)$(PREFIX)/lib/gpe-bluetooth/bnep-helper
	install -s rfcomm-helper $(DESTDIR)$(PREFIX)/lib/gpe-bluetooth/
	chmod u+s $(DESTDIR)$(PREFIX)/lib/gpe-bluetooth/rfcomm-helper
	install -d $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth
	install -m 644 pixmaps/bt-on.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	install -m 644 pixmaps/bt-off.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	install -m 644 pixmaps/cellphone.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	install -m 644 pixmaps/network.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	install -m 644 pixmaps/Computer.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	install -m 644 pixmaps/Printer.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/bluetooth/
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)/etc/bluetooth/
	install hciattach.sh $(DESTDIR)/etc/bluetooth/hciattach
	install -d $(DESTDIR)/etc/ppp/peers/
	install -m 644 peers/lap $(DESTDIR)/etc/ppp/peers/lap

clean:
	rm -f $(PACKAGE) $(OBJS) pin-ui.c pin-dbus.c

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
