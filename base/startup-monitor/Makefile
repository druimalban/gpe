PACKAGE = startup-monitor
PREFIX = /usr/local
CVSBUILD = yes
DEBUG = no
VERSION = 0.18-2

LINGUAS = 

MEMBERS = monitor

PACKAGE_CPPFLAGS = -D_GNU_SOURCE `pkg-config --cflags libstartup-notification-1.0` -DSN_API_NOT_YET_FROZEN `pkg-config --cflags glib-2.0 libmb`

ifeq ($(DEBUG),yes)
PACKAGE_CFLAGS += -O2 -g
PACKAGE_LDFLAGS = -g
else
PACKAGE_CFLAGS += -Os -fomit-frame-pointer
endif

PACKAGE_CFLAGS += -Wall
PACKAGE_LDFLAGS += `pkg-config --libs x11` -lXext -lmb `pkg-config --libs libstartup-notification-1.0` `pkg-config --libs glib-2.0 libmb`
PACKAGE_CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
PACKAGE_CFLAGS += -MD

.SUFFIXES: .d

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE) all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(PACKAGE_LDFLAGS) $(LDFLAGS)

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)/etc/X11/Xsession.d
	install $(PACKAGE).xsession $(DESTDIR)/etc/X11/Xsession.d/70startup-monitor

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
