/*
 * Ncview by David W. Pierce.  A visual netCDF file viewer.
 * Copyright (C) 1993 through 2010 David W. Pierce
 *
 * This program  is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as 
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
 * 6259 Caminito Carrean
 * San Diego, CA   92122
 * pierce@cirrus.ucsd.edu
 */

/*************************************************************************
 * Routines to handle a button being pressed.  My convention for the 
 * modifiers: MOD_1 means the standard action.  MOD_2 means an accelerated 
 * version of the standard aciton. MOD_3 means a backwards version of the 
 * standard action.  MOD_4 means an accelerated backwards version of the
 * standard action.
 *************************************************************************/

#include "ncview.includes.h"
#include "ncview.defines.h"
#include "ncview.protos.h"

#define DELAY_DELTA	350.0
#define DELAY_OFFSET	10L

extern Options options;

static int cur_button = BUTTON_PAUSE;

/*===========================================================================================*/
	int
which_button_pressed( void )
{
	return( cur_button );
}

/*===========================================================================================*/
	void
do_range( int modifier )
{
	init_saveframes();
	if( modifier == MOD_3 )
		view_set_range_frame();
	else
		view_set_range();
}

/*===========================================================================================*/
	void
do_dimset( int modifier )
{
	view_set_scan_dims();
}

/*===========================================================================================*/
	void
do_restart( int modifier )
{
	cur_button = BUTTON_PAUSE;

	in_timer_clear();

	set_scan_view( 0 );
	view_draw    ( TRUE, FALSE );

	in_timer_clear();
}

/*===========================================================================================*/
	void
do_rewind( int modifier )
{
	unsigned long delay_millisec;

	cur_button = BUTTON_REWIND;

	delay_millisec = (long)(DELAY_DELTA * options.frame_delay) + DELAY_OFFSET;
 
	in_timer_clear();

	if( modifier == MOD_2 ) {
		change_view( -10, FRAMES );
		in_timer_set( (XtTimerCallbackProc)do_rewind, (XtPointer)(MOD_2), delay_millisec );
		}
	else
		{
		change_view( -1, FRAMES );
		in_timer_set( (XtTimerCallbackProc)do_rewind, (XtPointer)(MOD_1), delay_millisec );
		}
}

/*===========================================================================================*/
	void
do_quit( int modifier )
{
	quit_app();
}

/*===========================================================================================*/
	void
do_backwards( int modifier )
{
	in_timer_clear();

	if( modifier == MOD_2 )
		change_view( -10, PERCENT );
	else
		change_view( -1, FRAMES );

	cur_button = BUTTON_PAUSE;
}

/*===========================================================================================*/
	void
do_pause( int modifier )
{
	cur_button = BUTTON_PAUSE;
	in_timer_clear();
}

/*===========================================================================================*/
	void
do_forward( int modifier )
{
	cur_button = BUTTON_PAUSE;
	in_timer_clear();

	if( modifier == MOD_2 )
		change_view( 10, PERCENT );
	else
		change_view( 1, FRAMES );
}

/*===========================================================================================*/
	void
do_fastforward( int modifier )
{
	unsigned long	delay_millisec;

	cur_button = BUTTON_FASTFORWARD;

	in_timer_clear();

	delay_millisec = (long)(DELAY_DELTA * options.frame_delay) + DELAY_OFFSET;

	if( modifier == MOD_2 ) {
		if( change_view( 10, FRAMES ) == 0 )
			in_timer_set( (XtTimerCallbackProc)do_fastforward, (XtPointer)(MOD_2), delay_millisec );
		}
	else
		{
		if( change_view( 1, FRAMES ) == 0 )
			in_timer_set( (XtTimerCallbackProc)do_fastforward, (XtPointer)(MOD_1), delay_millisec );
		}
}
		
/*===========================================================================================*/
	void
do_colormap_sel( int modifier )
{
	if( modifier == MOD_3 )
		in_install_prev_colormap( TRUE );
	else
		in_install_next_colormap( TRUE );
	view_draw( TRUE, FALSE );
	view_recompute_colorbar();
}

/*===========================================================================================*/
	void
do_invert_physical( int modifier )
{
	init_saveframes();
	if( options.invert_physical )
		options.invert_physical = FALSE;
	else
		options.invert_physical = TRUE;
	view_draw( TRUE, FALSE );
	redraw_dimension_info();
}

/*===========================================================================================*/
	void
do_data_edit( int modifier )
{
/* do_overlay(); */
	view_data_edit();
}

/*===========================================================================================*/
	void
do_invert_colormap( int modifier )
{
	init_saveframes();
	if( options.invert_colors )
		options.invert_colors = FALSE;
	else
		options.invert_colors = TRUE;
	view_draw( TRUE, FALSE );
	view_recompute_colorbar();
}

/*===========================================================================================*/
	void
do_set_minimum( int modifier )
{
}

/*===========================================================================================*/
	void
do_set_maximum( int modifier )
{
}

/*===========================================================================================*/
	void
do_blowup( int modifier )
{
	int view_var_is_valid = TRUE;

	if( modifier == MOD_3 )
		view_change_blowup( -1, TRUE, view_var_is_valid );

	else if( modifier == MOD_2 ) {
		/* Double the current blowup -- make image BIGGER */
		if( options.blowup > 0 )
			view_change_blowup( options.blowup, TRUE, view_var_is_valid );
		else	
			view_change_blowup( -(options.blowup)/2, TRUE, view_var_is_valid );
		}

	else if( modifier == MOD_4 ) {
		/* Halve the current blowup -- make image SMALLER */
		if( options.blowup > 0 ) 
			view_change_blowup( -(options.blowup/2), TRUE, view_var_is_valid );
		else
			view_change_blowup( options.blowup, TRUE, view_var_is_valid );
		}
		
	else
		view_change_blowup( 1, TRUE, view_var_is_valid );
	
	/* If we are shrinking magnification, then try re-saving
	 * the frames because now there might be enough room.
	 */
	init_saveframes();
	if( modifier == MOD_3 )
		options.save_frames = TRUE;
}

/*===========================================================================================*/
	void
do_transform( int modifier )
{
	init_saveframes();
	if( modifier == MOD_3 )
		view_change_transform( -1 );
	else
		view_change_transform( 1 );
}

/*===========================================================================================*/
	void
do_blowup_type( int modifier )
{
	init_saveframes();
	if( options.blowup_type == BLOWUP_REPLICATE )
		set_blowup_type( BLOWUP_BILINEAR );
	else
		set_blowup_type( BLOWUP_REPLICATE );
	view_draw( TRUE, FALSE );
}

/*===========================================================================================*/
	void
do_info( int modifier )
{
	view_information();
}

/*===========================================================================================*/
	void
do_options( int modifier )
{
	set_options();
}
