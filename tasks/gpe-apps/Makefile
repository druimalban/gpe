ipkg:
	rm -rf dist
	mkdir -p dist/CONTROL
	cp control dist/CONTROL
	ipkg-build dist

	rm -rf dist
	mkdir -p dist/CONTROL
	cp control1 dist/CONTROL/control
	ipkg-build dist

	rm -rf dist.c1
	mkdir -p dist.c1/CONTROL
	cp control.collateral1 dist.c1/CONTROL/control
	ipkg-build dist.c1

	rm -rf dist.c2
	mkdir -p dist.c2/CONTROL
	cp control.collateral2 dist.c2/CONTROL/control
	ipkg-build dist.c2
