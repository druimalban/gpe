#
# treecc-native_0.3.6.bb - OpenEmbedded bb file for the native compiled 
#		version of the DotGNU Tree Compiler Compiler ver. 0.3.6
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

include treecc_${PV}.bb
SECTION = "devel"
FILESDIR = "${@os.path.dirname(bb.data.getVar('FILE',d,1))}/treecc-${PV}"
MAINTAINER = "Kirill Kononenko <Kirill.Kononenko@gmail.com>"
PROVIDES = "treecc-native"
DEPENDS = "bison flex"
S = "${WORKDIR}/treecc-${PV}"
inherit native autotools

do_stage () {
      rm -f ${STAGING_BINDIR}/treecc
      echo ${STAGING_BINDIR}
      install -m 0755 ./treecc ${STAGING_BINDIR}/
} 
