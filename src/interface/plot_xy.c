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

#define DEFAULT_PLOT_LOCK_STATE True

extern Widget topLevel;
extern Options options;

static Widget
	plot_XY_popup_widget[MAX_PLOT_XY],
		plot_XY_canvas_widget[MAX_PLOT_XY],
		plot_close_button_widget[MAX_PLOT_XY],
		plot_print_button_widget[MAX_PLOT_XY],
		plot_dump_button_widget[MAX_PLOT_XY],
		plot_locked_button_widget[MAX_PLOT_XY],
		plot_XY_widget[MAX_PLOT_XY],
		plot_XY_xaxis_widget[MAX_PLOT_XY],
			plot_XY_xaxis_label[MAX_PLOT_XY],
			plot_XY_xaxis_menu[MAX_PLOT_XY],
			menu[MAX_PLOT_XY],
			menu_entry[MAX_PLOT_XY][20],
		plot_XY_log_widget[MAX_PLOT_XY],
			plot_XY_log_label_widget[MAX_PLOT_XY],
			plot_XY_xaxis_log_widget[MAX_PLOT_XY],
			plot_XY_yaxis_log_widget[MAX_PLOT_XY],
		plot_XY_range_widget[MAX_PLOT_XY],
			plot_XY_xaxis_range_widget[MAX_PLOT_XY],
			plot_XY_yaxis_range_widget[MAX_PLOT_XY];

static int	plot_XY_line[MAX_PLOT_XY][MAX_LINES_PER_PLOT],
		plot_XY_locked[MAX_PLOT_XY],
		n_lines_on_plot[MAX_PLOT_XY],
		n_menu_entries[MAX_PLOT_XY];

static int	pXY_pstyle[MAX_LINES_PER_PLOT] = {XtMARKER_DIAMOND,
						  XtMARKER_FCIRCLE,
						  XtMARKER_BOWTIE,
						  XtMARKER_DTRIANGLE,
						  XtMARKER_FLTRIANGLE},
		pXY_lstyle[MAX_LINES_PER_PLOT] = {XtLINE_SOLID,
						  XtLINE_SOLID,
						  XtLINE_DOTTED,
						  XtLINE_SOLID,
						  XtLINE_DOTTED },
		pXY_dimindex[MAX_LINES_PER_PLOT];

static char	*pXY_colorname[MAX_LINES_PER_PLOT] = {"red", "white", "blue",
					"green", "black" };

static int	n_plot_xy = 0;

static void 	plot_XY_close_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void 	plot_XY_print_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void 	plot_XY_dump_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void 	plot_XY_locked_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void 	dim_menu_sel_callback(Widget w, XtPointer client_data, XtPointer call_data);
int 		x_popup_XY_graph( long n, int dimindex, double *xvals, double *yvals, 
			char *xtitle, char *ytitle, char *title, char *legend, 
			Stringlist *scannable_dims );
static void 	xaxis_range_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void 	yaxis_range_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void 	plot_XY_xaxis_log_callback(Widget w, XtPointer client_data, XtPointer call_data);
static void 	plot_XY_yaxis_log_callback(Widget w, XtPointer client_data, XtPointer call_data);
void 		plot_XY_format_x_axis( Widget w, float val, char *s, size_t slen );
static void 	xy_track_pointer( Widget w, XtPointer client_data, XEvent *event, 
		Boolean *continue_to_dispatch );
static int 	locked_plot( void );
static int 	get_free_plot_xy_index( void );

static int last_popup_x = 18, last_popup_y = 18;

/***********************************************************************************/

	void
plot_xy_init( void )
{
	int	i;

	/* Mark the Plot-XY widgets as unused */
	for( i=0; i<MAX_PLOT_XY; i++ ) {
		plot_XY_popup_widget[i] = (Widget)(-1);
		n_lines_on_plot[i]      = 0;
		plot_XY_locked[i]       = FALSE;
		}
}

/* Returns the index number of the new popup plot */
	int
