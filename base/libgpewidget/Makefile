SONAME = libgpewidget.so.0

MEMBERS = gtkdatesel gtkdatecombo errorbox init

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
	$(CC) -nostartfiles -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME) -lgtk -lgdk_imlib $(LDFLAGS)

install: $(SONAME)
	install -D $(SONAME) $(DESTDIR)/usr/lib/$(SONAME)
	strip $(DESTDIR)/usr/lib/$(SONAME)

ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control postinst dist/CONTROL
	make DESTDIR=`pwd`/dist install
	chown -R root.root dist
	ipkg-build dist

clean:
	rm -f $(OBJS) $(SONAME)

