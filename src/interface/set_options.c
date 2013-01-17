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

#include "RadioWidget.h"

#define OVERLAY_FILENAME_WIDGET_WIDTH	500

#define OVERLAY_FILENAME_LEN		1024

/* Size of the widget that holds our colormap-selection widgets */
#define CMAP_VIEWPORT_WIDTH		585
#define CMAP_VIEWPORT_HEIGHT		300

/* Following is in pixels for the colormap's name */
#define CMAP_NAME_WIDTH			80		

/* This is the little test colorbar -- how wide and tall should it be? */
#define CMAP_CBAR_HEIGHT		28


static char overlay_filename[ OVERLAY_FILENAME_LEN ];

extern Widget		topLevel;
extern XtAppContext	x_app_context;
extern Options		options;
extern Pixmap 		open_circle_pixmap, closed_circle_pixmap; /* set in x_interface.c */
extern Server_Info	server;
extern Cmaplist		*colormap_list, *current_colormap_list;

static Widget
	opt_popup_widget,
		opt_popupcanvas_widget,
			opt_overlays_box_widget,
				opt_overlays_label_widget,
				opt_overlays_filename_box,
					opt_overlays_filename_spacer,
					opt_overlays_filename_widget,
			opt_shrink_box_widget,
				opt_shrink_label_widget,
			opt_cbsel_viewport_widget,
				opt_cbsel_form_widget,		/* "cbsel" == "colorbar selection" */
				opt_cbsel_label_widget,
				opt_cbsel_cmap_all_or_none_widget,
					opt_cbsel_enable_all_widget,
					opt_cbsel_enable_none_widget,

					/* For these arrays, there are N_CLORMAPS entries */
					*opt_cbsel_cmap_entry_widget,
						*opt_cbsel_enable_widget,
						*opt_cbsel_top_widget,
						*opt_cbsel_up_widget,
						*opt_cbsel_down_widget,
						*opt_cbsel_bottom_widget,
						*opt_cbsel_name_widget,
						*opt_cbsel_cbar_widget,
			opt_action_box_widget,
				opt_ok_widget,
				opt_cancel_widget;

static XImage **cbar_ximages;

static RadioWidget overlay_widget, shrink_widget;
static Cmaplist	*setopts_cmaplist = NULL;

static int opt_popup_done = FALSE;
static long opt_popup_result;

static XEvent	opt_event;

void opt_popup_callback		( Widget widget, XtPointer client_data, XtPointer call_data);
void overlays_filename_callback ( Widget widget, XtPointer client_data, XtPointer call_data);
void overlay_set_callback 	( Widget widget, XtPointer client_data, XtPointer call_data);
void overlay_unset_callback 	( Widget widget, XtPointer client_data, XtPointer call_data);

void shrink_set_callback 	( Widget widget, XtPointer client_data, XtPointer call_data);
void shrink_unset_callback 	( Widget widget, XtPointer client_data, XtPointer call_data);

/* Order of these must match defines of SHRINK_METHOD_MEAN and SHRINK_METHOD_MODE
 * in file ../ncview.defines.h!
 */
static char *shrink_labels[] = { "Average (Mean; for continuous data)", "Most common value (Mode; for categorical data)" };

/* Local callbacks */
void cbsel_enable_callback	( Widget w, XtPointer client_data, XtPointer call_data );
void cbsel_enable_all_callback	( Widget w, XtPointer client_data, XtPointer call_data );
void cbsel_enable_none_callback	( Widget w, XtPointer client_data, XtPointer call_data );
void cbsel_top_callback   	( Widget w, XtPointer client_data, XtPointer call_data );
void cbsel_up_callback    	( Widget w, XtPointer client_data, XtPointer call_data );
void cbsel_down_callback  	( Widget w, XtPointer client_data, XtPointer call_data );
void cbsel_bottom_callback	( Widget w, XtPointer client_data, XtPointer call_data );
void expose_cbsel_cbar    	();

static void cbsel_cmap_move  	( Cmaplist **pp_cmlist, int src, int dest );
static void cmaps_set_current	( Cmaplist *cmlist );
static XImage *make_cbar_ximage	( Cmaplist *cmlist, int cmap_number );
static void draw_cbar_ximage	( int cmap_number, XImage *ximage );
static void handle_colormap_list_change();

/********************************************************************************************/
	void
