CC = gcc
CFLAGS=-g -Wall
BINS=ipaq-sleep

all: $(BINS)

ipaq-sleep: ipaq-sleep.c
	$(CC) -o ipaq-sleep ipaq-sleep.c -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXext -lXss

clean:
	rm -f $(BINS) *.o

