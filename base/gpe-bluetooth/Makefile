PREFIX = /usr/local
SYSCONFDIR = /etc
PACKAGE = gpe-bluetooth
DEBUG = no
CVSBUILD = yes
GTK2 = yes
VERSION = 0.36

GLADE = bluez-pin-gpe.glade

MEMBERS = main dun sdp pan lap rfcomm pin-ui dbus pin-dbus headset obexserver obex-vcard obex-vcal obex-unknown obexclient obex-glib obexclient-dbus

ifeq ($(CVSBUILD),yes)
LIBGPEVTYPE_PC = libgpevtype-uninstalled
LIBGPEVTYPE_PC_EXTRA=PKG_CONFIG_PATH=../../base/libgpevtype
else
LIBGPEVTYPE_PC = libgpevtype
endif

PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) `pkg-config --cflags dbus-glib-1` `pkg-config --cflags libglade-2.0` `pkg-config --cflags mimedir-1.0` `$(LIBGPEVTYPE_PC_EXTRA) pkg-config --cflags $(LIBGPEVTYPE_PC)`
PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -DDBUS_API_SUBJECT_TO_CHANGE -DGPE_BLUETOOTH
PACKAGE_CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
PACKAGE_LDFLAGS += $(GPELIBS) -lbluetooth -lsdp `pkg-config --libs gthread-2.0` `pkg-config --libs dbus-glib-1` `pkg-config --libs libglade-2.0` -lopenobex `pkg-config --libs mimedir-1.0` `$(LIBGPEVTYPE_PC_EXTRA) pkg-config --libs $(LIBGPEVTYPE_PC)` -lsqlite

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS)) bnep-helper.d rfcomm-helper.d

all: $(PACKAGE) all-mo bnep-helper rfcomm-helper

ifeq ($(CVSBUILD),yes)
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
endif

extract-po: pin-ui.c pin-dbus.c

$(PACKAGE): pin-ui.h $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(PACKAGE_LDFLAGS)

bnep-helper: bnep-helper.o
	$(CC) -o $@ $< $(LDFLAGS) $(STANDARD_LDFLAGS) -lbluetooth

rfcomm-helper: rfcomm-helper.o
	$(CC) -o $@ $< $(LDFLAGS) $(STANDARD_LDFLAGS) -lbluetooth

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
	rm -f $(PACKAGE) $(OBJS) $(DEPS) bnep-helper rfcomm-helper bnep-helper.o rfcomm-helper.o

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg

-include $(DEPS)