set_options( void )
{
	int	llx, lly, urx, ury;
	int	err, new_overlay, new_shrink;
	Stringlist	*state_to_save;
	char	error_message[ 2040 ];
	long	l_current_overlay, l_current_shrink;

	/* Get current settings, apply them to the widgets */
	l_current_overlay = (long)(overlay_current()); 
	RadioWidget_set_current( overlay_widget, l_current_overlay );

	l_current_shrink = (long)(options.shrink_method);
	RadioWidget_set_current( shrink_widget, l_current_shrink );

	/* Make local duplicate of global colormap list.  We do this so that
	 * we can make changes only on the local list, and so discard all
	 * changes if the user clicks "Cancel". One ramification though is
	 * that this step duplicates the storage for the cmap list as well.
	 * That means that if the user cancels, we have to delete the duplicated
	 * local cmap list (setopts_cmaplist), but if the user accepts the
	 * changes, we delete the original list (colormap_list).
	 */
	setopts_cmaplist = dup_whole_cmaplist( colormap_list );  

	/* Apply information from the setopts_cmaplist to the widgets (name, etc) */
	cmaps_set_current( setopts_cmaplist );	

	/********** Pop up the window **********/
	x_get_window_position( &llx, &lly, &urx, &ury );
	XtVaSetValues( opt_popup_widget, XtNx, llx+10, XtNy, (ury+lly)/2, NULL );
	XtPopup      ( opt_popup_widget, XtGrabExclusive );

	if( options.display_type == PseudoColor )
		x_set_windows_colormap_to_current( opt_popup_widget );

	/**********************************************
	 * Main loop for the options popup widget
	 */
	while( ! opt_popup_done ) {
		/* An event will cause opt_popup_done to become TRUE */
		XtAppNextEvent( x_app_context, &opt_event );
		XtDispatchEvent( &opt_event );
		}
	opt_popup_done = FALSE;

	/**********************************************/

	XtPopdown( opt_popup_widget );


	/* Process our result, if user pressed 'OK'.  (Discard our results if
	 * user pressed 'Cancel'
	 */
	if( opt_popup_result == MESSAGE_OK ) {

		/* Do overlay */
		new_overlay = RadioWidget_query_current( overlay_widget );
		if( (new_overlay != l_current_overlay) || (new_overlay == overlay_custom_n()) ) {
			do_overlay( new_overlay, overlay_filename, FALSE );
			}
		else
			; /* printf( "no changes in overlay\n" ); */

		/* Do shrink */
		new_shrink = (int)(RadioWidget_query_current( shrink_widget ));
		if( new_shrink != l_current_shrink ) {
			options.shrink_method = new_shrink;
			invalidate_all_saveframes();
			change_view( 0, FRAMES );
			}

		/* Handle the change in the colormap list */
		handle_colormap_list_change();

		/* Save persistent options in our state file */
		state_to_save = get_persistent_state();	/* NOTE! NOT just the X state, entire state */
		if( (err = write_state_to_file( state_to_save )) != 0 ) {
			snprintf( error_message, 2000, "Error %d while trying to save options file. Look in launching window to see more information.\n", err );
			in_error( error_message );
			}
		stringlist_delete_entire_list( state_to_save );
		}

	else if( opt_popup_result == MESSAGE_CANCEL ) {

		/* Free up space from temporary colormap list the user
		 * was manipulating, they don't want it
		 */
		delete_cmaplist( setopts_cmaplist );
		}
	else
		{
		fprintf( stderr, "Error, got unknown message from options widget!\n" );
		exit(-1);
		}
}

/********************************************************************************************/
	void
handle_colormap_list_change()
{
	int		n_cm_enabled;
	Cmaplist	*cursor, *first_enabled_cm, *tmp_old_colormap_list;
	char		*colormap_name;

	/* Save pointer to OLD colormap list so we can delete it
	 */
	tmp_old_colormap_list = colormap_list;

	/* Transfer new colormap list to global one. NOTE that this is
	 * just copying the pointer, so with this step all the storage
	 * of the original colormap_list has just been orphaned. 
	 */
	colormap_list = setopts_cmaplist;

	/* Make sure at least one colormap is enabled. If none are,
	 * then enable the first one.
	 */
	first_enabled_cm = NULL;
	n_cm_enabled = colormap_list->enabled;
	if( colormap_list->enabled )
		first_enabled_cm = colormap_list;
	cursor = colormap_list;
	while( cursor->next != NULL ) {
		cursor = cursor->next;
		n_cm_enabled = n_cm_enabled + cursor->enabled;
		if( (first_enabled_cm == NULL ) && (cursor->enabled))
			first_enabled_cm = cursor;
		}
	if( n_cm_enabled == 0 ) {
		colormap_list->enabled = 1;
		first_enabled_cm = colormap_list;
		}

	/* Set our current colormap to the first enabled one */
	current_colormap_list = first_enabled_cm;

	/* Update display to reflect new colormap */
	colormap_name = x_change_colormap( 0, TRUE );
	in_set_label( LABEL_COLORMAP_NAME, colormap_name );
	view_draw( TRUE, FALSE );
	view_recompute_colorbar();

	/* Now that we are completely using the new colormap
	 * list, we can delete the original one
	 */
	delete_cmaplist( tmp_old_colormap_list );
}

