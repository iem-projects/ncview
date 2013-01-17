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

/******************************************************************************
 *
 *	These routines handle the tricky job of setting the proper "view"
 *	of the data file.  The "view" is the 2-D slice which is color-
 *	contoured in the main display window.  Selecting exactly *what* 
 *	2-D plane to contour from a complex, multi-dimensional, multi-variable
 *	data file is not a trivial task!  If all else fails, be prepared
 *	to enter the view using buttons.
 *
 *******************************************************************************/

/* Include files */
#include "ncview.includes.h"
#include "ncview.defines.h"
#include "ncview.protos.h"

/* External variables */
extern	Options options;
extern  NCVar *variables;
extern  FrameStore framestore;

View  *view = NULL;

/* See comments in routine "view_draw" */
static int 	lockout_view_changes = FALSE;

/* Saved x/y values that are on the XY plot, used later for
 * dumping out.
 */
static double *plot_XY_xvals = NULL, *plot_XY_yvals = NULL;

/* Saved dimension for the XY plot of the relevant index,
 * so that we know how to format dumps of that dim's values. Note
 * that all lines on a plot have the same X dimension.
 */
static NCDim *plot_XY_dim[MAX_PLOT_XY];

/* Constants local to routines in this file */
#define	BUTTONS_ALL_ON		1
#define	BUTTONS_TIMEAXIS_OFF	2
#define	BUTTONS_ALL_OFF		3

/* Prototypes applicable to routines used ONLY in this file */
static void 		determine_scan_axes( View *view, NCVar *var, View *old_view );
static void 		initial_determine_scan_axes( View *view, NCVar *var );
static void 		fill_view_data( View *v );
static void 		view_set_axis( View *local_view, int dimension, char *new_dim_name );
static void 		alloc_view_storage( View *view );
static void 		init_view( View **view, NCVar *var );
static void 		set_buttons( int to_state );
static void 		re_determine_scan_axes( View *new_view, NCVar *new_var, View *old_view );
static void 		set_scan_place( View *new_view, NCVar *var, View *old_view );
static void 		initial_set_scan_place( View *view, NCVar *var );
static void 		re_set_scan_place( View *new_view, NCVar *new_var, View *old_view );
static void		calculate_blowup( View *view, NCVar *var, int val_to_set_to );
static void 		draw_file_info( NCVar *var );
static void 		label_dimensions( View *view );
static void 		show_current_dim_values( View *view );
static void		flip_if_inverted( View *view );
static void 		set_range_labels( float min, float max );
static void 		set_scan_buttons( View *local_view );
static void 		view_data_edit_warn();
static void 		invalidate_variable( NCVar *var );
static void 		plot_XY_sc( size_t *start, size_t *count );
static void 		mouse_xy_to_data_xy( int mouse_x, int mouse_y, int blowup, size_t *data_x, size_t *data_y );
static int 		view_data_has_missing( View *v );
static void 		view_construct_scalar_coord_str( char *str, int slen );
static float 		view_calc_minval_float( float *arr, size_t n );
static float 		view_calc_maxval_float( float *arr, size_t n );
static void 		strip_trailing_zeros( char *s );

#define NFRAMES_RECORD	10
static int    n_new_frame_times=0;			/* Numer of valid entries in following two arrays */
static time_t new_frame_times[NFRAMES_RECORD];		/* TIME that new frame(s) were found */
static time_t new_frame_nframes[NFRAMES_RECORD];	/* NUMBER of new frames found at that time */

/********************************************************************************
 * Make the passed variable the new variable which can be scanned using the
 * buttons.
 */
	int
set_scan_variable( NCVar *var )
{
	View	*new_view, *old_view;
	size_t	*start, *count, x_size, y_size, scaled_x_size, scaled_y_size;
	long	i;
	int	changed_size, overlay2use;
	float	range_x, range_y;
	NCDim	*xdim, *ydim, *xdim_old, *xdim_new, *ydim_old, *ydim_new;

	if( options.debug ) {
		fprintf( stderr, "\n\n******************************************\nentering set_scan_variable with var=%s\n", var->name );
		fprintf( stderr, "var nims:%d\n", var->n_dims );
		for( i=0; i<var->n_dims; i++ )
			fprintf( stderr, "dim=%ld size=%ld\n", i, *(var->size + i) );
		}

	in_set_cursor_busy();

	set_buttons( BUTTONS_ALL_ON );
	unlock_plot();

	if( (view == NULL) || (view->x_axis_id == -1) || (view->y_axis_id == -1)) {
		/* A brand new variable to display!  Exciting! */
		if( options.debug )
			fprintf( stderr, "set_scan_variable: initializing view struct for new variable\n" );
		init_view( &view, var );
		set_blowup_type( options.blowup_type );

		/* Figure out what axes to use for X, Y, and Time,
		 * and set the current place based on those axes
		 */
		if( options.debug )
			fprintf( stderr, "...determining scan axes (NEW)\n" );
		determine_scan_axes( view, var, NULL );
		if( options.debug )
			fprintf( stderr, "...axes ids: scan=%d y=%d x=%d\n", 
				view->scan_axis_id, view->y_axis_id, view->x_axis_id );
		if( var->effective_dimensionality == 1 ) {
			start = (size_t *)malloc(view->variable->n_dims*sizeof(size_t));
			count = (size_t *)malloc(view->variable->n_dims*sizeof(size_t));
			for( i=0; i<view->variable->n_dims; i++ ) {
				*(start+i) = *(view->var_place+i);
				*(count+i) = 1L;
				}
			*(count+view->x_axis_id) = *(view->variable->size + view->x_axis_id);
			if( options.debug ) 
				fprintf( stderr, "set_scan_variable (A): about to call plot_XY_sc\n" );
			plot_XY_sc( start, count );
			free( start );
			free( count );
			in_popdown_2d_window();
			in_set_cursor_normal();
			return(0);
			}

		if( options.debug )
			fprintf( stderr, "...setting scan place (NEW)\n" );
		set_scan_place     ( view, var, NULL );

		/* Is the current field inverted?  If so, flip it back */
		if( options.debug )
			fprintf( stderr, "...determining if inverted (NEW)\n" );
		flip_if_inverted( view );
	
		/* How big should we initially make the picture? */
		if( options.debug )
			fprintf( stderr, "...calculating blowup (NEW)\n" );
		calculate_blowup( view, var, -99999 );	/* last val is flag meaning to do automatic calculation */

		/* Save the blowup we are using in the var structure so we can
		 * return to it later if we want
		 */
		var->user_set_blowup = options.blowup;
		if( options.debug )
			fprintf( stderr, "... ... new blowup=%d\n", options.blowup );
		}
	else
		{
		/* Make a NEW view structure, and save the old one, 
		 * because we still want to scavenge the information
		 * on what place we are at from the old variable.  This
		 * is so that when you switch variables, it will keep
		 * the same scan dimensions and place, if possible.
		 */
		if( options.debug )
			fprintf( stderr, "set_scan_variable: initializing view struct for old variable\n" );
		old_view = view;
		init_view( &new_view, var );

		/* Figure out what axes to use for X, Y, and Time,
		 * and set the current place based on those axes and
		 * the previous scan place.
		 */
		if( options.debug )
			fprintf( stderr, "...determing scan axes (PREVIOUS)\n" );
		determine_scan_axes( new_view, var, old_view );
		if( var->effective_dimensionality == 1 ) {
			view = new_view;
			start = (size_t *)malloc(view->variable->n_dims*sizeof(size_t));
			count = (size_t *)malloc(view->variable->n_dims*sizeof(size_t));
			for( i=0; i<view->variable->n_dims; i++ ) {
				*(start+i) = *(view->var_place+i);
				*(count+i) = 1L;
				}
			*(count+view->x_axis_id) = *(view->variable->size + view->x_axis_id);
			if( options.debug ) 
				fprintf( stderr, "set_scan_variable (B): about to call plot_XY_sc\n" );
			plot_XY_sc( start, count );
			free( start );
			free( count );
			in_popdown_2d_window();
			in_set_cursor_normal();
			return(0);
			}
		set_scan_place( new_view, var, old_view );

		/* Calculate new blowup.  If the old var was using the same display dimensions
		 * as the new var is using, then don't modify the current blowup.  Otherwise,
		 * calculate a new blowup.  This is more complicated than it might be because
		 * ncview can process multiple files, each of which might have its own mapping
		 * of dimension id to dimension.  So to really see if we are using the same
		 * dimensions as the previous variable, we have to go through the NAME of the
		 * dimensions.  These are guaranteed to be unique, according to the netCDF 
		 * standard.  (I.e., if two variables are using the "Longitude" dimension, you
		 * can know that it's the SAME Longitude dimension.)
		 */
		xdim_old = *(old_view->variable->dim + old_view->x_axis_id); 
		ydim_old = *(old_view->variable->dim + old_view->y_axis_id); 
		xdim_new = *(new_view->variable->dim + new_view->x_axis_id); 
		ydim_new = *(new_view->variable->dim + new_view->y_axis_id); 
		if( (xdim_old==NULL) || (ydim_old==NULL) || (xdim_new==NULL) || (ydim_new==NULL) ||
		    (strcmp(xdim_old->name, xdim_new->name) != 0) ||
		    (strcmp(ydim_old->name, ydim_new->name) != 0)) {
			if( options.debug )
				fprintf( stderr, "...axis change, recalculating blowup; old, new X dim=%s, %s; old, new Y dim=%s, %s\n",
					xdim_old->name, xdim_new->name, ydim_old->name, ydim_new->name );
			calculate_blowup( new_view, var, var->user_set_blowup );
			/* Save the blowup we are using in the var structure so we can
			 * return to it later if we want
			 */
			var->user_set_blowup = options.blowup;
			}

		/* If the new var has a different shape than the old var,
		 * then clear out any old, previous overlay we might be using.
		 */
		if( (xdim_new == NULL) || (xdim_old == NULL) || (ydim_new == NULL) || (ydim_old == NULL) || 
				(xdim_new->size != xdim_old->size) || (ydim_new->size != ydim_old->size))
			do_overlay(OVERLAY_NONE,NULL,TRUE);

		/* Release the old storage */
		free( old_view->data      );
		free( old_view->pixels    );
		free( old_view->var_place );

		view = new_view;
		}

	/* Set the tape-recorder style buttons to enable or disabled
	 * state, as appropriate for the selected variable and dimensions.
	 */
	set_scan_buttons( view );

	/* Allocate storage space for the data */
	alloc_view_storage( view );

	/* Actually read the data in from the file */
	if( options.debug )
		fprintf( stderr, "...reading data from file\n" );
	fill_view_data( view );

	if( options.save_frames == TRUE )
		{
		if( options.debug )
			fprintf( stderr, "calling init_saveframes from set_scan_variable\n" );
		init_saveframes();
		}

	/* Set the min and maxes of the data */
	if( !view->variable->have_set_range )
		init_min_max( var );

	/* If we are automatically putting on overlays, do so now */
	xdim = *(view->variable->dim + view->x_axis_id);
	ydim = *(view->variable->dim + view->y_axis_id);
	if( options.auto_overlay && in_report_auto_overlay() && xdim->is_lon && ydim->is_lat ) {
		/* Only put overlay on automatically if range is big enough that
		 * the coastlines can be recognized. 
		 */
		range_x = fabs(xdim->max - xdim->min);
		range_y = fabs(ydim->max - ydim->min);
		if( (range_x > 60.) && (range_y > 60.))
			overlay2use = OVERLAY_P8DEG;
		else if( (range_x > 10.) && (range_y > 10.))
			overlay2use = OVERLAY_P08DEG;
		else
			overlay2use = -1;
		if( (overlay2use != -1) && (! view_data_has_missing( view )))
			do_overlay(overlay2use,NULL,TRUE);
		}

	/* Convert the data to pixels; return on error condition */
	if( options.debug )
		fprintf( stderr, "...converting data to pixels\n" );
	lockout_view_changes = TRUE;
	if( data_to_pixels( view ) < 0 ) {
		in_timer_clear();
		if( view->variable->global_min == view->variable->global_max )
			invalidate_variable( view->variable );
		return( -1 );
		}
	lockout_view_changes = FALSE;

	/* put variable and file information on the screen */
	if( options.debug )
		fprintf( stderr, "...putting var & file info on screen\n" );
	draw_file_info( var );

	/* put the dimension information on the screen */
	if( options.debug )
		fprintf( stderr, "...putting dimension info on screen\n" );
	redraw_dimension_info();

	/* Draw the frame info on the screen */
	if( options.debug )
		fprintf( stderr, "...putting frame info on screen, scan_axis_id=%d\n", view->scan_axis_id );
	if( view->scan_axis_id == -1 ) 
		set_scan_view( 0L );
	else
		set_scan_view( *(view->var_place+view->scan_axis_id) );

	/* Actually draw the color contour map of the data! */
	if( options.debug )
		fprintf( stderr, "...drawing color contour field\n" );
	x_size = *(view->variable->size + view->x_axis_id);
	y_size = *(view->variable->size + view->y_axis_id);
	view_get_scaled_size( options.blowup, x_size, y_size, &scaled_x_size, &scaled_y_size );
	changed_size = in_set_2d_size( scaled_x_size, scaled_y_size );
	/* If we increased in size then we don't need to redraw,
	 * because an expansion generates an expose event, which
	 * is registered to call change_view.
	 */
	if( changed_size < 1 ) 
	 	change_view(0,FRAMES);

	/* Pop up the window if we are going to use it. */
	in_popup_2d_window();

	in_set_cursor_normal();

	if( options.debug )
		fprintf( stderr, "...recomputing colorbar\n" );
	view_recompute_colorbar();

	if( options.debug )
		fprintf( stderr, "exiting set_scan_variable.\n" );
	return( 0 );
}

/**************************************************************************************/
	static void
