#------------------------
# Makefile for GPE tasks
#------------------------
#
# * Change the content of a task:
#
#   1. edit       DEP_<task-name>
#   2. increment  VERSION
#
#
# * Add a task:
#
#   1. create a file <task-name>.control,
#      including _SOURCE_, _VERSION_ and _DEPS_ variables
#   2. create a variable DEP_<task-name>
#   3. add the task to ALL_TASKS
#
#   that's it.
#------------------------

VERSION = 0.45

ALL_TASKS = \
    gpe2 \
    \
    gpe-collateral-gtk2 \
    \
    gpe-task-base \
    gpe-task-pim \
    gpe-task-utils \
    gpe-task-comm \
    gpe-task-mmedia \
    gpe-task-games \
    gpe-task-extra-apps \
    gpe-task-extra-games \

default: $(ALL_TASKS)
	make printinfo

#-----------------------------------------------------
#DIST_FILES = Makefile $(patsubst %,%.control,$(ALL_TASKS))
PACKAGE = gpe-tasks-src

ifeq ($(CVSBUILD),yes)
BUILD = ../base/build
else
BUILD = build
endif

$(BUILD)/Makefile.dpkg_ipkg:
	@echo "Couldn't find Makefile.dpkg_ipkg.  Maybe you need to set 'CVSBUILD=yes'?"
	exit 1

include $(BUILD)/Makefile.dpkg_ipkg


TMP_FILE1 = __tmp1
TMP_FILE2 = __tmp2
% : %.control
	@rm -rf dist
	@mkdir -p dist/CONTROL
	@sed 's:_VERSION_:$(VERSION):'  < $<           > $(TMP_FILE1)
	@sed 's:_DEPS_:$(DEP_$@):'      < $(TMP_FILE1) > $(TMP_FILE2)
	@sed 's$$_SOURCE_$$$(SOURCE)$$' < $(TMP_FILE2) > dist/CONTROL/control
	@rm -rf $(TMP_FILE1) $(TMP_FILE2)
	@ipkg-build -o root -g root dist
ifeq ($(SIGN),yes)
	gpg -sb --armor $@_$(VERSION)_all.ipk
endif

clean:
	rm -rf dist *.ipk *.ipk.asc *~ .*~
	rm -rf familiar/dist familiar/*~


#====================================================
#================= TASKS DEFINITION =================

#---Meta tasks
DEP_gpe2 = \
    gpe-task-base, \
    gpe-task-comm, \
    gpe-task-mmedia, \
    gpe-task-pim, \
    gpe-task-utils, \
    gpe-task-games
#    gpe-task-extra-apps \


#---This is the base to run GPE
DEP_gpe-task-base = \
    \
    gpe-collateral-gtk2, \
    gdk-pixbuf2-loader-jpeg, \
    \
    gpe-bootsplash2, \
    gpe-dm, \
    gpe-login2, \
    gpe-session-scripts, \
    gpe-terminal, \
    \
    keylaunch, \
    esd, \
    detect-stylus, \
    gpe-soundserver, \
    gpe-mimedir-tools, \
    teleport, \
    gpe-keylock, \
    \
    matchbox, \
    xmonobut, \
    minilite, \
    minibat, \
    minipredict, \
    matchbox-panel-hacks, \
    mbinputmgr, \
    \
    xstroke, \
    rosetta, \
    xkbd, \
    mbmerlin, \
    \
    startup-monitor, \
    \
    xtscal, \
    gpe-su2, \
    gpe-conf, \
    gpe-confd, \
    xst, \
    \
    gpe-what, \
    gpe-question2, \
    gpe-announce2, \
    \
    gpe-wlancfg, \
    gpe-aerial, \
    hotplug-dbus, \
    gpe-autostarter, \
    nsqld
#FIXME: could be refined (contains hardware dependant packages)

DEP_gpe-collateral-gtk2 = \
    gtk2-theme-angelistic, \
    ttf-bitstream-vera

#------------------------------------------
# The following aims to be USER LEVEL tasks
#------------------------------------------

#------------------------------------------ PIM applications
DEP_gpe-task-pim = \
    gpe-task-base, \
    gpe-calendar2, \
    gpe-soundbite2, \
    gpe-contacts2, \
    gpe-sketchbook2, \
    gpe-today, \
    gpe-todo2, \
    \
    gpe-minicalc2, \
    gpe-calculator2, \
    \
    figment, \
    gpe-timesheet2, \
    greader, \
    gpe-watch
#FIXME: 2 calculators!

#------------------------------------------ System utilities
DEP_gpe-task-utils = \
    gpe-task-base, \
    gpe-edit2, \
    gpe-taskmanager

#------------------------------------------ Communication apps
DEP_gpe-task-comm = \
    gpe-task-base, \
    dillo2, \
    gpe-irc, \
    linphone
#FIXME: add email/contacts

#------------------------------------------ Multimedia apps
DEP_gpe-task-mmedia = \
    gpe-task-base, \
    gpe-gallery, \
    gpe-nmf, \
    gpe-mixer

#------------------------------------------ Extra apps
DEP_gpe-task-extra-apps = \
    gpe-task-base, \
    gpe-filemanager2

#------------------------------------------ Games
DEP_gpe-task-games = \
    gpe-tetris2, \
    xdemineur, \
    gpe-lights, \
    gpe-code2, \
    gpe-julia2, \
    gpe-othello2, \
    gsoko2, \
    pdamaze
#FIXME: couple of games to move to extra-games.

#------------------------------------------ Extra games
DEP_gpe-task-extra-games = \
    repton, \
    sfcave-sdl, \
    snes9x

