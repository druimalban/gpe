PREFIX = /usr/local
PACKAGE = libeventdb
#DEBUG = yes 
VERSION = 0.05

LINGUAS = 

MEMBERS = event-db old-db 

SONAME = libeventdb.so.0
CONTROL = control

CFLAGS = `pkg-config --cflags glib-2.0` -DG_DISABLE_DEPRECATED
LDFLAGS = `pkg-config --libs glib-2.0` -lsqlite -L../libgpewidget -lgpewidget

CFLAGS += -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PACKAGE)\" -D_GNU_SOURCE -Wall -fPIC

ifeq ($(DEBUG),yes)
CFLAGS += -g #-O2 -g
LDFLAGS += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -MD

CPPFLAGS += -I. -I../libgpewidget

HEADERS = event-db.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: libeventdb.so

libeventdb.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS)

.c.s:;
	$(CC) $(CFLAGS) -S $<

install-devel:
	for i in $(HEADERS); do install -m 644 -D $$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/libeventdb.so

clean:
	rm -f $(OBJS) $(SONAME) libeventdb.so $(DEPS)

install: $(SONAME) install-mo
	install -D $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)
	strip $(DESTDIR)$(PREFIX)/lib/$(SONAME)

include ../build/Makefile.translation
include ../build/Makefile.dpkg_ipkg
-include $(DEPS)