x_popup_XY_graph( long n, int dimindex, double *xvals, double *yvals, char *x_axis_title,
	char *y_axis_title, char *title, char *legend, Stringlist *scannable_dims )
{
	int	i, index, color_id;
	long	l_index;
	Stringlist *sl;

	if( options.debug )
		fprintf( stderr, "entering x_popup_XY_graph\n" );

	if( scannable_dims == NULL ) {
		in_error( "Internal error: got NULL scannable_dims in x_popup_XY_graph!" );
		exit(-1);
		}
		
	if( (index = locked_plot()) != -1 ) {
		/* Add a line to an already-existing plot */

		n_lines_on_plot[index]++;
		plot_XY_line[index][n_lines_on_plot[index]] = SciPlotListCreateFromDouble( 
				plot_XY_widget[index], n, xvals, 
				yvals, legend );
		color_id = SciPlotAllocNamedColor( 
				plot_XY_widget[index], 
				pXY_colorname[n_lines_on_plot[index]] );
		SciPlotListSetStyle( 
				plot_XY_widget[index], 
				plot_XY_line[index][n_lines_on_plot[index]],
				color_id,
				pXY_pstyle[n_lines_on_plot[index]], 
				color_id,
				pXY_lstyle[n_lines_on_plot[index]] );
		SciPlotUpdate( plot_XY_widget[index] ); 
		if( n_lines_on_plot[index] >= (MAX_LINES_PER_PLOT-1) ) {
			plot_XY_locked[index] = FALSE;
			XtVaSetValues( plot_locked_button_widget[index], XtNstate, FALSE, NULL );
			}
		}
	else
		{
		/* Create a new plot! */

		if( n_plot_xy == MAX_PLOT_XY ) {
			x_error( "Reached maximum # of XY plots!" );
			return(-1);
			}
		
		index = get_free_plot_xy_index();
		n_lines_on_plot[index] = 0;

		plot_XY_popup_widget[index] = XtVaCreatePopupShell(
			title,
			transientShellWidgetClass,
			topLevel,
			NULL );
	
		plot_XY_canvas_widget[index] = XtVaCreateManagedWidget(
			"PlotXY_canvas",
			formWidgetClass,
			plot_XY_popup_widget[index],
			XtNborderWidth, 0,
			NULL);
	
		plot_close_button_widget[index] = XtVaCreateManagedWidget(
			"Close",
			commandWidgetClass,
			plot_XY_canvas_widget[index],
			NULL);
	
		plot_print_button_widget[index] = XtVaCreateManagedWidget(
			"Print",
			commandWidgetClass,
			plot_XY_canvas_widget[index],
			XtNfromHoriz, 	plot_close_button_widget[index],
			NULL);
	
		plot_dump_button_widget[index] = XtVaCreateManagedWidget(
			"Dump",
			commandWidgetClass,
			plot_XY_canvas_widget[index],
			XtNfromHoriz, 	plot_print_button_widget[index],
			NULL);
	
       	 	plot_locked_button_widget[index] = XtVaCreateManagedWidget(
       	         	"Locked",
       	         	toggleWidgetClass,
       	         	plot_XY_canvas_widget[index],
			XtNfromHoriz, 	plot_dump_button_widget[index],
			XtNstate,	DEFAULT_PLOT_LOCK_STATE,
       		        NULL);

       	 	XtAddCallback( plot_close_button_widget[index],     XtNcallback, 
				plot_XY_close_callback, (XtPointer)MESSAGE_OK);

       	 	XtAddCallback( plot_print_button_widget[index],     XtNcallback, 
					plot_XY_print_callback, (XtPointer)MESSAGE_OK);

       	 	XtAddCallback( plot_dump_button_widget[index],     XtNcallback, 
					plot_XY_dump_callback, (XtPointer)MESSAGE_OK);

       	 	XtAddCallback( plot_locked_button_widget[index],     XtNcallback, 
				plot_XY_locked_callback, (XtPointer)MESSAGE_OK);

		plot_XY_widget[index] = XtVaCreateManagedWidget(
			"plotXY",
			sciplotWidgetClass,
			plot_XY_canvas_widget[index],
			XtNheight, 	(XtArgVal)300,
			XtNwidth, 	(XtArgVal)650,
			XtNxLabel, 	x_axis_title,
			XtNyLabel, 	y_axis_title,
       	         	XtNxLog,        False,
			XtNplotTitle,	title,
       	         	XtNtop,         XtChainTop,
       	         	XtNleft,        XtChainLeft,
			XtNshowLegend,	True,
			XtNshowTitle,	True,
       	         	XtNright,       XtChainRight,
			XtNfromVert, 	plot_close_button_widget[index],
       		        NULL);

		plot_XY_xaxis_widget[index] = XtVaCreateManagedWidget(
			"plot_XY_xaxis_widget",
			boxWidgetClass,
			plot_XY_canvas_widget[index],
			XtNorientation, XtorientHorizontal,
			XtNfromVert, plot_XY_widget[index],
			NULL );

			plot_XY_xaxis_label[index] = XtVaCreateManagedWidget(
				"plot_XY_xaxis_label",
				labelWidgetClass,
				plot_XY_xaxis_widget[index],
				XtNlabel, "X Axis:",
				XtNborderWidth, 0,
				NULL );
				
			plot_XY_xaxis_menu[index] = XtVaCreateManagedWidget(
				"plot_XY_menu",
				menuButtonWidgetClass,
				plot_XY_xaxis_widget[index],
				XtNfromHoriz, plot_XY_xaxis_label[index],
				XtNlabel, scannable_dims->string,
				NULL );

			menu[index] = XtVaCreatePopupShell(
				"menu",
				simpleMenuWidgetClass, 
				plot_XY_xaxis_menu[index], 
				NULL );

			sl = scannable_dims;
			i = 0;
			while( sl != NULL ) {
				menu_entry[index][i] = XtVaCreateManagedWidget(
					sl->string,
					smeBSBObjectClass,
					menu[index],
					NULL );
				XtAddCallback( menu_entry[index][i], XtNcallback,
					dim_menu_sel_callback, NULL );
				sl = sl->next;
				i++;
				}
			n_menu_entries[index] = i;

		plot_XY_log_widget[index] = XtVaCreateManagedWidget(
			"plot_XY_log_widget",
			boxWidgetClass,
			plot_XY_canvas_widget[index],
			XtNorientation, XtorientHorizontal,
			XtNfromVert, plot_XY_widget[index],
			XtNfromHoriz, plot_XY_xaxis_widget[index],
			NULL );

			plot_XY_log_label_widget[index] =  XtVaCreateManagedWidget(
				"Use Log:",
				labelWidgetClass,
				plot_XY_log_widget[index],
				XtNborderWidth, 0,
				NULL);

			plot_XY_xaxis_log_widget[index] =  XtVaCreateManagedWidget(
				"X",
				toggleWidgetClass,
				plot_XY_log_widget[index],
				XtNfromHoriz, plot_XY_log_label_widget[index],
				NULL);

       	 		XtAddCallback( plot_XY_xaxis_log_widget[index], XtNcallback, 
				plot_XY_xaxis_log_callback, (XtPointer)MESSAGE_OK);
		
			plot_XY_yaxis_log_widget[index] =  XtVaCreateManagedWidget(
				"Y",
				toggleWidgetClass,
				plot_XY_log_widget[index],
				XtNfromHoriz, plot_XY_xaxis_log_widget[index],
				NULL);

       	 		XtAddCallback( plot_XY_yaxis_log_widget[index], XtNcallback, 
				plot_XY_yaxis_log_callback, (XtPointer)MESSAGE_OK);

		plot_XY_range_widget[index] = XtVaCreateManagedWidget(
			"plot_XY_range_widget",
			boxWidgetClass,
			plot_XY_canvas_widget[index],
			XtNorientation, XtorientHorizontal,
			XtNfromVert, plot_XY_widget[index],
			XtNfromHoriz, plot_XY_log_widget[index],
			XtNborderWidth, 0,
			NULL );

			plot_XY_xaxis_range_widget[index] = XtVaCreateManagedWidget(
				"X Range",
				commandWidgetClass,
				plot_XY_range_widget[index],
				NULL);

			XtAddCallback( plot_XY_xaxis_range_widget[index], XtNcallback,
				xaxis_range_callback, NULL );
		
			plot_XY_yaxis_range_widget[index] = XtVaCreateManagedWidget(
				"Y Range",
				commandWidgetClass,
				plot_XY_range_widget[index],
				XtNfromHoriz, plot_XY_xaxis_range_widget[index],
				NULL);

			pXY_dimindex[index] = dimindex;
			XtAddCallback( plot_XY_yaxis_range_widget[index], XtNcallback,
				yaxis_range_callback, NULL );
		
		/*********** SciPlot stuff ****************/
		if( options.debug )
			fprintf( stderr, "x_popup_XY_graph: about to start sciplot stuff\n" );

		plot_XY_line[index][0] = SciPlotListCreateFromDouble( 
				plot_XY_widget[index], 
				n, xvals, yvals, legend );

		color_id = SciPlotAllocNamedColor( 
				plot_XY_widget[index], 
				pXY_colorname[0] );

		SciPlotListSetStyle( 
				plot_XY_widget[index], 
				plot_XY_line[index][0],
				color_id,
				pXY_pstyle[0],
				color_id,
				pXY_lstyle[0] );

		SciPlotAddXAxisCallback(
				plot_XY_widget[index], 
				&plot_XY_format_x_axis );

		if( options.debug )
			fprintf( stderr, "x_popup_XY_graph: about to sciplot update\n" );

		SciPlotUpdate( plot_XY_widget[index] ); 

		/* Move the widget to a place which does not cover the
		 * last popped-up widget and pop it up.
		 */
		XtVaSetValues( plot_XY_popup_widget[index], 
			XtNx, last_popup_x, XtNy, last_popup_y, NULL );
		if( options.debug )
			fprintf( stderr, "x_popup_XY_graph: about to popup sciplot\n" );
		XtPopup( plot_XY_popup_widget[index], XtGrabNone );

		/* Add the mouse tracking function */
		l_index = (long)index;
		XtAddEventHandler( plot_XY_widget[index],
			PointerMotionMask | ButtonPressMask,
			False,
			xy_track_pointer,
			(XtPointer)l_index );

		/* Move the popup place so the next one won't cover this one. */
		last_popup_x += 10;
		last_popup_y += 10;

		plot_XY_locked[index] = DEFAULT_PLOT_LOCK_STATE;

		n_plot_xy++;
		}
	if( options.debug )
		fprintf( stderr, "x_popup_XY_graph: leaving\n" );

	return(index);
}

	int
