PACKAGE = sfcave-sdl
PREFIX = /usr/local
DEBUG = yes
CVSBUILD = yes
VERSION = 0.1
#STATIC_LIBSTDC++ = yes

LINGUAS = 

CPPFLAGS = -D_GNU_SOURCE
ifeq ($(DEBUG),yes)
CXXFLAGS += -O2 -g
LDFLAGS = -g
else
CXXFLAGS += -Os -fomit-frame-pointer
endif
CXXFLAGS += -Wall `sdl-config --cflags`
LDFLAGS += `sdl-config --libs` -lSDL_image -lSDL_mixer -lSDL_gfx 
ifeq ($(STATIC_LIBSTDC++),yes)
LDFLAGS += -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic
else
LDFLAGS += -lstdc++
endif
CPPFLAGS += -DPACKAGE=\"$(PACKAGE)\" -DPREFIX=\"$(PREFIX)\" -DPACKAGE_LOCALE_DIR=\"$(PREFIX)/share/locale\"
CXXFLAGS += -MD

.SUFFIXES: .d

SOURCES = 	animatedimage.cpp \
		bfont.cpp \
		font.cpp \
		game.cpp \
		menu.cpp \
		help.cpp \
		player.cpp \
		random.cpp \
		sfcave.cpp \
		sfcave_game.cpp \
		gates_game.cpp \
		fly_game.cpp \
		flyterrain.cpp \
		sound.cpp \
		terrain.cpp \
		settings.cpp \
		starfield.cpp \
		util.cpp

OBJS = $(patsubst %.cpp,%.o,$(SOURCES))
DEPS = $(patsubst %.cpp,%.d,$(SOURCES))

ifeq ($(CVSBUILD),yes)
BUILD = ../../base/build
else
BUILD = build
endif

all: $(PACKAGE)

$(PACKAGE): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

sounds = explosion.wav  ingame.mod  music.mod  thrust.wav
images = explosion.bmp  score_font.bmp     sfcave_text.bmp  unsel_menu_font.bmp \
	 help_font.bmp  sel_menu_font.bmp  title_font.bmp

install-program: all $(PACKAGE).desktop
	install -d $(DESTDIR)$(PREFIX)/bin
	install -s $(PACKAGE) $(DESTDIR)$(PREFIX)/bin/$(PACKAGE)
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 644 $(PACKAGE).png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 $(PACKAGE).desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)$(PREFIX)/share/sfcave-sdl/images
	for i in $(images); do install -m 644 images/$$i $(DESTDIR)$(PREFIX)/share/sfcave-sdl/images/; done
	install -d $(DESTDIR)$(PREFIX)/share/sfcave-sdl/sounds
	for i in $(sounds); do install -m 644 sounds/$$i $(DESTDIR)$(PREFIX)/share/sfcave-sdl/sounds/; done

clean:
	rm -f $(PACKAGE) $(OBJS) $(DEPS)

include $(BUILD)/Makefile.translation
include $(BUILD)/Makefile.dpkg_ipkg
-include $(DEPS)