/********************************************************************************************/
	void
set_options_init()
{
	int	n_colormaps;
	char	cmap_widget_name[1024];
	Widget	prev_widget;
	XImage 	*pxi;
	long	ll;

	opt_popup_widget = XtVaCreatePopupShell(
		"Set Options",
		transientShellWidgetClass,
		topLevel,
		NULL );

	opt_popupcanvas_widget = XtVaCreateManagedWidget(
		"opt_popupcanvas",
		formWidgetClass,
		opt_popup_widget,
		NULL);

	opt_overlays_box_widget = XtVaCreateManagedWidget(
		"opt_overlays_box_widget",	
		formWidgetClass,	
		opt_popupcanvas_widget,
		XtNorientation, XtorientVertical,
		NULL );

		opt_overlays_label_widget = XtVaCreateManagedWidget(
			"opt_overlays_label_widget",	
			labelWidgetClass,	
			opt_overlays_box_widget,
			XtNlabel, "Overlays:",
			XtNborderWidth, 0, 
			NULL );
	
		/* REMEMBER THIS IS **NOT** A WIDGET!! */
		overlay_widget = RadioWidget_init( 
			opt_overlays_box_widget,	/* parent */
			opt_overlays_label_widget, 	/* widget to be fromVert */
			(long)overlay_n_overlays(), 
			(long)overlay_current(), 
			overlay_names(), 
			&open_circle_pixmap, &closed_circle_pixmap,
			overlay_set_callback, overlay_unset_callback );

		opt_overlays_filename_box = XtVaCreateManagedWidget(
			"opt_overlays_filename_box",	
			boxWidgetClass,	
			opt_overlays_box_widget,
			XtNorientation, XtorientHorizontal,
			XtNfromVert, overlay_widget->widget,
			XtNborderWidth, 0, 
			NULL );

			opt_overlays_filename_spacer = XtVaCreateManagedWidget(
				"opt_overlays_filename_spacer",	
				labelWidgetClass,	
				opt_overlays_filename_box,
				XtNlabel, "     ",
				XtNborderWidth, 0, 
				NULL );

			opt_overlays_filename_widget = XtVaCreateManagedWidget(
				"opt_overlays_filename_widget",	
				commandWidgetClass,	
				opt_overlays_filename_box,
				XtNwidth, OVERLAY_FILENAME_WIDGET_WIDTH,
				XtNjustify, XtJustifyLeft,
				XtNsensitive, FALSE,
				XtNlabel, "Click to select custom overlay file",
				NULL );

		 XtAddCallback( opt_overlays_filename_widget, XtNcallback,
			overlays_filename_callback, (XtPointer)NULL );
	
	opt_shrink_box_widget = XtVaCreateManagedWidget(
		"opt_shrink_box_widget",	
		formWidgetClass,	
		opt_popupcanvas_widget,
		XtNorientation, XtorientVertical,
		XtNfromVert, opt_overlays_box_widget,
		NULL );

		opt_shrink_label_widget = XtVaCreateManagedWidget(
			"opt_shrink_label_widget",	
			labelWidgetClass,	
			opt_shrink_box_widget,
			XtNlabel, "Method to use when shrinking data:",
			XtNborderWidth, 0, 
			NULL );

		shrink_widget = RadioWidget_init( 
			opt_shrink_box_widget,		/* parent */
			opt_shrink_label_widget, 	/* widget to be fromVert */
			2L,
			0L,
			shrink_labels,
			&open_circle_pixmap, &closed_circle_pixmap,
			NULL, NULL );

	opt_cbsel_viewport_widget = XtVaCreateManagedWidget(
		"Colorbar selection viewport",
		viewportWidgetClass,
		opt_popupcanvas_widget,
		XtNfromVert, opt_shrink_box_widget,
		XtNwidth, CMAP_VIEWPORT_WIDTH,
		XtNheight, CMAP_VIEWPORT_HEIGHT,
		XtNallowHoriz, False,
		XtNallowVert,  True,
		NULL );


	opt_cbsel_form_widget = XtVaCreateManagedWidget(
		"opt_cbsel_form_widget",
		formWidgetClass,
		opt_cbsel_viewport_widget,
		NULL );

		opt_cbsel_label_widget = XtVaCreateManagedWidget(
			"opt_cbsel_label_widget",	
			labelWidgetClass,	
			opt_cbsel_form_widget,
			XtNlabel, "Select colormaps to enable, and the order to show them:",
			XtNborderWidth, 0, 
			NULL );

		opt_cbsel_cmap_all_or_none_widget = XtVaCreateManagedWidget(
			"opt_cbsel_cmap_all_or_none_widget",
			boxWidgetClass,	
			opt_cbsel_form_widget,
			XtNorientation, XtorientHorizontal,
			XtNfromVert, opt_cbsel_label_widget,
			XtNborderWidth, 1, 
			NULL );

				opt_cbsel_enable_all_widget = XtVaCreateManagedWidget(
					"opt_cbsel_enable_all_widget",
					commandWidgetClass,	
					opt_cbsel_cmap_all_or_none_widget,
					XtNlabel, "Enable All",
					XtNborderWidth, 1, 
					NULL );
				XtAddCallback( opt_cbsel_enable_all_widget, XtNcallback, cbsel_enable_all_callback, 0 );

				opt_cbsel_enable_none_widget = XtVaCreateManagedWidget(
					"opt_cbsel_enable_none_widget",
					commandWidgetClass,	
					opt_cbsel_cmap_all_or_none_widget,
					XtNlabel, "Disable All",
					XtNfromHoriz, opt_cbsel_enable_all_widget,
					XtNborderWidth, 1, 
					NULL );
				XtAddCallback( opt_cbsel_enable_none_widget, XtNcallback, cbsel_enable_none_callback, 0 );

		/* Get number of colormaps that we know about.  Note that there is a
		 * subtlety here.  We make the widgets early on, and only once.
		 * We get the number of colormaps from the GLOBAL colormap list.
		 * That means that we cannot later add to the colormap list, else
		 * there will be more on the list than the number of widgets we
		 * allocate here.
		 */
		n_colormaps = x_n_colormaps( colormap_list );
		if( n_colormaps > 9999 ) {
			fprintf( stderr, "Error, too many colormaps, max is 9999\n" );
			exit(-1);
			}

		/* Make space for all our widgets that go with each colormap */
		opt_cbsel_cmap_entry_widget = (Widget *)malloc( n_colormaps * sizeof( Widget * ));
		opt_cbsel_enable_widget     = (Widget *)malloc( n_colormaps * sizeof( Widget * ));
		opt_cbsel_top_widget        = (Widget *)malloc( n_colormaps * sizeof( Widget * ));
		opt_cbsel_up_widget         = (Widget *)malloc( n_colormaps * sizeof( Widget * ));
		opt_cbsel_down_widget       = (Widget *)malloc( n_colormaps * sizeof( Widget * ));
		opt_cbsel_bottom_widget     = (Widget *)malloc( n_colormaps * sizeof( Widget * ));
		opt_cbsel_name_widget       = (Widget *)malloc( n_colormaps * sizeof( Widget * ));
		opt_cbsel_cbar_widget       = (Widget *)malloc( n_colormaps * sizeof( Widget * ));
		cbar_ximages 		    = (XImage **)malloc( n_colormaps * sizeof( XImage * ));

		prev_widget = opt_cbsel_cmap_all_or_none_widget;

		for( ll=0; ll<n_colormaps; ll++ ) {

			snprintf( cmap_widget_name, 1000, "opt_cbsel_cmap_entry_widget_%04ld", ll );
			opt_cbsel_cmap_entry_widget[ll] = XtVaCreateManagedWidget(
				cmap_widget_name,
				boxWidgetClass,	
				opt_cbsel_form_widget,
				XtNorientation, XtorientHorizontal,
				XtNfromVert, prev_widget,
				XtNborderWidth, 0, 
				NULL );
			prev_widget = opt_cbsel_cmap_entry_widget[ll];

				snprintf( cmap_widget_name, 1000, "opt_cbsel_cmap_name_widget_%04ld", ll );
				opt_cbsel_name_widget[ll] = XtVaCreateManagedWidget(
					cmap_widget_name,
					labelWidgetClass,	
					opt_cbsel_cmap_entry_widget[ll],
					XtNlabel, "-----",
					XtNborderWidth, 0, 
					XtNwidth, CMAP_NAME_WIDTH,
					NULL );

				snprintf( cmap_widget_name, 1000, "opt_cbsel_cmap_enable_widget_%04ld", ll );
				opt_cbsel_enable_widget[ll] = XtVaCreateManagedWidget(
					cmap_widget_name,
					toggleWidgetClass,	
					opt_cbsel_cmap_entry_widget[ll],
					XtNlabel, "Enable",
					XtNborderWidth, 1, 
					XtNstate, False,
					XtNfromHoriz, opt_cbsel_name_widget[ll],
					NULL );
				XtAddCallback( opt_cbsel_enable_widget[ll], XtNcallback, cbsel_enable_callback, (XtPointer)ll );

				snprintf( cmap_widget_name, 1000, "opt_cbsel_cmap_top_widget_%04ld", ll );
				opt_cbsel_top_widget[ll] = XtVaCreateManagedWidget(
					cmap_widget_name,
					commandWidgetClass,	
					opt_cbsel_cmap_entry_widget[ll],
					XtNlabel, "Top",
					XtNfromHoriz, opt_cbsel_enable_widget[ll],
					XtNborderWidth, 1, 
					NULL );
				XtAddCallback( opt_cbsel_top_widget[ll], XtNcallback, cbsel_top_callback, (XtPointer)ll );

				snprintf( cmap_widget_name, 1000, "opt_cbsel_cmap_up_widget_%04ld", ll );
				opt_cbsel_up_widget[ll] = XtVaCreateManagedWidget(
					cmap_widget_name,
					commandWidgetClass,	
					opt_cbsel_cmap_entry_widget[ll],
					XtNlabel, "^",
					XtNfromHoriz, opt_cbsel_top_widget[ll],
					XtNborderWidth, 1, 
					NULL );
				XtAddCallback( opt_cbsel_up_widget[ll], XtNcallback, cbsel_up_callback, (XtPointer)ll );

				snprintf( cmap_widget_name, 1000, "opt_cbsel_cmap_down_widget_%04ld", ll );
				opt_cbsel_down_widget[ll] = XtVaCreateManagedWidget(
					cmap_widget_name,
					commandWidgetClass,	
					opt_cbsel_cmap_entry_widget[ll],
					XtNfromHoriz, opt_cbsel_up_widget[ll],
					XtNlabel, "v",
					XtNborderWidth, 1, 
					NULL );
				XtAddCallback( opt_cbsel_down_widget[ll], XtNcallback, cbsel_down_callback, (XtPointer)ll );

				snprintf( cmap_widget_name, 1000, "opt_cbsel_cmap_bottom_widget_%04ld", ll );
				opt_cbsel_bottom_widget[ll] = XtVaCreateManagedWidget(
					cmap_widget_name,
					commandWidgetClass,	
					opt_cbsel_cmap_entry_widget[ll],
					XtNfromHoriz, opt_cbsel_down_widget[ll],
					XtNlabel, "Bottom",
					XtNborderWidth, 1, 
					NULL );
				XtAddCallback( opt_cbsel_bottom_widget[ll], XtNcallback, cbsel_bottom_callback, (XtPointer)ll );

				snprintf( cmap_widget_name, 1000, "opt_cbsel_cmap_cbar_widget_%04ld", ll );
				opt_cbsel_cbar_widget[ll] = XtVaCreateManagedWidget(
					cmap_widget_name,
					simpleWidgetClass,
					opt_cbsel_cmap_entry_widget[ll],
					XtNfromHoriz, opt_cbsel_bottom_widget[ll],
					XtNwidth, options.n_colors,
					XtNheight, CMAP_CBAR_HEIGHT,
					NULL );

				XtAddEventHandler( opt_cbsel_cbar_widget[ll], ExposureMask, FALSE,
					(XtEventHandler)expose_cbsel_cbar, (XtPointer)ll );

				/* NOTE that the following initializes the ximages to be in the same
				 * order as the global colormap list.  I think this should be OK;
				 * it then gets modified locally so that it is always current.
				 */
				pxi = make_cbar_ximage( colormap_list, ll );	/* "pointer to XImage" */
				cbar_ximages[ll] = pxi;

			}	/* end of loop over colormaps */

	opt_action_box_widget = XtVaCreateManagedWidget(
		"opt_action_box_widget",	
		boxWidgetClass,	
		opt_popupcanvas_widget,
		XtNorientation, XtorientHorizontal,
		XtNfromVert, opt_cbsel_viewport_widget,
		NULL );
	
		opt_ok_widget = XtVaCreateManagedWidget(
			"OK",	
			commandWidgetClass,	
			opt_action_box_widget,
			NULL );
	
       	 	XtAddCallback( opt_ok_widget, XtNcallback, 
			opt_popup_callback, (XtPointer)MESSAGE_OK );
	
		opt_cancel_widget = XtVaCreateManagedWidget(
			"Cancel",	
			commandWidgetClass,	
			opt_action_box_widget,
			NULL );

       	 	XtAddCallback( opt_cancel_widget, XtNcallback, 
			opt_popup_callback, (XtPointer)MESSAGE_CANCEL );
}

