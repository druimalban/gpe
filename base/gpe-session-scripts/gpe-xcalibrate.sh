#! /bin/sh
if test -x /usr/X11R6/bin/xcalibrate; then
    # don't look at xcalibrate's exit code, it always returns 1:
    /usr/X11R6/bin/xcalibrate;
    /usr/X11R6/bin/xcalibrate -view >/etc/xcalibrate.conf;
fi
