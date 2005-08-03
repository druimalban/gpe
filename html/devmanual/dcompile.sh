#!/bin/bash
#
# Compile DocBook documents into several output formats.
#
# Godoy.
# 19991230 - Initial release.
# 20000117 - Placed the options using "case" and parameters passed 
#        via command line. The pages on the Zope are already updated.
#         --- Removed to public version (/home/ldp). 
# 20000120 - Placed the call to use the books.dtd.
# 20000126 - Placed the commands for the index generation.
# 

# If the jade is already installed, disconsider the line bellow.
 JADE=/usr/bin/jade

# If the jade package is already installed, disconsider the line bellow.
# JADE=/usr/bin/openjade

DOCUMENT=$1
shift 1
TYPE=$1

. ~/.bash_profile
. ~/.bashrc

case $TYPE in
    html)
       rm -f *.htm
       rm -f *.html
       perl ./collateindex.pl -N -o index.sgml 
       jade -t sgml -V html-index -d /usr/share/sgml/docbook/stylesheet/dsssl/modular/html/docbook.dsl $DOCUMENT.sgml
       perl ./collateindex.pl -o index.sgml HTML.index
       $JADE -t sgml -i html -d /usr/share/sgml/docbook/stylesheet/dsssl/modular/html/docbook.dsl #html $DOCUMENT.sgml
    ;;
    rtf)
       rm -f $DOCUMENT.rtf
       perl ./collateindex.pl -N -o index.sgml 
       jade -t sgml -V html-index -d /usr/share/sgml/docbook/stylesheet/dsssl/modular/html/docbook.dsl $DOCUMENT.sgml
       perl ./collateindex.pl -o indice.sgml HTML.index
       $JADE -t rtf -V rtf-backend -d /usr/share/sgml/docbook/stylesheet/dsssl/modular/print/docbook.dsl -d /home/ldp/SGML/conectiva/books.dsl#print $DOCUMENT.sgml
    ;;
    xml)
       rm -f $DOCUMENT.xml
       perl ./collateindex.pl -N -o index.sgml 
       jade -t sgml -V html-index -d /usr/share/sgml/docbook/stylesheet/dsssl/modular/html/docbook.dsl $DOCUMENT.sgml
       perl ./collateindex.pl -o indice.sgml HTML.index
       $JADE -t sgml -i xml -d /home/ldp/SGML/style/xsl/docbook/html/docbook.xsl $DOCUMENT.sgml
    ;;
    tex)
       rm -f $DOCUMENT.tex
       perl ./collateindex.pl -N -o indice.sgml 
       jade -t sgml -V html-index -d /usr/share/sgml/docbook/stylesheet/dsssl/modular/html/docbook.dsl $DOCUMENT.sgml
       perl ./collateindex.pl -o indice.sgml HTML.index
       $JADE -t tex -V tex-backend -d /usr/share/sgml/docbook/stylesheet/dsssl/modular/print/docbook.dsl -d /home/ldp/SGML/conectiva/livros.dsl#print $DOCUMENT.sgml
    ;;
    dvi)
       rm -f $DOCUMENT.tex
       rm -f $DOCUMENT.dvi
       perl ./collateindex.pl -N -o indice.sgml 
       jade -t sgml -V html-index -d /usr/share/sgml/docbook/stylesheet/dsssl/modular/html/docbook.dsl $DOCUMENT.sgml
       perl ./collateindex.pl -o indice.sgml HTML.index
       $JADE -t tex -V tex-backend -d /usr/share/sgml/docbook/stylesheet/dsssl/modular/print/docbook.dsl -d /home/ldp/SGML/conectiva/livros.dsl#print $DOCUMENT.sgml
       jadetex $DOCUMENT.tex
    ;;
    mirror)
       rm -f $DOCUMENT.tex
       rm -f $DOCUMENT.dvi
       rm -f $DOCUMENT.mirror.ps
       perl ./collateindex.pl -N -o indice.sgml 
       jade -t sgml -V html-index -d /usr/share/sgml/docbook/stylesheet/dsssl/modular/html/docbook.dsl $DOCUMENT.sgml
       perl ./collateindex.pl -o indice.sgml HTML.index
       $JADE -t tex -V tex-backend -d /usr/share/sgml/docbook/stylesheet/dsssl/modular/print/docbook.dsl -d /home/ldp/SGML/conectiva/livros.dsl#print $DOCUMENT.sgml
       jadetex $DOCUMENT.tex
       dvips -h /home/ldp/estilos/skel/mirr.hd -O 1.5cm,3cm -f $DOCUMENT.dvi -o $DOCUMENT.mirror.ps
    ;;
    ps)
       rm -f $DOCUMENT.tex
       rm -f $DOCUMENT.dvi
       rm -f $DOCUMENT.ps
       perl ./collateindex.pl -N -o indice.sgml 
       jade -t sgml -V html-index -d /usr/share/sgml/docbook/stylesheet/dsssl/modular/html/docbook.dsl $DOCUMENT.sgml
       perl ./collateindex.pl -o indice.sgml HTML.index
       $JADE -t tex -V tex-backend -d /usr/share/sgml/docbook/stylesheet/dsssl/modular/print/docbook.dsl #print $DOCUMENT.sgml
       jadetex $DOCUMENT.tex
       dvips -The 1.5cm,3cm -f $DOCUMENT.dvi -o $DOCUMENT.ps
    ;;
    *)
       echo "How to use: $0 file {html|tex|rtf|xml|ps|dvi|mirror}"
       exit 1
       esac

exit 0