/********************************************************************************************/
        void
opt_popup_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
	opt_popup_result = (long)client_data;
	opt_popup_done   = TRUE;
}

/********************************************************************************************/
        void
overlays_filename_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
	int 	message;
	char	*filename, overlay_base_dir[1024];

	determine_overlay_base_dir( overlay_base_dir, 1024 );
	message = file_select( &filename, overlay_base_dir );
	if( message == MESSAGE_OK ) {
		if( strlen(filename) >= OVERLAY_FILENAME_LEN ) {
			fprintf( stderr, "Error, length of overlay filename is too long.  Length=%ld, max=%d\n",
				strlen(filename), OVERLAY_FILENAME_LEN );
			exit(-1);
			}
		strcpy( overlay_filename, filename );
		options_set_overlay_filename( overlay_filename );
		}
	else
		overlay_filename[0] = '\0';
}

/********************************************************************************************/
	void
options_set_overlay_filename( char *fn )
{
        XtVaSetValues( opt_overlays_filename_widget, XtNlabel, fn, 
			XtNwidth, OVERLAY_FILENAME_WIDGET_WIDTH, NULL );
}

/********************************************************************************************/
        void
overlay_set_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
	long butno;

	butno = (long)client_data;

	if( butno == 4 )
		XtVaSetValues( opt_overlays_filename_widget, XtNsensitive, TRUE, NULL );
}

