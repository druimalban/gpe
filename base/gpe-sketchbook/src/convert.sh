#!/bin/bash
#
# 2004-01-23
#
# 1. copies user sketches to the new folder under ~/.gpe
# 
# 2. generate SQL INSERT queries to register the sketches into the database
#

REPOS=~/.sketchbook
FILES=`cd $REPOS && ls *.png`

NEW_REPOS=~/.gpe/sketchbook
mkdir  $NEW_REPOS &> /dev/null

for file in $FILES
  do
  install --mode=644  --preserve-timestamps $REPOS/$file $NEW_REPOS/$file
done


n=0

echo "# type, title, created timestamp, updated timestamp, url"

for file in $FILES
  do

  #--Update date
  listing=`ls -l --time-style=+%s  $REPOS/$file`
  t=${listing%* *}
  updated=${t#* * * * * * *}

  #--Creation date
  # assuming format 2002-08-18_09-12-42.png
  #                 _123456789_12345678
  _YY=${file:0:4}
  _MM=${file:5:2}
  _DD=${file:8:2}
  _hh=${file:11:2}
  _mm=${file:14:2}
  _ss=${file:17:2}

  created=`date --date="$_MM/$_DD/$_YY  $_hh:$_mm:$_ss" +%s 2> /dev/null || echo $updated`


  (( n += 1 ))

  #                             type  title                   created     updated     url
  echo "INSERT INTO notes VALUES(0, 'Imported Sketch "$n"', "$created", "$updated", '"$NEW_REPOS/$file"');"

done

exit 0
