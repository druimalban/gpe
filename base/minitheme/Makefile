PREFIX = /usr/local
PACKAGE = minitheme
DEBUG = no
CVSBUILD = yes
GTK2 = yes
#USE_LIBMBDOCK = yes

MEMBERS = minitheme tray

ifneq ($(USE_LIBMBDOCK),yes)
MEMBERS += tray
endif

CFLAGS = `pkg-config --cflags gtk+-2.0`
CONTROL = control1

CFLAGS += -D_GNU_SOURCE -Wall

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif

ifeq ($(CVSBUILD),yes)
CFLAGS += -I../libgpewidget -I../libxsettings-client -I../libxsettings
LDFLAGS += -L../libgpewidget -L../libxsettings-client -L../libxsettings
else
CFLAGS += -I/usr/include/gpe
endif
LDFLAGS += -lgpewidget `pkg-config --libs gtk+-2.0`
ifeq ($(USE_LIBMBDOCK),yes)
LDFLAGS += -lmbdock -L/usr/X11R6/lib 
endif
LDFLAGS += -lXsettings-client

CFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

OBJS = $(patsubst %,%.o,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE) all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: install-program install-mo

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/

clean:
	rm -f $(PACKAGE) $(OBJS)

include ../build/Makefile.translation
include ../build/Makefile.dpkg_ipkg
