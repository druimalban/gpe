SONAME = libgpewidget.so.0

MEMBERS = gtkdatesel gtkdatecombo

CFLAGS = `gtk-config --cflags` -D_GNU_SOURCE -Wall -fPIC
ifeq ($(DEBUG),yes)
CFLAGS += -O2 -g
LDFLAGS = -g
else
CFLAGS += -Os -fomit-frame-pointer
endif

OBJS = $(patsubst %,%.o,$(MEMBERS))

all: libgpewidget.so

libgpewidget.so: $(SONAME)
	ln -sf $^ $@

$(SONAME): $(OBJS)
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) $(LDFLAGS)

install: $(SONAME)
	install -D $(SONAME) $(DESTDIR)/usr/lib/$(SONAME)
	strip $(DESTDIR)/usr/lib/$(SONAME)

ipkg:
	make DESTDIR=`pwd`/dist install
	ipkg-build dist

clean:
	rm -f $(OBJS) $(SONAME)

