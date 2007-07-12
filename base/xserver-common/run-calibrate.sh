#! /bin/sh

if [ -x /usr/bin/xtscal ]; then
	exec /usr/bin/xtscal
else
	if [ -x /usr/bin/ts_calibrate ]; then
	        exec /usr/bin/ts_calibrate
	fi
fi

exec xtscal

