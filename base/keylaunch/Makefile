CC        = gcc
CFLAGS    = -g -O2 -Wall
XROOT     = /usr/X11R6
INCLUDES  = -I$(XROOT)/include
LIBS      = -lX11 
LDPATH    = -L$(XROOT)/lib

PROG      = keylaunch

# Next two lines were edited/added for Debian packaging
DESTDIR	  =
PREFIX    = $(DESTDIR)/usr
RCFILE1	  = .$(PROG)rc
RCFILE2   = /etc/$(PROG)rc
DEFINES   = -DRCFILE1=\"$(RCFILE1)\" -DRCFILE2=\"$(RCFILE2)\" #-DDEBUG

OBJS      = $(PROG).o  

all: $(PROG) 

$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LDPATH) $(LIBS) -o $@

$(OBJS): %.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

install: all
	install $(PROG) $(PREFIX)/bin

clean:
	rm -f $(PROG) $(OBJS)
