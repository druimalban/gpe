#
# gpe-gtksharp_1.0.8.bb - OpenEmbedded bb file for the cross-compiled 
#	 	versions of the Gtk# for GPE DotNET 0.1.5
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
DEPENDS = "glib-2.0 pango gtk+ libxml2"
PROVIDES = "gpe-gtk-sharp-1.9.3"
SRC_URI = "http://handhelds.org/~krokas/gpe-gtk-sharp-1.9.3.tar.gz"

CFLAGS = "-O0 -g"

inherit autotools

do_compile () {
    CUR_DIR=$(pwd)
    cd ${CUR_DIR}/gdk/glue/
    oe_runmake
    cd ${CUR_DIR}/glade/glue/
    oe_runmake
    cd ${CUR_DIR}/glib/glue/
    oe_runmake
    cd ${CUR_DIR}/gnome/glue/
    oe_runmake
    cd ${CUR_DIR}/gtk/glue/
    oe_runmake
    cd ${CUR_DIR}/pango/glue/
    oe_runmake
}
do_install () {
}
