PACKAGE = nsqld
PREFIX = /usr/local
DEBUG = no
VERSION = 0.5.3
CVSBUILD = yes
SASL = no
STRIP = strip

LINGUAS = 

MEMBERS = nsqld common

PACKAGE_CPPFLAGS += $(STANDARD_CPPFLAGS)
PACKAGE_CFLAGS += $(STANDARD_CFLAGS) `pkg-config --cflags glib-2.0`
PACKAGE_LDFLAGS += -lsqlite `pkg-config --libs glib-2.0`

ifeq ($(SASL),yes)
PACKAGE_LDFLAGS += -lsasl2
PACKAGE_CPPFLAGS += -DUSE_SASL
endif

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
	install -d $(DESTDIR)$(PREFIX)/bin
	$(STRIP) $(PACKAGE)
	install $(PACKAGE) $(DESTDIR)$(PREFIX)/bin

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