set_scan_buttons( View *local_view )
{
	static int	set_state;
	char		*label    = NULL;
	char		scalar_coord_str[1024];

	set_state = BUTTONS_ALL_ON;

	/* If there is no scan axis, then disable the buttons
	 * which access it.
	 */
	if( local_view->scan_axis_id == -1 ) {
		set_state = BUTTONS_TIMEAXIS_OFF;
		label     = "No scan axis";
		}

	/* If the scan axis is currently appearing as the X or
	 * Y axis, then disable the buttons which step the 
	 * scan axis (since they are ALL being displayed at
	 * the moment!)
	 */
	if( (local_view->scan_axis_id == local_view->x_axis_id) ||
	    (local_view->scan_axis_id == local_view->y_axis_id) ) {
		set_state = BUTTONS_TIMEAXIS_OFF;
		label = "Scan axis is displayed";
		}

	set_buttons( set_state );
	in_set_label( LABEL_SCAN_PLACE, label );

	view_construct_scalar_coord_str( scalar_coord_str, 1020 );
	in_set_label( LABEL_SCALAR_DIMS, scalar_coord_str );
}

/********************************************************************************
 * Change the view we currently have on the data; i.e., scan along
 * the scan-axis.  'interpretation' can be either FRAMES or PERCENT, and
 * indicates how in interpret the passed delta value.
 */
	int
change_view( int delta, int interpretation )
{
	size_t	size;
	long	place;
	float	provisional_delta;

	if( view == NULL )	/* This happens because this routine is called    */
		return(0);	/* when Expose events are generated, and one is   */
				/* generated before the view has been initialized */

	if( delta != 0 ) {
		if( view->data_status == VDS_EDITED ) {
			fprintf( stderr, "warning! flushing changes!\n" );
			}
		view->data_status = VDS_INVALID;
		}

	/* Apply the skip */
	if( interpretation == FRAMES )
		delta *= view->skip;

	if(view->scan_axis_id == -1) {
		if( delta == 0 ) {
			view_draw( FALSE, FALSE );
			return(0);
			}
		else
			{
			fprintf( stderr, 
				"called change_view with no scan_axis\n" );
			exit( -1 );
			}
		}

	if( interpretation == PERCENT ) {
		/* Delta is in percent of total size */
		size              = *(view->variable->size  + view->scan_axis_id);
		provisional_delta = (float)size * (float)delta/100.0;
		if( (int)provisional_delta == 0 ) {
			if( delta < 0 )
				delta = -1;
			else
				delta = 1;
			}
		else
			delta = (int)provisional_delta;
		}

	place = *(view->var_place + view->scan_axis_id) + delta;
	size  = *(view->variable->size  + view->scan_axis_id);

	/* Have we incremented past the maximum allowed value?
	 * If we have, then reset to ZERO, not just (size modulo
	 * step), so that when stepping with a skip > 1, we can
	 * save frames and come back to the same frames in the
	 * framestore.
	 */
	if( place >= (long)size ) {
		place = 0L;
		if( options.beep_on_restart )
			beep();
		}
		
	/* Have we decremented below the minimum allowed value? */
	if( place < 0L )
		place = size - 1L;

	set_scan_view( place );
	return( view_draw( TRUE, FALSE ) );
}

/********************************************************************************
 * Set the time place of the view to the specified location.
 */
	void
set_scan_view( size_t scan_place )
{
	char	temp_string[1024], view_place[1024], scalar_coord_str[1024];
	size_t	size;
	char	*dim_name;
	double	new_dimval, bound_min, bound_max;
	nc_type	type;
	NCDim	*dim;
	int	has_bounds;

	/* If there is no valid scan axis, immediately return.  That 
	 * case is delt with exclusively at start-up, when selecting
	 * a new variable.
	 */
	if( view->scan_axis_id == -1 )
		return;

	size = *(view->variable->size + view->scan_axis_id);
	if( scan_place >= size ) {
		fprintf( stderr, "ncview: set_scan_view: internal error; trying to " );
		fprintf( stderr, "set to a place larger than exists\n" );
		fprintf( stderr, "size: %ld   attempted place: %ld\n", size, scan_place+1 );
		fprintf( stderr, "resetting to zero\n" );
		scan_place = 0;
		}
		
	dim = *(view->variable->dim + view->scan_axis_id);
	dim_name = dim->name;
	*(view->var_place + view->scan_axis_id) = scan_place;
	snprintf( view_place, 1023, "frame %1ld/%1ld ", scan_place+1, size );

	/* type is the data type of the dimension--can be float or character */
	type = fi_dim_value( view->variable, view->scan_axis_id, scan_place, &new_dimval, 
			temp_string, &has_bounds, &bound_min, &bound_max, view->var_place );
	if( type == NC_DOUBLE ) {
		if( dim->timelike && options.t_conv ) {
			fmt_time( temp_string, 1024, new_dimval, dim, 1 );
			strcat( view_place, temp_string );
			if( has_bounds ) {
				snprintf( temp_string, 1023, " (%d bnds:", has_bounds );
				strcat( view_place, temp_string );

				fmt_time( temp_string, 1024, bound_min, dim, 0 );
				strcat( view_place, temp_string );

				strcat( view_place, " -> " );

				fmt_time( temp_string, 1024, bound_max, dim, 0 );
				strcat( view_place, temp_string );

				strcat( view_place, ")" );
				}
			}
		else
			{
			snprintf( temp_string, 1023, "%lg", new_dimval );
			if( has_bounds ) {
				snprintf( temp_string, 1023, " (%d bnds:", has_bounds );
				strcat( view_place, temp_string );

				snprintf( temp_string, 1023, "%lg", bound_min );
				strcat( view_place, temp_string );

				strcat( view_place, " -> " );

				snprintf( temp_string, 1023, "%lg", bound_max );
				strcat( view_place, temp_string );

				strcat( view_place, ")" );
				}
			}
		}
	else
		; /* don't have to do anything, since string-type dimval
		   * is already in variable "temp_string"
		   */
	in_set_label( LABEL_SCAN_PLACE, view_place );
	in_set_cur_dim_value( dim_name, temp_string );
	view->data_status = VDS_INVALID;
	if( options.want_extra_info ) {
		in_set_label( LABEL_CCINFO_2, temp_string );
		}

	/* Construct string showing the values of the scalar coordinates
	 * for this variable, if any 
	 */
	view_construct_scalar_coord_str( scalar_coord_str, 1020 );
	in_set_label( LABEL_SCALAR_DIMS, scalar_coord_str );
}

/********************************************************************************
 * draw the current view onto the display
 */
	int
view_draw( int allow_framestore_usage, int force_range_to_frame )
{
	long		i; 
	size_t		x_size, y_size, scan_size, scaled_x_size, scaled_y_size, framesize, frameno;
	static int	last_x_size=0, last_y_size=0;
	int		must_recalc_range;
	float		min, max, dat;

	/* The reason why we have to lockout the possiblity that this
	 * routine is called WHILE it is executing is tricky.  The 
	 * 'data_to_pixels' call, below and elsewhere, can result in a modal
	 * dialog being popped up, but the ccontour window can still
	 * get 'expose' events.  In that case multiple modal dialogs would
	 * be popped up, to conflict at random, unless there were some
	 * way of locking out entry to this subroutine while it is actively
	 * being executed or the other modal dialogs are popped up.
	 */
	if( lockout_view_changes )
		return(0);
	lockout_view_changes = TRUE;

	/* These can happen because this routine is called when the ccontour
	 * window gets 'expose' events, which happens on program startup,
	 * before the view has been initialized.
	 */
	if( (view == NULL) || (view->data == NULL)) { 
		lockout_view_changes = FALSE;
		return(0);
		}

	x_size = *(view->variable->size + view->x_axis_id);
	y_size = *(view->variable->size + view->y_axis_id);

	/* If we need to adjust the range to the current frame, then do so */
	must_recalc_range = force_range_to_frame || options.autoscale;
	if( must_recalc_range ) {
		min = 1.0e35;
		max = -min;

		for( i=0; i<x_size*y_size; i++ ) {
			dat = *((float *)(view->data)+i);
			if( dat != dat ) 
				dat = view->variable->fill_value;
			if( ! close_enough( dat, view->variable->fill_value) && (dat != FILL_FLOAT)) {
				if( dat > max )
					max = dat;
				if( dat < min )
					min = dat;
				}
			}

		view->variable->user_min = min;
		view->variable->user_max = max;
		set_range_labels( min, max );
		view->data_status = VDS_INVALID;
		invalidate_all_saveframes();	/* note we invalidate all frames, so even if allow_framestore_useage is TRUE, it won't happen */
		view_recompute_colorbar();
		}

	view_get_scaled_size( options.blowup, x_size, y_size, &scaled_x_size, &scaled_y_size );

	framesize = scaled_x_size * scaled_y_size;
	if( view->scan_axis_id == -1 )
		frameno = 0;
	else
		frameno = *(view->var_place + view->scan_axis_id);

	if( options.debug ) {
		fprintf( stderr, "in view_draw:\n" );
		fprintf( stderr, "	x_size, y_size:%ld %ld\n",
						x_size, y_size );
		fprintf( stderr, "	scan_axis_id:%d\n",
						view->scan_axis_id);
		fprintf( stderr, "	scan_place:%ld\n",
						frameno );
		}

	/* Is this frame stored in the framestore? */
	if( framestore.valid && allow_framestore_usage ) {
		if( *(framestore.frame_valid + frameno) == TRUE ) {
			if( options.debug )
				printf( "drawing from framestore...\n" );
			in_draw_2d_field( (framestore.frame + frameno*framesize), 
				scaled_x_size, scaled_y_size, frameno );
			lockout_view_changes = FALSE;

			if( view->scan_axis_id != -1 ) {
				scan_size  = *(view->variable->size  + view->scan_axis_id);
				if( (frameno == (scan_size-1)) && (which_button_pressed() == BUTTON_PAUSE)) {
					in_timer_set( (XtTimerCallbackProc)view_check_new_data, 0, 1000L );
					}
				}
			return(0);
			}
		}

	if( view->data_status == VDS_INVALID ) {
		if( options.debug )
			printf( "Reading data to contour...\n" );
		fill_view_data( view );
		}
	else
		{
		if( options.debug )
			printf( "NOT reading data to contour, since data is valid (%d)\n", view->data_status );
		}

	if( options.debug )
		printf( "Calling data_to_pixels...\n" );
	if( data_to_pixels( view ) < 0 ) {
		in_timer_clear();
		if( view->variable->global_min == view->variable->global_max )
			invalidate_variable( view->variable );
		lockout_view_changes = FALSE;
		return( -1 );
		}

	if( (last_x_size != scaled_x_size) ||
	    (last_y_size != scaled_y_size)) {
		last_x_size = scaled_x_size;
		last_y_size = scaled_y_size;
		in_set_2d_size  ( scaled_x_size, scaled_y_size );
		}

	if( options.debug )
		printf( "Calling draw_2d_field...\n" );
	in_draw_2d_field( view->pixels, scaled_x_size, scaled_y_size, frameno );

	if( framestore.valid == TRUE ) {
		for( i=0; i<framesize; i++ )
			*(framestore.frame + frameno*framesize + i) = *(view->pixels + i);
		*(framestore.frame_valid + frameno) = TRUE;
		}

	/* If we just drew the last time entry for this var, then
	 * set up a callback that waits 1 second and checks for
	 * the var having new data in it.
	 */
	if( view->scan_axis_id != -1 ) {
		scan_size  = *(view->variable->size  + view->scan_axis_id);
		if( (frameno == (scan_size-1)) && (which_button_pressed() == BUTTON_PAUSE)) {
			in_timer_set( (XtTimerCallbackProc)view_check_new_data, 0, 1000L );
			}
		}

	lockout_view_changes = FALSE;
	return( 0 );
}

/********************************************************************************
 * Checks if the file has grown since we last saw it
 */
	void
