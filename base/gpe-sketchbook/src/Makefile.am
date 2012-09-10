
INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir)

DEFS = @DEFS@ -DPREFIX=\"@prefix@\" -D_GNU_SOURCE \
	   -DPACKAGE_LOCALE_DIR=\"@gpesketchbook_localedir@\"

bin_PROGRAMS = gpe-sketchbook

gpe_sketchbook_LDADD = $(DEPS_LIBS) -lm

gpe_sketchbook_SOURCES = \
	cursor_eraser_data.h \
	db.c db.h \
	dock.c dock.h \
	files.c files.h \
	gpe-sketchbook.c gpe-sketchbook.h \
	preferences.c preferences.h \
	selector.c selector.h \
	selector-cb.c selector-cb.h \
	selector-gui.c selector-gui.h \
	sketchpad.c sketchpad.h \
	sketchpad-cb.c sketchpad-cb.h \
	sketchpad-gui.c sketchpad-gui.h

clean-local:
	rm -f *~

#--reminders
fix: fixme
fixme:
	@echo "NO! that does NOT fix anything! ;)"
	echo "" && grep 'FIXME:' *.h *.c -n
	@echo "+----------------+"
	@echo ' Left to fix: ' `grep 'FIXME:' *.h *.c|wc -l`
	@echo "+----------------+"
note:
	grep 'NOTE:' *.h *.c -n

