#! /bin/sh

if [ -x /usr/bin/ts_calibrate ]; then
	exec /usr/bin/ts_calibrate
else
	if [ -x /usr/bin/xtscal ]; then
	        exec /usr/bin/xtscal
	fi
fi

exec xtscal

