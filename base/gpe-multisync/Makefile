PACKAGE = gpe-multisync
PREFIX = /usr/local
DEBUG = no
CVSBUILD = yes
VERSION = 0.1

LINGUAS = 

MEMBERS = gui gpe_sync config connection calendar contacts todo

GLADECFLAGS = `pkg-config --cflags libglade-2.0`
GLADELDFLAGS += `pkg-config --libs libglade-2.0`

CPPFLAGS = -D_GNU_SOURCE -I../libnsqlc -I../libgpevtype
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g 
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -fPIC -Wall $(GLADECFLAGS) `pkg-config --cflags mimedir-1.0`
LDFLAGS += $(GLADELDFLAGS) -L../libnsqlc -lnsqlc `pkg-config --libs mimedir-1.0` -L../libgpevtype -lgpevtype
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

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/lib/multisync
	install -s $(TARGET) $(DESTDIR)$(PREFIX)/lib/multisync/$(TARGET)
	mkdir -p $(DESTDIR)$(PREFIX)/share/$(PACKAGE)
	install -m 644 gpe_sync.glade $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/gpe_sync.glade

clean:
	rm -f $(PACKAGE) $(TARGET) $(OBJS) $(DEPS) $(PACKAGE).desktop

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
