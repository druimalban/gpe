PREFIX = /usr/local
PACKAGE = teleport
DEBUG = yes
CVSBUILD = yes
VERSION = 0.34

LINGUAS = 

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) `libgcrypt-config --cflags`
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) `libgcrypt-config --libs` -lsqlite

MEMBERS = teleport crypt displays dm-crypt keygen
KG_MEMBERS = tp-keygen keygen

DEPS = $(patsubst %,%.d,$(MEMBERS) $(KG_MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
KG_OBJS = $(patsubst %,%.o,$(KG_MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS) $(KG_MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: teleport all-mo tp-keygen

teleport: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

tp-keygen: $(KG_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

clean:
	rm -f teleport tp-keygen $(OBJS) $(DEPS) $(KG_OBJS)

install-program: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s teleport $(DESTDIR)$(PREFIX)/bin/
	install -s tp-keygen $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 teleport.png $(DESTDIR)$(PREFIX)/share/pixmaps
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 teleport.desktop $(DESTDIR)$(PREFIX)/share/applications

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
