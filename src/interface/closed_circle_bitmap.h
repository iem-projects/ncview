/*
 * Ncview by David W. Pierce.  A visual netCDF file viewer.
 * Copyright (C) 1993 through 2010 David W. Pierce
 *
 * This program  is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 3, as 
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * David W. Pierce
 * 6259 Caminito Carrena
 * San Diego, CA  92122
 * pierce@cirrus.ucsd.edu
 */

#define closed_circle_bitmap_width 16
#define closed_circle_bitmap_height 16
static unsigned char closed_circle_bitmap_bits[] = {
   0x00, 0x00, 0xc0, 0x07, 0xf0, 0x1f, 0xf8, 0x3f, 0xf8, 0x3f, 0xfc, 0x7f,
   0xfc, 0x7f, 0xfc, 0x7f, 0xfc, 0x7f, 0xfc, 0x7f, 0xf8, 0x3f, 0xf8, 0x3f,
   0xf0, 0x1f, 0xc0, 0x07, 0x00, 0x00, 0x00, 0x00};
