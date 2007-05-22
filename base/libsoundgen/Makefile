PREFIX = /usr/local
DEBUG = no
VERSION = 0.5
CVSBUILD = yes
PACKAGE = libsoundgen

MEMBERS = soundgen
SONAME = libsoundgen.so.1

ifeq ($(DEBUG),yes)
PACKAGE_CPPFLAGS += -g -pg -I./gpe
PACKAGE_CFLAGS += -g -pg -I./gpe $(GPECFLAGS) `pkg-config --cflags gthread-2.0`  -fPIC
PACKAGE_LDFLAGS += -g -pg $(GPELIBS) `pkg-config --libs gthread-2.0` -lm -lesd
else
PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -I./gpe
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) `pkg-config --cflags gthread-2.0`  -fPIC
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) `pkg-config --libs gthread-2.0` -lm -lesd
endif

HEADERS = soundgen.h buzzer.h

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
	install $(PACKAGE).pc $(DESTDIR)$(PREFIX)/lib/pkgconfig/$(PACKAGE).pc
	install -d $(DESTDIR)$(PREFIX)/include/gpe
	for i in $(HEADERS); do install -m 644 gpe/$$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/libsoundgen.so

install-devel: install-program
	install -d $(DESTDIR)$(PREFIX)/include/gpe
	for i in $(HEADERS); do install -m 644 gpe/$$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/libsoundgen.so

clean:
	rm -f $(OBJS) $(SONAME) libsoundgen.so $(DEPS)

include $(BUILD)/Makefile.dpkg_ipkg
include $(BUILD)/Makefile.translation

-include $(DEPS)
