## Process this file with automake to produce Makefile.in

INCLUDES = \
	@GTK_CFLAGS@

bin_PROGRAMS = gpe-sketchbook

gpe_sketchbook_SOURCES = \
	gpe-sketchbook.c gpe-sketchbook.h \
					\
	selector.c       selector.h \
	selector-cb.c    selector-cb.h \
	selector-gui.c   selector-gui.h \
					\
	sketchpad.c      sketchpad.h \
	sketchpad-cb.c   sketchpad-cb.h \
	sketchpad-gui.c  sketchpad-gui.h \
	cursor_eraser_data.h	\
					\
	files.c          files.h \
					\
	dialog.c         dialog.h \
	about.c          about.h

gpe_sketchbook_LDADD = @GTK_LIBS@

#--------------------------------------------------
#--------------- my targets

fixme:
	@echo "NO! that does NOT fix anything! ;)"
	grep 'FIXME:' *.h *.c -n
	@echo "+----------------+"
	@echo ' Left to fix: ' `grep 'FIXME:' *.h *.c|wc -l`
	@echo "+----------------+"
note:
	grep 'NOTE:' *.h *.c -n

PROG = $(PACKAGE)
x: $(PROG)
	./$(PROG)

DIST_NAME = $(PACKAGE)-$(VERSION)
IPKG_NAME = $(PACKAGE)_$(VERSION)_arm.ipk

arm: $(PROG) strip
	cp $(PROG) $(PACKAGE)-$(VERSION)-arm-striped
	@echo "WARNING: take care that it is the right architecture!"

strip: $(PROG)
	strip $(PROG)

ipkg:
	cp $(PACKAGE)-$(VERSION)-arm-striped $(PROG)
	cd .. && rm -rf familiar/dist
	cd .. && mkdir -p familiar/dist/CONTROL
	cd .. && echo "Package: " $(PACKAGE)  > familiar/control
	cd .. && echo "Section: base"        >> familiar/control
	cd .. && echo "Priority: optional"   >> familiar/control
	cd .. && echo "Version: " $(VERSION) >> familiar/control
	cd .. && echo "Architecture: arm"    >> familiar/control
	cd .. && echo "Maintainer: Luc Pionchon <luc.pionchon@welho.com>" >> familiar/control
	cd .. && echo "Depends: libgtk1.2, libgpewidget0" >> familiar/control
	cd .. && echo "Description: a notebook to sketch your notes" >> familiar/control
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
	~/bin/move-FROM-handhelds.org $(DIST_NAME) src/$(PACKAGE)-$(VERSION)-arm-striped
#	~/bin/move-FROM-handhelds.org $(DIST_NAME) $(IPKG_NAME)
