PREFIX = /usr/local
PACKAGE = gpe-confd
DEBUG = no
CVSBUILD = yes
VERSION = 0.14

MEMBERS = main xsettings-manager

ifeq ($(CVSBUILD),yes)
PACKAGE_CFLAGS += -I../libxsettings
PACKAGE_LDFLAGS += -L../libxsettings
endif

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS)
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) -lsqlite `pkg-config --libs glib-2.0` -lXsettings
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) `pkg-config --cflags glib-2.0`

OBJS = $(patsubst %,%.o,$(MEMBERS))
DEPS = $(patsubst %,%.d,$(MEMBERS))
SOURCES = $(patsubst %,%.c,$(MEMBERS))

ifeq ($(CVSBUILD),yes)
BUILD = ../build
else
BUILD = build
endif

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(PACKAGE_LDFLAGS)

install-program: $(PACKAGE)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	mkdir -p $(DESTDIR)/etc/X11/Xsession.d
	install $(PACKAGE).xs $(DESTDIR)/etc/X11/Xsession.d/70gpe-confd

clean:
	rm -f $(PACKAGE)
	rm -f $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
