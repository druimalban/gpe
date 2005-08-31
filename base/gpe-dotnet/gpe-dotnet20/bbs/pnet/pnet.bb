#
# pnet_0.7.2.bb - OpenEmbedded bb file for the native and cross-compiled 
#	 	versions of the Portable .NET 0.7.2 execution engine
#
# Copyright (C) 2005  Kirill Kononenko <Kirill.Kononenko@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

DESCRIPTION = "Portable .NET execution engine from dotGNU"
SECTION = "devel"
LICENSE = "GPL-2"
HOMEPAGE = "http://www.southern-storm.com.au"
MAINTAINER = "Kirill Kononenko <Kirill.Kononenko@gmail.com>"
PRIORITY = "optional"
PROVIDES = "pnet"
inherit autotools

do_fetch () {
cd ${WORKDIR}
export CVS_RSH="ssh"
cvs -z4 -d :ext:anoncvs@savannah.gnu.org:/cvsroot/dotgnu-pnet co ./pnet/
if (test -f pnet_cvs.patch); then 
echo nothing
else
wget http://handhelds.org/~krokas/pnet_cvs.patch
patch -p0 < pnet_cvs.patch 
fi
}

do_unpack () {
}

do_configure () {
# make clean
# CFLAGS="-O0 -g -march=armv4 -mtune=xscale"
./auto_gen.sh
oe_runconf
}
