include ./config.makefile

ifeq ($(GTK2),yes)
CONTROL_FILE = familiar/control1
else
CONTROL_FILE = familiar/control
endif


SOURCES = $(patsubst %,src/%.c,$(MEMBERS))

PIXMAPS = `cd pixmaps && ls *.png`

all: $(PACKAGE) all-mo

#--package
$(PACKAGE):
	cd src && make $(PACKAGE)
arm:
	cd src && make arm

#--deployment
#ARM_BIN = $(PACKAGE)-$(VERSION)-arm-striped
ARM_BIN = $(PACKAGE)
install:  install-mo $(PACKAGE).desktop #all
	for i in $(PIXMAPS); do \
	install -D pixmaps/$$i     $(DESTDIR)$(PREFIX)/share/gpe/pixmaps/default/$(PACKAGE)/$$i;\
	done
	install -D src/$(ARM_BIN)  $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -D $(PACKAGE).menu $(DESTDIR)$(PREFIX)/lib/menu/$(PACKAGE)
	install -D $(PACKAGE).png  $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png
	install -D -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/$(PACKAGE)

clean: clean-po
	rm -rf *~ .*~
	cd src && make clean

include ../build/Makefile.translation
include ../build/Makefile.dpkg_ipkg

