PREFIX = /usr/local
PACKAGE = libdm
DEBUG = yes

LINGUAS = 

MEMBERS = migrate

SONAME = libdm.so.0
CONTROL = control

CFLAGS += -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PACKAGE)\" -D_GNU_SOURCE -Wall -fPIC
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -MD `pkg-config --cflags gtk+-2.0`
LDFLAGS += `pkg-config --libs gtk+-2.0`

HEADERS = 

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: libdm.so mt

mt: mt.c
	$(CC) -o $@ $^ -I/usr/X11R6/include -L/usr/X11R6/lib -lX11

libdm.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS)

.c.s:;
	$(CC) $(CFLAGS) -S $<

install: $(SONAME)
	install -D $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)
	strip $(DESTDIR)$(PREFIX)/lib/$(SONAME)

install-devel:
	for i in $(HEADERS); do install -m 644 -D $$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/libdotdesktop.so

clean:
	rm -f $(OBJS) $(SONAME) libdotdesktop.so $(DEPS)

-include $(DEPS)
include ../build/Makefile.dpkg_ipkg

