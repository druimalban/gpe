#!/bin/sh

gpe-question --icon /usr/share/pixmaps/gpe-logout.png --question "<span weight="bold" size="larger">Are you sure you want to log out?</span> 

Unsaved data from applications will be lost." --buttons !gtk-cancel "ok:Log out" && mbcontrol -exit
