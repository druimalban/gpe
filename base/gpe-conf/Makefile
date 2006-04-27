PACKAGE = gpe-conf
VERSION = 0.1.31
CVSBUILD = yes
DEBUG = no
INSTALL = install
PREFIX = /usr/local
DESTDIR = /
PACKAGETOOL = no
STRIP = strip
MACHINE = unknown

CFLAGS += -MD

LINGUAS = pt ro de fr cs nl sv sk ru pt_BR zh_CN vi rw sl ko

EXTRA_DESKTOPS = \
	gpe-conf-admin.desktop.in \
	gpe-conf-cardinfo.desktop.in \
	gpe-conf-screen.desktop.in \
	gpe-conf-keys.desktop.in \
	gpe-conf-login-setup.desktop.in \
	gpe-conf-network.desktop.in \
	gpe-conf-ownerinfo.desktop.in \
	gpe-conf-serial.desktop.in \
	gpe-conf-sleep.desktop.in \
	gpe-conf-theme.desktop.in \
	gpe-conf-time.desktop.in \
	gpe-conf-users.desktop.in \
	gpe-conf-sysinfo.desktop.in \
	gpe-conf-usb.desktop.in \
	gpe-conf-sound.desktop.in

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` 
GTKLDFLAGS = `pkg-config --libs gtk+-2.0 gdk-2.0` 
XCFLAGS = `pkg-config --cflags x11`
XLIBS = `pkg-config --libs x11`
GPECFLAGS = `pkg-config --cflags libgpewidget`
GPELIBS = `pkg-config --libs libgpewidget`

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -I. 
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) $(GTKCFLAGS) -I. $(XCFLAGS)  
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) $(GTKLIBS) $(XLIBS) -lXsettings-client -lcrypt 

ifeq ($(DEBUG),yes)
PACKAGE_CFLAGS += -g -DDEBUG
else
PACKAGE_CFLAGS += -Os -fomit-frame-pointer
endif

PACKAGE_CFLAGS += -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\" -D_GNU_SOURCE -DMACHINE=\"$(MACHINE)\"
PACKAGE_CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"


MEMBERS  = main applets parser suid
MEMBERS += misc
MEMBERS += unimplemented
MEMBERS += timeanddate
MEMBERS += screen/main screen/brightness  screen/rotation screen/calibrate screen/callbacks screen/xset 
MEMBERS += keys/kbd
MEMBERS += network
MEMBERS += theme
MEMBERS += keys/keyctl
MEMBERS += ownerinfo
MEMBERS += login-setup
MEMBERS += sleep/main sleep/interface sleep/callbacks sleep/conf sleep/confGUI 
MEMBERS += users/interface users/callbacks  
MEMBERS += cfgfile
MEMBERS += gpe-admin
MEMBERS += widgets/sp-color-slider
MEMBERS += serial
MEMBERS += usb
MEMBERS += cardinfo
MEMBERS += tools/interface
MEMBERS += tasks
MEMBERS += users/passwd
MEMBERS += sound/sound
MEMBERS += sound/soundctrl
MEMBERS += sound/alarmctrl
MEMBERS += keys/keyboard
MEMBERS += keys/keys
MEMBERS += device

MEMBERS_INFO  = main-info
MEMBERS_INFO += storage
MEMBERS_INFO += logread
MEMBERS_INFO += tools/interface
MEMBERS_INFO += battery
MEMBERS_INFO += sysinfo
MEMBERS_INFO += parser

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES_CONF = $(patsubst %,%.c,$(MEMBERS))

OBJS_INFO = $(patsubst %,%.o,$(MEMBERS_INFO))
DEPS_INFO = $(patsubst %,%.d,$(MEMBERS_INFO))
SOURCES_INFO = $(patsubst %,%.c,$(MEMBERS_INFO))

SOURCES += $(SOURCES_CONF) $(SOURCES_INFO) 

PIXMAPS = gpe-config-apm.png
PIXMAPS += gpe-config-screen.png
PIXMAPS += gpe-config-kbd.png
PIXMAPS += gpe-config-keyctl.png
PIXMAPS += gpe-config-mixer.png
PIXMAPS += gpe-config-mouse.png
PIXMAPS += gpe-config-network.png
PIXMAPS += gpe-config-ownerinfo.png
PIXMAPS += gpe-config-screensaver.png
PIXMAPS += gpe-config-sleep.png
PIXMAPS += gpe-config-software.png
PIXMAPS += gpe-config-theme.png
PIXMAPS += gpe-config-time.png
PIXMAPS += gpe-config.png
PIXMAPS += gpe-config-users.png
PIXMAPS += gpe-config-login-setup.png
PIXMAPS += gpe-config-admin.png
PIXMAPS += gpe-config-storage.png
PIXMAPS += gpe-config-serial.png
PIXMAPS += gpe-config-cardinfo.png
PIXMAPS += pccard-cdrom.png pccard-ide.png pccard-memory.png
PIXMAPS += pccard-parport.png pccard-scsi.png pccard-unknown.png
PIXMAPS += pccard-network.png
PIXMAPS += pccard-usb.png
PIXMAPS += pccard-serial.png
PIXMAPS += gpe-config-battery.png
PIXMAPS += gpe-config-sysinfo.png
PIXMAPS += ibutton1.png
PIXMAPS += ibutton2.png
PIXMAPS += ibutton3.png
PIXMAPS += ibutton4.png
PIXMAPS += ibutton5.png
PIXMAPS += system-info.png
PIXMAPS += distribution.png
PIXMAPS += device-info.png
PIXMAPS += ipaq-s1.png
PIXMAPS += gpe-config-sound.png

all: $(PACKAGE) gpe-info

$(PACKAGE): $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS) $(PACKAGE_LDFLAGS)

gpe-info: $(OBJS_INFO)
	$(CC) -g -o $@ $(OBJS_INFO) $(LDFLAGS) $(PACKAGE_LDFLAGS)
	
install-program: all all-desktop
	mkdir -p $(DESTDIR)/CONTROL
	sed -e s/VERSION/$(VERSION)/ < familiar/control1 > $(DESTDIR)/CONTROL/control
	mkdir -p $(DESTDIR)/etc/gpe
	$(INSTALL) -m 644 familiar/gpe-config.conf $(DESTDIR)/etc/gpe/
	$(INSTALL) -m 644 data/key-commands $(DESTDIR)/etc/gpe/
	$(INSTALL) -m 644 data/key-layout $(DESTDIR)/etc/gpe/
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	$(INSTALL) -m 4755 $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/gpe-conf
	$(STRIP) $(DESTDIR)$(PREFIX)/bin/gpe-conf
	$(INSTALL) gpe-info $(DESTDIR)$(PREFIX)/bin/gpe-info
	$(STRIP) $(DESTDIR)$(PREFIX)/bin/gpe-info
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	for i in $(PIXMAPS); do install -m 644 -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/pixmaps/$$i; done
	mkdir -p  $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 644 *.desktop $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/share/gpe-conf
	$(INSTALL) -m 644 data/activate.wav $(DESTDIR)$(PREFIX)/share/gpe-conf/activate.wav
	$(INSTALL) -d $(DESTDIR)/etc/X11/Xsession.d
	$(INSTALL) -m755 sound/soundsettings.sh $(DESTDIR)/etc/X11/Xsession.d/55soundsettings

clean:
	rm -f $(PACKAGE) gpe-info $(OBJS) $(OBJS_INFO) *~ */*~ *.ipk *.upload.html *.asc *.desktop *.d
	rm -f screen/*.d pcmcia/*.d sleep/*.d widgets/*.d tools/*.d users/*.d sound/*.d ipaqscreen/*.d
	rm -f keys/*.d
	
	
	
	
include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation

-include $(DEPS)
