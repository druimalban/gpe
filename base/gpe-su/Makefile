PREFIX = /usr/local
PACKAGE = gpe-su
DEBUG = yes
CVSBUILD = yes
VERSION = 0.10

#FIXME: add: ru sr
LINGUAS = nl pt fr sk cs de sv ro

MEMBERS = gpe-su

CPPFLAGS += $(STANDARD_CPPFLAGS)
CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS)
LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS)

LDFLAGS += -lutil

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

install-program: all $(PACKAGE).desktop
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
