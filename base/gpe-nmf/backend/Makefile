.SUFFIXES: .so

CFLAGS = -O2 -g -Wall

CFLAGS += `pkg-config --cflags glib-2.0` `pkg-config --cflags libxml-2.0`
LDFLAGS += `pkg-config --libs glib-2.0`

CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\" -I../../vorbis-fixp/lib

CODECS = d-vorbis.so d-mad.so
BACKEND = libgpenmf.so.0
BACKEND_SO = libgpenmf.so

MEMBERS = decoder stream file_stream playlist_db playlist_xml player audio
MAD_MEMBERS = mad playlist_id3
OGG_MEMBERS = vorbis playlist_ogg

OBJS = $(patsubst %,%.o,$(MEMBERS))
OGG_OBJS = $(patsubst %,%.o,$(OGG_MEMBERS))
MAD_OBJS = $(patsubst %,%.o,$(MAD_MEMBERS))

DEPS = $(patsubst %,%.d,$(MEMBERS) $(OGG_MEMBERS) $(MAD_MEMBERS))

all: $(CODECS) $(BACKEND_SO)

d-vorbis.so: $(OGG_OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -nostartfiles -lpthread -L../../vorbis-fixp/lib -lvorbis -lvorbisfile

d-mad.so: $(MAD_OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -nostartfiles -lmad -lpthread -lid3tag

$(BACKEND_SO): $(BACKEND)
	ln -sf $^ $@

$(BACKEND): $(OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -nostartfiles `pkg-config --libs libxml-2.0` -Wl,-soname -Wl,$(BACKEND) -lesd -g

-include $(DEPS)

clean:
	rm -f $(DEPS) $(OBJS) $(OGG_OBJS) $(MAD_OBJS) $(BACKEND) d-vorbis.so d-mad.so

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/lib/gpe-nmf/decoders
	install -s $(BACKEND) $(DESTDIR)$(PREFIX)/lib/$(BACKEND)
	for i in $(CODECS); do install -s $$i $(DESTDIR)$(PREFIX)/lib/gpe-nmf/decoders/$$i; done
