PACKAGE = minibat
PREFIX = /usr/local
DEBUG = no
CVSBUILD = yes
VERSION = 0.15

LINGUAS = 

MEMBERS = minibat

CPPFLAGS = -D_GNU_SOURCE -DHAVE_APM_H `pkg-config --cflags xft`
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif

LDFLAGS = -L/usr/X11R6/lib -lX11 -lXext -lmb `pkg-config --libs xft` -lapm

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

all: $(PACKAGE) all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
#install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -m 644 $(PACKAGE)-ac.png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -m 644 $(PACKAGE)-battery.png $(DESTDIR)$(PREFIX)/share/pixmaps/

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
