#
# gpe-dotnet_0.2.0.bb - OpenEmbedded bb file of the GPE DotNet ver. 0.2.0
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

DESCRIPTION = ".NET for GPE"
SECTION = "devel"
LICENSE = "GPL-2"
HOMEPAGE = "http://www.southern-storm.com.au"
MAINTAINER = "Kirill Kononenko <Kirill.Kononenko@gmail.com>"
PRIORITY = "optional"
DEPENDS = "treecc-native pnet-native-0.7.2 pnet-gpe-0.7.2 libxft libxrender gpe-gtk-sharp-1.9.3"
PROVIDES = "gpe-dotnet"
FILES_${PN} += "/usr/lib/cscc/lib/*.* /usr/share/cscc/config/*.default"
PKG_${PN} = "gpe-dotnet"
# CFLAGS = "-O0 -g"

inherit autotools

do_fetch () {
cd ${WORKDIR}
export CVS_RSH="ssh"
cvs -z4 -d :ext:anoncvs@savannah.gnu.org:/cvsroot/dotgnu-pnet co ./pnetlib/
}

do_unpack () {
}

do_configure () {
    CUR_DIR=$(pwd)
    cd ..
    cd pnetlib
    for i in $(dir); do 
    cp -r ${i} ${CUR_DIR}    
    done
    cd ..
    rm -r -f pnetlib
    cd ${CUR_DIR}
    ./auto_gen.sh
    oe_runconf
}


do_install () {
    rm -r -f ${D}/usr/lib/cscc/lib/
    mkdir -p ${D}/usr/lib/cscc/lib/
    rm -r -f ${D}/usr/share/cscc/config/
    mkdir -p ${D}/usr/share/cscc/config/
    mkdir -p ${D}/usr/bin/
    rm -f ${D}/usr/bin/ilrun
    rm -f ${D}/usr/bin/ildd
    install -m 0755 ${WORKDIR}/../pnet-gpe-0.7.2-r0/install/pnet-gpe/usr/bin/ilrun ${D}/usr/bin/
    install -m 0755 ${WORKDIR}/../pnet-gpe-0.7.2-r0/install/pnet-gpe/usr/bin/ildd ${D}/usr/bin/
    install -m 0755 ${WORKDIR}/${P}/DotGNU.Images/DotGNU.Images.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/DotGNU.Misc/DotGNU.Misc.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/DotGNU.SSL/DotGNU.SSL.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/DotGNU.Terminal/DotGNU.Terminal.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/Xsharp/Xsharp.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/Xsharp/.libs/libXsharpSupport.a ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/Xsharp/.libs/libXsharpSupport.la ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/Xsharp/.libs/libXsharpSupport.lai ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/Xsharp/.libs/libXsharpSupport.so ${D}/usr/lib/cscc/lib/  
    install -m 0755 ${WORKDIR}/${P}/Xsharp/.libs/libXsharpSupport.so.0 ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/Xsharp/.libs/libXsharpSupport.so.0.0.0 ${D}/usr/lib/cscc/lib/      
    install -m 0755 ${WORKDIR}/${P}/Xsharp/.libs/Xsharp*.o ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/OpenSystem.Platform/OpenSystem.Platform.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/SharpZipLib/ICSharpCode.SharpZipLib.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/System/System.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/System.Configuration.Install/System.Configuration.Install.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/System.Deployment/System.Deployment.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/System.Design/System.Design.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/System.Drawing/System.Drawing.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/System.Drawing.Xsharp/System.Drawing.Xsharp.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/System.EnterpriseServices/System.EnterpriseServices.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/System.Net.IrDA/System.Net.IrDA.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/System.Windows.Forms/System.Windows.Forms.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/System.Xml/System.Xml.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/runtime/mscorlib.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/compat/Accessibility.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/compat/sysglobl.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/I18N/*.dll ${D}/usr/lib/cscc/lib/
    install -m 0755 ${WORKDIR}/${P}/config/machine.default ${D}/usr/share/cscc/config/
    install -m 0755 ${WORKDIR}/${P}/tools/pinvoke.map ${D}/usr/lib/cscc/lib/
    BACK_DIR=$(pwd)
    cd ${WORKDIR}
    cd ..
    cd 'gpe-gtk-sharp-1.9.3-r0/gpe-gtk-sharp-1.9.3/'
    CUR_DIR=$(pwd)
    install -m 0755 ${CUR_DIR}/gdk/glue/.libs/*.so ${D}/usr/lib/cscc/lib/
    install -m 0755 ${CUR_DIR}/glib/glue/.libs/*.so ${D}/usr/lib/cscc/lib/
    install -m 0755 ${CUR_DIR}/gtk/glue/.libs/*.so ${D}/usr/lib/cscc/lib/
    install -m 0755 ${CUR_DIR}/pango/glue/.libs/*.so ${D}/usr/lib/cscc/lib/
    install -m 0755 ${CUR_DIR}/dlls/*.dll ${D}/usr/lib/cscc/lib/
    cd ${BACK_DIR}
}