view_check_new_data( int unused )
{
	size_t 	file_var_size[MAX_NC_DIMS], *t, n_other;
	int	i, has_grown, ierr, t_ncid, timelike_index;
	size_t	dt, nt_new, n_scan_entries, n_extra_frames, storage_size, old_nt;
	char	message[1024], rate_units[50];
	time_t	tt;
	long	nframes_tot, delta_time;
	float	rate_per_sec, rate_per_min, rate_per_hour, rate_per_day, rate,
		min, max, *data, avg;

	in_timer_clear();

	timelike_index = 0;

	/* We only check for a file being extended if ncview is currently
	 * displaying the last time step.  We do not check if, for example,
	 * a Hoovermuller diagram is being displayed.
	 */
	if( view->scan_axis_id != timelike_index ) 	/* in netcdf v3, simple way to check for time dimension */
		return;

	/* Get size of the var in the last file */
	t_ncid = netcdf_fi_initialize( view->variable->last_file->filename );
	t = netcdf_fi_var_size( t_ncid, view->variable->name );
	for( i=0; i<view->variable->n_dims; i++ )
		file_var_size[i] = t[i];
	free( t );
	ierr = nc_close( t_ncid );

	has_grown = 0;
	if( file_var_size[ timelike_index ] > view->variable->last_file->var_size[ timelike_index ] ) {
		has_grown = 1;
		}

	if( ! has_grown ) {
		in_timer_set( (XtTimerCallbackProc)view_check_new_data, 0, 1000L );
		return;
		}

	dt = file_var_size[timelike_index] - view->variable->last_file->var_size[timelike_index];
	nt_new = view->variable->size[timelike_index] + dt;

	/* Make our informative label */
	tt = time(NULL);	/* Time this new frame was found, in seconds since epoch */
	if( n_new_frame_times == NFRAMES_RECORD ) {
		for( i=0; i<(NFRAMES_RECORD-1); i++ ) {
			new_frame_times[i]   = new_frame_times[i+1];
			new_frame_nframes[i] = new_frame_nframes[i+1];
			}
		n_new_frame_times = NFRAMES_RECORD-1;
		}
	new_frame_times[n_new_frame_times] = tt;
	new_frame_nframes[n_new_frame_times] = dt;
	n_new_frame_times++;

	if( n_new_frame_times < 2 ) 
		snprintf( message, 1023, "New frame found %s", ctime(&tt) );
	else
		{
		nframes_tot = 0;
		for( i=0; i<(n_new_frame_times-1); i++ )
			nframes_tot += new_frame_nframes[i];
		delta_time = new_frame_times[n_new_frame_times-1] - new_frame_times[0];
		rate_per_sec = (float)(nframes_tot)/(float)(delta_time);
		rate_per_min = rate_per_sec   * 60.0;
		rate_per_hour = rate_per_min  * 60.0;
		rate_per_day  = rate_per_hour * 24.0;

		if( rate_per_sec > 0.5 ) {
			rate = rate_per_sec;
			strcpy( rate_units, "/sec" );
			}
		else if( rate_per_min > 0.5 ) {
			rate = rate_per_min;
			strcpy( rate_units, "/min" );
			}
		else if( rate_per_hour > 0.5 ) {
			rate = rate_per_hour;
			strcpy( rate_units, "/hour" );
			}
		else
			{
			rate = rate_per_day;
			strcpy( rate_units, "/day" );
			}
		snprintf( message, 1023, "New frame found %s (Rate=%.2f%s)", ctime(&tt), rate, rate_units);
		for( i=0; i<strlen(message); i++ )
			if( message[i] == 10 )
				message[i] = ' ';
		}
	in_set_label( LABEL_TITLE, message );

	/* See if we need to reallocate the framestore */
	if( nt_new >= framestore.nt ) {
		old_nt = framestore.nt;
		n_scan_entries = *(view->variable->size + view->scan_axis_id);
		n_extra_frames = floor( n_scan_entries * 0.2 ) + 1;
		if( n_extra_frames < 25 )
			n_extra_frames = 25;
		framestore.nt = nt_new + n_extra_frames;
		storage_size  = framestore.nx * framestore.ny * framestore.nt;

		if( options.debug )
			printf( "reallocating framestore to new nt=%ld\n", framestore.nt );

		framestore.frame = (ncv_pixel *)realloc( framestore.frame, storage_size*sizeof(ncv_pixel) );
		if( framestore.frame == NULL ) {
			framestore.valid = FALSE;
			return;
			}

		framestore.frame_valid = (int *)realloc( framestore.frame_valid, framestore.nt*sizeof(int) );
		if( framestore.frame_valid == NULL ) {
			framestore.valid = FALSE;
			return;
			}

		/* Initialize to NOT a valid frame for the new frames */
		for( i=old_nt; i<framestore.nt; i++ )
			*(framestore.frame_valid+i) = FALSE;
		}

	view->variable->size[ timelike_index ] = nt_new;
	view->variable->last_file->var_size[ timelike_index ] += dt;

	/* Resync so we will read the last time entry */
	ierr = nc_sync( view->variable->last_file->id );

	/* Special check: if we were started with no range in the variable,
	 * but now we have one, then reset the displayed range
	 */
	if( view->variable->auto_set_no_range ) {
		n_other = 1L;
		for( i=0; i<view->variable->n_dims; i++ )
			if( i != timelike_index ) 
				n_other *= view->variable->size[i];
		data = (float *)malloc( sizeof(float)*n_other );
		if( data == NULL ) {
			fprintf( stderr, "Error, failed to allocate data array in routine view_check_new_data (size=%ld bytes)\n",
				sizeof(float)*n_other );
			exit(-1);
			}
		get_min_max_onestep( view->variable, n_other, nt_new, data, &min, &max, 0 );
		free( data );
		if( min != max ) {
			view->variable->auto_set_no_range = 0;
			if( (min < 0) && (max > 0)) {
				if( -min > max ) {
					min = min * 3.0;
					max = -min;
					}
				else
					{
					max = max * 3.0;
					min = -max;
					}
				}
			else if( min >= 0 ) {
				if( min == 0 )
					max = max * 3.0;
				else
					{
					avg = (min+max)*0.5;
					min = avg - (avg-min)*3.0;
					if( min < 0 )
						min = 0;
					max = avg + (max-avg)*3.0;
					}
				}
			else
				{
				if( max == 0 )
					min = min * 3.0;
				else
					{
					avg = (min+max)*0.5;
					min = avg - (avg-min)*3.0;
					max = avg + (max-avg)*3.0;
					if( max > 0 )
						max = 0;
					}
				}
				
			view->variable->user_min = min;
			view->variable->user_max = max;
			set_range_labels( min, max );
			view->data_status = VDS_INVALID;
			invalidate_all_saveframes();
			view_recompute_colorbar();
			}
		}

	/* Jump to the last frame and display it.
	 * NOTE that this ALSO sets the timer to call this
	 * routine again as a side effect
	 */
	change_view( dt, FRAMES );
}

/********************************************************************************
 * Determine what axes we should display the data using.  This returns
 * a three element Stringlist*; the first is the 'scan' axis, the second
 * is the 'Y' axis, and the third is the 'X' axis.  If there is no valid
 * scan axis, return a zero length string ("") as that dimension.
 * If old_view is not NULL, then old_view is assumed to be the view which was
 * in use immediately preceeding a change of view; in that case, if 
 * possible, use the same view as before.  The exception to this is if
 * the previous variable was only a 1-d variable; in that case we do not
 * want to limit ourselves to its restrictions if we can avoid them.
 */
	static void
determine_scan_axes( View *view, NCVar *var, View *old_view )
{
	initial_determine_scan_axes( view, var );

	if( view->scan_axis_id != -1 ) 
		view->plot_XY_axis = view->scan_axis_id;
	else if( view->x_axis_id != -1 ) 
		view->plot_XY_axis = view->x_axis_id;
	/* We can't do an XY plot of dimensions that have a count
	 * of one.  In that case, try to set it to something else.
	 */
	if( *(view->variable->size + view->plot_XY_axis) == 1 ) {
		if( (view->scan_axis_id != -1) &&  
		    (*(view->variable->size + view->scan_axis_id) > 1))
			view->plot_XY_axis = view->scan_axis_id;

		else if( (view->x_axis_id != -1) &&  
		    (*(view->variable->size + view->x_axis_id) > 1))
			view->plot_XY_axis = view->x_axis_id;

		else if( (view->y_axis_id != -1) &&  
		    (*(view->variable->size + view->y_axis_id) > 1))
			view->plot_XY_axis = view->y_axis_id;
		}

	if( (old_view != NULL) && (old_view->variable->effective_dimensionality > 1))
		{
		re_determine_scan_axes( view, var, old_view );

		/* We can exit the above code with the X and Y dimensions
		 * the same, if the newly picked variable has less dimensions than
		 * the old one BUT some overlap of dimensions.  Check for
		 * this, and give up on picking the X and Y dimensions this
		 * way if this is the case.
		 */
		if( view->x_axis_id == view->y_axis_id )
			initial_determine_scan_axes( view, var );

		/* Don't let an axis be BOTH scan AND x or y */
		if( view->x_axis_id == view->scan_axis_id )
			view->scan_axis_id = -1;
		if( view->y_axis_id == view->scan_axis_id )
			view->scan_axis_id = -1;

		/* Final sanity checks! */
		if( (view->x_axis_id == -1) || 
		    (view->y_axis_id == -1) ||
		    (view->x_axis_id > view->variable->n_dims) ||
		    (view->y_axis_id > view->variable->n_dims))
			initial_determine_scan_axes( view, var );
		}
}

/**************************************************************************************/
	static void
initial_determine_scan_axes( View *view, NCVar *var )
{
	Stringlist	*dimlist;
	int		n_dims;

	if( options.debug ) fprintf( stderr, "initial_determine_scan_axes: entering for var %s\n", var->name );

	/* Get a list of all possible scannable dimensions */
	dimlist = fi_scannable_dims( var->first_file->id, var->name );

	if( options.debug ) {
		fprintf( stderr, "initial_determine_scan_axes: scannable dims:\n" );
		stringlist_dump( dimlist );
		}

	/* For now, just pick the last two to be the Y and X axes.
	 */ 
	n_dims = stringlist_len( dimlist );
	switch( n_dims ) {
		case 1:
			view->scan_axis_id = -1;
			view->y_axis_id    = -1;
			view->x_axis_id    = fi_dim_name_to_id( 
					var->first_file->id,
					var->name,
					dimlist->string );
			break;

		case 2:
			view->scan_axis_id = -1;
			view->y_axis_id    = fi_dim_name_to_id( 
					var->first_file->id,
					var->name,
					dimlist->string );
			if( view->y_axis_id == -1 ) {
				fprintf( stderr, "initial_determine_scan_axes: internal error: dim >%s< was indicated by routine fi_scannable_dims to be a scannable dim for var >%s<, but routine fi_dim_name_to_id did not find that dim for the var\n",
					dimlist->string, var->name );
				exit(-1);
				}
			dimlist            = dimlist->next;
			view->x_axis_id    = fi_dim_name_to_id( 
					var->first_file->id,
					var->name,
					dimlist->string );
			if( view->x_axis_id == -1 ) {
				fprintf( stderr, "initial_determine_scan_axes: internal error: dim >%s< was indicated by routine fi_scannable_dims to be a scannable dim for var >%s<, but routine fi_dim_name_to_id did not find that dim for the var\n",
					dimlist->string, var->name );
				exit(-1);
				}
			break;

		default:
			/* By default, set the scan dimension to the first of the scannable
	 		* dims, because that one will be the 'time' dimension by netCDF
	 		* standards.
	 		*/
			view->scan_axis_id = fi_dim_name_to_id( 
					var->first_file->id,
					var->name,
					dimlist->string );
			if( view->scan_axis_id == -1 ) {
				fprintf( stderr, "initial_determine_scan_axes: internal error: dim >%s< was indicated by routine fi_scannable_dims to be a scannable dim for var >%s<, but routine fi_dim_name_to_id did not find that dim for the var\n",
					dimlist->string, var->name );
				exit(-1);
				}
			dimlist            = dimlist->next;

			/* Go to the second to the last entry */
			while( ((Stringlist *)(dimlist->next))->next != NULL )
				dimlist = dimlist->next;
			view->y_axis_id    = fi_dim_name_to_id( 
					var->first_file->id,
					var->name,
					dimlist->string );
			if( view->y_axis_id == -1 ) {
				fprintf( stderr, "initial_determine_scan_axes: internal error: dim >%s< was indicated by routine fi_scannable_dims to be a scannable dim for var >%s<, but routine fi_dim_name_to_id did not find that dim for the var\n",
					dimlist->string, var->name );
				exit(-1);
				}
			dimlist            = dimlist->next;
			view->x_axis_id    = fi_dim_name_to_id( 
					var->first_file->id,
					var->name,
					dimlist->string );
			if( view->x_axis_id == -1 ) {
				fprintf( stderr, "initial_determine_scan_axes: internal error: dim >%s< was indicated by routine fi_scannable_dims to be a scannable dim for var >%s<, but routine fi_dim_name_to_id did not find that dim for the var\n",
					dimlist->string, var->name );
				exit(-1);
				}
			break;
		}

	if( options.debug ) fprintf( stderr, "initial_determine_scan_axes: exiting with axis_ids: scan=%d y=%d x=%d\n",
		view->scan_axis_id, view->y_axis_id, view->x_axis_id );
}

/********************************************************************************
 * Actually go to the data file and read the data in, putting the result
 * in the view structure.
 */
	static void
fill_view_data( View *v )
{
	size_t	*count;
	int	i;

	if( v->data_status == VDS_VALID )
		return;

	count = (size_t *)malloc( v->variable->n_dims * sizeof( size_t ));

	/* By default, count of 1 for all uninteresting dimensions */
	for( i=0; i<v->variable->n_dims; i++ ) 
		*(count+i) = 1;

	/* Do full count of the X and Y axes so we can display the whole 2D field */
	*(count+v->x_axis_id) = *(v->variable->size + v->x_axis_id);
	*(count+v->y_axis_id) = *(v->variable->size + v->y_axis_id);

	if( options.debug || options.show_sel ) {
		printf( "-var %s -start \\(", v->variable->name );
		for( i=v->variable->n_dims-1; i >= 0; i-- ) {
			printf( "%1ld", 1 + (*(v->var_place+i)) );
			if( i != 0 )
				printf( "," );
			}
		printf( "\\) -count \\(" );
		for( i=v->variable->n_dims-1; i >= 0; i-- ) {
			printf( "%1ld", *(count+i) );
			if( i != 0 )
				printf( "," );
			}
		printf( "\\) %s\n", v->variable->first_file->filename );
		}

	fi_get_data( v->variable, v->var_place, count, v->data );

	v->data_status = VDS_VALID;
	free( count );
}

/********************************************************************************
 * Alter the amount by which we are blowing up pixels
 */
	void
