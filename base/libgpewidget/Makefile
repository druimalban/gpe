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
	ln -s $^ $@

$(SONAME): $(OBJS)
	gcc -shared -o $@ $^ -Wl,-soname -Wl,$(SONAME)