get_free_plot_xy_index( void )
{
	int	i;

	for( i=0; i<MAX_PLOT_XY; i++ )
		if( plot_XY_popup_widget[i] == (Widget)(-1) )
			return( i );

	fprintf( stderr, "Internal error -- can't find free XY plot index!\n" );
	exit( -1 );
	/* Make compiler error checkers happy */
	return( 0 );
}

        static void
plot_XY_locked_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
	int	i, j;

	i = 0;
	while( (plot_locked_button_widget[i] != widget) && (i < MAX_PLOT_XY) )
		i++;
	if( i == MAX_PLOT_XY ) {
		fprintf( stderr, "Internal error!  Can't find widget to change lock on!\n" );
		exit( -1 );
		}

	/* We can't deliberately lock this plot if it already
	 * has the maximum number of lines it can hold.
	 */
	if( n_lines_on_plot[i] == (MAX_LINES_PER_PLOT-1) ) {
		x_error( "You can't lock a plot which already\nhas the maximum number of plots!\n" );
		XtVaSetValues( plot_locked_button_widget[i], XtNstate, FALSE, NULL );
		return;
		}

	if( plot_XY_locked[i] == TRUE )
		plot_XY_locked[i] = FALSE;
	else
		{
		/* We can only have one locked plot at a time, so 
		 * if this one is to be locked, make sure none of
		 * the others are!
		 */
		for( j=0; j<MAX_PLOT_XY; j++ )
			plot_XY_locked[j] = FALSE;
		plot_XY_locked[i] = TRUE;
		}
}

        static void
