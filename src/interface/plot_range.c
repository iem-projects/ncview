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
#include <X11/Xatom.h>

#define	MINMAX_TEXT_WIDTH	100

extern 	Widget 		topLevel;
extern 	XtAppContext 	x_app_context;
extern  Options		options;

static Widget
	plot_range_popup_widget = NULL,
		plot_range_popupcanvas_widget,
			plot_range_auto_widget,
			plot_range_manual_widget,
			plot_range_min_label_widget,
			plot_range_min_text_widget,
			plot_range_max_label_widget,
			plot_range_max_text_widget,
			plot_range_ok_widget,
			plot_range_cancel_widget;

typedef struct {
	float	min, max;
} Min_Max_Struct;

static int	plot_range_popup_done = FALSE, plot_range_popup_result;
static XEvent   plot_range_event;

void 	plot_range_popup_callback( Widget widget, XtPointer client_data, XtPointer call_data);
void 	plot_range_symmetric_callback( Widget w, XtPointer client_data, XtPointer call_data);

/*******************************************************************************/

	int
x_plot_range( float old_min, float old_max, 
	 float *new_min, float *new_max,
	 int *autoscale )
{
	int	llx, lly, urx, ury;
	char	plot_range_min_string[128], plot_range_max_string[128], 
		*tstr;

	snprintf( plot_range_min_string, 127, "%g", old_min );
	snprintf( plot_range_max_string, 127, "%g", old_max );

	*new_min = old_min;
	*new_max = old_max;

	XtVaSetValues( plot_range_min_text_widget, XtNstring, plot_range_min_string, NULL );
	XtVaSetValues( plot_range_max_text_widget, XtNstring, plot_range_max_string, NULL );

	x_get_window_position( &llx, &lly, &urx, &ury );
	XtVaSetValues( plot_range_popup_widget, XtNx, llx + (urx-llx)/3, 
					   XtNy, lly + (ury-lly)/3, NULL );
	XtPopup      ( plot_range_popup_widget, XtGrabExclusive );

	if( options.display_type == PseudoColor )
		x_set_windows_colormap_to_current( plot_range_popup_widget );

	/**********************************************
	 * Main loop for the plot_range popup widget
	 */
	while( ! plot_range_popup_done ) {
		/* An event will cause plot_range_popup_done to become TRUE */
		XtAppNextEvent( x_app_context, &plot_range_event );
		XtDispatchEvent( &plot_range_event );
		}
	plot_range_popup_done = FALSE;

	/**********************************************/

	if( plot_range_popup_result != MESSAGE_CANCEL ) {
		XtVaGetValues( plot_range_min_text_widget, XtNstring, &tstr, NULL );
		sscanf( tstr, "%g", new_min );
		XtVaGetValues( plot_range_max_text_widget, XtNstring, &tstr, NULL );
		sscanf( tstr, "%g", new_max );
		}
	XtPopdown( plot_range_popup_widget );

	return( plot_range_popup_result );
}

	void
x_plot_range_init()
{
	if( options.display_type == TrueColor )
		plot_range_popup_widget = XtVaCreatePopupShell(
			"Set plot_range",
			transientShellWidgetClass,
			topLevel,
			NULL );
	else
		plot_range_popup_widget = XtVaCreatePopupShell(
			"Set plot_range",
			transientShellWidgetClass,
			topLevel,
			NULL );

	plot_range_popupcanvas_widget = XtVaCreateManagedWidget(
		"plot_range_popupcanvas",
		formWidgetClass,
		plot_range_popup_widget,
		XtNborderWidth, 0,
		NULL);

	plot_range_auto_widget = XtVaCreateManagedWidget(
		"plot_range_auto_widget",	
		commandWidgetClass,	
		plot_range_popupcanvas_widget,
		XtNlabel, "Auto",
		XtNwidth, 60,
		NULL );

	plot_range_manual_widget = XtVaCreateManagedWidget(
		"plot_range_manual_widget",	
		commandWidgetClass,	
		plot_range_popupcanvas_widget,
		XtNlabel, "Manual",
		XtNwidth, 60,
		XtNfromHoriz, plot_range_auto_widget,
		NULL );

	plot_range_min_label_widget = XtVaCreateManagedWidget(
		"plot_range_min_label",	
		labelWidgetClass,	
		plot_range_popupcanvas_widget,
		XtNlabel, "Min:",
		XtNwidth, 60,
		XtNborderWidth, 0,
		XtNfromVert, plot_range_auto_widget,
		NULL );

	plot_range_min_text_widget = XtVaCreateManagedWidget(
		"plot_range_min_text",	
		asciiTextWidgetClass,	
		plot_range_popupcanvas_widget,
		XtNeditType, XawtextEdit,
		XtNfromHoriz, plot_range_min_label_widget,
		XtNfromVert, plot_range_auto_widget,
		XtNlength, 128,
		XtNwidth, MINMAX_TEXT_WIDTH,
		NULL );

	plot_range_max_label_widget = XtVaCreateManagedWidget(
		"plot_range_max_label",	
		labelWidgetClass,	
		plot_range_popupcanvas_widget,
		XtNlabel, "Max:",
		XtNborderWidth, 0,
		XtNwidth, 60,
		XtNfromVert, plot_range_min_label_widget,
		NULL );

	plot_range_max_text_widget = XtVaCreateManagedWidget(
		"plot_range_max_text",	
		asciiTextWidgetClass,	
		plot_range_popupcanvas_widget,
		XtNeditType, XawtextEdit,
		XtNwidth, MINMAX_TEXT_WIDTH,
		XtNfromVert, plot_range_min_text_widget,
		XtNfromHoriz, plot_range_max_label_widget,
		NULL );

	plot_range_ok_widget = XtVaCreateManagedWidget(
		"OK",
		commandWidgetClass,
		plot_range_popupcanvas_widget,
		XtNfromVert, plot_range_max_text_widget,
		NULL);

        XtAddCallback( plot_range_ok_widget, XtNcallback, 
		plot_range_popup_callback, (XtPointer)MESSAGE_OK);

	plot_range_cancel_widget = XtVaCreateManagedWidget(
			"Cancel",
			commandWidgetClass,
			plot_range_popupcanvas_widget,
			XtNfromHoriz, plot_range_ok_widget,
			XtNfromVert, plot_range_max_text_widget,
			NULL);

        XtAddCallback( plot_range_cancel_widget, XtNcallback,
			plot_range_popup_callback, (XtPointer)MESSAGE_CANCEL);
}

        void
plot_range_popup_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
	long	l_client_data;

	l_client_data = (long)client_data;
	plot_range_popup_result = (int)l_client_data;
	plot_range_popup_done   = TRUE;
}

