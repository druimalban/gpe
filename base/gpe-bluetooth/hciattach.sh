#!/bin/sh

case `cat /proc/hal/model` in
	3800)
		exec hciattach -n /dev/ttySB0 bcsp 230400
		;;
	3900)
		exec hciattach -n /dev/tts/1 bcsp 921600
		;;
	*)
		echo "unknown machine type" >&2
		exit 1
		;;
esac
