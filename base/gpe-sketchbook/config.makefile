VERSION = 0.2.7-1

PACKAGE = gpe-sketchbook

LINGUAS = fr pt de zh_TW ro sv nl cs sk ru sr
MEMBERS = \
	gpe-sketchbook       \
        preferences          \
	dock                 \
	selector	     \
	selector-cb	     \
	selector-gui	     \
	sketchpad	     \
	sketchpad-cb	     \
	sketchpad-gui	     \
	files 

ifndef PREFIX
PREFIX = /usr/local
endif

ifndef DEBUG
DEBUG = no
endif

ifndef CVSBUILD
CVSBUILD = yes
endif
