PREFIX = /usr/local
PACKAGE = libtododb
#DEBUG = yes 
CVSBUILD = yes
VERSION = 0.06

LINGUAS =

MEMBERS = todo-db old-db 

SONAME = libtododb.so.0

CFLAGS = `pkg-config --cflags glib-2.0` -DG_DISABLE_DEPRECATED -I../libgpepimc
LDFLAGS = `pkg-config --libs glib-2.0` -lsqlite -L../libgpewidget -lgpewidget -L../libgpepimc -lgpepimc

CFLAGS += -DPREFIX=\"$(PREFIX)\" -DPACKAGE=\"$(PACKAGE)\" -D_GNU_SOURCE -Wall -fPIC

ifeq ($(DEBUG),yes)
CFLAGS += -g #-O2 -g
LDFLAGS += -g
else
CFLAGS += -Os -fomit-frame-pointer
endif
CFLAGS += -MD

CPPFLAGS += -I. -I../libgpewidget

HEADERS = gpe/todo-db.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: libtododb.so

libtododb.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS)

.c.s:;
	$(CC) $(CFLAGS) -S $<

install-devel:
	for i in $(HEADERS); do install -m 644 -D $$i $(DESTDIR)$(PREFIX)/include/$$i; done
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/libtododb.so

clean:
	rm -f $(OBJS) $(SONAME) libtododb.so $(DEPS)

install-program: $(SONAME)
	install -Ds $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
