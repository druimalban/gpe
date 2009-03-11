PREFIX = /usr/local
PACKAGE = gpe-autostarter
DEBUG = no
CVSBUILD = yes
VERSION = 0.12

MEMBERS = autostarter net bluetooth

PACKAGE_CFLAGS += $(STANDARD_CFLAGS) `pkg-config --cflags glib-2.0` `pkg-config --cflags dbus-glib-1` -DDBUS_API_SUBJECT_TO_CHANGE
PACKAGE_LDFLAGS += `pkg-config --libs glib-2.0` `pkg-config --libs dbus-glib-1` `pkg-config --libs x11`

export PKG_CONFIG_PATH := $(BUILD):$(PKG_CONFIG_PATH)

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))

all: $(PACKAGE) all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)/etc/X11/Xsession.d
	install $(PACKAGE).xsession $(DESTDIR)/etc/X11/Xsession.d/50$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)

