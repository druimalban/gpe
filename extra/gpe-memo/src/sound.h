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

#ifndef SOUND_H
#define SOUND_H

#include <glib/gtypes.h>

#define RATE 16000

int file_fd (const gchar *filename, int mode);

int record_fd (void);

int play_fd (void);

void sound_flow (int infd, int outfd, int (*filter) (int, int));

void sound_play (int infd, int outfd);

void sound_record (int infd, int outfd);

void sound_stop (void);

gint sound_get_length (const gchar *filename);


#endif
