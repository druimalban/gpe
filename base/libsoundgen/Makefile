PREFIX = /usr/local
DEBUG = no
VERSION = 0.2

MEMBERS = soundgen
SONAME = libsoundgen.so.1

CFLAGS += -Wall
LDFLAGS += -lm -lesd
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

.SUFFIXES: .d

CPPFLAGS += -I./gpe

HEADERS = soundgen.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: libsoundgen.so

libsoundgen.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS)

libsoundgen.a: $(OBJS)
	ar cq $@ $^

install-program: $(SONAME)
	install -d $(DESTDIR)$(PREFIX)/lib
	install -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)

install-devel:
	for i in $(HEADERS); do install -m 644 -D $$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/libsoundgen.so

clean:
	rm -f $(OBJS) $(SONAME) libsoundgen.so $(DEPS)

include ../build/Makefile.dpkg_ipkg
include ../build/Makefile.translation

-include $(DEPS)
