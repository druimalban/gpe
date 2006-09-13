#!/bin/bash
wins=`xprop -root | grep '_NET_CLIENT_LIST(WINDOW)' | sed -e 's/^.*id #//' -e 's/,//g'`
for i in $wins; do
    echo win=$i
    xprop -id $i _NET_WM_ICON 
    #| sed 's/=\( [^,]*, [^,]*, [^,]*,\).*/=\1/'

    echo
done