/********************************************************************************************/
        void
overlay_unset_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
	long butno;

	butno = (long)client_data;

	if( butno == 4 )
		XtVaSetValues( opt_overlays_filename_widget, XtNsensitive, FALSE, NULL );
}

/********************************************************************************************/
        void
shrink_set_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
	printf("entering shrink_set callback with client_data=%ld\n", (long)client_data );
}

/********************************************************************************************/
        void
shrink_unset_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
	printf("entering shrink_unset callback with client_data=%ld\n", (long)client_data );
}

/********************************************************************************************/
	void
cbsel_enable_all_callback( Widget w, XtPointer client_data, XtPointer call_data )
{
	int		i;
	Cmaplist	*cursor;

	/* Turn on the 'enabled' bit on all colormaps */
	cursor = setopts_cmaplist;
	i = 0;
	while( cursor != NULL ) {
		cursor->enabled = 1;
		cursor = cursor->next;

		/* Update widgets */
		XtVaSetValues( *(opt_cbsel_enable_widget+i),  XtNstate, True, NULL );

		i++;
		}
}

/********************************************************************************************/
	void
cbsel_enable_none_callback( Widget w, XtPointer client_data, XtPointer call_data )
{
	int		i;
	Cmaplist	*cursor;

	/* Turn off the 'enabled' bit on all colormaps */
	cursor = setopts_cmaplist;
	i = 0;
	while( cursor != NULL ) {
		cursor->enabled = 0;
		cursor = cursor->next;

		/* Update widgets */
		XtVaSetValues( *(opt_cbsel_enable_widget+i),  XtNstate, False, NULL );

		i++;
		}
}

