#
# pnet-gpe_cvs.bb - OpenEmbedded bb file for the cross-compiled version of the 
#			Portable .NET 0.7.2 execution engine
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

include pnet.bb
SECTION = "devel"
FILESDIR = "${@os.path.dirname(bb.data.getVar('FILE',d,1))}/pnet"
EAINTAINER = "Kirill Kononenko <Kirill.Kononenko@gmail.com>"
S = "${WORKDIR}/pnet"
PROVIDES = "pnet-gpe-0.7.2"
DEPENDS = "treecc-native"
inherit autotools

do_install () {
    mkdir -p ${D}/usr/bin
    cd ${D}/usr/bin/
    install -m 0766 ${WORKDIR}/pnet/engine/ilrun ${D}/usr/bin/
    install -m 0766 ${WORKDIR}/pnet/ildd/ildd ${D}/usr/bin/
    echo ${D}
    echo ${WORKDIR}
    echo ${P}
}


