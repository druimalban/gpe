PACKAGE = gpe-clock
PREFIX = /usr/local
DEBUG = no
GTK2 = yes
CVSBUILD = yes
VERSION = 0.10

LINGUAS = 

MEMBERS = clock da-clock

GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CPPFLAGS = -D_GNU_SOURCE -I../libgpewidget -I../libeventdb
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g -lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -Wall $(GTKCFLAGS)
LDFLAGS += $(GTKLDFLAGS)
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

.SUFFIXES: .d

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
	$(CC) -o $@ $^ -L../libgpewidget -L../libeventdb -lgpewidget -leventdb $(LDFLAGS)

install-program: all $(PACKAGE).desktop
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
