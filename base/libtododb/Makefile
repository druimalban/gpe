PREFIX = /usr/local
PACKAGE = libtododb
#DEBUG = yes 
CVSBUILD = yes
VERSION = 0.08

LINGUAS =

MEMBERS = todo-db old-db 

SONAME = libtododb.so.0

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -I.
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) `pkg-config --cflags glib-2.0` -DG_DISABLE_DEPRECATED -I../libgpepimc -fPIC
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) `pkg-config --libs glib-2.0` -lsqlite -L../libgpepimc -lgpepimc

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
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS) $(PACKAGE_LDFLAGS)

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