view_change_blowup( int delta, int redraw_flag, int view_var_is_valid )
{
	size_t	x_size, y_size, scaled_x_size, scaled_y_size;
	char	blowup_label[32];
	int	changed_size;

	in_set_cursor_busy();

	/* Sequence of 'options.blowup' should be: ..., -4, -3, -2, 1, 2, 3, ... */
	if( delta > 0 ) {
		if( options.blowup + delta == 0 ) 
			options.blowup = 2;
		else if( options.blowup + delta == -1 )
			options.blowup = 1;
		else
			options.blowup += delta;
		}
	else if( delta < 0 ) {
		if( options.blowup + delta == 0 ) 
			options.blowup = -2;
		else if( options.blowup + delta == -1 )
			options.blowup = -3;
		else
			options.blowup += delta;
		}

	if( options.blowup > 0 ) 
		snprintf( blowup_label, 31, "M X%1d", options.blowup );
	else
		snprintf( blowup_label, 31, "M 1/%1d", -options.blowup );

        in_set_label( LABEL_BLOWUP, blowup_label );

	if( view_var_is_valid ) {
		view->variable->user_set_blowup = options.blowup;
		}

	free( view->pixels );

	x_size       = *(view->variable->size + view->x_axis_id);
	y_size       = *(view->variable->size + view->y_axis_id);
	view_get_scaled_size( options.blowup, x_size, y_size, &scaled_x_size, &scaled_y_size );

	view->pixels = (void *)malloc( scaled_x_size*scaled_y_size*sizeof(ncv_pixel) );

	if( options.save_frames == TRUE ) {
		if( options.debug )
			fprintf( stderr, "calling init_saveframes from view_change_blowup\n" );
		init_saveframes();
		}

	if( redraw_flag ) {
		changed_size = in_set_2d_size( scaled_x_size, scaled_y_size );
		/* Have to test and see if we shrunk; no expose event
		 * is generated in such a case, which would automatically
		 * trigger this call without us having to do it.
		 */
		if( changed_size < 0 ) 
			view_draw( FALSE, FALSE );
		}
	in_set_cursor_normal();
}

/**************************************************************************************/
	void
redraw_ccontour()
{
printf( "got an expose event\n" );
	view_draw( TRUE, FALSE ); 
}

/************************************************************************
 * This routine handles the case where the user presses the button 
 * which has the current dimension value, indicating that they want
 * it to change.
 */
	void
view_change_cur_dim( char *dim_name, int modifier )
{
	int	dimid, fileid, has_bounds;
	nc_type	type;
	double	new_dimval, bound_min, bound_max;
	size_t	place, size;
	long	delta, prov_place;
	NCDim	*dim;
	char	temp_string[1024];

	if( view == NULL ) {
		in_error( "Please select a variable first" );
		return;
		}

	if( view->data_status == VDS_EDITED ) 
		view_data_edit_warn();

	fileid = view->variable->first_file->id;
	dimid  = fi_dim_name_to_id( fileid,
				view->variable->name, dim_name );
	if( (dimid == view->x_axis_id) ||
	    (dimid == view->y_axis_id) )
		return;

	dim = *(view->variable->dim + dimid);

	/* Modifier 1 is the standard action */
	if( modifier == MOD_1 ) {
		*(view->var_place+dimid) = *(view->var_place+dimid)+1L;
		if( *(view->var_place+dimid) > *(view->variable->size+dimid)-1L )
			*(view->var_place+dimid) = 0L;
		}
	else if( modifier == MOD_2 ) {
		/* Modifier 2 means "do it faster" */
		size  = *(view->variable->size+dimid);
		delta = (int)(0.1*(float)size);
		*(view->var_place+dimid) = *(view->var_place+dimid)+(long)delta;
		if( *(view->var_place+dimid) > *(view->variable->size+dimid)-1L )
			*(view->var_place+dimid) = 0L;
		}
	else
		{
		/* Modifier 3 means to go backwards */
		prov_place = *(view->var_place+dimid)-1L;
		if( prov_place < 0L )
			*(view->var_place+dimid) = *(view->variable->size+dimid) -1L;
		else
			*(view->var_place+dimid) = prov_place;
		}

	place = *(view->var_place+dimid);

	type  = fi_dim_value( view->variable, dimid, place, &new_dimval, temp_string, 
		&has_bounds, &bound_min, &bound_max, view->var_place );
	if( type == NC_DOUBLE ) {
		if( dim->timelike && options.t_conv ) {
			fmt_time( temp_string, 1024, new_dimval, dim, 1 );
			}
		else
			snprintf( temp_string, 1023, "%lg", new_dimval );
		}
	in_set_cur_dim_value( dim_name, temp_string );

	if( options.debug )
		fprintf( stderr, "calling init_saveframes from view_change_cur_dim\n" );

	view->data_status = VDS_INVALID;
	init_saveframes();

	view_draw( TRUE, FALSE ); /* 'TRUE' because we initialized saveframes above */
}

/**********************************************************************
 * This is ultimately what changes what the X and Y dims are.  It is
 * called when the interface button requesting that a change to the
 * scan dimension is pressed, and itself calls the routine which asks
 * the user to make a new selection.
 */
	void
view_set_scan_dims( void )
{
	Stringlist *dim_list, *new_dim_list = NULL, *inv_dim_list, *s;
	int	   changed_something = FALSE;
	NCVar	   *v;
	char	   *cur_x_name, *cur_y_name;
	char	   scan_dim[256];
	int        new_x_id, new_y_id, message;

	v          = view->variable;
	cur_x_name = (*(v->dim+view->x_axis_id))->name;
	cur_y_name = (*(v->dim+view->y_axis_id))->name;

	dim_list = fi_scannable_dims( v->first_file->id, v->name );
	strcpy( scan_dim, dim_list->string );

	/* Pop up the dialog box which asks for the user's selection */
	message  = in_set_scan_dims( dim_list, cur_x_name, 
				cur_y_name, &new_dim_list );
	if( message == MESSAGE_CANCEL )
		return;

	/* A special check: we don't allow transposition of the data
	 * because of the huge performance penalty it would be to
	 * auto-transform it back to the desired configuration.  To
	 * take care of this, if the axes are transposed, then warn
	 * the user, and flip them.
	 */
	s        = new_dim_list;
	new_y_id = fi_dim_name_to_id( v->first_file->id, v->name, s->string );
	s        = s->next;
	new_x_id = fi_dim_name_to_id( v->first_file->id, v->name, s->string );
	if( new_x_id < new_y_id ) {
		message = in_dialog( "Transposing the data is not allowed.\nI'm switching the axes....", NULL, TRUE );
		if( message == MESSAGE_CANCEL )
			return;
		inv_dim_list = NULL;
		stringlist_add_string( &inv_dim_list, s->string, NULL, SLTYPE_NULL );
		stringlist_add_string( &inv_dim_list, new_dim_list->string, NULL, SLTYPE_NULL );
		new_dim_list = inv_dim_list;
		}
	if( new_x_id == new_y_id ) {
		in_error( "Please pick dimensions for the X and\nY axis which are not the same." );
		return;
		}

	in_set_cursor_busy();

	if( strcmp( cur_y_name, new_dim_list->string ) != 0 ) {
		view_set_axis( view, DIMENSION_Y, new_dim_list->string );
		changed_something = TRUE;
		}

	new_dim_list = new_dim_list->next;
	if( strcmp( cur_x_name, new_dim_list->string ) != 0 ) {
		view_set_axis( view, DIMENSION_X, new_dim_list->string );
		changed_something = TRUE;
		}

	if( changed_something == TRUE ) {
		/* In general, we want to set the scan axis back to the
		 * unlimited axis if possible, because no unlimited
		 * dimension ever comes up in the pop-up box to be able
		 * to set it that way.  Use the previously saved value.
		 */
		view_set_axis( view, DIMENSION_SCAN, scan_dim );
		flip_if_inverted( view );
		redraw_dimension_info();
		view->data_status = VDS_INVALID;
		alloc_view_storage( view );
		init_saveframes();
		set_scan_buttons( view );
		view_draw( TRUE, FALSE ); /* 'TRUE' because we initialized saveframes above */
		}

	in_set_cursor_normal();
}

/**************************************************************************************/
	static void
view_set_axis( View *local_view, int dimension, char *new_dim_name )
{
	int	new_id, old_id;
	NCVar	*v;

	v      = local_view->variable;
	new_id = -1;
	old_id = -1;

	switch( dimension ) {
		case DIMENSION_X:
			new_id = fi_dim_name_to_id( v->first_file->id, 
						v->name, new_dim_name );
			if( options.debug ) 
				fprintf( stderr, "setting dim X to %s\n", 
						new_dim_name );
			old_id = local_view->x_axis_id;
			local_view->x_axis_id = new_id;
			*(local_view->var_place+new_id) = 0L;
			break;

		case DIMENSION_Y:
			new_id = fi_dim_name_to_id( v->first_file->id, 
						v->name, new_dim_name );
			if( options.debug ) 
				fprintf( stderr, "setting dim Y to %s\n", 
						new_dim_name );
			old_id = local_view->y_axis_id;
			local_view->y_axis_id = new_id;
			*(local_view->var_place+new_id) = 0L;
			break;

		case DIMENSION_SCAN:
			if( strlen( new_dim_name ) == 0 ) {
				set_buttons( BUTTONS_TIMEAXIS_OFF );
				local_view->scan_axis_id = -1;
				return;
				}
			set_buttons( BUTTONS_ALL_ON );
			new_id = fi_dim_name_to_id( v->first_file->id, 
						v->name, new_dim_name );
			old_id = local_view->scan_axis_id;
			local_view->scan_axis_id = new_id;
			if( options.debug ) 
				fprintf( stderr, "setting dim SCAN to %s\n", 
						new_dim_name );
			break;

		case DIMENSION_NONE:
			new_id = fi_dim_name_to_id( v->first_file->id, 
						v->name, new_dim_name );
			if( options.debug ) 
				fprintf( stderr, "setting dim NONE to %s\n", 
						new_dim_name );
			return;
		}
				
	if( old_id == new_id )
		return;

	in_indicate_active_dim( dimension, new_dim_name );
}

/**************************************************************************************/
	static void
alloc_view_storage( View *view )
{
	size_t	x_size, y_size, tot_size, scaled_x_size, scaled_y_size;

	/* Allocate storage space for the data in the view structure
	 */

	if( view->data_status == VDS_EDITED )
		view_data_edit_warn();
	view->data_status = VDS_INVALID;
		
	if( view->data   != NULL )
		free( view->data   );
	if( view->pixels != NULL )
		free( view->pixels );
	x_size       = *(view->variable->size + view->x_axis_id);
	y_size       = *(view->variable->size + view->y_axis_id);
	view_get_scaled_size( options.blowup, x_size, y_size, &scaled_x_size, &scaled_y_size );

	tot_size     = x_size*y_size*sizeof(float);
	view->data   = (void *)malloc( tot_size );
	if( view->data == NULL ) {
		fprintf( stderr, "ncview: can't allocate data array\n" );
		fprintf( stderr, "variable name: %s\n", view->variable->name );
		fprintf( stderr, "requested size: %ldx%ld\n", x_size, y_size );
		fprintf( stderr, "x axis id:%d  y axis id:%d\n",
			view->x_axis_id, view->y_axis_id );
		fprintf( stderr, "x axis name:%s  y axis name:%s\n",
			fi_dim_id_to_name( view->variable->first_file->id,
					   view->variable->name,
					   view->x_axis_id ),
			fi_dim_id_to_name( view->variable->first_file->id,
					   view->variable->name,
					   view->y_axis_id ) );
		exit( -1 );
		}
	view->pixels = (ncv_pixel *)malloc( scaled_x_size*scaled_y_size*sizeof(ncv_pixel) );
	if( view->pixels == NULL ) {
		fprintf( stderr, "ncview: can't allocate pixel array\n" );
		fprintf( stderr, "variable name: %s\n", view->variable->name );
		fprintf( stderr, "requested size: %ld x %ld\n", 
				scaled_x_size, 
				scaled_y_size );
		fprintf( stderr, "x axis id:%d  y axis id:%d\n",
			view->x_axis_id, view->y_axis_id );
		fprintf( stderr, "x axis name:%s  y axis name:%s\n",
			fi_dim_id_to_name( view->variable->first_file->id,
					   view->variable->name,
					   view->x_axis_id ),
			fi_dim_id_to_name( view->variable->first_file->id,
					   view->variable->name,
					   view->y_axis_id ) );
		exit( -1 );
		}
}

/********************************************************************
 * The user has requested that we change the displayed range;
 * pop up appropriate dialog windows, get the new min and max
 * values from the user, and set them.
 */
	void
view_set_range( void )
{
	float	new_min, new_max;
	int	message, allvars;
	NCVar	*cursor;

	message = x_range( view->variable->user_min, view->variable->user_max, 
		view->variable->global_min, view->variable->global_max, 
		&new_min, &new_max, &allvars );
	if( message == MESSAGE_CANCEL )
		return;

	view->variable->user_min = new_min;
	view->variable->user_max = new_max;
	set_range_labels( new_min, new_max );
	view->data_status = VDS_INVALID;
	invalidate_all_saveframes();
	view_draw( TRUE, FALSE ); /* 'TRUE' because we just invalidated all saveframes */

	if( allvars == TRUE ) {
		cursor = variables;
		while( cursor != NULL ) {
			cursor->user_min = new_min;
			cursor->user_max = new_max;
			cursor->have_set_range = TRUE;
			cursor = cursor->next;
			}
		}

	view_recompute_colorbar();
}

/**************************************************************************************/
	static void
set_range_labels( float min, float max )
{
	char	*units, *var_long_name;
	char	temp_label[4096], extra_label[4096];

	units = fi_var_units( view->variable->first_file->id, 
				view->variable->name );
	var_long_name = fi_long_var_name( view->variable->first_file->id, 
					view->variable->name );
	if( units == NULL ) {
		snprintf( temp_label, 4095, "displayed range: %g to %g (%g to %g shown)", 
			view->variable->global_min, 
			view->variable->global_max,
			view->variable->user_min,
			view->variable->user_max );
		if( var_long_name != NULL )
			snprintf( extra_label, 4095, "%s (%g to %g)", 
					limit_string(var_long_name),
					view->variable->user_min,
					view->variable->user_max );
		else
			snprintf( extra_label, 4095, "%s (%g to %g)", 
					limit_string(view->variable->name),
					view->variable->user_min,
					view->variable->user_max );
		}
	else
		{
		snprintf( temp_label, 4095, "displayed range: %g to %g %s (%g to %g shown)",
			view->variable->global_min,
			view->variable->global_max, 
			limit_string(units),
			view->variable->user_min,
			view->variable->user_max );
		if( var_long_name != NULL )
			snprintf( extra_label, 4095, "%s (%g to %g %s)", 
					limit_string(var_long_name),
					view->variable->user_min,
					view->variable->user_max, 
					limit_string(units) );
		else
			snprintf( extra_label, 4095, "%s (%g to %g %s)", 
					limit_string(view->variable->name),
					view->variable->user_min,
					view->variable->user_max, 
					limit_string(units) );
		}

	in_set_label( LABEL_DATA_EXTREMA, temp_label );

	if( options.want_extra_info ) {
		in_set_label( LABEL_CCINFO_1, extra_label );
		}

}

