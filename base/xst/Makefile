PREFIX = /usr/local
PACKAGE = xst
CVSBUILD = yes
DEBUG = no
VERSION = 0.12

MEMBERS = xst

GTKCFLAGS = `pkg-config --cflags glib-2.0`
GTKLDFLAGS += `pkg-config --libs glib-2.0`

CPPFLAGS += -D_GNU_SOURCE

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

LDFLAGS += $(GTKLDFLAGS) -L../libxsettings-client -lXsettings-client -Wl,-rpath-link,../libxsettings
CPPFLAGS += $(GTKCFLAGS) -I../libxsettings -I../libxsettings-client

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: $(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
