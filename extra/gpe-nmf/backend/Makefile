.SUFFIXES: .so

ifeq ($(CVSBUILD),yes)
BUILD = ../../build
else
BUILD = ../build
endif


PACKAGE_CFLAGS = -Os -g -Wall

PACKAGE_CFLAGS += `pkg-config --cflags glib-2.0` -DFILE_OFFSET_BITS=64 -I../../libgpewidget
PACKAGE_LDFLAGS += `pkg-config --libs glib-2.0`

PACKAGE_CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\" -I../../vorbis-fixp/lib

LVORBIS = -L../../vorbis-fixp/lib

BACKEND = libgpenmf.so.0
BACKEND_SO = libgpenmf.so

MEMBERS = playlist_db playlist_xml playlist_m3u gstplayer
PACKAGE_LDFLAGS += `pkg-config --libs gstreamer-$(GST_VERSION)` -lpthread
PACKAGE_CPPFLAGS += `pkg-config --cflags gstreamer-$(GST_VERSION)`

OBJS = $(patsubst %,%.o,$(MEMBERS))
OGG_OBJS = $(patsubst %,%.o,$(OGG_MEMBERS))
MAD_OBJS = $(patsubst %,%.o,$(MAD_MEMBERS))

DEPS = $(patsubst %,%.d,$(MEMBERS) $(OGG_MEMBERS) $(MAD_MEMBERS))

all: $(CODECS) $(BACKEND_SO)

d-vorbis.so: $(OGG_OBJS)
	$(CC) -shared -o $@ $^ $(PACKAGE_LDFLAGS) -nostartfiles -lpthread $(LVORBIS) -lvorbis -lvorbisfile

d-mad.so: $(MAD_OBJS)
	$(CC) -shared -o $@ $^ $(PACKAGE_LDFLAGS) -nostartfiles -lmad -lpthread -lid3tag

$(BACKEND_SO): $(BACKEND)
	ln -sf $^ $@

$(BACKEND): $(OBJS)
	$(CC) -shared -o $@ $^ $(PACKAGE_LDFLAGS) -nostartfiles -Wl,-soname -Wl,$(BACKEND) -g

-include $(DEPS)

clean:
	rm -f $(DEPS) $(OBJS) $(OGG_OBJS) $(MAD_OBJS) $(BACKEND) d-vorbis.so d-mad.so

install-program: all
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	install -s $(BACKEND) $(DESTDIR)$(PREFIX)/lib/$(BACKEND)
	for i in $(CODECS); do install -s $$i $(DESTDIR)$(PREFIX)/lib/gpe-nmf/decoders/$$i; done

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