/*************************************************************************
 * Reset the current data range based on the currently showing 
 * frame ONLY, rather than the global min and maxes.
 */
	void
view_set_range_frame( void )
{
	view_draw( TRUE, TRUE );
}

/**************************************************************************************/
	void
beep()
{
	fprintf( stderr, "" );
	fflush(  stderr );
}

/**************************************************************************************/
	void
init_saveframes()
{
	long	i;
	size_t	storage_size, n_scan_entries, xsize, ysize, n_extra_frames;
	char	err_message[132];

	if( options.save_frames == FALSE )
		return;

	if( framestore.frame != NULL ) {
		free( framestore.frame );
		free( framestore.frame_valid );
		}

	if( view->scan_axis_id == -1 ) {
		n_scan_entries = 1;
		n_extra_frames = 0;
		}
	else
		{
		n_scan_entries = *(view->variable->size + view->scan_axis_id);
		n_extra_frames = floor( n_scan_entries * 0.1 ) + 1;
		if( n_extra_frames < 10 )
			n_extra_frames = 10;
		}
	framestore.nt = n_scan_entries + n_extra_frames;

	xsize = *(view->variable->size + view->x_axis_id);
	ysize = *(view->variable->size + view->y_axis_id);
	view_get_scaled_size( options.blowup, xsize, ysize, &(framestore.nx), &(framestore.ny) );

	storage_size =  framestore.nx * framestore.ny * framestore.nt;

	if( options.debug ) {
		fprintf( stderr, "initializing saveframes:\n" );
		fprintf( stderr, "	n_scan_entries: %ld\n", n_scan_entries );
		fprintf( stderr, "	n_extra_frames: %ld\n", n_extra_frames );
		fprintf( stderr, "	frame size: %ld\n", 
				*(view->variable->size + view->x_axis_id) *
				*(view->variable->size + view->y_axis_id) );
		fprintf( stderr, "	total storage size:%ld\n", storage_size );
		}

	framestore.frame = (ncv_pixel *)malloc( storage_size*sizeof( ncv_pixel ));
	if( framestore.frame == NULL ) {
		framestore.valid = FALSE;
		snprintf( err_message, 131, "Can't allocate space for frame store.\nRequested size: %.1f MB",
				(float)(storage_size*sizeof( ncv_pixel ))/1000000. );
		options.save_frames = FALSE;
		in_error( err_message );
		}
	else
		{
		framestore.valid = TRUE;
		framestore.frame_valid = (int *)(malloc( framestore.nt * sizeof( int )));
		for( i=0; i<framestore.nt; i++ )
			*(framestore.frame_valid+i) = FALSE;
		}
}

/**************************************************************************************/
	void
invalidate_all_saveframes()
{
	size_t	i;

	if( view == NULL )
		return;

	if( (view->scan_axis_id == -1) || ( framestore.valid == FALSE ))
		return;

	for( i=0L; i<framestore.nt; i++ )
		*(framestore.frame_valid+i) = FALSE;
}

/**************************************************************************************/
/* Initialize a new view structure and set defaults */
	static void
init_view( View **view, NCVar *var )
{
	int	i;

	(*view) = (View *)malloc( sizeof( View ));
	if( (*view) == NULL ) {
		fprintf( stderr, "failed on allocation of view structure\n" );
		exit( -1 );
		}
	(*view)->data         = NULL;
	(*view)->data_status  = VDS_INVALID;
	(*view)->pixels       = NULL;
	(*view)->x_axis_id    = -1;
	(*view)->y_axis_id    = -1;
	(*view)->scan_axis_id = -1;
	(*view)->skip         =  1;

	(*view)->variable  = var;
	(*view)->var_place = (size_t *)malloc(var->n_dims * sizeof( size_t ));
	for( i=0; i<var->n_dims; i++ )
		*((*view)->var_place + i) = 0;

	(*view)->plot_XY_axis   = -1;
	(*view)->plot_XY_nlines = 0;
}

/**************************************************************************************/
	static void
set_buttons( int to_state )
{
	switch (to_state ) {

	    case BUTTONS_ALL_ON:
		in_set_sensitive( BUTTON_RESTART, 	  TRUE );
		in_set_sensitive( BUTTON_REWIND, 	  TRUE );
		in_set_sensitive( BUTTON_BACKWARDS, 	  TRUE );
		in_set_sensitive( BUTTON_PAUSE, 	  TRUE );
		in_set_sensitive( BUTTON_FORWARD, 	  TRUE );
		in_set_sensitive( BUTTON_FASTFORWARD, 	  TRUE );
		in_set_sensitive( BUTTON_COLORMAP_SELECT, TRUE );
		in_set_sensitive( BUTTON_INVERT_PHYSICAL, TRUE );
		in_set_sensitive( BUTTON_INVERT_COLORMAP, TRUE );
		in_set_sensitive( BUTTON_BLOWUP, 	  TRUE );
		in_set_sensitive( BUTTON_TRANSFORM, 	  TRUE );
		in_set_sensitive( BUTTON_PRINT, 	  TRUE );
		in_set_sensitive( BUTTON_DIMSET, 	  TRUE );
		in_set_sensitive( BUTTON_RANGE, 	  TRUE );
		in_set_sensitive( BUTTON_BLOWUP_TYPE,	  TRUE );
		in_set_sensitive( BUTTON_EDIT,	  	  TRUE );
		in_set_sensitive( BUTTON_INFO,	  	  TRUE );
		break;

	    case BUTTONS_TIMEAXIS_OFF:
		in_set_sensitive( BUTTON_RESTART, 	  FALSE );
		in_set_sensitive( BUTTON_REWIND, 	  FALSE );
		in_set_sensitive( BUTTON_BACKWARDS, 	  FALSE );
		in_set_sensitive( BUTTON_FORWARD, 	  FALSE );
		in_set_sensitive( BUTTON_FASTFORWARD, 	  FALSE );
		break;
		
	    case BUTTONS_ALL_OFF:
		in_set_sensitive( BUTTON_RESTART, 	  FALSE );
		in_set_sensitive( BUTTON_REWIND, 	  FALSE );
		in_set_sensitive( BUTTON_BACKWARDS, 	  FALSE );
		in_set_sensitive( BUTTON_PAUSE, 	  FALSE );
		in_set_sensitive( BUTTON_FORWARD, 	  FALSE );
		in_set_sensitive( BUTTON_FASTFORWARD, 	  FALSE );
		in_set_sensitive( BUTTON_COLORMAP_SELECT, FALSE );
		in_set_sensitive( BUTTON_INVERT_PHYSICAL, FALSE );
		in_set_sensitive( BUTTON_INVERT_COLORMAP, FALSE );
		in_set_sensitive( BUTTON_TRANSFORM, 	  FALSE );
		in_set_sensitive( BUTTON_BLOWUP, 	  FALSE );
		in_set_sensitive( BUTTON_PRINT, 	  FALSE );
		in_set_sensitive( BUTTON_DIMSET, 	  FALSE );
		in_set_sensitive( BUTTON_RANGE, 	  FALSE );
		in_set_sensitive( BUTTON_BLOWUP_TYPE,	  FALSE );
		in_set_sensitive( BUTTON_EDIT,	  	  FALSE );
		in_set_sensitive( BUTTON_INFO,	  	  FALSE );
		break;

	default:
		fprintf( stderr, "ncview: set_buttons: unknown to_state: %d\n",
			to_state );
		break;
	}
}

/**************************************************************************************/
	static void
re_determine_scan_axes( View *new_view, NCVar *new_var, View *old_view )
{
	NCVar	*old_var;
	int	old_n_scannable_dims, i, dim_index;
	NCDim	*old_dim;

	old_var = old_view->variable;
	old_n_scannable_dims = stringlist_len( 
		fi_scannable_dims( old_var->first_file->id, old_var->name) );

	for( i=0; i<old_n_scannable_dims; i++ ) {
		old_dim = *(old_var->dim+i);

		/* the dim is set to NULL if it is not scannable */
		if( old_dim != NULL ) {

			/* dim_index is the index in the *new* variable of
		 	 * the *old* dimension
		 	 */
			dim_index = fi_dim_name_to_id( new_var->first_file->id,
					new_var->name, old_dim->name );
			if( dim_index != -1 ) {
				/* This dimension is in the new variable. 
				 * Set to be the same dimension as it used to
				 * be.
				 */

				if( i == old_view->x_axis_id )
					new_view->x_axis_id = dim_index;

				else if(  i == old_view->y_axis_id )
					new_view->y_axis_id = dim_index;

				else if(  i == old_view->scan_axis_id )
					new_view->scan_axis_id = dim_index;
				}
			}
		}
}
					
/***************************************************************************
 * Set the current place for the variable, i.e., the index into the
 * scan dimensions at which we want to view it.  It is assumed if 
 * old_view is not NULL then old_view is the view used previously to
 * the current one; in that case, try to set the new view to the same
 * place as the old view, if possible.
 */
	static void
set_scan_place( View *new_view, NCVar *var, View *old_view )
{
	/* Initially, always set to zero */
	initial_set_scan_place( new_view, var );

	/* If there is some additional information based on the
	 * old view, use that.
	 */
	if( old_view != NULL )
		re_set_scan_place( new_view, var, old_view );

	/* All place information for the displayed axes MUST be
	 * set to zero!!
	 */
	*(new_view->var_place+new_view->x_axis_id) = 0L;
	*(new_view->var_place+new_view->y_axis_id) = 0L;
}

/**************************************************************************************/
	static void
initial_set_scan_place( View *view, NCVar *var )
{
	int	i;

	for( i=0; i<var->n_dims; i++ )
		*(view->var_place+i) = 0L;
		
}

/**************************************************************************************/
	static void
re_set_scan_place( View *new_view, NCVar *new_var, View *old_view )
{
	int	i, dim_index;
	NCDim	*new_dim;
	NCVar	*old_var;
	size_t	old_place;

	old_var = old_view->variable;
	
	for( i=0; i<new_var->n_dims; i++ ) {
		/* dim_index is the dimension ID of the NEW dimension
		 * in the OLD variable.  -1 if the new dimension does
		 * not exist in the old variable.
		 */
		new_dim   = *(new_var->dim+i);
		if( new_dim != NULL ) {
			dim_index = fi_dim_name_to_id( old_var->first_file->id,
					old_var->name, new_dim->name );
			if( dim_index != -1 ) {
				old_place = *(old_view->var_place+dim_index);
				if( old_place < *(new_var->size+i) )
					*(new_view->var_place+i) = old_place;
				}
			}
		}
}

/**************************************************************************************/
/* If 'val_to_set_to' is -99999, then the blowup is calculated automatically,
 * otherwise the blowup is set to val_to_set_to
 */
	static void
calculate_blowup( View *view, NCVar *var, int val_to_set_to )
{
	size_t	x_size, y_size;
	float	fbx, fby, f_x_size, f_y_size, f_blowup;
	int	ifbx, view_var_is_valid;

	if( options.small )
		return;

	view_var_is_valid = 0;

	if( val_to_set_to != -99999 ) {
		while( options.blowup > val_to_set_to ) {
			view_change_blowup( -1, FALSE, view_var_is_valid );			
			}
		while( options.blowup < val_to_set_to ) {
			view_change_blowup( 1, FALSE, view_var_is_valid );			
			}
		return;
		}

	/* If the picture is too small, start out by blowing it up some */
	x_size = *(var->size + view->x_axis_id);
	y_size = *(var->size + view->y_axis_id);
	while( (options.blowup*x_size < options.blowup_default_size) && 
	       (options.blowup*y_size < options.blowup_default_size) ) {
		view_change_blowup( 1, FALSE, view_var_is_valid );
		}

	/* If picture is too big, reduce it some */
	f_x_size = (float)x_size;
	f_y_size = (float)y_size;
	f_blowup = (float)options.blowup;
	fbx = f_blowup * f_x_size / (double)options.blowup_default_size;
	fby = f_blowup * f_y_size / (double)options.blowup_default_size;
	fbx = (fbx > fby) ? fbx : fby;
	if( fbx > 3 ) {
		ifbx = -(int)fbx;
		view_change_blowup(ifbx,FALSE, view_var_is_valid);
		}
}

/**************************************************************************************/
	static void
draw_file_info( NCVar *var )
{
	char	*title, *units, *var_long_name;
	char	range_label[256], temp_label[256];

	title = fi_title( var->first_file->id );
	if( title == NULL )
		in_set_label( LABEL_TITLE, PROGRAM_ID );
	else
		in_set_label( LABEL_TITLE, title );

	units = fi_var_units( var->first_file->id, var->name );
	if( units == NULL ) {
		if( (var->global_min != var->user_min) || (var->global_max != var->user_max)) 
			snprintf( range_label, 255, "%g to %g (%g to %g shown)", 
					var->global_min, 
					var->global_max,
					var->user_min,
					var->user_max );
		else
			snprintf( range_label, 255, "%g to %g", 
					var->global_min,
					var->global_max );
		}
	else
		{
		if( (var->global_min != var->user_min) || (var->global_max != var->user_max))
			snprintf( range_label, 255, "%g to %g %s (%g to %g shown)", 
					var->global_min, 
					var->global_max,
					limit_string(units),
					var->user_min,
					var->user_max );
		else
			snprintf( range_label, 255, "%g to %g %s", 
					var->global_min,
					var->global_max,
					limit_string(units) );
		}
	snprintf( temp_label, 255, "displayed range: %s", range_label );
	in_set_label( LABEL_DATA_EXTREMA, temp_label );

	var_long_name = fi_long_var_name( view->variable->first_file->id, 
					view->variable->name );
	if( var_long_name == NULL ) {
		snprintf( temp_label, 255, "variable=%s", limit_string(view->variable->name) );
		in_set_label( LABEL_SCANVAR_NAME, temp_label );
		if( options.want_extra_info ) {
			snprintf( temp_label, 255, "%s (%s)", limit_string(view->variable->name), 
								range_label );
			in_set_label( LABEL_CCINFO_1, temp_label );
			}
		}
	else
		{
		snprintf( temp_label, 255, "displaying %s", limit_string(var_long_name) );
		in_set_label( LABEL_SCANVAR_NAME, temp_label );
		if( options.want_extra_info ) {
			snprintf( temp_label, 255, "%s (%s)",  limit_string(var_long_name), range_label );
			in_set_label( LABEL_CCINFO_1, temp_label );
			}
		}
}