plot_XY_dump_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
	int	i, message;
	char	filename[1024];
	FILE	*f;

	i = 0;
	while( (plot_dump_button_widget[i] != widget) && (i < MAX_PLOT_XY) )
		i++;
	if( i == MAX_PLOT_XY ) {
		fprintf( stderr, "Internal error!  Can't find widget to dump from!\n" );
		exit( -1 );
		}

	snprintf( filename, 1022, "ncview.dump" );
	message = x_dialog( "File to dump to:", filename, TRUE );
	if( message != MESSAGE_OK )
		return;

	if( (f = fopen( filename, "w" )) == NULL ) {
		x_error( "Cannot open file for writing!" );
		return;
		}

	SciPlotExportData( plot_XY_widget[i], f );
	fclose( f );
}


        static void
plot_XY_print_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
	int	i, message;
	char	filename[256];

	strcpy( filename, "ncview.ps" );
	i = 0;
	while( (plot_print_button_widget[i] != widget) && (i < MAX_PLOT_XY) )
		i++;
	if( i == MAX_PLOT_XY ) {
		fprintf( stderr, "Internal error!  Can't find widget to print from!\n" );
		exit( -1 );
		}

	message = x_dialog( "File to print to:", filename, TRUE );
	if( message != MESSAGE_OK )
		return;

	SciPlotPSCreate( plot_XY_widget[i], filename );
}

        static void
