PREFIX = /usr/local
PACKAGE = libschedule
#DEBUG = yes
CVSBUILD = yes
VERSION = 0.14

LINGUAS = 

MEMBERS = atd

SONAME = libschedule.so.0

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS) -I.
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) `pkg-config --cflags glib-2.0` -DG_DISABLE_DEPRECATED -fPIC
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) `pkg-config --libs glib-2.0` 

HEADERS = schedule.h

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: libschedule.so

libschedule.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-devel:
	install -d $(DESTDIR)$(PREFIX)/include/gpe
	for i in $(HEADERS); do install -m 644 gpe/$$i $(DESTDIR)$(PREFIX)/include/gpe/$$i; done
	install -d $(DESTDIR)$(PREFIX)/lib
	ln -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/libschedule.so

clean:
	rm -f $(OBJS) $(SONAME) libschedule.so $(DEPS)

install-program: $(SONAME) install-mo
	install -d $(DESTDIR)$(PREFIX)/lib
	install -s $(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
