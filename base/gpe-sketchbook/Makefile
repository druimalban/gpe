include config.makefile

all: $(PACKAGE) #mo

#--package
$(PACKAGE):
	cd src && make $(PACKAGE)
arm:
	cd src && make arm

##--i10n
#mo:
#	make -C po
#
#install-po:
#	make -C po install DESTDIR=$(DESTDIR) PREFIX=$(PREFIX)
#
#update-po: po/$(PACKAGE).pot
#
#po/$(PACKAGE).pot: $(SOURCES)
#	xgettext -k_ -o po/$(PACKAGE).pot $^

#--deployment
ARM_BIN = $(PACKAGE)-$(VERSION)-arm-striped
install: all #install-po
	for i in $(PIXMAPS); do install -D pixmaps/$$i $(DESTDIR)$(PREFIX)/share/$(PACKAGE)/pixmaps/$$i; done
	install -D src/$(ARM_BIN) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -D $(PACKAGE).menu $(DESTDIR)$(PREFIX)/lib/menu/$(PACKAGE)
	install -D $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/$(PACKAGE).png


CONTROL = familiar/dist/CONTROL
ipkg:
	rm -rf familiar/dist
	mkdir -p $(CONTROL)
	echo "Package: "  $(PACKAGE)   > $(CONTROL)/control
	echo "Section: "  $(SECTION)  >> $(CONTROL)/control
	echo "Priority: " $(PRIORITY) >> $(CONTROL)/control
	echo "Version: "  $(VERSION)  >> $(CONTROL)/control
	echo "Architecture: arm"      >> $(CONTROL)/control
	echo "Maintainer: "$(MAINTAINER)" <"$(MAINTAINER_MAIL)">" >> $(CONTROL)/control
	echo "Depends: "  $(DEPENDS)  >> $(CONTROL)/control
	echo "Description: "$(PACKAGE_DESCR) >> $(CONTROL)/control
	cp familiar/postinst $(CONTROL)
	make DESTDIR=`pwd`/familiar/dist PREFIX=/usr install
	chown -R root.root familiar/dist
#	ipkg-build familiar/dist || ./utils/ipkg-build familiar/dist
	./utils/ipkg-build familiar/dist

DIST_NAME = $(PACKAGE)-$(VERSION)
#DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(EXTRA_DIST)
dist:
	rm -rf $(DIST_NAME)       # needs improvment! :)
	mkdir $(DIST_NAME)
	cp Makefile config.makefile $(DIST_NAME)
	mkdir $(DIST_NAME)/utils
	cp utils/????* $(DIST_NAME)/utils
	mkdir $(DIST_NAME)/src
	cp src/*.c src/*.h src/Makefile src/.cvsignore $(DIST_NAME)/src
	mkdir $(DIST_NAME)/pixmaps
	cp pixmaps/*.png $(DIST_NAME)/pixmaps
	mkdir $(DIST_NAME)/familiar
	cp familiar/postinst $(DIST_NAME)/familiar
	tar zcf $(DIST_NAME).tgz $(DIST_NAME)
	rm -rf $(DIST_NAME)