plot_XY_close_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
	int	i;

	i = 0;
	while( (plot_close_button_widget[i] != widget) && (i < MAX_PLOT_XY) )
		i++;
	if( i == MAX_PLOT_XY ) {
		fprintf( stderr, "Internal error!  Can't find widget to close XY plot!\n" );
		exit( -1 );
		}

	XtDestroyWidget( plot_XY_popup_widget[i] );

	/* Mark this spot as free */
	plot_XY_popup_widget[i] = (Widget)(-1);
	n_plot_xy--;
	n_lines_on_plot[i] = 0;
	plot_XY_locked[i]  = FALSE;
}

	void
close_all_XY_plots( void )
{
	int	i;

	for(i=0; i<n_plot_xy; i++) {
		XtDestroyWidget( plot_XY_popup_widget[i] );

		/* Mark this spot as free */
		plot_XY_popup_widget[i] = (Widget)(-1);
		n_lines_on_plot[i] = 0;
		plot_XY_locked[i]  = FALSE;
		}
	n_plot_xy = 0;
}

	void
unlock_plot( void )
{
	int	i;

	for( i=0; i<MAX_PLOT_XY; i++ )
		if( plot_XY_locked[i] == TRUE ) {
			plot_XY_locked[i] = FALSE;
			XtVaSetValues( plot_locked_button_widget[i], XtNstate, FALSE, NULL );
			}
}

	int
locked_plot( void )
{
	int	i;

	for( i=0; i<MAX_PLOT_XY; i++ )
		if( plot_XY_locked[i] == TRUE ) 
			return( i );
	
	return( -1 );
}

	static void
id_calling_menu_widget( Widget w, int *index, int *entry )
{
	int	i, j;

	for( i=0; i<n_plot_xy; i++ )
		for( j=0; j<n_menu_entries[i]; j++ )
			if( w == menu_entry[i][j] ) {
				*index = i;
				*entry = j;
				return;
				}
	x_error( "ERROR!  Didn't find matching menu entry!!\nI am probably going to crash now.  Bye." );
}

	static void 	
dim_menu_sel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	int	index=0, entry;
	String	label;

	XtVaGetValues( w, XtNlabel, &label, NULL );

	id_calling_menu_widget( w, &index, &entry );
	XtVaSetValues( plot_XY_xaxis_menu[index], XtNlabel, label, NULL );
	view_set_XY_plot_axis( label );
}

	static void 	
