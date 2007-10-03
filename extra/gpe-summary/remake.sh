 #!/bin/sh
#make clean
af-sb-init.sh stop
#rm config.sub
#rm config.guess
#rm ltmain.sh
#./autogen.sh --force
dpkg-buildpackage -rfakeroot 
#dpkg -i ../
dpkg -i ../gpesummary_0.7.0_armel.deb