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
 * 6259 Caminito Carrena
 * San Diego, CA   92122
 * dpierce@ucsd.edu
 */

/*****************************************************************************
 *
 *	interface.c
 *
 *	Routines in this file are used to form the graphical interface
 *	between the user and ncview.  If you want to install ncview under
 *	a new graphical operating system, only routines in this file should
 *	have to be implemented.
 *
 *****************************************************************************/

#include "../ncview.includes.h"
#include "../ncview.defines.h"
#include "../ncview.protos.h"

/************************************************************************
 * All of the following routines must be provided for each user        
 * interface model!!
 * Required elements of the interface:
 *
 * 1) A 2-D field capable of displaying a byte array with a colormap.
 *
 * 2) Writable labels, a list of which is given in 'ncview.defines.h'
 *
 * 3) Various buttons, a list of which is given in 'ncview.defines.h'
 */


/****************************************************************************
 * Parse any dispaly-specific arguments 
 */
	void
in_parse_args( int *p_argc, char **argv )
{
	x_parse_args( p_argc, argv );
}

/****************************************************************************
 * Initialize the interface 
 */
	void
in_initialize()
{
	x_initialize();
}

/****************************************************************************
 * Set the value of the label element indicated by the passed id 
 * to the passed string 
 */
	void
in_set_label( int label_id, char *string )
{
	if( string == NULL )
		return;

	/* GCC compiler barfs on this x_set_label( label_id, limit_string(string) ); */
	x_set_label( label_id, string );
}

/****************************************************************************
 * Indicate that the passed variable name is the active one.  How to
 * do this is up to the discretion of the implementer; could have a 
 * label, or a lit-up button, etc.
 */
	void
in_indicate_active_var( char *var_name )
{
	x_indicate_active_var( var_name );
}

/****************************************************************************
 * Vector through this routine when a new display variable has been 
 * selected by the user, by pressing some sort of button.
 */
	void
in_variable_selected( char *var_name )
{
	NCVar	*var;

	if( (var = get_var( var_name )) == NULL ) {
		fprintf( stderr, "ncview: in_variable_selected: internal error " );
		fprintf( stderr, "no variable with name >%s< found on variable list\n",
					var_name );
		exit( -1 );
		}

	set_scan_variable( var );
}

/****************************************************************************
 * Set the cursor to 'busy'
 */
	void
in_set_cursor_busy()
{
	x_set_cursor_busy();
}

/****************************************************************************
 * Set the cursor to 'normal'
 */
	void
in_set_cursor_normal()
{
	x_set_cursor_normal();
}

/****************************************************************************
 * Vector through this routine when the user has pressed a button indication
 * that they want to change the current value of one of the dimensions.
 */
	void
in_change_current( char *dim_name, int modifier )
{
	view_change_cur_dim( dim_name, modifier );
}

/****************************************************************************
 * Execute a loop which scans for user-interface events, and
 * handles them appropriately.
 */
	void
in_process_user_input()
{
	x_process_user_input();
}

/****************************************************************************
 * Put a 2-D byte array of the given width and height up
 * on the screen. 
 */
	void
in_draw_2d_field( ncv_pixel *data, size_t width, size_t height,
	size_t timestep )
{
	x_draw_2d_field( data, width, height, timestep );
}

/****************************************************************************
 * Create a colormap and fill it with the passed values.  Note that the
 * 256 color values are always filled out, although the actual number
 * of colors--options.n_colors--may be larger or smaller!!
 */
	void
in_create_colormap( char *colormap_name, unsigned char r[256],
			unsigned char g[256], unsigned char b[256] )
{
	x_create_colormap( colormap_name, r, g, b );
}

/****************************************************************************
 * Install the next colormap in the queue.  Returns the name of the 
 * newly installed map 
 */
	char *
in_install_next_colormap( int do_widgets )
{
	char	*colormap_name;

	colormap_name = x_change_colormap( 1, do_widgets );
	if( do_widgets )
		in_set_label( LABEL_COLORMAP_NAME, colormap_name );
	return( colormap_name );
}

