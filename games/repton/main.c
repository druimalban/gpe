/*	$Id$	*/

/*
 * Copyright (c) 1996
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>

#include "repton.h"
#include "pathnames.h"
#include "version.h"

static void
check_files(void)
{
	int i, fd1, fd2;

	static char *filenames[] = {
		PATH_BITMAPS "/main.bmp",
		PATH_BITMAPS "/passwd.bmp",
		PATH_BITMAPS "/credits.bmp",
		PATH_BITMAPS "/endlevel.bmp",
		PATH_BITMAPS "/endepis.bmp",
		PATH_BITMAPS "/start.bmp",
		PATH_MAPS "/prelude.rep"
	};

	for (i = 0; filenames[i] != NULL; i++)
		if (access(filenames[i], R_OK) != 0) {
			fprintf(stderr, "Cannot read %s\n", filenames[i]);
			fprintf(stderr, "This means that you don't have installed repton into your / root directory.\n");
			exit(1);
		}
}

int sndfd;

int
main(int argc, char **argv)
{
	int optn = 0;

	if (argc > 1)
		if (!strcmp(argv[1], "-n"))
			optn = 1;

	printf("Loading Repton for Linux %s...\n", version_string);
	printf("Please report problems to Sandro Sigala <ssigala@globalnet.it>.\n");

	printf("Checking program data...\n");
	check_files();

	if (!optn) {
		printf("Initializing sound (`-n' option to disable)...\n");
		init_wave_device();
		sound = sound_wave;
	} else {
		printf("Using PC speaker for sound.\n");
		sound = sound_speaker;
	}

	main_screen();

	if (!optn)
		done_wave_device();

	printf("Thanks for playing Repton for Linux. Have a nice day!\n");

	return 0;
}
