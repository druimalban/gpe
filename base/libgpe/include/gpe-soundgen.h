/* gpe-soundgen.c - Generate sine-wave sounds and play them on /dev/dsp.

   Copyright (C) 2002 by Nils Faerber <nils@kernelconcepts.de>

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _GPE_SOUNDGEN_H
#define _GPE_SOUNDGEN_H

/*
 * Opens the sound device and returns a FD to it (int snddev)
 */
int gpe_soundgen_init(void);

/*
 * Plays a sine wave tone of frequency freq for duration #msecs
 * Upon first call a buffer for the samples is allocated and statically kept
 * until the frequency is changed or gpe_soundgen_final() is called
 */
void gpe_soundgen_play_tone1(int snddev, unsigned int freq,
				unsigned int duration);
/*
 * Plays a sine wave tone of frequency freq for duration #msecs
 * Upon first call a buffer for the samples is allocated and statically kept
 * until the frequency is changed or gpe_soundgen_final() is called
 * This second function call is provided to save calculation cost so that
 * two different buffers for two tones can be kept.
 */
void gpe_soundgen_play_tone2(int snddev, unsigned int freq,
				unsigned int duration);

void gpe_soundgen_pause(int snddev, unsigned int duration);

/*
 * Closes and frees the sound device (normally /dev/dsp) again
 * and most importantly also frees the buffers allocate for tone1 and tone2
 */
int gpe_soundgen_final(int snddev);

#endif