/****************************************************************************
 * Install the previous colormap in the queue.  Returns the name of 
 * the newly installed map 
 */
	char *
in_install_prev_colormap( int do_widgets )
{
	char	*colormap_name;

	colormap_name = x_change_colormap( -1, do_widgets );
	in_set_label( LABEL_COLORMAP_NAME, colormap_name );
	return( colormap_name );
}

/****************************************************************************
 * Set the size of the 2-D field display element to the passed values. 
 * Returns -1 if display shrunk, 0 if no change, 1 if expanded.
 */
	int
in_set_2d_size( size_t width, size_t height )
{
	static 	size_t last_width=0, last_height=0;
	int	retval;

	if( (width != last_width) || (height != last_height) ) {
		if( width > last_width ) 
			retval = 1;
		else
			retval = -1;
		x_set_2d_size( width, height );
		last_width  = width;
		last_height = height;
		in_flush();
		return( retval );
		}
	return( 0 );
}

/****************************************************************************
 * Called when a button is pressed. Argument 'button_id' indicates which 
 * button was pressed.  Argument modifier should ideally take on one of 
 * 4 values: MOD_1, MOD_2, MOD_3, and MOD_4, which are used in a generalized
 * sense to mean "normal action", "accelerated action", "backwards action",
 * and "accelerated backwards action".  If these are not available, just always 
 * use MOD_1.
 */
	void
in_button_pressed( int button_id, int modifier )
{
	switch( button_id ) {
		case BUTTON_RANGE:
			do_range( modifier );
			break;

		case BUTTON_DIMSET:
			do_dimset( modifier );
			break;

		case BUTTON_TRANSFORM:
			do_transform( modifier );
			break;

		case BUTTON_BLOWUP:
			do_blowup( modifier );
			break;

		case BUTTON_QUIT:	     
			do_quit( modifier );
			break;

		case BUTTON_RESTART:	     
			do_restart( modifier );
			break;

		case BUTTON_REWIND:	     
			do_rewind( modifier );
			break;

		case BUTTON_BACKWARDS:	     
			do_backwards( modifier );
			break;

		case BUTTON_PAUSE:	     
			do_pause( modifier );
			break;

		case BUTTON_FORWARD:	     
			do_forward( modifier );
			break;

		case BUTTON_FASTFORWARD:   
			do_fastforward( modifier );
			break;

		case BUTTON_COLORMAP_SELECT: 
			do_colormap_sel( modifier );
			break;

		case BUTTON_INVERT_PHYSICAL: 
			do_invert_physical( modifier );
			break;

		case BUTTON_INVERT_COLORMAP: 
			do_invert_colormap( modifier );
			break;

		case BUTTON_MINIMUM:
			do_set_minimum( modifier );
			break;

		case BUTTON_MAXIMUM:
			do_set_maximum( modifier );
			break;

		case BUTTON_BLOWUP_TYPE:
			do_blowup_type( modifier );
			break;

		case BUTTON_EDIT:
			do_data_edit( modifier );
			break;

		case BUTTON_INFO:
			do_info( modifier );
			break;

		case BUTTON_PRINT:
			do_print();
			break;

		case BUTTON_OPTIONS:
			do_options( modifier );
			break;

		default:
			fprintf( stderr, "in_button_pressed: unknown " );
			fprintf( stderr, "button id: %d\n", button_id  );
			exit( -1 );
		}
}

/*****************************************************************************
 * Allow the user to set the X and Y dimensions, through any means
 * desired.  The best is to pop up a dialog box and have the user
 * press buttons to indicate the desired choice.  Returned is a
 * Stringlist containing first the name of the Y dimension, then
 * the name of the X dimension.
 */
	int
in_set_scan_dims( Stringlist *dim_list, char *x_axis_name, char *y_axis_name, Stringlist **new_dim_list )
{
	return( x_set_scan_dims( dim_list, x_axis_name, y_axis_name, new_dim_list ) );
}

/*****************************************************************************
 * Clear any pending timer actions
 */
	void
in_timer_clear()
{
	x_timer_clear();
}

