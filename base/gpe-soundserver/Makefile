PREFIX = /usr/local
PACKAGE = gpe-soundserver
DEBUG = yes
CVSBUILD = yes

MEMBERS = server

CFLAGS = -D_GNU_SOURCE -Wall
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g #-lefence
else
CFLAGS += -Os -fomit-frame-pointer
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"

LDFLAGS += -L/usr/X11R6/lib -lX11

OBJS = $(patsubst %,%.o,$(MEMBERS))

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -d $(DESTDIR)/etc/X11/Xsession.d
	install $(PACKAGE).xsession $(DESTDIR)/etc/X11/Xsession.d/50sound-server

clean:
	rm -f $(PACKAGE) $(OBJS)

include ../build/Makefile.dpkg_ipkg
