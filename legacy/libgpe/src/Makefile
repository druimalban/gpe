PACKAGE = libgpe
VERSION = 0.2
LINGUAS = 

MEMBERS = gpe-exec xsi gpe-soundgen gpe-rtcd gpepreferences
HEADERS = libgpe.h gpe-exec.h xsi.h gpe-soundgen.h gpe-rtcd.h gpepreferences.h 

SONAME = libgpe.so.1

ifndef PREFIX
PREFIX = /usr/local
endif
ifndef DEBUG
DEBUG = no
endif
ifndef CVSBUILD
CVSBUILD = yes
endif

OBJS    = $(patsubst %,%.o,$(MEMBERS))
DEPS    = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

CPPFLAGS += -I../include

CC = gcc
CFLAGS   = -Wall -O2 `pkg-config --cflags glib-2.0`

LDFLAGS += `pkg-config --libs glib-2.0`
LDFLAGS += -L/usr/X11R6/lib -lX11
LDFLAGS += -lm      #gpe-soundgen
LDFLAGS += -lsqlite #preferences

all: libgpe.so #$(PACKAGE).pc $(PACKAGE)-uninstalled.pc

libgpe.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -Wl,-soname -Wl,$@ -o $@ $^ $(LDFLAGS)

install-program: $(SONAME)
	mkdir -p $(DESTDIR)$(PREFIX)/lib/
	install $(SONAME) $(DESTDIR)$(PREFIX)/lib/
	#strip $(DESTDIR)$(PREFIX)/lib/$(SONAME)

install-devel: #$(PACKAGE).pc
	for i in $(HEADERS); do install -m 644 -D ../include/$$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	ln -sf $(SONAME) $(DESTDIR)$(PREFIX)/lib/libgpe.so
#	install -d $(DESTDIR)$(PREFIX)/lib/pkgconfig
#	install -m 644 $(PACKAGE).pc $(DESTDIR)$(PREFIX)/lib/pkgconfig/

clean: clean-po
	rm -rf familiar/dist $(OBJS) $(DEPS) $(SONAME) libgpe.so #$(PACKAGE).pc $(PACKAGE)-uninstalled.pc


#--GPE Standard Makefiles
ifeq ($(CVSBUILD),yes)
BUILD = ../../build
else
BUILD = build
endif

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
