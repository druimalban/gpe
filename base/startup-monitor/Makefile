PACKAGE = startup-monitor
PREFIX = /usr/local
DEBUG = no
VERSION = 0.15

LINGUAS = 

MEMBERS = monitor

CPPFLAGS = -D_GNU_SOURCE `pkg-config --cflags libstartup-notification-1.0` -DSN_API_NOT_YET_FROZEN `pkg-config --cflags glib-2.0`

ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif

CFLAGS += -Wall
LDFLAGS += -L/usr/X11R6/lib -lX11 -lXext -lmb `pkg-config --libs libstartup-notification-1.0` `pkg-config --libs glib-2.0`
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CFLAGS += -MD

.SUFFIXES: .d

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

all: $(PACKAGE) all-mo

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: all install-mo
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)/etc/X11/Xsession.d
	install $(PACKAGE).xsession $(DESTDIR)/etc/X11/Xsession.d/70startup-monitor

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include ../build/Makefile.translation
include ../build/Makefile.dpkg_ipkg
-include $(DEPS)
