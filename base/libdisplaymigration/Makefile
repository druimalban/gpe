PREFIX = /usr/local
PACKAGE = libdisplaymigration
#DEBUG = yes
CVSBUILD = yes
VERSION = 0.99

LINGUAS = 

MEMBERS = migrate

SONAME = $(PACKAGE).so.0

PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GTKCFLAGS) -fPIC
PACKAGE_CPPFLAGS += -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PACKAGE)\"
PACKAGE_LDFLAGS += $(GTKLIBS) -lgcrypt

HEADERS = libdisplaymigration/displaymigration.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE).so

$(PACKAGE).so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: $(SONAME)
	install -d $(DESTDIR)$(PREFIX)/lib
	install $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)

install-devel:
	for i in $(HEADERS); do install -m 644 -D $$i $(DESTDIR)$(PREFIX)/include/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(PACKAGE).so

clean:
	rm -f $(OBJS) $(SONAME) $(PACKAGE).so $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
