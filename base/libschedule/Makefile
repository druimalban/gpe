PREFIX = /usr/local
PACKAGE = libschedule
#DEBUG = yes 
VERSION = 0.11

LINGUAS = 

MEMBERS = atd

SONAME = libschedule.so.0

CFLAGS = `pkg-config --cflags glib-2.0` -DG_DISABLE_DEPRECATED
LDFLAGS = `pkg-config --libs glib-2.0` 

CFLAGS += -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PACKAGE)\" -D_GNU_SOURCE -Wall -fPIC

ifeq ($(DEBUG),yes)
CFLAGS += -g #-O2 -g
LDFLAGS += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -MD

CPPFLAGS += -I.

HEADERS = schedule.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: libschedule.so

libschedule.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS)

.c.s:;
	$(CC) $(CFLAGS) -S $<

install-devel:
	install -d $(DESTDIR)$(PREFIX)/include/gpe
	for i in $(HEADERS); do install -m 644 gpe/$$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	install -d $(DESTDIR)$(PREFIX)/lib
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/libeventdb.so

clean:
	rm -f $(OBJS) $(SONAME) libeventdb.so $(DEPS)

install-program: $(SONAME) install-mo
	install -d $(DESTDIR)$(PREFIX)/lib
	install -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)

include ../build/Makefile.translation
include ../build/Makefile.dpkg_ipkg
-include $(DEPS)
