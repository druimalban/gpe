MEMBERS = main gtkdatesel day_view week_view future_view event-ui
CFLAGS = `gtk-config --cflags` -Os -Wall -fomit-frame-pointer

OBJS = $(patsubst %,%.o,$(MEMBERS))

gpe-calendar: $(OBJS)
	$(CC) -o $@ $^ `gtk-config --libs`

clean:
	rm -f gpe-calendar $(OBJS)