/*****************************************************************************
 * Add a timed callback to the passed procedure, with the passed argument.
 */
	void
in_timer_set( XtTimerCallbackProc procedure, XtPointer arg, unsigned long delay_millisec )
{
	x_timer_set( procedure, arg, delay_millisec );
}

/*****************************************************************************
 * Set the sensitivity to the passed button_id to 'True'.  (I.e., 
 * it is currently "greyed out"; undo that.)
 */
	void
in_set_sensitive( int button_id, int state )
{
	x_set_sensitive( button_id, state );
}

/*****************************************************************************
 * Set the sensitivity of the passed variable button to the given state
 * (either 'True' or 'False').
 */
 	void
in_var_set_sensitive( char *var_name, int sensitivity )
{
	x_set_var_sensitivity( var_name, sensitivity );
}

/*****************************************************************************
 * Clear the existing dimension buttons.
 */
	void
in_clear_dim_buttons()
{
	x_clear_dim_buttons();
}

/*****************************************************************************
 * Set a dimension button
 */
	void
in_indicate_active_dim( int dimension, char *dim_name )
{
	x_indicate_active_dim( dimension, dim_name );
}

/*****************************************************************************
 * Indicate an error condition which can be continued from.
 */
	void
in_error( char *message )
{
	in_dialog( message, NULL, FALSE );
}

/*****************************************************************************
 * Pop up a dialog box, and get a string of input in return.   Returns
 * either MESSAGE_OK or MESSAGE_CANCEL.  If you want a return string
 * given, pass a PRE-ALLOCATED string in "ret_string"!  The initial 
 * value of ret_string will be shown in the dialog, and overwritten.
 */
	int
in_dialog( char *message, char *ret_string, int want_cancel_button )
{
	return(  x_dialog( message, ret_string, want_cancel_button ) );
}

/*****************************************************************************
 * Fill out the dimension information labels for the indicated 
 * dimension number.
 */
	void
in_fill_dim_info( NCDim *d, int please_flip )
{
	x_fill_dim_info( d, please_flip );
}

/*****************************************************************************
 * Fill out just the "current" field for a dimension
 */
	void
in_set_cur_dim_value( char *dim_name, char *string )
{
	x_set_cur_dim_value( dim_name, string );
}

/***************************************************************************
 * Flush any pending requests to the display
 */
	void
in_flush()
{
	x_flush();
}

/***************************************************************************
 * This is CALLED BY the displying routines, and contains the reported
 * X and Y positions of the cursor in the color-contour window.
 */
	void
report_position( int x, int y, unsigned int button_mask )
{
	view_report_position( x, y, button_mask );
}

/***************************************************************************/
/* This is called by regular routines which want to know where the
 * cursor is currently inside the color-contour window.
 */
	void
in_query_pointer_position( int *x, int *y )
{
	x_query_pointer_position( x, y );
}

/***************************************************************************/
	void
in_set_edit_place( size_t index, int x, int y, int nx, int ny )
{
	x_set_edit_place( index, x, y, nx, ny );
}

/***************************************************************************/
	void
in_change_dat( size_t index, float new_val )
{
	view_change_dat( index, new_val );
}	

/***************************************************************************/
	void
in_data_edit_dump( void )
{
	view_data_edit_dump();
}

/***************************************************************************/
	int
in_popup_XY_graph( size_t n, int dimindex, double *xvals, double *yvals, char *x_axis_title,
	char *y_axis_title, char *title, char *legend, Stringlist *scannable_dims )
{
	return(x_popup_XY_graph( n, dimindex, xvals, yvals, x_axis_title, y_axis_title, 
			title, legend, scannable_dims ));
}

/***************************************************************************/
	void
in_display_stuff( char *s, char *var_name )
{
	x_display_stuff( s, var_name );
}

/***************************************************************************/
	void
in_popup_2d_window()
{
	x_popup_2d_window();
}

/***************************************************************************/
	void
in_popdown_2d_window()
{
	x_popdown_2d_window();
}

/***************************************************************************/
	int
in_report_auto_overlay( void )
{
	return( x_report_auto_overlay() );
}

