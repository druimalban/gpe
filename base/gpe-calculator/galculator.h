/*
 *  galculator.h - general definitions.
 *	part of galculator
 *  	(c) 2002 Simon Floery (simon.floery@gmx.at)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#define PROG_NAME "gcalculator"
#define PROG_VERSION "0.7"

/* CS_xxxx define flags for current_status. */

#define CS_DEC 0
#define CS_HEX 1
#define CS_OCT 2
#define CS_BIN 3

#define CS_DEG 0
#define CS_RAD 1
#define CS_GRAD 2

#define CS_FMOD_FLAG_INV 0
#define CS_FMOD_FLAG_HYP 1

typedef struct {
	unsigned char	number:2;
	unsigned char	angle:2;
	unsigned char	fmod:2;
} s_current_status;

s_current_status current_status;

typedef struct {
	char		*button_label;
	double		(*func[4])(double);
} s_function_list;

typedef struct {
	char		*button_label;
	double		constant;
} s_constant_list;

typedef struct {
	char		*button_label;
	void		(*func)();
} s_gfunc_list;
