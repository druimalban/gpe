PREFIX = /usr/local
PACKAGE = gpe-dataconf
CVSBUILD = yes
USE_USQLD = yes

DIR_PO := ../po

GTKCFLAGS = `pkg-config --cflags gtk+-2.0` 
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CPPFLAGS += -D_GNU_SOURCE

CFLAGS = $(GTKCFLAGS) -g -I../../libgpewidget -I../include -Wall -I../../libgpedb/include
CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

ifeq ($(USE_USQLD),yes)
CFLAGS += -DUSE_USQLD -I../../usqld/src
LDFLAGS += -lusqld
else
LDFLAGS += -lsqlite
endif

CFLAGS += -MD

MEMBERS = usqld-cfgdb callbacks interface main support db questionfx

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: gpe-dataconf all-mo

$(PACKAGE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -L../../libgpewidget -lgpewidget $(LDFLAGS) $(GTKLDFLAGS) -L../../libgpedb/src -lgpedb

clean:
	rm -f gpe-dataconf $(OBJS) $(DEPS)

install-program: all
	install -D gpe-dataconf $(DESTDIR)$(PREFIX)/bin/gpe-dataconf
	strip $(DESTDIR)$(PREFIX)/bin/gpe-dataconf

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
