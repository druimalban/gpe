PREFIX = /usr/local
PACKAGE = gpe-setlocale
#DEBUG = yes
CVSBUILD = yes
VERSION = 0.10

MEMBERS = gpe-setlocale

CFLAGS += -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS += -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
LDFLAGS += -L/usr/X11R6/lib -lX11

OBJS = $(patsubst %,%.o,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS)

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation
