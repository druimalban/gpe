#!/bin/sh
module_id() {
    grep "Module ID" /proc/hal/assets | sed "s/.*://"
    }

case $1 in
'start')
	DIR=/usr/share/gpe/bootsplash
	FN=""
# start off server in conventional location.
	echo `module_id`
	case `module_id` in
		" iPAQ 3100" )	    FN="3100" ;;
		" iPAQ 3600" )	    FN="3600" ;;
		" iPAQ 3700" )	    FN="3700" ;;
		" iPAQ 3800" )	    FN="3800" ;;
	esac
	[ $FN ] && gunzip -c < $DIR/$FN > /dev/fb0
	;;
'stop')
;;
*)
esac
