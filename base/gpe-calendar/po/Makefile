LINGUAS = de pt_PT

.SUFFIXES: .mo .po

mo-files = $(patsubst %,%.mo,$(LINGUAS))

all: $(mo-files)

install: DESTDIR := `pwd`/../translation/familiar/dist
install: PREFIX  := $(PREFIX)
install: all
	for i in $(LINGUAS); do install -D $$i.mo $(DESTDIR)$(PREFIX)/share/locale/$$i/LC_MESSAGES/gpe-calendar.mo; done

.po.mo:;
	msgfmt -o $@ $<

# ------------------------------------------------------------------------

VERSION    = $(shell grep 'Version: '    ../familiar/control | cut -d ' ' -f 2)
MAINTAINER = $(shell grep 'Maintainer: ' ../familiar/control | cut -d ' ' -f 2-)
LINGUAS-LC = $(shell echo $(LINGUAS) | tr '[A-Z_]' '[a-z-]')

translation-package: install
	rm -rf ../translation
	install -D ../../build/familiar/control.translation  ../translation/familiar/control
	install    ../../build/familiar/postinst.translation ../translation/familiar
	sed -e 's/<version>/$(VERSION)/' ../translation/familiar/control > ../translation/familiar/control.version
	sed -e 's/<maintainer>/$(MAINTAINER)/' ../translation/familiar/control.version > ../translation/familiar/control.maintainer
	for i in $(LINGUAS-LC); do \
		sed -e 's/<package>/$(PACKAGE)/' ../translation/familiar/control.maintainer > ../translation/familiar/control.package; \
		sed -e "s/<language>/$$i/" ../translation/familiar/control.package > ../translation/familiar/control.lang.$$i; \
		mkdir -p ../translation/familiar/$$i/dist/CONTROL; \
		cp ../translation/familiar/control.lang.$$i ../translation/familiar/$$i/dist/CONTROL/control; \
		cp ../translation/familiar/postinst ../translation/familiar/$$i/dist/CONTROL; \
		chown -R root.root ../translation/familiar/$$i/dist; \
		ipkg-build ../translation/familiar/$$i/dist; \
		mv *.ipk ..; \
	done
