#! /bin/sh
if test -x /usr/X11R6/bin/xcalibrate; then
    # don't look at xcalibrate's exit code, it always returns 1:
    /usr/X11R6/bin/xcalibrate;
    /usr/X11R6/bin/xcalibrate -view >/etc/xcalibrate.conf;

    ## Beginning with xcalibrate 0.2-2, we could use the line below,
    ## but it's maybe too dangerous until ipkg supports versioned dependencies:    
    # /usr/X11R6/bin/xcalibrate && /usr/X11R6/bin/xcalibrate -view >/etc/xcalibrate.conf;
fi
if test -x /usr/bin/xtscal; then
    xtscal >/etc/xcalibrate.conf
fi