/********************************************************************************************/
	void
cbsel_enable_callback( Widget w, XtPointer client_data, XtPointer call_data )
{
	int		i;
	long		cmap_num;
	Cmaplist	*cursor;

	cmap_num = (long)client_data;

	/* Flip the 'enabled' bit on this colormap */
	i = 0;
	cursor = setopts_cmaplist;
	while( cursor != NULL ) {
		if( i == cmap_num ) {
			cursor->enabled = 1 - cursor->enabled;
			break;
			}
		cursor = cursor->next;
		i++;
		}
}

/********************************************************************************************/
	void
cbsel_top_callback( Widget w, XtPointer client_data, XtPointer call_data )
{
	long	l_cmap_num;
	int	cmap_num;

	l_cmap_num = (long)client_data;
	cmap_num   = l_cmap_num;

	while( cmap_num > 0 ) {
		cbsel_cmap_move( &setopts_cmaplist, cmap_num, cmap_num-1 );
		cmap_num--;
		}
}

/********************************************************************************************/
	void
cbsel_up_callback( Widget w, XtPointer client_data, XtPointer call_data )
{
	long	l_cmap_num;
	int	cmap_num;

	l_cmap_num = (long)client_data;
	cmap_num = l_cmap_num;

	if( cmap_num > 0 ) 	/* can't move number 0 "up" */
		cbsel_cmap_move( &setopts_cmaplist, cmap_num, cmap_num-1 );
}

/********************************************************************************************/
	void
cbsel_down_callback( Widget w, XtPointer client_data, XtPointer call_data )
{
	long	l_cmap_num;
	int	cmap_num;

	l_cmap_num = (long)client_data;
	cmap_num = l_cmap_num;

	if( cmap_num < (x_n_colormaps(setopts_cmaplist)-1)) 	/* can't move bottom one "down" */
		cbsel_cmap_move( &setopts_cmaplist, cmap_num, cmap_num+1 );
}

/********************************************************************************************/
	void
cbsel_bottom_callback( Widget w, XtPointer client_data, XtPointer call_data )
{
	long	l_cmap_num;
	int	cmap_num;

	l_cmap_num = (long)client_data;
	cmap_num = l_cmap_num;

	while( cmap_num < (x_n_colormaps(setopts_cmaplist)-1)) {
		cbsel_cmap_move( &setopts_cmaplist, cmap_num, cmap_num+1 );
		cmap_num++;
		}
}

/********************************************************************************************
 * src is the index on the cmlist of the colormap we are moving. dest is the index
 * that the src is supposed to be moved to.
 * Note we need to be passed a pointer to a pointer to the cmaplist, since the passed
 * routine supplies what it thinks is a pointer to the head of the list. If we rearrange
 * entries, what was the head won't necessarily still be the head any more. 
 */
	static void
