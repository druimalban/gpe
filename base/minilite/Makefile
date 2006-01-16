PACKAGE = minilite
PREFIX = /usr
CVSBUILD = yes
DEBUG = no
VERSION = 0.50
INSTALL = install
LINGUAS = 

MEMBERS = minilite

ifeq ($(DEBUG),yes)
PACKAGE_CFLAGS += -O2 -g
PACKAGE_LDFLAGS = -g 
else
PACKAGE_CFLAGS += -Os -fomit-frame-pointer
endif

ifeq ($(CVSBUILD),yes)
PACKAGE_CFLAGS +=  -I../libgpewidget
PACKAGE_LDFLAGS += -L../libgpewidget
endif

PACKAGE_CFLAGS += $(STANDARD_CFLAGS) $(GPECFLAGS) $(GTKCFLAGS)
PACKAGE_LDFLAGS += $(STANDARD_LIBS) $(GPELIBS) $(GTKLIBS) 

PACKAGE_CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
PACKAGE_CFLAGS += -MD -D_GNU_SOURCE

.SUFFIXES: .d

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE) all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE).desktop
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps/
	$(INSTALL) -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
