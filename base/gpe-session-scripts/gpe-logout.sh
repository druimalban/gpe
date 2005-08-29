#!/bin/sh

gpe-question --icon /usr/share/pixmaps/gpe-logout.png --question "<span weight='bold' size='larger'>Are you sure you want to log out?</span> 

Unsaved data from applications will be lost." --buttons !gtk-cancel "ok:Log out"

# check for button number returned by gpe-question:
if [ $? -eq 1 ]; then
    echo "Logout.";
  if [ -x /usr/bin/matchbox-remote ]; then
    /usr/bin/matchbox-remote -exit;
  else
    matchbox-remote -exit;
fi
# hack to run on simpad too
    killall metacity; 
else
    echo "Logout cancelled.";
fi

