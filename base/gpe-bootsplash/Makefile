#set up the symlinks for the package
all:

symlinks:
	ln -sf 3600 dist/usr/share/gpe/bootsplash/3700
	ln -sf ../init.d/gpe-bootsplash.sh dist/etc/rc2.d/S00gpe-bootsplash.sh

ipk: symlinks
	ipkg-build dist
