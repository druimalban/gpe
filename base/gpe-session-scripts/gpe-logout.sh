#!/bin/sh

gpe-question --icon /usr/share/pixmaps/gpe-logout.png --question "Are you sure you want to log out?" --buttons "ok:Log out" !gtk-cancel && mbcontrol -exit