/**************************************************************************************/
	void
redraw_dimension_info()
{
	int	i, please_flip;
	NCDim	*d, *y_dim;
	Stringlist *dimlist;
	NCVar	*var;
	char	*cur_y_name;

	var     = view->variable;
	dimlist = fi_scannable_dims( var->first_file->id, var->name );

	y_dim      = *(var->dim+view->y_axis_id);
	cur_y_name = y_dim->name;

	x_init_dim_info( dimlist );

	for( i=0; i<var->n_dims; i++ )
		if( (d = *(var->dim+i)) != NULL ) {
			please_flip = ((strcmp(d->name, cur_y_name)==0) && 
						options.invert_physical);
			in_fill_dim_info( d, please_flip ); 
			}

	show_current_dim_values( view );
	label_dimensions( view );
}

/**************************************************************************************/
	static void
show_current_dim_values( View *view )
{
	int	dimid, has_bounds;
	NCVar	*var;
	Stringlist *scannable_dims;
	size_t	place;
	double	new_dimval, bound_min, bound_max;
	char	temp_string[1024], *dim_name;
	nc_type	type;

	var = view->variable;
	scannable_dims   = fi_scannable_dims( var->first_file->id, var->name );

	while( scannable_dims != NULL ) {
		dim_name   = scannable_dims->string;
		dimid      = fi_dim_name_to_id( 
					var->first_file->id,
					var->name,
					dim_name );

		place = *(view->var_place+dimid);

		type  = fi_dim_value( view->variable, dimid, place, &new_dimval, temp_string,
			&has_bounds, &bound_min, &bound_max, view->var_place );
		if( type == NC_DOUBLE )
			snprintf( temp_string, 1023, "%lg", new_dimval );
		in_set_cur_dim_value( dim_name, temp_string );
		scannable_dims = scannable_dims->next;
		}
}

/**************************************************************************************/
	static void
label_dimensions( View *view )
{
	NCDim	*dim;
	char	*dim_name;

	if( view->x_axis_id != -1 ) {
		dim      = *(view->variable->dim+view->x_axis_id);
		dim_name = dim->name;
		in_indicate_active_dim( DIMENSION_X, dim_name );
		in_set_cur_dim_value  ( dim_name, "-X-" );
		}
	
	if( view->y_axis_id != -1 ) {
		dim      = *(view->variable->dim+view->y_axis_id);
		dim_name = dim->name;
		in_indicate_active_dim( DIMENSION_Y, dim_name );
		in_set_cur_dim_value  ( dim_name, "-Y-" );
		}

	if( view->scan_axis_id != -1 ) {
		dim      = *(view->variable->dim+view->scan_axis_id);
		dim_name = dim->name;
		in_indicate_active_dim( DIMENSION_SCAN, dim_name );
		}
}

/**************************************************************************************/
	static void
flip_if_inverted( View *view )
{
	NCDim	*y_dim;

	if( options.no_autoflip )
		return;

	if( view->y_axis_id == -1 )
		return;

	y_dim = *(view->variable->dim+view->y_axis_id);
	if( y_dim->min > y_dim->max )
		options.invert_physical = TRUE;
	else
		options.invert_physical = FALSE;

	x_force_set_invert_state( options.invert_physical );
}

/**************************************************************************************/
/* This reports the mouse location in the main (2-D color contour) window */
	void
view_report_position( int x, int y, unsigned int button_mask )
{
	size_t	data_x, data_y, x_size, y_size;
	int	type, has_bounds, i, x_is_mapped, y_is_mapped;
	float	val;
	double	new_dimval, bound_min, bound_max;
	char	current_value_label[80], temp_string[1024];
	char	xdim_str[80], ydim_str[80];
	NCDim	*xdim, *ydim;
	size_t	virt_cursor_pos[MAX_NC_DIMS];

	/* This can happen if you display a 2-d variable, then
	 * display a variable with no valid range (thereby setting
	 * the view to NULL), then roll back over the 2-d color
	 * window. Fix thanks to Matthew Bettencourt @ usm.edu */
	if( ! view )
		return;

	/* This can happen if we click on a 2-d variable, then click 
	 * on a 1-d variable, then move the pointer back over the
	 * displayed colormap of the (old) 2-d variable.
	 */
	if( view->variable->effective_dimensionality == 1 ) 
		return;

	if( view->data_status == VDS_INVALID ) {
		fill_view_data( view );
		view->data_status = VDS_VALID;
		}

	mouse_xy_to_data_xy( x, y, options.blowup, &data_x, &data_y );

	x_size = *(view->variable->size + view->x_axis_id);
	y_size = *(view->variable->size + view->y_axis_id);

	/* Make sure we don't go outside the limits */
	data_x = ( (data_x >= x_size ) ? x_size-1 : data_x );
	data_y = ( (data_y >= y_size ) ? y_size-1 : data_y );

	/* Invert Y because the reporting counts from the
	 * UPPER LEFT, not the lower left like we want
	 * it to.  If the *picture* is inverted, don't flip
	 * y!
	 */
	if( !options.invert_physical )
		data_y = y_size - data_y - 1;
	
	/* Get the value of the data field under the cursor */
	val = *((float *)view->data + data_x + data_y*x_size);

	/* Get the values of the X and Y indices. 
	* 'type' is the data type of the dimension--can be float or character 
	*/
	xdim = *(view->variable->dim + view->x_axis_id);
	ydim = *(view->variable->dim + view->y_axis_id);

	x_is_mapped = (view->variable->dim_map_info[ view->x_axis_id ] != NULL);
	y_is_mapped = (view->variable->dim_map_info[ view->y_axis_id ] != NULL);
	if( 1 || x_is_mapped || y_is_mapped ) {
		/* Get virtual position in all dims for this mouse cursor point */
		for( i=0; i<view->variable->n_dims; i++ ) 
			virt_cursor_pos[i] = *(view->var_place+i);
		virt_cursor_pos[ view->x_axis_id ] = data_x;
		virt_cursor_pos[ view->y_axis_id ] = data_y;
		}

	type = fi_dim_value( view->variable, view->x_axis_id, data_x, &new_dimval, 
			temp_string, &has_bounds, &bound_min, &bound_max, virt_cursor_pos );
	if( type == NC_DOUBLE ) {
		if( (xdim != NULL) && xdim->timelike && options.t_conv )
			fmt_time( xdim_str, 79, new_dimval, xdim, 1 );
		else
			snprintf( xdim_str, 79, "%.7lg", new_dimval );
		}
	else
		strncpy( xdim_str, temp_string, 80 );

	type = fi_dim_value( view->variable, view->y_axis_id, data_y, &new_dimval, 
			temp_string, &has_bounds, &bound_min, &bound_max, virt_cursor_pos );
	if( type == NC_DOUBLE ) {
		if( (ydim != NULL) && ydim->timelike && options.t_conv )
			fmt_time( ydim_str, 79, new_dimval, ydim, 1 );
		else
			snprintf( ydim_str, 79, "%.7lg", new_dimval );
		}
	else
		strncpy( ydim_str, temp_string, 80 );

	snprintf( current_value_label, 79, "Current: (i=%1ld, j=%1ld) %g (x=%s, y=%s)\n", 
				data_x, data_y, val, xdim_str, ydim_str );
	in_set_label( LABEL_DATA_VALUE, current_value_label );
}

/**************************************************************************************/
	static void
view_construct_scalar_coord_str( char *str, int slen ) 
{
	size_t	ts;
	FDBlist	*fdb;
	int	isc, nsc, fdb_index, space_avail;
	NCDim_map_info *sdmi;
	float	fval, fmin, fmax;
	char	*funits, tstr[1024], v1[100], v2[100];
	int	displaying_along_time_dim, nfiles;

	str[0] = '\0';

	if( (view == NULL) || (view->variable == NULL) || (view->variable->n_scalar_coords == 0) 
				|| (view->var_place == NULL)) {
		return;
		}

	/* Get the time step (value of first dim, which must be the only 
	 * dim that is virtually concatenated along different files). 
	 */
	ts = *(view->var_place);

	/* The file associated with this time step */
	fdb = view->variable->timestep_2_fdb[ts];

	/* The integer position of this FDB on the var's list of FDBs. 
	 * We need this because the scalar data is cached by FDB, *not*
	 * by the variable's timestep
	 */
	fdb_index = fdb->index;

	/* See if we are displaying ALONG the time dim. If we are, then we 
	 * check to see if the scalar coords change along the time
	 * dim. If they do, we just print <varies>. Otherwise, we print
	 * the value as per usual 
	 */
	displaying_along_time_dim = ((view->x_axis_id == 0) || (view->y_axis_id == 0));

	/* Get all scalar dims associated with this var and this file */
	nsc = view->variable->n_scalar_coords;
	for( isc=0; isc<nsc; isc++ ) {
		sdmi   = view->variable->scalar_dim_map_info[isc];
		fval   = sdmi->data_cache[fdb_index];	/* NOTE! this is NOT [timtestep], it's [fdb_number] */
		snprintf( v1, 95, "%f", fval );
		strip_trailing_zeros( v1 );
		funits = sdmi->coord_var_units;

		if( displaying_along_time_dim && (! sdmi->scalar_all_same)) {
			nfiles = view->variable->last_file->index + 1;	/* number of files this var lives in */
			fmin = view_calc_minval_float( sdmi->data_cache, nfiles );
			fmax = view_calc_maxval_float( sdmi->data_cache, nfiles );
			snprintf( v1, 95, "%f", fmin );
			strip_trailing_zeros( v1 );
			snprintf( v2, 95, "%f", fmax );
			strip_trailing_zeros( v2 );
			snprintf( tstr, 1020, "%s=%s -> %s %s", sdmi->coord_var_name, v1, v2, funits );
			}
		else
			snprintf( tstr, 1020, "%s=%s %s", sdmi->coord_var_name, v1, funits );

		if( isc != (nsc-1)) 
			strcat( tstr, "; " );

		/* Only add on this new string if there is room for it */
		space_avail = (slen-2) - strlen(str);
		if( space_avail <= 5 ) return;
		if( strlen(tstr) < space_avail )
			strcat( str, tstr );
		else
			{
			tstr[ space_avail ] = '\0';
			strcat( str, tstr );
			return;
			}
		}
}

/**************************************************************************************/
/* This reports the mouse location in the popup XY graph (the SciPlot widget) */
	void
view_report_position_vals( float xval, float yval, int plot_index )
{
	char	current_value_label[80], temp[80];
	NCDim	*dim;

	dim = plot_XY_dim[plot_index];

	/* If the X dimension is timelike, consider formatting that value */
	if( (dim != NULL) && dim->timelike && options.t_conv ) {
		fmt_time( temp, 79, xval, dim, 1 );
		snprintf( current_value_label, 79, "Current: x=%s, y=%g",
		                temp, yval );
		}
	else
		snprintf( current_value_label, 79, "Current: x=%g, y=%g", 
				xval, yval );
	in_set_label( LABEL_DATA_VALUE, current_value_label );
}

/**************************************************************************************/
	void
set_dataedit_place()
{
	size_t	data_x, data_y, orig_data_y, x_size, y_size;
	int	x, y;
	size_t	index;

	if( view->data_status == VDS_INVALID ) {
		fill_view_data( view );
		view->data_status = VDS_VALID;
		}

	in_query_pointer_position( &x, &y );
	if( (x < 0) || (y < 0) ) 
		return;

	mouse_xy_to_data_xy( x, y, options.blowup, &data_x, &data_y );

	x_size = *(view->variable->size + view->x_axis_id);
	y_size = *(view->variable->size + view->y_axis_id);

	/* Make sure we don't go outside the limits */
	data_x = ( (data_x >= x_size ) ? x_size-1 : data_x );
	data_y = ( (data_y >= y_size ) ? y_size-1 : data_y );

	orig_data_y = data_y;

	/* Invert Y because the reporting counts from the
	 * UPPER LEFT, not the lower left like we want
	 * it to.  If the *picture* is inverted, don't flip
	 * y!
	 */
	if( !options.invert_physical )
		data_y = y_size - data_y - 1;
	
	index =  data_x + orig_data_y*x_size;
	in_set_edit_place( index, data_x, orig_data_y, x_size, y_size );
}

/**************************************************************************************/
	void
