PREFIX = /usr/local
PACKAGE = gpe-login
CVSBUILD = yes
GTK2 = yes
VERSION = 0.76
DEBUG = no
LINGUAS = 

CONTROL_GPE_LOGIN = control1

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS)
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS)

ifeq ($(CVSBUILD),yes)
PACKAGE_CPPFLAGS += -I../gpe-ownerinfo
PACKAGE_LDFLAGS_OWNERINFO += -L../gpe-ownerinfo
endif

PACKAGE_LDFLAGS_OWNERINFO += -lgpe-ownerinfo

MEMBERS = gpe-login

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

EXTRA_DESKTOPS = gpe-logout.desktop.in

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

export PKG_CONFIG_PATH = $(BUILD)

all: gpe-login gpe-lock-display

gpe-login: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS) $(PACKAGE_LDFLAGS_OWNERINFO) -lcrypt

gpe-lock-display: gpe-lock-display.o
	$(CC) -o $@ $^ $(LDFLAGS) `pkg-config --libs x11`

install-program: gpe-login gpe-lock-display
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s gpe-login $(DESTDIR)$(PREFIX)/bin/gpe-login
	install -s gpe-lock-display $(DESTDIR)$(PREFIX)/bin/gpe-lock-display
	install -d $(DESTDIR)/etc/X11
	install -m 755 gpe-login.setup $(DESTDIR)/etc/X11/gpe-login.setup
	install -m 644 gpe-login.keylaunchrc $(DESTDIR)/etc/X11/gpe-login.keylaunchrc
	install -m 644 X11/gpe-login.gtkrc $(DESTDIR)/etc/X11/gpe-login.gtkrc
	install -m 755 X11/gpe-login.pre-session $(DESTDIR)/etc/X11/gpe-login.pre-session
	install -d $(DESTDIR)/etc/apm/resume.d
	install -m 644 gpe-login.suspend $(DESTDIR)/etc/apm/resume.d/S98lock-display
	install -d $(DESTDIR)/etc/X11/Xsession.d
	install -m 755 gpe-login.session $(DESTDIR)/etc/X11/Xsession.d/50autolock
	install -d $(DESTDIR)/etc/X11/Xinit.d
	install -m 755 gpe-login.xinit $(DESTDIR)/etc/X11/Xinit.d/99gpe-login
	install -d $(DESTDIR)/etc/gpe
	install -m 644 locale.alias $(DESTDIR)/etc/gpe/
	install -d $(DESTDIR)/etc/sysconfig
	install -m 644 gpe-login.sysconfig $(DESTDIR)/etc/sysconfig/gpelogin

clean: clean-po
	rm -f gpe-login $(OBJS) $(DEPS) gpe-lock-display gpe-lock-display.o gpe-lock-display.d

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation-auto-linguas
-include $(DEPS)