xaxis_range_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	int	i, result, autoscale;
	float	old_min, old_max, new_min, new_max;

	i = 0;
	while( (plot_XY_xaxis_range_widget[i] != w) && (i < MAX_PLOT_XY) )
		i++;
	if( i == MAX_PLOT_XY ) {
		fprintf( stderr, "Internal error!  Can't find widget to set X range!\n" );
		exit( -1 );
		}

	SciPlotQueryXScale (plot_XY_widget[i], &old_min, &old_max);

	autoscale = FALSE;
	result = x_plot_range( old_min, old_max, &new_min, &new_max, &autoscale );
	if( result == MESSAGE_CANCEL ) 
		return;

	if( autoscale ) 
		SciPlotSetXAutoScale( plot_XY_widget[i] );
	else
		SciPlotSetXUserScale( plot_XY_widget[i], (double)new_min, (double)new_max );

	SciPlotUpdate( plot_XY_widget[i] ); 
}

	static void 	
yaxis_range_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	int	i, result, autoscale;
	float	old_min, old_max, new_min, new_max;

	i = 0;
	while( (plot_XY_yaxis_range_widget[i] != w) && (i < MAX_PLOT_XY) )
		i++;
	if( i == MAX_PLOT_XY ) {
		fprintf( stderr, "Internal error!  Can't find widget to set X range!\n" );
		exit( -1 );
		}

	SciPlotQueryYScale (plot_XY_widget[i], &old_min, &old_max);

	autoscale = FALSE;
	result = x_plot_range( old_min, old_max, &new_min, &new_max, &autoscale );
	if( result == MESSAGE_CANCEL ) 
		return;

	if( autoscale ) 
		SciPlotSetYAutoScale( plot_XY_widget[i] );
	else
		SciPlotSetYUserScale( plot_XY_widget[i], (double)new_min, (double)new_max );

	SciPlotUpdate( plot_XY_widget[i] ); 
}

	static void 	
plot_XY_xaxis_log_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	int	i;
	Boolean	cur_state;

	i = 0;
	while( (plot_XY_xaxis_log_widget[i] != w) && (i < MAX_PLOT_XY) )
		i++;
	if( i == MAX_PLOT_XY ) {
		fprintf( stderr, "Internal error!  Can't find widget to set X axis log mode!\n" );
		exit( -1 );
		}

	XtVaGetValues( w, XtNstate, &cur_state, NULL );
	XtVaSetValues( plot_XY_widget[i], XtNxLog, cur_state, NULL );
	SciPlotUpdate( plot_XY_widget[i] ); 
}

	static void 	
plot_XY_yaxis_log_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	int	i;
	Boolean	cur_state;

	i = 0;
	while( (plot_XY_yaxis_log_widget[i] != w) && (i < MAX_PLOT_XY) )
		i++;
	if( i == MAX_PLOT_XY ) {
		fprintf( stderr, "Internal error!  Can't find widget to set Y axis log mode!\n" );
		exit( -1 );
		}

	XtVaGetValues( w, XtNstate, &cur_state, NULL );
	XtVaSetValues( plot_XY_widget[i], XtNyLog, cur_state, NULL );
	SciPlotUpdate( plot_XY_widget[i] ); 
}

	void 	
plot_XY_format_x_axis( Widget w, float val, char *s, size_t slen )
{
	int i = 0;
	while( (plot_XY_widget[i] != w) && (i < MAX_PLOT_XY) )
		i++;
	if( i == MAX_PLOT_XY ) {
		fprintf( stderr, "Internal error!  Can't find widget to set Y axis log mode!\n" );
		exit( -1 );
		}

	view_plot_XY_fmt_x_val( val, pXY_dimindex[i], s, slen );
}

	void
xy_track_pointer( Widget w, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch )
{
	Window root_return, child_window_return;
	int	root_x, root_y, win_x, win_y, index;
	unsigned int keys_and_buttons;
	float	data_x, data_y;
	long	l_index;

	l_index = (long)client_data;
	index = (int)l_index;

	if( XQueryPointer( XtDisplay(topLevel),
			XtWindow( plot_XY_widget[index] ),
			&root_return,
			&child_window_return,
			&root_x,
			&root_y,
			&win_x,
			&win_y,
			&keys_and_buttons ) ) {
		data_x = SciPlotScreenToDataX( plot_XY_widget[index], win_x );
		data_y = SciPlotScreenToDataY( plot_XY_widget[index], win_y );
		view_report_position_vals( data_x, data_y, index );
		}
}

