/*	$Id$	*/

/*
 * Copyright (c) 1996, 1997
 *	Sandro Sigala, Brescia, Italy.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Sandro Sigala.
 * 4. Neither the name of the author nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/ioctl.h>
#include <sys/kd.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "repton.h"
#include "pathnames.h"
#include "soundIt.h"

#define SOUND(freq,dur) ((dur) << 16 | (1193180 / freq))

/*
 * Better sounds would be nice.
 */
void
sound_speaker(int sound)
{
	switch (sound) {
	case SND_MOUSE:
	case SND_GOT_DIAMOND:
		ioctl(STDIN_FILENO, KDMKTONE, SOUND(2000,100));
		break;
	case SND_GOT_TIME:
		ioctl(STDIN_FILENO, KDMKTONE, SOUND(5000,100));
		break;
	case SND_GOT_KEY:
		ioctl(STDIN_FILENO, KDMKTONE, SOUND(4000,100));
		break;
	case SND_GOT_CROWN:
		ioctl(STDIN_FILENO, KDMKTONE, SOUND(3000,100));
		break;
	case SND_ENTERED_CAGE:
	case SND_KILLED_MONSTER:
		ioctl(STDIN_FILENO, KDMKTONE, SOUND(200,100));
		break;
	case SND_END_LEVEL:
		ioctl(STDIN_FILENO, KDMKTONE, SOUND(600,100));
		break;
	case SND_DIE:
		ioctl(STDIN_FILENO, KDMKTONE, SOUND(200,200));
		break;
	case SND_TRANSPORTER:
		ioctl(STDIN_FILENO, KDMKTONE, SOUND(3500,50));
		break;
	}
}

static Sample samples[SND_NUM_SOUNDS];

static void
read_sound_file(int sound, char *filename)
{
	if (Snd_loadRawSample(filename, &samples[sound]))
		warnx("%s: No such file. Playing without this sound FX",
		      filename);
}

void
init_wave_device(void)
{
	read_sound_file(SND_GOT_DIAMOND, PATH_SOUNDS "/doo_eb3.ub");
	read_sound_file(SND_GOT_KEY, PATH_SOUNDS "/bridge1.ub");
	read_sound_file(SND_GOT_TIME, PATH_SOUNDS "/cp70_eb2.ub");
	read_sound_file(SND_GOT_CROWN, PATH_SOUNDS "/music_box.ub");
	read_sound_file(SND_ENTERED_CAGE, PATH_SOUNDS "/click.ub");
	read_sound_file(SND_KILLED_MONSTER, PATH_SOUNDS "/chirp.ub");
	read_sound_file(SND_DIE, PATH_SOUNDS "/screech.ub");
	read_sound_file(SND_TRANSPORTER, PATH_SOUNDS "/choo.ub");
	read_sound_file(SND_MOUSE, PATH_SOUNDS "/dx_ep_a2.ub");
	read_sound_file(SND_FALLING_ROCK, PATH_SOUNDS "/pinknois.ub");
	read_sound_file(SND_START_LEVEL, PATH_SOUNDS "/space.ub");
	read_sound_file(SND_END_LEVEL, PATH_SOUNDS "/orchhit.ub");
	Snd_init(SND_NUM_SOUNDS, samples, 8000, 8, "/dev/dsp");
}

void
done_wave_device(void)
{
	Snd_restore();
}

void
sound_wave(int sound)
{
	static int lastch = 0;
	lastch++;
	if (lastch == 8)
		lastch = 0;
	Snd_effect(sound, lastch);
}