cbsel_cmap_move( Cmaplist **pp_cmlist, int src, int dest ) 
{
	int		n_colormaps;
	Cmaplist	**cm_ptrs, *cursor, *t_cmp, *cm_src, *cm_dest;
	int		i;
	XImage		*txi;

	/* It's actually rather annoyingly involved swapping elements on a doubly 
	 * linked list, when you take accout of the fact that they could be the
	 * head or tail of the list, might be in consecutive order, etc.
	 * My method is to make a simple list of the pointers, swap those, and
	 * then rebuild all the references. It may be slightly slower, but it 
	 * sure is a hell of a lot easier to understand and debug!!
	 */
	n_colormaps = x_n_colormaps( *pp_cmlist );
	if( (src < 0) || (src >= n_colormaps) || (dest < 0) || (dest >= n_colormaps)) {
		fprintf( stderr, "Internal error: bad src or dest, needs to be between 1 and number of colormaps\n" );
		fprintf( stderr, "src=%d dest=%d n_colormaps=%d\n", src, dest, n_colormaps );
		exit(-1);
		}
	cm_ptrs = (Cmaplist **)malloc( sizeof( Cmaplist * ) * n_colormaps );
	cursor  = *pp_cmlist;
	i = 0;
	while( cursor != NULL ) {
		cm_ptrs[i++] = cursor;
		cursor = cursor->next;
		}

	cm_src  = cm_ptrs[src];
	cm_dest = cm_ptrs[dest];

	/* Do the swap */
	t_cmp = cm_ptrs[dest];
	cm_ptrs[dest] = cm_ptrs[src];
	cm_ptrs[src ] = t_cmp;

	/* Rebuild the double links */
	i = 0;
	*pp_cmlist = cm_ptrs[0];	/* set head of the list */
	cursor  = *pp_cmlist;
	while( cursor != NULL ) {
		t_cmp = cm_ptrs[i];
		if( i == 0 )
			t_cmp->prev = NULL;
		else
			t_cmp->prev = cm_ptrs[i-1];
		if( i == (n_colormaps-1))
			t_cmp->next = NULL;
		else
			t_cmp->next = cm_ptrs[i+1];

		cursor = cursor->next;
		i++;
		}

	free( cm_ptrs );

	XtVaSetValues( *(opt_cbsel_name_widget+src),  XtNlabel, cm_dest->name, XtNwidth, CMAP_NAME_WIDTH, NULL );
	XtVaSetValues( *(opt_cbsel_name_widget+dest), XtNlabel, cm_src->name,  XtNwidth, CMAP_NAME_WIDTH, NULL );

	/* Switch the ximages (little colorbars) */
	draw_cbar_ximage( src,  cbar_ximages[dest] );
	draw_cbar_ximage( dest, cbar_ximages[src]  );
	txi                = cbar_ximages[dest];
	cbar_ximages[dest] = cbar_ximages[src];
	cbar_ximages[src ] = txi;

	/* Switch the 'Enabled' status */
	XtVaSetValues( *(opt_cbsel_enable_widget+src),  XtNstate, cm_dest->enabled, NULL );
	XtVaSetValues( *(opt_cbsel_enable_widget+dest), XtNstate, cm_src->enabled,  NULL );
}

/********************************************************************************************/
	void 
expose_cbsel_cbar( Widget w, XtPointer client_data, XExposeEvent *event, Boolean *continue_to_dispatch )
{
	int	cmap_number;
	long	l_cmap_number;

	l_cmap_number = (long)client_data;
	cmap_number = l_cmap_number;

	if( event->type != Expose ) 
		return;

	if( (event->count == 0) && (event->width > 1) && (event->height > 1))
		draw_cbar_ximage( cmap_number, cbar_ximages[cmap_number] );
}

