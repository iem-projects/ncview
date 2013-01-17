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


#include "../ncview.includes.h"
#include "../ncview.defines.h"
#include "../ncview.protos.h"

extern Widget topLevel;

/****************************************************************/

	void
x_flush()
{
	XFlush( XtDisplay( topLevel ));
}

	unsigned char
interp( int i, int range_i, unsigned char *mat, int n_entries )
{
	float	frac_i, float_target, interp_frac;
	int	int_target;
	unsigned char loval, hival, ret_val;

	frac_i       = (float)i/(float)range_i;	/* 0 <= frac_i < 1 */
	float_target = (float)n_entries * frac_i; /* 0 <= float_target < n_entris */
	int_target   = (int)float_target;	/* 0 <= int_target <= n_entries-1 */
	interp_frac  = float_target - (float)int_target;

	loval = *(mat+int_target);
	if( int_target >= (n_entries-1) )
		return( loval );

	hival = *(mat+int_target+1);

	ret_val = (unsigned char)
			((float)loval + interp_frac*((float)hival-(float)loval));
	ret_val = (ret_val > 255) ? 255 : ret_val;
	return( ret_val );
}

