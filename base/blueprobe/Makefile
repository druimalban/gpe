PREFIX = /usr/local
PACKAGE = blueprobe
DEBUG = no
CVSBUILD = yes
VERSION = 0.16

MEMBERS = blueprobe

PACKAGE_CFLAGS += $(STANDARD_CFLAGS)
PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS)

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
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

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
