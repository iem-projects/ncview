/* These routines make truecolor data from simple byte-scaled data, using the supplied
 * colormap and the information about the server's byte depth, byte ordering, etc.
 *
 * Part of Ncview by David W. Pierce
 * Copyright (C) 1993 through 2011 David W. Pierce
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
 */

#include "../ncview.includes.h"
#include "../ncview.defines.h"
#include "../ncview.protos.h"

/* These are declared and set in x_interface.c */
extern Server_Info	server;

/* Following are local to this file only */
static void make_tc_data_16( unsigned char *data, long width, long height, XColor *color_list,
		unsigned char *tc_data );
static void make_tc_data_24( unsigned char *data, long width, long height, XColor *color_list,
		unsigned char *tc_data );
static void make_tc_data_32( unsigned char *data, long width, long height, XColor *color_list,
		unsigned char *tc_data );

/*************************************************************************************************/
/* Converts the byte-scaled data to truecolor representation,
 * using the passed array of N XColors (typically N ~ 256).
 */
void make_tc_data( unsigned char *data, long width, long height, XColor *color_list,
	unsigned char *tc_data )
{
	switch (server.bytes_per_pixel) {
		case 4: make_tc_data_32( data, width, height, color_list, tc_data );
			break;

		case 3:
			make_tc_data_24( data, width, height, color_list, tc_data );
			break;

		case 2:
			make_tc_data_16( data, width, height, color_list, tc_data );
			break;

		default:
			fprintf( stderr, "Sorry, I am not set up to produce ");
			fprintf( stderr, "images of %d bytes per pixel.\n", 
					server.bytes_per_pixel );
			exit( -1 );
			break;
		}
}

/*************************************************************************************************/
static void make_tc_data_16( unsigned char *data, long width, long height, XColor *color_list,
		unsigned char *tc_data )
{
	int	i, j, pix;
	size_t	pad_offset, po_val;

	/* pad to server.bitmap_pad bits if required */
	pad_offset = 0L;
	po_val     = 0L;
	if( (width%2 != 0) && (server.bits_per_pixel != server.bitmap_pad) ) 
		po_val = 2L;

	/****************************************
	 *    Least significant bit first
	 ****************************************/
	for( j=0; j<height; j++ ) {
		for( i=0; i<width; i++ ) {

			pix = *(data+i+j*width);

			*(tc_data+i*2+j*(width*2)+pad_offset) = 
				(unsigned char)((color_list+pix)->blue>>server.shift_blue & server.mask_blue);

			*(tc_data+i*2+1+j*(width*2)+pad_offset) = 
				(unsigned char)((color_list+pix)->green>>server.shift_green_upper & server.mask_green_upper);


			*(tc_data+i*2+j*(width*2)+pad_offset) +=
				(unsigned char)((color_list+pix)->green>>server.shift_green_lower & server.mask_green_lower );

			*(tc_data+i*2+1+j*(width*2)+pad_offset) +=
				(unsigned char)((color_list+pix)->red>>server.shift_red & server.mask_red );
			}
		pad_offset += po_val;
		}
}

/*************************************************************************************************/
static void make_tc_data_24( unsigned char *data, long width, long height, XColor *color_list,
		unsigned char *tc_data )
{
	int	i, j, pix, o_r, o_g, o_b;
	long	pad_offset, po_val;

	if( server.rgb_order == ORDER_RGB ) {
		o_r = 2;
		o_g = 1;
		o_b = 0;
		}
	else
		{
		o_r = 0;
		o_g = 1;
		o_b = 2;
		}

	/* pad to server.bitmap_pad bits if required */
	pad_offset = 0L;
	po_val     = 0L;
	if( (((width*3)%4) != 0) && (server.bits_per_pixel != server.bitmap_pad) ) 
		po_val = (server.bitmap_pad/8) - (width*3)%4;

	for( j=0; j<height; j++ ) {
		for( i=0; i<width; i++ ) {

			pix = *(data+i+j*width);
			*(tc_data+i*3+o_b+j*(width*3)+pad_offset) = 
				(char)((color_list+pix)->blue>>8);
			*(tc_data+i*3+o_g+j*(width*3)+pad_offset) = 
				(char)((color_list+pix)->green>>8);
			*(tc_data+i*3+o_r+j*(width*3)+pad_offset) = 
				(char)((color_list+pix)->red>>8);
			}
		pad_offset += po_val;
		}
}

/*************************************************************************************************/
static void make_tc_data_32( unsigned char *data, long width, long height, XColor *color_list,
		unsigned char *tc_data )
{
	int	i, j, pix, o_r, o_g, o_b;

	if( server.rgb_order == ORDER_RGB ) {
		o_r = 2;
		o_g = 1;
		o_b = 0;
		}
	else
		{
		o_r = 0;
		o_g = 1;
		o_b = 2;
		}
	if( server.byte_order == MSBFirst ) {
		o_r++;
		o_g++;
		o_b++;
		}

	for( i=0; i<width; i++ )
	for( j=0; j<height; j++ ) {
		pix = *(data+i+j*width);
		*(tc_data+i*4+o_b+j*(width*4)) = 
			(char)((color_list+pix)->blue>>8);
		*(tc_data+i*4+o_g+j*(width*4)) = 
			(char)((color_list+pix)->green>>8);
		*(tc_data+i*4+o_r+j*(width*4)) = 
			(char)((color_list+pix)->red>>8);
		}
}
	

