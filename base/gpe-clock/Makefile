PACKAGE = gpe-clock
PREFIX = /usr/local
DEBUG = no
GTK2 = yes
CVSBUILD = yes
VERSION = 0.8

#FIXME: add: de ro ru sr sk sv
LINGUAS = nl pt cs fr

MEMBERS = clock 

CPPFLAGS = -D_GNU_SOURCE -I../libschedule $(GPECFLAGS)

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g
else
CFLAGS += -Os -fomit-frame-pointer
endif

CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD -Wall

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
	$(CC) -o $@ $^ $(GPELIBS) -L../libschedule -lschedule $(LDFLAGS)

install-program: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
