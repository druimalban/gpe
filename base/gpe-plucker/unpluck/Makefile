# -DPLUCKER_VERSION="1.6.1"
# -DPLUCKER_CONFIG_DIR="/usr/local/etc"
# -DSYS_CONFIG_FILE_NAME="pluckerrc"
# -DUSER_CONFIG_FILE_NAME=".pluckerrc"
# -DFILE_SEPARATOR_CHAR_S="/"
# -DOS_SECTION_NAME="posix"

CC	=	gcc
AR	=	ar
RANLIB	=	ranlib

CFLAGS	=	-Wall -O2 -g

CFLAGS +=	-DPLUCKER_VERSION="1.6.1" -DPLUCKER_CONFIG_DIR="/etc"
CFLAGS +=	-DSYS_CONFIG_FILE_NAME="pluckerrc" -DUSER_CONFIG_FILE_NAME=".pluckerrc"
CFLAGS +=	-DFILE_SEPARATOR_CHAR_S="/" -DOS_SECTION_NAME="posix"

SRC	=	config.c unpluck.c util.c
OBJS	=	${SRC:.c=.o}

TARGET	=	libunpluck.a

all:	$(TARGET)

$(TARGET):	$(OBJS)
	$(AR) r $(TARGET) $(OBJS)
	$(RANLIB) $(TARGET)

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)