/***************************************************************************************************/
static XImage *make_cbar_ximage( Cmaplist *cmlist, int cmap_number )
{
	Screen  *screen;
	Display *display;
	static 	XImage *ximage;
	unsigned char *tc_data;
	static unsigned char *cbar_data=NULL;
	char	*cmap_name;
	int	ii, jj, cmap_enabled;
	XColor	*color_list;

	if( cmlist == NULL )	/* Don't have our colormap list yet */
		return(NULL);

	/* Can't draw little colorbars on a pseudo color display... */
	if( options.display_type == PseudoColor )
		return(NULL);

	if( cbar_data == NULL ) {
		/* Make SAME data for each colorbar, then it gets translated to true color
		 * using each individual color map
		 */
		cbar_data = (unsigned char *)malloc( sizeof(unsigned char)*options.n_colors*CMAP_CBAR_HEIGHT );
		for( jj=0; jj<CMAP_CBAR_HEIGHT; jj++ )
		for( ii=0; ii<options.n_colors;  ii++ )
			*(cbar_data+ii+jj*options.n_colors) = (unsigned char)ii + options.n_extra_colors;
		}

	display = XtDisplay( topLevel );
	screen  = XtScreen ( topLevel );        /* Note: a pointer to Screen struct, NOT the integer screen number (Use DefaultScreen(display) for that) */
	if( display == NULL )
		return(NULL);
	if( screen == NULL )
		return(NULL);

	x_colormap_info( cmlist, cmap_number, &cmap_name, &cmap_enabled, &color_list );

	/* Make the little colorbar */
	tc_data = (unsigned char *)malloc( server.bitmap_unit*options.n_colors*CMAP_CBAR_HEIGHT );
	make_tc_data( cbar_data, options.n_colors, CMAP_CBAR_HEIGHT, color_list, tc_data );

	ximage  = XCreateImage(
		display,
		XDefaultVisualOfScreen( screen ),
		XDefaultDepthOfScreen ( screen ),
		ZPixmap,
		0,
		(char *)tc_data,
		(unsigned int)options.n_colors, (unsigned int)CMAP_CBAR_HEIGHT,
		32, 0 );

	/* NOTE: can't free tc_data here, seems to be kept in the ximage structure 
	 * and must be valid.  Guess it copies the pointer, not the data
	 */

	return( ximage );
}

/***************************************************************************************************/
static void draw_cbar_ximage( int cmap_number, XImage *ximage )
{
	Screen  *screen;
	Display *display;
	GC	gc;
	XGCValues values;

	gc = XtGetGC( opt_popup_widget, (XtGCMask)0, &values );

	/* Check to make sure everything is cool before we try to set the image;
	 * sometimes with X, events can arrive before things have been instanciated
	 */
	if( opt_cbsel_cbar_widget == NULL )
		return;
	if( opt_cbsel_cbar_widget[cmap_number] == NULL )
		return;
	if( XtWindow( opt_cbsel_cbar_widget[cmap_number] ) == (Window)NULL )
		return;

	display = XtDisplay( topLevel );
	screen  = XtScreen ( topLevel );        /* Note: a pointer to Screen struct, NOT the integer screen number (Use DefaultScreen(display) for that) */

	XPutImage(
		display,
		XtWindow( opt_cbsel_cbar_widget[cmap_number] ),
		gc,
		ximage,
		0, 0, 0, 0,
		(unsigned int)options.n_colors, (unsigned int)CMAP_CBAR_HEIGHT );
}

/********************************************************************************************************/
/* Give the LOCAL COPY of the global colormap list (which we may modify during the course of this
 * funcation, and possibly discard if the user selects "cancel" instead of "OK"), this sets the
 * widget names and colormaps to reflect that local colormap list ("cmlist").
 */
static void cmaps_set_current( Cmaplist *cmlist )
{
	int	ncmaps, ii;
	char	*cmap_name;
	int	cmap_enabled;
	XColor	*color_list;

	ncmaps = x_n_colormaps( cmlist );
	if( ncmaps == 0 ) return;

	for( ii=0; ii<ncmaps; ii++ ) {
		/* Get colormap name and whether or not it is enabled */
		x_colormap_info( cmlist, ii, &cmap_name, &cmap_enabled, &color_list );
		XtVaSetValues( opt_cbsel_name_widget[ii], 
			XtNlabel, cmap_name, 
			XtNwidth, CMAP_NAME_WIDTH,
			NULL );

		/* Set the little colorbar image to the correct ximage */
		draw_cbar_ximage( ii, cbar_ximages[ii] );

		/* Set whether or not this colormap is enabled */
		XtVaSetValues( opt_cbsel_enable_widget[ii], 
			XtNstate, cmap_enabled, 
			NULL );
		}
}

/******************************************************************************************************
 * We save persistent options in a stringlist, which gets written out to our .rc file. This
 * routine generates the stringlist to save from the point of view of the X interface
 * routines. I.e., it generates a stringlist with all the options that the X interface routines
 * would like to save.
 * 
 * This generates and allocates space for a new stringlist, so when the calling routine
 * is done with the stringlist (by saving it to a file, or example), then the calling 
 * routine must delete the stringlist as so:
 *
 *	state_to_save = get_persistent_X_state();
 * 	stringlist_delete_entire_list( state_to_save );
 */
 	Stringlist *
get_persistent_X_state()
{
	Stringlist 	*state_to_save = NULL;	/* signal we want to create new stringlist */
	int		err;
	char	error_message[ 2040 ];

	if( (err = colormap_options_to_stringlist( &state_to_save )) != 0 ) {
		snprintf( error_message, 2000, "Error %d while trying to save colormap options. Look in launching window to see more information.\n", err );
		in_error( error_message );
		}

	return( state_to_save );
}