set_min_from_curdata()
{
	size_t	data_x, data_y, x_size, y_size;
	int	x, y;
	float	val;

	if( view->data_status == VDS_INVALID ) {
		fill_view_data( view );
		view->data_status = VDS_VALID;
		}

	in_query_pointer_position( &x, &y );
	mouse_xy_to_data_xy( x, y, options.blowup, &data_x, &data_y );

	x_size = *(view->variable->size + view->x_axis_id);
	y_size = *(view->variable->size + view->y_axis_id);

	/* Make sure we don't go outside the limits */
	data_x = ( (data_x >= x_size ) ? x_size-1 : data_x );
	data_y = ( (data_y >= y_size ) ? y_size-1 : data_y );

	/* Invert Y because the reporting counts from the
	 * UPPER LEFT, not the lower left like we want
	 * it to.  If the *picture* is inverted, don't flip
	 * y!
	 */
	if( !options.invert_physical )
		data_y = y_size - data_y - 1;
	
	/* Get the value of the data field under the cursor */
	val = *((float *)view->data + data_x + data_y*x_size);

	view->variable->user_min = val;
	set_range_labels( val, view->variable->user_max );
	init_saveframes();
	view_draw( TRUE, FALSE ); /* 'TRUE' because we just invalidated saveframes */

	view_recompute_colorbar();
}

/**************************************************************************************/
	void
set_max_from_curdata()
{
	size_t	data_x, data_y, x_size, y_size;
	int	x, y;
	float	val;

	if( view->data_status == VDS_INVALID ) {
		fill_view_data( view );
		view->data_status = VDS_VALID;
		}

	in_query_pointer_position( &x, &y );
	mouse_xy_to_data_xy( x, y, options.blowup, &data_x, &data_y );

	x_size = *(view->variable->size + view->x_axis_id);
	y_size = *(view->variable->size + view->y_axis_id);

	/* Make sure we don't go outside the limits */
	data_x = ( (data_x >= x_size ) ? x_size-1 : data_x );
	data_y = ( (data_y >= y_size ) ? y_size-1 : data_y );

	/* Invert Y because the reporting counts from the
	 * UPPER LEFT, not the lower left like we want
	 * it to.  If the *picture* is inverted, don't flip
	 * y!
	 */
	if( !options.invert_physical )
		data_y = y_size - data_y - 1;
	
	/* Get the value of the data field under the cursor */
	val = *((float *)view->data + data_x + data_y*x_size);

	view->variable->user_max = val;
	set_range_labels( val, view->variable->user_max );
	init_saveframes();
	view_draw( TRUE, FALSE ); /* 'TRUE' because we just invalidated saveframes */

	view_recompute_colorbar();
}

/**************************************************************************************/
	void
view_data_edit( void )
{
	int	i, j, j2;
	size_t	x_size, y_size;
	char	**line_array;
	size_t	index, n_entries;
	float	val;

	x_size = *(view->variable->size + view->x_axis_id);
	y_size = *(view->variable->size + view->y_axis_id);

	n_entries  = x_size * y_size;
	line_array = (char **)malloc( sizeof(char *)*n_entries );

	index = 0L;
	for( j=0; j<y_size; j++)
	for( i=0; i<x_size; i++) {
		if( options.invert_physical )
			j2 = j;
		else
			j2 = y_size - j - 1;
		val = *((float *)view->data + i + j2*x_size);
		line_array[index] = (char *)malloc( 32 );
		snprintf( line_array[index], 31, "%-10.5g", val );
		index++;
		}
	line_array[index] = NULL;

	x_dataedit( line_array, x_size );
}

/**************************************************************************************/
	void
view_change_dat( size_t index, float new_val )
{
	size_t	x_size, y_size, scaled_x_size, scaled_y_size, x, y;

	view->data_status = VDS_EDITED;

	x_size = *(view->variable->size + view->x_axis_id);
	y_size = *(view->variable->size + view->y_axis_id);
	view_get_scaled_size( options.blowup, x_size, y_size, &scaled_x_size, &scaled_y_size );

	y = index / x_size;
	x = index - (y*x_size);
	if( !options.invert_physical )
		y = y_size - y - 1;

	printf( "changed (%3ld,%3ld) from %9f to %9f\n", x, y, 
		*((float *)view->data + x + (x_size)*y), new_val );

	*((float *)view->data + x + (x_size)*y) = new_val;
	init_saveframes();
	lockout_view_changes = TRUE;
	if( data_to_pixels( view ) < 0 ) {
		in_timer_clear();
		if( view->variable->global_min == view->variable->global_max )
			invalidate_variable( view->variable );
		return;
		}
	lockout_view_changes = FALSE;
	in_set_2d_size  ( scaled_x_size, scaled_y_size );
	in_draw_2d_field( view->pixels, scaled_x_size, scaled_y_size, 0 );
}

/**************************************************************************************/
	void
view_data_edit_dump( void )
{
	char	filename[132], *dim_name, *var_name;
	int	message, ncid, dims[2];
	size_t	x_size, y_size, start[2], count[2];
	int	x_dimid, y_dimid, varid, err;

	if( view->data_status != VDS_EDITED ) {
		fprintf( stderr, "Warning!  Data is NOT CHANGED!\n" );
		}

	strcpy( filename, "dump.data" );
	
	message = in_dialog( "Filename to dump data to:", filename, TRUE );
	if( message == MESSAGE_OK ) {
		ncid = nccreate( filename, NC_CLOBBER );

		x_size = *(view->variable->size + view->x_axis_id);
		y_size = *(view->variable->size + view->y_axis_id);

		dim_name = (*(view->variable->dim + view->x_axis_id))->name;
		x_dimid = ncdimdef( ncid, dim_name, x_size );
		dim_name = (*(view->variable->dim + view->y_axis_id))->name;
		y_dimid = ncdimdef( ncid, dim_name, y_size );

		var_name = view->variable->name;
		dims[0] = y_dimid;
		dims[1] = x_dimid;
		varid = ncvardef( ncid, var_name, NC_FLOAT, 2, dims );

		ncattput( ncid, varid, "missing_value", NC_FLOAT, 1, 
					&view->variable->fill_value );
		ncendef ( ncid );

		start[0] = 0L;
		start[1] = 0L;
		count[0] = y_size;
		count[1] = x_size;
		err = nc_put_vara_float( ncid, varid, start, count, view->data );
		if( err != NC_NOERR ) {
			fprintf( stderr, "Error writing data to new netcdf file!!\n" );
			fprintf( stderr, "%s\n", nc_strerror(err) );
			}
		ncclose( ncid );
		}
}

/**************************************************************************************/
	static void
view_data_edit_warn()
{
	int	message;

	message = in_dialog( "Warning!  Data edits will be lost unless you save them now.\nSave them now?", NULL, TRUE );
	if( message == MESSAGE_CANCEL ) 
		return;

	view_data_edit_dump();
}

/**************************************************************************************
 * Plot all the data along the plot_XY_axis for this (X,Y) position.  This is
 * the routine called when the user selects an (X,Y) position by clicking on
 * the 2-D color contour window.  
 */
	void
plot_XY()
{
	int	X_axis, i, x_window, y_window;
	size_t	*start, *count, data_x, data_y, x_size, y_size, n;

	X_axis = view->plot_XY_axis;
	if( X_axis == -1 ) {
		in_error( "Error! I have no valid axis to plot along!\n" );
		return;
		}

	if( options.debug )
		fprintf( stderr, "plot_XY: entering with axisid=%d\n", X_axis );

	in_query_pointer_position( &x_window, &y_window );
	if( (x_window < 0) || (y_window < 0) ) {
		fprintf( stderr, "OUT OF WINDOW!!\n" );
		return;
		}

	mouse_xy_to_data_xy( x_window, y_window, options.blowup, &data_x, &data_y );

	x_size = *(view->variable->size + view->x_axis_id);
	y_size = *(view->variable->size + view->y_axis_id);

	/* Make sure we don't go outside the limits */
	data_x = ( (data_x >= x_size ) ? x_size-1 : data_x );
	data_y = ( (data_y >= y_size ) ? y_size-1 : data_y );

	/* Invert Y because the reporting counts from the
	 * UPPER LEFT, not the lower left like we want
	 * it to.  If the *picture* is inverted, don't flip
	 * y!
	 */
	if( !options.invert_physical )
		data_y = y_size - data_y - 1;

	start = (size_t *)malloc(view->variable->n_dims*sizeof(size_t));
	count = (size_t *)malloc(view->variable->n_dims*sizeof(size_t));

	/* Compute start and count arrays for data to plot.  Note that
	 * the ordering of the following lines is important.  We first
	 * set to the base variable place.  We then insert the place
	 * in the window that was clicked upon.  We then set the X
	 * axis to have a full count of all elements be plotted.
	 */
        n = *(view->variable->size + X_axis);
	for( i=0; i<view->variable->n_dims; i++ ) {
		*(start+i) = *(view->var_place+i);
		*(count+i) = 1L;
		}
	*(start + view->x_axis_id) = data_x;
	*(start + view->y_axis_id) = data_y;

	/* Handle the lines on the plot.
	 */
	view->plot_XY_nlines++;
	if( view->plot_XY_nlines > MAX_LINES_PER_PLOT )
		view->plot_XY_nlines = 1;

	/* Save position so we can later replot if X axis changes
	 */
	for( i=0; i<view->variable->n_dims; i++ ) {
		view->plot_XY_position[ view->plot_XY_nlines-1 ][i] = *(start+i);
		if( options.debug ) 
			fprintf( stderr, "Setting position for line %d, dim %d: %ld\n",
				view->plot_XY_nlines-1, i, *(start+i) );
		}

	*(start+X_axis) = 0L;
	*(count+X_axis) = n;

	if( options.debug ) 
		fprintf( stderr, "plot_XY: about to call plot_XY_sc\n" );
	plot_XY_sc( start, count );

	if( options.debug )
		fprintf( stderr, "plot_XY: exiting\n" );
}

/**************************************************************************************
 * Set the axis along which to plot to the passed name.
 */
	void
view_set_XY_plot_axis( String label )
{
	int		dim_to_plot, i, j;
	size_t		*start, *count;
	char		message[1024];

	if( options.debug )
		fprintf( stderr, "entering view_set_XY_plot_axis with dim=%s\n", label );

	dim_to_plot = -1;
	for( i=0; i<view->variable->n_dims; i++ )
		if( (*(view->variable->dim + i) != NULL) &&
		    (strcmp( (*(view->variable->dim + i))->name, label) == 0) )
			dim_to_plot = i;
	
	if( dim_to_plot == -1 ) {
		snprintf( message, 1023, "Error: I can't find dimension %s in variable %s!\n",
			label, view->variable->name );
		in_error( message );
		return;
		}

	if( options.debug )
		fprintf( stderr, "view_set_XY_plot_axis: found dim to plot: %d\n", dim_to_plot );

	view->plot_XY_axis = dim_to_plot;

	start = (size_t *)malloc( sizeof(size_t)*view->variable->n_dims );
	count = (size_t *)malloc( sizeof(size_t)*view->variable->n_dims );

	unlock_plot();

	for( i=0; i<view->plot_XY_nlines; i++ ) {
		/* Change start and count to reflect new axis selection */
		for( j=0; j<view->variable->n_dims; j++ ) {
			*(start+j) = view->plot_XY_position[i][j];
			*(count+j) = 1L;
			}
		*(start+view->plot_XY_axis) = 0L;
		*(count+view->plot_XY_axis) = *(view->variable->size + view->plot_XY_axis);
		if( options.debug ) 
			fprintf( stderr, "view_set_XY_plot_axis: about to call plot_XY_sc\n" );
		plot_XY_sc( start, count );
		}
	free( start );
	free( count );
}

/**************************************************************************************
 * Plot all the data along the specified start and count
 */
	static void
