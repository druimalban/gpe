## Process this file with automake to produce Makefile.in

INCLUDES = \
	@GTK_CFLAGS@

bin_PROGRAMS = gpe-sketchbook

gpe_sketchbook_SOURCES = \
	gpe-sketchbook.c gpe-sketchbook.h \
	_support.c       _support.h \
	_interface.c     _interface.h \
	selector.c       selector.h \
	sketchpad.c      sketchpad.h \
	sketchpad-cb.c   sketchpad-cb.h \
	files.c          files.h \
	callbacks.c      callbacks.h \
	cursor_eraser_data.h

gpe_sketchbook_LDADD = @GTK_LIBS@

#--------------------------------------------------
#--------------- my targets

fixme:
	grep FIXME *.h *.c -n
	@echo "+----------------+"
	@echo ' Left to fix: ' `grep FIXME *.h *.c|wc -l`
	@echo "+----------------+"
note:
	grep 'NOTE:' *.h *.c -n

PROG = $(PACKAGE)
x: $(PROG)
	./$(PROG)

DIST_NAME = $(PACKAGE)-$(VERSION)
IPKG_NAME = $(PACKAGE)_$(VERSION)_arm.ipk

strip: $(PROG)
	strip $(PROG)
	@echo "WARNING: take care that it is the right architecture!"

ipkg:
	cd .. && rm -rf familiar/dist
	cd .. && mkdir -p familiar/dist/CONTROL
	cd .. && cp familiar/control familiar/postinst familiar/dist/CONTROL
	cd .. && make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	cd .. && mkdir -v -p familiar/dist/usr/lib/menu
	cd .. && cp gpe-sketchbook.menu familiar/dist/usr/lib/menu/gpe-sketchbook
	cd .. && mkdir -v -p familiar/dist/usr/share/pixmaps
	cd .. && cp gpe-sketchbook.png  familiar/dist/usr/share/pixmaps/
	cd .. && chown -R root.root familiar/dist
	cd .. && ./ipkg-build familiar/dist
	@echo --------------------------------
	@echo proj version $(VERSION)
	@echo ipkg 
#`grep ersion ../familiar/control`

trans-pc-to-handheld:
	cd .. && make dist
	cd .. && ~/bin/move-TO-handhelds.org `pwd` $(DIST_NAME).tar.gz

trans-handheld-to-pc:
	~/bin/move-FROM-handhelds.org $(DIST_NAME) src/$(PROG)
#	~/bin/move-FROM-handhelds.org $(DIST_NAME) $(IPKG_NAME)
