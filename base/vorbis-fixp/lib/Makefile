CC = gcc
AR = ar

CFLAGS = -Wall -fomit-frame-pointer

# For StrongARM CPUs
CFLAGS += -mcpu=strongarm

# Note that -O2 doesn't produce as good results as the -O with all the
# specific optimisations below.
#CFLAGS += -O2
CFLAGS += -O -fforce-mem -fforce-addr -fthread-jumps -fcse-follow-jumps -fcse-skip-blocks -fexpensive-optimizations -fregmove -fschedule-insns2 -fstrength-reduce

OBJS = 	framing.o bitwise.o info.o sharedbook.o codebook.o \
	synthesis.o block.o window.o mdct.o lsp.o \
	registry.o time0.o mapping0.o floor0.o floor1.o res0.o

all: libfixpvorbis.a libvorbis-fixp.so.0

%.o: %.c .%.d

%.s: %.c .%.d
	$(CC) $(CFLAGS) -S $<

.%.d: %.c
	$(CC) -MM $< > $@

include $(OBJS:%.o=.%.d)

libfixpvorbis.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

libvorbis-fixp.so.0: libfixpvorbis.a
	gcc -shared -o $@ -Wl,--whole-archive $< -Wl,--no-whole-archive -Wl,-soname -Wl,libvorbis-fixp.so.0

clean:
	rm -f libfixpvorbis.a *.o *.s *.d *~
