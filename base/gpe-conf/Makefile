PACKAGE = gpe-conf
VERSION = 0.1.14
NATIVE = yes
CVSBUILD = yes
DEBUG = no
INSTALL = install
PREFIX = /usr/local
DESTDIR = /
PACKAGETOOL = no
STRIP = strip
MACHINE = unknown


LINGUAS = pt ro de fr cs nl sv sk ru

ifeq ($(MACHINE),h3600)
PACKAGE_CPPFLAGS += -DMACH_IPAQ
PACKAGE_CFLAGS += -DMACH_IPAQ
endif

ifeq ($(MACHINE),h3900)
PACKAGE_CPPFLAGS += -DMACH_IPAQ -DMACH_IPAQPXA
PACKAGE_CFLAGS += -DMACH_IPAQ -DMACH_IPAQPXA
endif

ifeq ($(MACHINE),simpad)
PACKAGE_CPPFLAGS += -DMACH_SIMPAD
PACKAGE_CFLAGS += -DMACH_SIMPAD
endif

EXTRA_DESKTOPS = \
	gpe-conf-admin.desktop.in \
	gpe-conf-cardinfo.desktop.in \
	gpe-conf-ipaqscreen.desktop.in \
	gpe-conf-kbd.desktop.in \
	gpe-conf-keyctl.desktop.in \
	gpe-conf-login-setup.desktop.in \
	gpe-conf-network.desktop.in \
	gpe-conf-ownerinfo.desktop.in \
	gpe-conf-serial.desktop.in \
	gpe-conf-sleep.desktop.in \
	gpe-conf-theme.desktop.in \
	gpe-conf-time.desktop.in \
	gpe-conf-users.desktop.in \
	gpe-conf-sysinfo.desktop.in	

ifeq ($(CVSBUILD),yes)
PACKAGE_CPPFLAGS += -I../libxsettings -I../libxsettings-client
PACKAGE_LDFLAGS += -L../libxsettings -L../libxsettings-client
BUILD = ../build
else
BUILD = build
endif

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` 
GTKLDFLAGS = `pkg-config --libs gtk+-2.0 gdk-2.0` 

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -I. 
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GTKCFLAGS) $(GPECFLAGS) -I.
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GTKLDFLAGS) $(GPELIBS) -lXsettings -lXsettings-client -lcrypt -L/usr/X11R6/lib -lXrandr 

#Uncomment this if you use the old GTK2 crosstoolchain
#PACKAGE_LDFLAGS +=  /skiff/local/arm-linux/lib/X11/libXinerama.a

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
MEMBERS += ipaqscreen/main ipaqscreen/brightness  ipaqscreen/rotation ipaqscreen/calibrate ipaqscreen/callbacks ipaqscreen/xset 
MEMBERS += kbd
MEMBERS += network
MEMBERS += theme
MEMBERS += keyctl
MEMBERS += ownerinfo
MEMBERS += login-setup
MEMBERS += sleep/main sleep/interface sleep/callbacks sleep/conf sleep/confGUI 
MEMBERS += users/interface users/callbacks  
MEMBERS += cfgfile
MEMBERS += gpe-admin
MEMBERS += widgets/sp-color-slider
MEMBERS += serial
MEMBERS += cardinfo
MEMBERS += tools/interface
MEMBERS += tasks

MEMBERS_INFO  = main-info
MEMBERS_INFO += storage
MEMBERS_INFO += logread
MEMBERS_INFO += tools/interface
MEMBERS_INFO += battery
MEMBERS_INFO += sysinfo
MEMBERS_INFO += parser


ifeq ($(PACKAGETOOL),yes)
EXTRA_DESKTOP += gpe-conf-packages.desktop.in 
EXTRA_DESKTOP += gpe-conf-keyboard.desktop.in
MEMBERS += packages keyboard
PACKAGE_CFLAGS += -D PACKAGETOOL
endif

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES_CONF = $(patsubst %,%.c,$(MEMBERS))

OBJS_INFO = $(patsubst %,%.o,$(MEMBERS_INFO))
DEPS_INFO = $(patsubst %,%.d,$(MEMBERS_INFO))
SOURCES_INFO = $(patsubst %,%.c,$(MEMBERS_INFO))

SOURCES += $(SOURCES_CONF) $(SOURCES_INFO) 

PIXMAPS = gpe-config-apm.png
PIXMAPS += gpe-config-ipaqscreen.png
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
PIXMAPS += gpe-config-keyboard.png
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

ifeq ($(PACKAGETOOL),yes)
PIXMAPS += gpe-config-packages.png
endif


all: $(PACKAGE) gpe-info

$(PACKAGE): $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS) $(PACKAGE_LDFLAGS)
	cp $(PACKAGE) gpe-conf.pc
	$(STRIP) $(PACKAGE)

gpe-info: $(OBJS_INFO)
	$(CC) -g -o $@ $(OBJS_INFO) $(LDFLAGS) $(PACKAGE_LDFLAGS)
	$(STRIP) gpe-info
	
install-program: all all-desktop
	mkdir -p $(DESTDIR)/CONTROL
	sed -e s/VERSION/$(VERSION)/ < familiar/control1 > $(DESTDIR)/CONTROL/control
	mkdir -p $(DESTDIR)/etc/gpe
	$(INSTALL) -m 644 familiar/gpe-config.conf $(DESTDIR)/etc/gpe/
	$(INSTALL) -m 644 keyboards.def $(DESTDIR)/etc/gpe/
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	$(INSTALL) -m 4755 $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/gpe-conf
	$(STRIP) $(DESTDIR)$(PREFIX)/bin/gpe-conf
	$(INSTALL) gpe-info $(DESTDIR)$(PREFIX)/bin/gpe-info
	$(STRIP) $(DESTDIR)$(PREFIX)/bin/gpe-info
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	for i in $(PIXMAPS); do install -m 644 -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/pixmaps/$$i; done
	mkdir -p  $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default
	$(INSTALL) -m 644 pixmaps/ipaq.png $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/ipaq.png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 644 *.desktop $(DESTDIR)$(PREFIX)/share/applications

clean:
	rm -f $(PACKAGE) gpe-info gpe-conf.pc $(OBJS) $(OBJS_INFO) *~ */*~ *.ipk *.o sleep/*.o *.upload.html *.asc *.desktop

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
