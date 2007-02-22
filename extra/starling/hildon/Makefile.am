desktopdir = /usr/share/applications/hildon
desktop_DATA = starling.desktop

servicesdir = /usr/share/dbus-1/services
services_DATA = com.nokia.starling.service

EXTRA_DIST = starling.desktop.in com.nokia.starling.service.in

install-data-hook:
	ln -s /usr/share/applications/hildon/starling.desktop \
	 /etc/others-menu/0010_starling.desktop