PREFIX = /usr/local
PACKAGE = miniclipboard
CVSBUILD = yes
DEBUG = yes
VERSION = 0.3

MEMBERS = miniclipboard mbpixbuf mbtray

PACKAGE_CFLAGS += -Wall `pkg-config --cflags x11`
ifeq ($(DEBUG),yes)
PACKAGE_CFLAGS += -O2 -g
LDFLAGS = -g 
else
PACKAGE_CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(`pkg-config --libs x11`) -lXpm -lXext

OBJS = $(patsubst %,%.o,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all
	install -D $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
