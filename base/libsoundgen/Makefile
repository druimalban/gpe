PREFIX = /usr/local
DEBUG = no
VERSION = 0.3
CVSBUILD = yes
PACKAGE = libsoundgen

MEMBERS = soundgen
SONAME = libsoundgen.so.1

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -I./gpe
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) -fPIC
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) -lm -lesd

HEADERS = soundgen.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: libsoundgen.so

libsoundgen.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: $(SONAME)
	install -d $(DESTDIR)$(PREFIX)/lib
	install -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)

install-devel:
	for i in $(HEADERS); do install -m 644 -D $$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/libsoundgen.so

clean:
	rm -f $(OBJS) $(SONAME) libsoundgen.so $(DEPS)

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation

-include $(DEPS)
