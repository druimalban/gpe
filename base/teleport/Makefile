PREFIX = /usr/local
PACKAGE = teleport
DEBUG = yes
CVSBUILD = yes
VERSION = 0.33

LINGUAS = 

ifeq ($(CVSBUILD),yes)
# temporary hack to work around lack of pkg-config for libdisplaymigration
PACKAGE_CPPFLAGS += -I../libdisplaymigration
PACKAGE_LDFLAGS += -L../libdisplaymigration
endif
PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) `libgcrypt-config --cflags`
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) $(GPELIBS) -ltododb -ldisplaymigration -lgpepimc `libgcrypt-config --libs` -lsqlite

MEMBERS = teleport crypt displays keygen tp-keygen

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: teleport all-mo tp-keygen

teleport: teleport.o crypt.o displays.o keygen.o
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

tp-keygen: tp-keygen.o keygen.o
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

clean:
	rm -f teleport tp-keygen tp-keygen.o $(OBJS) $(DEPS)

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s teleport $(DESTDIR)$(PREFIX)/bin/
	install -s tp-keygen $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 teleport.png $(DESTDIR)$(PREFIX)/share/pixmaps
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 teleport.desktop $(DESTDIR)$(PREFIX)/share/applications

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
