PREFIX = /usr/local
PACKAGE = blueprobe
DEBUG = no
CVSBUILD = yes
VERSION = 0.12

MEMBERS = blueprobe

CPPFLAGS += -D_GNU_SOURCE

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

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
	install -d $(DESTDIR)/etc/sysconfig
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -d $(DESTDIR)/etc/init.d
	install $(PACKAGE).init $(DESTDIR)/etc/init.d/$(PACKAGE)
	install -d $(DESTDIR)/etc/rcS.d
	ln -s ../init.d/$(PACKAGE) $(DESTDIR)/etc/rcS.d/S97blueprobe

clean:
	rm -f $(PACKAGE)
	rm -f $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
