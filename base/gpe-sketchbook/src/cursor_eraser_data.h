/* gpe-sketchbook -- a sketches notebook program for PDA
 * Copyright (C) 2002 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#define small_width   cursor_eraser_0_data_width
#define small_height  cursor_eraser_0_data_height
#define small_bits    cursor_eraser_0_data_bits
#define medium_width  cursor_eraser_2_data_width
#define medium_height cursor_eraser_2_data_height
#define medium_bits   cursor_eraser_2_data_bits
#define large_width   cursor_eraser_5_data_width
#define large_height  cursor_eraser_5_data_height
#define large_bits    cursor_eraser_5_data_bits
#define xlarge_width  cursor_eraser_20_data_width
#define xlarge_height cursor_eraser_20_data_height
#define xlarge_bits   cursor_eraser_20_data_bits

#define cursor_eraser_0_data_width 1
#define cursor_eraser_0_data_height 1
static unsigned char cursor_eraser_0_data_bits[] = { 0x01 };

#define cursor_eraser_2_data_width 2
#define cursor_eraser_2_data_height 2
static unsigned char cursor_eraser_2_data_bits[] = { 0x03, 0x03 };

#define cursor_eraser_5_data_width 5
#define cursor_eraser_5_data_height 5
static unsigned char cursor_eraser_5_data_bits[] = { 0x0e, 0x1b, 0x11, 0x1b, 0x0e };

#define cursor_eraser_20_data_width 20
#define cursor_eraser_20_data_height 20
static unsigned char cursor_eraser_20_data_bits[] = {
   0x80, 0x1f, 0x00, 0x60, 0x60, 0x00, 0x10, 0x80, 0x00, 0x08, 0x00, 0x01,
   0x04, 0x00, 0x02, 0x02, 0x00, 0x04, 0x02, 0x00, 0x04, 0x01, 0x00, 0x08,
   0x01, 0x00, 0x08, 0x01, 0x00, 0x08, 0x01, 0x00, 0x08, 0x01, 0x00, 0x08,
   0x01, 0x00, 0x08, 0x02, 0x00, 0x04, 0x02, 0x00, 0x04, 0x04, 0x00, 0x02,
   0x08, 0x00, 0x01, 0x10, 0x80, 0x00, 0x60, 0x60, 0x00, 0x80, 0x1f, 0x00
};
