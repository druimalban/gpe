PREFIX = /usr/local
PACKAGE = xst
CVSBUILD = yes
DEBUG = no
VERSION = 0.14

MEMBERS = xst

ifeq ($(CVSBUILD),yes)
PACKAGE_LDFLAGS += -L../libxsettings-client -Wl,-rpath-link,../libxsettings
PACKAGE_CPPFLAGS += -I../libxsettings -I../libxsettings-client
endif

PACKAGE_CFLAGS += $(STANDARD_CFLAGS) `pkg-config --cflags glib-2.0`
PACKAGE_LDFLAGS += $(STANDARD_LDFLAGS) `pkg-config --libs glib-2.0` -lXsettings-client 
PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS)

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
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
