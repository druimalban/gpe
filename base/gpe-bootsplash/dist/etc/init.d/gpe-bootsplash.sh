#!/bin/sh

module_id() {
    # Get model name
    echo `grep "^Hardware" /proc/cpuinfo | sed -e "s/.*: *//" | tr a-z A-Z`
}

depmod

case $1 in
'start')
	case `module_id` in
		"HP IPAQ H3100" )
			ARGS="--mono" ;;
		"HP IPAQ H3600" | "HP IPAQ H3700" | "HP IPAQ H3900" )
		    	ARGS="--flip" ;;
		"HP IPAQ H3800" | "HP IPAQ H5400" | "HP IPAQ H2200" )
			;;
	esac
	/usr/bin/gpe-bootsplash $ARGS
	;;
'stop')
;;
*)
esac
