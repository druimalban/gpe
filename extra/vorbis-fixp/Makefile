CC = gcc
CFLAGS = -O2 -Wall -fomit-frame-pointer

all: decoder vplay

libfixpvorbis:
	make -C lib

decoder: decoder_example.c libfixpvorbis
	$(CC) $(CFLAGS) -o $@ decoder_example.c lib/libfixpvorbis.a -lm 

vplay: vplay.c fmtheaders.h
	$(CC) -O2 vplay.c -o vplay

clean:
	rm -f decoder vplay *.o *.d *~
	make -C lib clean
