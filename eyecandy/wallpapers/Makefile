PREFIX = /usr
WALLPAPERS = gnu bigeyetux penguin

all:
	for i in $(WALLPAPERS); do \
	  rm -rf familiar/dist; \
	  mkdir -p familiar/dist/CONTROL; \
	  echo "Package: wallpaper-ayo-$$i" > familiar/dist/CONTROL/control; \
	  cat familiar/control >> familiar/dist/CONTROL/control; \
	  mkdir -p familiar/dist/$(PREFIX)/share/wallpapers/; \
	  install -m 644 ayo_$$i.png familiar/dist/$(PREFIX)/share/wallpapers/ayo_$$i.png; \
	  fakeroot "chown -R root.root familiar/dist; ipkg-build familiar/dist";\
	done

	rm -rf familiar/dist
	mkdir -p familiar/dist/CONTROL
	echo "Package: wallpapers-ayo-all" > familiar/dist/CONTROL/control
	cat familiar/control >> familiar/dist/CONTROL/control
	echo -n "Depends: " >> familiar/dist/CONTROL/control
	mkdir -p familiar/dist/$(PREFIX)/share/wallpapers/
	for i in $(WALLPAPERS); do \
	  echo -n "wallpapers-ayo-$$i, " >> familiar/dist/CONTROL/control; \
	done
	cat familiar/dist/CONTROL/control | sed -e "s/, $$//" > familiar/dist/CONTROL/control
	fakeroot "chown -R root.root familiar/dist; ipkg-build familiar/dist"
	rm -rf familiar/dist

#	if test -e familiar/conffiles; then install -m 644 familiar/conffiles familiar/dist/CONTROL; fi
#	if test -e familiar/preinst;   then install familiar/preinst   familiar/dist/CONTROL; fi
#	if test -e familiar/postinst;  then install familiar/postinst  familiar/dist/CONTROL; fi
#	if test -e familiar/prerm;     then install familiar/prerm     familiar/dist/CONTROL; fi
#	if test -e familiar/postrm;    then install familiar/postrm    familiar/dist/CONTROL; fi
