#------------------------
# Makefile for GPE tasks
#------------------------
#
# * Change the content of a task:
#
#   1. edit       DEP_<task-name>
#   2. incremente VER_<task-name>
#
#
# * Add a task:
#
#   1. create a file <task-name>.control, including _VERSION_ and _DEPS_ variables
#   2. create a variable DEP_<task-name>
#   3. create a variable VER_<task-name>
#   4. add the task to ALL_TASKS
#
#   that's it.
#------------------------

ALL_TASKS = \
    gpe2 \
    gpe-apps2 \
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

TMP_FILE = temporary_file_which_will_be_destroyed_without_warning
% : %.control
	@rm -rf dist
	@mkdir -p dist/CONTROL
	@sed 's:_VERSION_:$(VER_$@):' < $< > $(TMP_FILE)
	@sed 's:_DEPS_:$(DEP_$@):' < $(TMP_FILE) > dist/CONTROL/control
	@rm -rf $(TMP_FILE)
	@ipkg-build -o root -g root dist
	gpg -sb --armor $@_$(VER_$@)_all.ipk

clean:
	rm -rf dist *.ipk *.ipk.asc *~ .*~

#====================================================
#================= TASKS DEFINITION =================

#---Meta tasks
VER_gpe-apps2 = 0.42
DEP_gpe-apps2 = \
    gpe-task-base, \
    gpe-task-comm, \
    gpe-task-mmedia, \
    gpe-task-pim, \
    gpe-task-utils \
#    gpe-task-extra-apps \


#---This is the base to run GPE
VER_gpe-task-base = 0.7
DEP_gpe-task-base = \
    \
    gpe-collateral-gtk2, \
    gdk-pixbuf2-loader-jpeg, \
    \
    gpe-bootsplash2, \
    gpe-dm, \
    gpe-login2, \
    gpe-session-scripts, \
    gpe-setbg2, \
    \
    keylaunch, \
    esd, \
    detect-stylus, \
    gpe-soundserver, \
    gpe-mimedir-tools, \
    teleport, \
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
    xkbd, \
    mbmerlin, \
    \
    startup-monitor, \
    \
    xtscal, \
    gpe-su2, \
    gpe-conf2, \
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
    gpe-autostarter
#FIXME: could be refined (contains hardware dependant packages)

VER_gpe-collateral-gtk2 = 0.05
DEP_gpe-collateral-gtk2 = \
    gtk2-theme-angelistic, \
    ttf-bitstream-vera

#------------------------------------------
# The following aim to be USER LEVEL tasks
#------------------------------------------

#------------------------------------------ PIM applications
VER_gpe-task-pim = 0.5
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
VER_gpe-task-utils = 0.3
DEP_gpe-task-utils = \
    gpe-task-base, \
    gpe-edit2, \
    gpe-terminal, \
    gpe-taskmanager

#------------------------------------------ Communication apps
VER_gpe-task-comm = 0.2
DEP_gpe-task-comm = \
    gpe-task-base, \
    dillo2, \
    gpe-irc
#FIXME: add email/contacts

#------------------------------------------ Multimedia apps
VER_gpe-task-mmedia = 0.2
DEP_gpe-task-mmedia = \
    gpe-task-base, \
    gpe-gallery, \
    gpe-nmf, \
    gpe-mixer

#------------------------------------------ Extra apps
VER_gpe-task-extra-apps = 0.3
DEP_gpe-task-extra-apps = \
    gpe-task-base, \
    gpe-filemanager2

#------------------------------------------ Games
VER_gpe-task-games = 0.2
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
VER_gpe-task-extra-games = 0.2
DEP_gpe-task-extra-games = \
    repton, \
    sfcave-sdl, \
    snes9x

