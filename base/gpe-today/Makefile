PACKAGE = gpe-today
PREFIX = /usr/local
VERSION = 0.02
CVSBUILD = yes
#DEBUG = yes 
LINGUAS = pt

MEMBERS = main todo calendar date

GTKCFLAGS = `pkg-config --cflags gtk+-2.0`
GTKLDFLAGS += `pkg-config --libs gtk+-2.0`

CPPFLAGS = -I../libgpewidget -I../libeventdb -I../libtododb -I../librootimage -D_GNU_SOURCE

ifeq ($(DEBUG),yes)
CFLAGS = -g
LDFLAGS = -lefence
else
CFLAGS = -Os -fomit-frame-pointer
endif

CFLAGS += -Wall $(GTKCFLAGS)
LDFLAGS += $(GTKLDFLAGS) -L/usr/local/lib -static  -static-libgcc -L../libeventdb -leventdb -L../libtododb -ltododb -L../librootimage -lrootimage
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
#CFLAGS += -MD

.SUFFIXES: .d

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

PIXMAPS = calendar.png date.png tasks.png

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE) all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ -L../libgpewidget -lgpewidget -lsqlite $(LDFLAGS)

install-program: all $(PACKAGE).desktop
	for i in $(PIXMAPS); do install -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/pixmaps/$$i; done
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)
	install -m 644 -D gtkrc $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/gtkrc

clean: clean-po
	rm -f $(PACKAGE) $(OBJS) $(DEPS) $(PACKAGE).desktop *~
	rm -rf familiar/dist

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
