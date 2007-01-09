CC = gcc-3.1
AR = ar

CFLAGS = -Wall -fomit-frame-pointer

# For StrongARM CPUs
CFLAGS += -mcpu=strongarm

# Note that -O2 doesn't produce as good results as the -O with all the
# specific optimisations below.
CFLAGS += -O2 -fomit-frame-pointer -frename-registers
#CFLAGS += -O -fforce-mem -fforce-addr -fthread-jumps -fcse-follow-jumps -fcse-skip-blocks -fexpensive-optimizations -fregmove -fschedule-insns2 -fstrength-reduce

OBJS = 	framing.o bitwise.o info.o sharedbook.o codebook.o \
	synthesis.o block.o window.o mdct.o lsp.o \
	registry.o time0.o mapping0.o floor0.o floor1.o res0.o

FOBJS = vorbisfile.o

all: libfixpvorbis.a libvorbis-fixp.so.0 libvorbisfile-fixp.a libvorbisfile-fixp.so.0 libvorbis.so libvorbisfile.so

%.o: %.c .%.d

%.s: %.c .%.d
	$(CC) $(CFLAGS) -S $<

.%.d: %.c
	$(CC) -MM $< > $@

include $(OBJS:%.o=.%.d)

libvorbisfile-fixp.a: $(FOBJS)
	$(AR) rcs $@ $(FOBJS)

libvorbisfile-fixp.so.0: libvorbisfile-fixp.a
	gcc -shared -nostartfiles -o $@ -Wl,--whole-archive $< -Wl,--no-whole-archive -Wl,-soname -Wl,libvorbisfile-fixp.so.0

libfixpvorbis.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

libvorbis-fixp.so.0: libfixpvorbis.a
	gcc -shared -nostartfiles -o $@ -Wl,--whole-archive $< -Wl,--no-whole-archive -Wl,-soname -Wl,libvorbis-fixp.so.0

libvorbisfile.so: libvorbisfile-fixp.so.0
	ln -sf $^ $@

libvorbis.so: libvorbis-fixp.so.0
	ln -sf $^ $@

clean:
	rm -f *.a *.o *.s *.d *.so.0 *.so *~
