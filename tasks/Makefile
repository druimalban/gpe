#------------------------
# Makefile for GPE tasks
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

% : %.control
	@rm -rf dist
	@mkdir -p dist/CONTROL
	@sed 's:_DEPS_:$(DEP_$@):' < $< > dist/CONTROL/control
	@ipkg-build -o root -g root dist

clean:
	rm -rf dist *.ipk *.ipk.asc *~ .*~

#---Meta tasks
DEP_gpe-apps = \
    gpe-task-base, \
    gpe-task-comm, \
    gpe-task-mmedia, \
    gpe-task-pim, \
    gpe-task-utils \
#    gpe-task-extra-apps \


#---This is the base to run GPE
VER_gpe-task-base = 0.2
DEP_gpe-task-base = \
    \
    gpe-collateral-gtk2, \
    gdk-pixbuf2-loader-jpeg, \
    \
    gpe-bootsplash2, \
    gpe-dm, \
    gpe-login2, \
    gpe-ownerinfo, \
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
    \
    xstroke, \
    xkbd, \
    \
    startup-monitor, \
    \
    xcalibrate, \
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
    hotplug-dbus
#FIXME: could be refined (contains hardware dependant packages)

DEP_gpe-collateral-gtk2 = \
    libgtk2.0-0, \
    gtk2-theme-angelistic, \
    ttf-bitstream-vera

#---The following tasks aims to be user-level tasks
DEP_gpe-task-pim = \
    gpe-task-base, \
    gpe-calendar2, \
    gpe-soundbite2, \
    gpe-contacts2, \
    gpe-ownerinfo, \
    gpe-sketchbook2, \
    gpe-today, \
    gpe-todo2, \
    \
    gpe-minicalc2, \
    gpe-calculator2
#FIXME: 2 calculators!

DEP_gpe-task-utils = \
    gpe-task-base, \
    gpe-edit2, \
    gpe-terminal, \
    gpe-filemanager2, \
    gpe-taskmanager

DEP_gpe-task-comm = \
    gpe-task-base, \
    dillo2, \
    gpe-irc
#FIXME: add email/contacts

DEP_gpe-task-mmedia = \
    gpe-task-base, \
    gpe-gallery, \
    gpe-nmf, \
    gpe-mixer

DEP_gpe-task-extra-apps = \
    gpe-task-base, \
    figment, \
    gpe-timesheet2, \
    greader

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

DEP_gpe-task-extra-games = \
    repton, \
    sfcave-sdl, \
    snes9x

