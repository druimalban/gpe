PACKAGE = gpe-multisync
PREFIX = /usr/local
DEBUG = no
CVSBUILD = yes
VERSION = 0.1

LINGUAS = 

MEMBERS = gui gpe_sync

GLADECFLAGS = `pkg-config --cflags libglade-2.0`
GLADELDFLAGS += `pkg-config --libs libglade-2.0`

CPPFLAGS = -D_GNU_SOURCE -I../libnsqlc
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g 
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -Wall $(GLADECFLAGS)
LDFLAGS += $(GLADELDFLAGS) -L../libnsqlc -lnsqlc
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

TARGET = gpe_sync.so

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -Wl,-Bsymbolic -Wl,--version-script,gpe_sync.map

install-program: all $(PACKAGE).desktop
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS) $(PACKAGE).desktop

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
