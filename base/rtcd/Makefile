#
# Makefile for Real Time Clock deamon
#

CC	=	gcc

CFLAGS	=	-Wall -Os #-g -DDEBUG
LDLAGS	=
OBJS	=	rtcd.o
PRGS	=	rtcd

all:	$(PRGS)
rtcd:	$(OBJS)
	$(CC) $(CFLAGS) -o rtcd $(OBJS)

.PHONY:	install clean

clean:
	rm -f *.o $(PRGS)
