.SUFFIXES: .so

CFLAGS = -O2 -g -Wall

CFLAGS += `pkg-config --cflags glib-2.0` `pkg-config --cflags libxml-2.0`
LDFLAGS += `pkg-config --libs glib-2.0`

CODECS = d-vorbis.so d-mad.so
BACKEND = libgpenmf.so.0

MEMBERS = decoder stream file_stream playlist_db playlist_xml
MAD_MEMBERS = mad playlist_id3
OGG_MEMBERS = vorbis playlist_ogg

OBJS = $(patsubst %,%.o,$(MEMBERS))
OGG_OBJS = $(patsubst %,%.o,$(OGG_MEMBERS))
MAD_OBJS = $(patsubst %,%.o,$(MAD_MEMBERS))

DEPS = $(patsubst %,%.d,$(MEMBERS) $(OGG_MEMBERS) $(MAD_MEMBERS))

all: $(CODECS) $(BACKEND)

d-vorbis.so: $(OGG_OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -nostartfiles -lvorbisfile -lpthread

d-mad.so: $(MAD_OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -nostartfiles -lmad -lpthread -lid3tag

$(BACKEND): $(OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -nostartfiles `pkg-config --libs libxml-2.0` -Wl,-soname -Wl,$(BACKEND)

-include $(DEPS)

clean:
	rm -f $(DEPS) $(OBJS) $(OGG_OBJS) $(MAD_OBJS) $(BACKEND) d-vorbis.so d-mad.so