plot_XY_sc( size_t *start, size_t *count )
{
	size_t	i_size;
	int	n_misplace=30, n_missing_eliminated;
	long	i, j, k, n, misplace_index[30];
	float	t_xval, t_yval, tol, *tmp_yvals;
	double	y_min, y_max, temp_double, bound_min, bound_max;
	char	x_axis_title[132], y_axis_title[132], temp2_string[128], legend[512];
	char	title[512], *file_title, temp_string[128], *dim_name, *units, *long_name;
	char	message[512];
	int	has_bounds, type, all_same, have_done_one, dim_to_plot, plot_index;
	Stringlist *dimlist;
	size_t	virt_cursor_place[MAX_NC_DIMS];

	if( options.debug ) {
		fprintf( stderr, "plot_XY_sc: entering\n" );
		for( i=0; i<view->variable->n_dims; i++ )
			fprintf( stderr, "i=%ld start=%ld count=%ld\n",
				i, *(start+i), *(count+i) );
		}

	if( options.show_sel ) {
		printf( "-var %s -start \\(", view->variable->name );
		for( i=view->variable->n_dims-1; i >= 0; i-- ) {
			printf( "%1ld", 1 + (*(start+i)) );
			if( i != 0 )
				printf( "," );
			}
		printf( "\\) -count \\(" );
		for( i=view->variable->n_dims-1; i >= 0; i-- ) {
			printf( "%1ld", *(count+i) );
			if( i != 0 )
				printf( "," );
			}
		printf( "\\) %s\n", view->variable->first_file->filename );
		}

	/* The axis we want to plot must be the one with more
	 * than one count.
	 */
	dim_to_plot = -1;
	for( i=0; i<view->variable->n_dims; i++ )
		if( *(count+i) > 1 ) {
			if( dim_to_plot != -1 ) {
				in_error( "Error!  I found more than one dimension to plot!\n" );
				return;
				}
			dim_to_plot = i;
			}
	if( dim_to_plot == -1 ) {
		in_error( "Error!  I found no dimension to plot!\n" );
		return;
		}
	dim_name = (*(view->variable->dim + dim_to_plot))->name;
		
        n = *(view->variable->size + dim_to_plot);

	if( plot_XY_xvals != NULL ) 
		free( plot_XY_xvals );
	if( options.debug ) 
		fprintf( stderr, "about to malloc %ld floats (x vals)\n", n );
	plot_XY_xvals = (double *)malloc(n*sizeof(double));
	if( plot_XY_xvals == NULL ) {
		fprintf( stderr, "malloc failed on allocation of X data for plot!\n" );
		fprintf( stderr, "requested # bytes=%ld\n", n*sizeof(double) );
		exit( -1 );
		}

	if( plot_XY_yvals != NULL )
		free( plot_XY_yvals );
	if( options.debug ) 
		fprintf( stderr, "about to malloc %ld floats (y vals)\n", n );
	plot_XY_yvals = (double *)malloc(n*sizeof(double));
	if( plot_XY_yvals == NULL ) {
		fprintf( stderr, "malloc failed on allocation of Y data for plot!\n" );
		fprintf( stderr, "requested # bytes=%ld\n", n*sizeof(double) );
		exit( -1 );
		}

	tmp_yvals = (float *)malloc(n*sizeof(float));
	if( tmp_yvals == NULL ) {
		fprintf( stderr, "malloc failed on allocation of tmp Y data for plot!\n" );
		fprintf( stderr, "requested # bytes=%ld\n", n*sizeof(float) );
		exit( -1 );
		}

	in_set_cursor_busy();

	/* Get the X values for the plot. */
	for(i_size=0L; i_size<n; i_size++) {

		for( i=0; i<view->variable->n_dims; i++ )
			virt_cursor_place[i] = *(view->var_place + i);
		virt_cursor_place[dim_to_plot] = i_size;

		type = fi_dim_value( view->variable, dim_to_plot, i_size, &temp_double, 
				temp_string, &has_bounds, &bound_min, &bound_max, virt_cursor_place );
		if( type == NC_DOUBLE ) 
			*(plot_XY_xvals+i_size) = temp_double;
		else
			*(plot_XY_xvals+i_size) = (double)i_size;
		}
	/* If there is a range of the axis of 0, commonly because
	 * the dimvar has only fill values, then the plotting widget
	 * crashes.  Hack to avoid this problem.
	 */
	if( *plot_XY_xvals == *(plot_XY_xvals+n-1) ) 
		for(i=0; i<n; i++ )
			*(plot_XY_xvals+i) = (double)i;

	/* Get the y values (values to be plotted) */
	fi_get_data( view->variable, start, count, tmp_yvals );

	/* Eliminate the missing values */
	j = 0;
	n_missing_eliminated = 0;
	tol = fabs(view->variable->fill_value * 1.e-5);
	for( i=0; i<n; i++ ) {
		t_xval = *(plot_XY_xvals+i);
		t_yval = *(tmp_yvals+i);
		if( (fabs((double)(t_yval - view->variable->fill_value)) > tol) &&
		    (t_yval < 9.e36)) {
			*(plot_XY_xvals+j) = t_xval;
			*(plot_XY_yvals+j) = (double)t_yval;
			j++;
			}
		else
			{
			n_missing_eliminated++;
			if( n_missing_eliminated < n_misplace )
				misplace_index[n_missing_eliminated-1] = i;
			}
		}
	free( tmp_yvals );
	if( n != j ) {
		printf( "Note: %ld missing values were eliminated along axis \"%s\"; index= ", 
							n-j, dim_name );
		for( k=0; k<(n_missing_eliminated<n_misplace?n_missing_eliminated:n_misplace); k++ )
			printf( " %ld", misplace_index[k]+1 );
		if( n_missing_eliminated > n_misplace )
			printf( "...\n" );
		else
			printf( "\n" );
		n = j;
		}
	if( n == 0 ) {
		in_error( "All values are missing!\n" );
		return;
		}

	/* Get the X axis title */
	strncpy( x_axis_title, (*(view->variable->dim + dim_to_plot))->name, 100 );
	units    = fi_dim_units( view->variable->first_file->id, dim_name );
	if( units != NULL ) {
		strcat( x_axis_title, " (" );
		strcat( x_axis_title, units );
		strcat( x_axis_title, ")" );
		}

	/* Another hack to fix the plotter widget...it barfs if all
	 * the Y values are the same thing.  Fix this case.
	 */
	all_same = TRUE;
	y_min    = 1.e35;
	y_max    = -1.e35;
	for( i=1; i<n; i++ ) {
		if( *(plot_XY_yvals+i) != *plot_XY_yvals ) 
			all_same = FALSE;
		if( *(plot_XY_yvals+i) < y_min ) 
			y_min = *(plot_XY_yvals+i);
		if( *(plot_XY_yvals+i) > y_max ) 
			y_max = *(plot_XY_yvals+i);
		}
	if( all_same ) {
		in_set_cursor_normal();
		snprintf( message, 511, "All values are identical: %f\n", *plot_XY_yvals ); 
		in_error( message );
		return;
		}

	/* Get the Y (which is the active variable) axis title */
	strncpy( y_axis_title, view->variable->name, 100 );
	units = fi_var_units( view->variable->first_file->id, view->variable->name );
	if( units != NULL ) {
		strcat( y_axis_title, " (" );
		strcat( y_axis_title, units );
		strcat( y_axis_title, ")" );
		}

	/* Get the overall plot title */
	if( (long_name = fi_long_var_name( view->variable->first_file->id, 
				view->variable->name )) != NULL )
		strncpy( title, long_name, 200 );
	else
		strncpy( title, view->variable->name, 200 );
	if( (file_title = fi_title( view->variable->first_file->id )) != NULL ) {
		strcat( title, " from " );
		strcat( title, file_title );
		}

	/* Make the legend */
	legend[0] = '(';
	legend[1] = '\0';
	have_done_one = FALSE;
	for( i=0; i<view->variable->n_dims; i++ ) 
		/* if( (i != dim_to_plot) && ((*(start+i) != 0) || (*(count+i) != 1))) { */
		if( (i != dim_to_plot) && (*(view->variable->dim+i) != NULL)) {
			if( have_done_one )
				strcat( legend, ", " );
			strncat( legend, (*(view->variable->dim + i))->name, 100 );
			have_done_one = TRUE;
			}
	strcat( legend, ") = (" );
	have_done_one = FALSE;
	for( i=0; i<view->variable->n_dims; i++ ) 
		if( (i != dim_to_plot) && (*(view->variable->dim+i) != NULL)) {
			if( have_done_one )
				strcat( legend, ", " );
			type = fi_dim_value( view->variable, i, *(start+i), &temp_double, temp_string,
					&has_bounds, &bound_min, &bound_max, view->var_place );
			if( type == NC_DOUBLE ) {
				snprintf( temp2_string, 127, "%lg", temp_double );
				strncat( legend, temp2_string, 100 );
				}
			else
				strncat( legend, temp_string, 100 );
			have_done_one = TRUE;
			}
	strcat( legend, ")" );

	/* Get list of scannable dimensions, which are possible axes
	 * that we could plot along.  This will be displayed in the
	 * pulldown menu of possible dimensions to plot along, so arrange
	 * it so that the current dimension we are plotting along is first.
	 */
	dimlist = NULL;
	/* Put the current dim first on the list */
	stringlist_add_string( &dimlist, (*(view->variable->dim + dim_to_plot))->name, NULL, SLTYPE_NULL );
	for( i=0; i<view->variable->n_dims; i++ )
		/* We are using here the fact that view->variable->dim was initialized
		 * so that non-scannable dims were set to NULL.  However, this can still
		 * fail in the case of the 'time' dimension, which is always included
		 * even if there is a count of only 1 along it.  Get rid of that case
		 * by making sure the size > 1.
		 */
		if( (*(view->variable->dim+i) != NULL) && (i != dim_to_plot)
		    && (*(view->variable->size+i) > 1)) 
			stringlist_add_string( &dimlist, (*(view->variable->dim + i))->name, NULL, SLTYPE_NULL );

	if( options.debug ) {
		fprintf( stderr, "about to call in_popup_XY_graph...\n" );
		fprintf( stderr, "     n     =%ld\n", n );
		fprintf( stderr, "     dim   =%i\n", dim_to_plot );
		fprintf( stderr, "     xtitle=%s\n", x_axis_title );
		fprintf( stderr, "     ytitle=%s\n", y_axis_title );
		fprintf( stderr, "     title =%s\n", title );
		}
	plot_index = in_popup_XY_graph( n, dim_to_plot, plot_XY_xvals, 
			plot_XY_yvals, x_axis_title, y_axis_title, 
			title, legend, dimlist );
	
	/* Save the X dimension for this plot, so that we can later format
	 * that dim's time values if requested (while the mouse is inside
	 * that plot's window).
	 */
	if( plot_index != -1 ) 
		plot_XY_dim[plot_index] = *(view->variable->dim + dim_to_plot);

	in_set_cursor_normal();
	if( options.debug ) 
		fprintf( stderr, "plot_XY_sc: leaving\n" );
}

/**************************************************************************************/
	void
view_plot_XY_fmt_x_val( float val, int dimindex, char *s, size_t s_len )
{
	NCDim	*dim;

	dim = *(view->variable->dim + dimindex);
	if( dim->timelike && options.t_conv )
		fmt_time( s, s_len-1, val, dim, 1 );
	else
		snprintf( s, s_len-1, "%g", val );
}

/**************************************************************************************/
	void
view_information( void )
{
	in_display_stuff( netcdf_att_string( view->variable->first_file->id,
						view->variable->name ),  
			view->variable->name );
}

/**************************************************************************************/
	static void
invalidate_variable( NCVar *var )
{
	x_set_var_sensitivity( view->variable->name, FALSE );
	set_buttons( BUTTONS_ALL_OFF );
	view = NULL;
	options.blowup = 1;
}

/**************************************************************************************/
	void
view_get_scaled_size( int blowup, size_t old_nx, size_t old_ny, size_t *new_nx, size_t *new_ny )
{
	double d_new_nx, d_new_ny, d_old_nx, d_old_ny, d_blowup, epsilon;

	if( blowup > 0 ) {
		*new_nx = blowup * old_nx;
		*new_ny = blowup * old_ny;
		return;
		}

	/* Now we know blowup < 0 */
	d_old_nx = (double)old_nx;
	d_old_ny = (double)old_ny;
	d_blowup = (double)(-blowup);
	
	d_new_nx = ceil( d_old_nx/d_blowup );
	d_new_ny = ceil( d_old_ny/d_blowup );

	epsilon = .00001;
	*new_nx = (size_t)(d_new_nx + epsilon);
	*new_ny = (size_t)(d_new_ny + epsilon);
}

/**************************************************************************************/
	void
mouse_xy_to_data_xy( int mouse_x, int mouse_y, int blowup, size_t *data_x, size_t *data_y )
{
	int b;

	if( blowup > 0 ) {
		*data_x = mouse_x / options.blowup;
		*data_y = mouse_y / options.blowup;
		return;
		}

	
	b = -blowup;
	*data_x = mouse_x * b + (int)((double)b/2.0);
	*data_y = mouse_y * b + (int)((double)b/2.0);
}

/*======================================================================================
 * Return TRUE if there is *any* missing data in the current view, and FALSE otherwise
 */
	int
view_data_has_missing( View *v )
{
	size_t 	nx, ny, i;
	float	dat;

	if( (v == NULL) || (v->variable == NULL))
		return(TRUE);

	if( v->x_axis_id < 0 ) 
		return(TRUE);
	nx = *(v->variable->size + v->x_axis_id);

	if( v->y_axis_id < 0 ) 
		ny = 1;
	else
		ny = *(v->variable->size + v->y_axis_id);

	for( i=0; i<nx*ny; i++ ) {
		dat = *((float *)(v->data) + i);
		if( close_enough( dat, v->variable->fill_value) || (dat == FILL_FLOAT)) 
			return(TRUE);
		}

	return(FALSE);
}

/***************************************************************************
 * Change the current data transformation
 */
	void
view_change_transform( int delta )
{
	options.transform += delta;
	if( options.transform > N_TRANSFORMS )
		options.transform = 1;
	if( options.transform < 1 )
		options.transform = N_TRANSFORMS;

	switch( options.transform ) {
		case TRANSFORM_NONE: in_set_label( LABEL_TRANSFORM, "Linear" ); break;
		case TRANSFORM_LOW : in_set_label( LABEL_TRANSFORM, "Low"    ); break;
		case TRANSFORM_HI  : in_set_label( LABEL_TRANSFORM, "Hi"     ); break;
		default:
			fprintf( stderr, "ncview: change_transform: unknown transform %d\n",
				options.transform );
			exit( -1 );
		}

	view_draw( TRUE, FALSE );
	view_recompute_colorbar();
}

/***************************************************************************/
void view_recompute_colorbar( void )
{
	/* The user might ask to rearrange colormaps before any
	 * variable is selected. In that event, return 
	 * immediately
	 */
	if( (view == NULL) || (view->variable == NULL))
		return;

	if( options.debug ) {
		fprintf( stderr, "view_recompute_colorbar: entering\n" );
		fprintf( stderr, "view_recompute_colorbar: about to call x_create_colorbar with user_min=%f user_max=%f transform=%d\n",
				view->variable->user_min, view->variable->user_max, options.transform );
		}

	x_create_colorbar( view->variable->user_min, view->variable->user_max, options.transform );

	if( options.debug )
		fprintf( stderr, "view_recompute_colorbar: about to call x_draw_colorbar" );
	x_draw_colorbar();

	if( options.debug )
		fprintf( stderr, "view_recompute_colorbar: exiting\n" );
}

/***************************************************************************/
	static float
view_calc_minval_float( float *arr, size_t n )
{
	float	retval=1.e30;
	size_t	i;
	
	for( i=0L; i<n; i++ )
		if( arr[i] < retval )
			retval = arr[i];
	return( retval );
}

/***************************************************************************/
	static float
view_calc_maxval_float( float *arr, size_t n )
{
	float	retval=-1.e30;
	size_t	i;
	
	for( i=0L; i<n; i++ )
		if( arr[i] > retval )
			retval = arr[i];
	return( retval );
}

/***************************************************************************/
	static void
strip_trailing_zeros( char *s )
{
	int	i;

	i = strlen(s) - 1;
	while( (i > 0 ) && (s[i] == '0' ) && (s[i-1] != '.')) 
		s[i--] = '\0';
}

