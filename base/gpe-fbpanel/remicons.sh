#!/bin/bash
wins=`xprop -root | grep '_NET_CLIENT_LIST(WINDOW)' | sed -e 's/^.*id #//' -e 's/,//g'`
for i in $wins; do
    echo win=$i
    xprop -id $i -remove _NET_WM_ICON
    echo
done
