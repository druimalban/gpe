PREFIX = /usr/local
SYSCONFDIR = /etc
PACKAGE = gpe-bluetooth
DEBUG = no
CVSBUILD = yes
GTK2 = yes
VERSION = 0.33

GLADE = bluez-pin-gpe.glade

MEMBERS = main dun sdp pan lap rfcomm pin-ui dbus pin-dbus headset obexserver obex-vcard obex-vcal obex-unknown obexclient obex-glib obexclient-dbus obexclient-file

CFLAGS += -MD `pkg-config --cflags gtk+-2.0` `pkg-config --cflags dbus-glib-1` `pkg-config --cflags libglade-2.0` -Wall `pkg-config --cflags mimedir-1.0` -I../libgpevtype

CPPFLAGS += -D_GNU_SOURCE -DDBUS_API_SUBJECT_TO_CHANGE -DGPE_BLUETOOTH

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g # -lefence
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
LDFLAGS += -lgpewidget `pkg-config --libs gtk+-2.0` -lbluetooth -lsdp `pkg-config --libs gthread-2.0` `pkg-config --libs dbus-glib-1` `pkg-config --libs libglade-2.0` -lopenobex `pkg-config --libs mimedir-1.0` -L../libgpevtype -lgpevtype -lsqlite

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))

all: $(PACKAGE) all-mo bnep-helper rfcomm-helper

pin-ui.c: ../bluez-pin/pin-ui.c
	rm -f $@
	cp $< $@
	chmod a-w $@

pin-ui.h: ../bluez-pin/pin-ui.h
	rm -f $@
	cp $< $@
	chmod a-w $@

pin-dbus.c: ../bluez-pin/pin-dbus.c
	rm -f $@
	cp $< $@
	chmod a-w $@

$(GLADE): ../bluez-pin/$(GLADE)
	rm -f $@
	cp $< $@
	chmod a-w $@

bluez.conf: ../bluez-pin/bluez.conf
	rm -f $@
	cp $< $@
	chmod a-w $@

extract-po: pin-ui.c pin-dbus.c

$(PACKAGE): pin-ui.h $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

bnep-helper: bnep-helper.c

rfcomm-helper: rfcomm-helper.c

install-program: all $(GLADE) bluez.conf
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
	mkdir -p $(DESTDIR)$(PREFIX)/share/bluez-pin/
	install -m 644 $(GLADE) $(DESTDIR)$(PREFIX)/share/bluez-pin/
	mkdir -p $(DESTDIR)$(SYSCONFDIR)/dbus-1/system.d/
	install -m 644 bluez.conf $(DESTDIR)$(SYSCONFDIR)/dbus-1/system.d/

clean:
	rm -f $(PACKAGE) $(OBJS) pin-ui.c pin-dbus.c pin-ui.h

include $(BUILD)/Makefile.translation-auto-linguas
include $(BUILD)/Makefile.dpkg_ipkg

-include $(DEPS)

