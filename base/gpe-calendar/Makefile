MEMBERS = main gtkdatesel day_view week_view future_view event-ui about
CFLAGS = `gtk-config --cflags` -Os -Wall -fomit-frame-pointer

OBJS = $(patsubst %,%.o,$(MEMBERS))

gpe-calendar: $(OBJS)
	$(CC) -o $@ $^ `gtk-config --libs` -lgdk_imlib

clean:
	rm -f gpe-calendar $(OBJS)
