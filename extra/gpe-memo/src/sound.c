/*
* This file is part of GPE-Memo
*
* Copyright (C) 2007 Alberto García Hierro
*	<skyhusker@rm-fr.net>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version
* 2 of the License, or (at your option) any later version.
*/

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#include <esd.h>

#include "sound.h"
#include "gsm-codec.h"

static pid_t child = 0;
extern gboolean stop;

static void
sigint_handler (int signum)
{
    stop = TRUE;
}

int
file_fd (const char *filename, int mode)
{
    switch (mode) {
        case O_RDWR:
        case O_WRONLY:
            return open (filename, O_CREAT|O_TRUNC|mode, S_IRUSR|S_IWUSR);
        case O_RDONLY:
            return open (filename, mode);
        default:
            return -1;
    }
}

int
record_fd (void)
{
    return esd_record_stream_fallback (ESD_BITS16 | 
            ESD_STEREO | ESD_STREAM | ESD_RECORD,
            RATE, NULL, NULL);
}

int
play_fd (void)
{
    return esd_play_stream_fallback (ESD_BITS16 |
            ESD_STEREO | ESD_STREAM | ESD_PLAY,
            RATE, NULL, NULL);
}

void
sound_flow (int infd, int outfd, int (*filter) (int,int))
{
    sound_stop ();

    switch ((child = fork())) {
        case -1:
            perror ("fork()");
            break;
        case 0:
            signal (SIGINT, sigint_handler);
            filter (infd, outfd);
            close (infd);
            close (outfd);
            exit (0);
            break;
        default:
            break;
    }

}

void
sound_stop (void)
{
    if (child > 0) {
        kill (child, SIGINT);
        waitpid (child, NULL, 0);
        child = 0;
    }
}

void
sound_play (int infd, int outfd)
{
    sound_flow (infd, outfd, sound_decode);
}

void
sound_record (int infd, int outfd)
{
    sound_flow (infd, outfd, sound_encode);
}

gint
sound_get_length (const gchar *filename)
{
    struct stat buf;

    if (stat (filename, &buf) < 0)
        return 0;

    return buf.st_size / (1650*2);
}

