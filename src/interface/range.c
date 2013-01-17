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
	range_popup_widget = NULL,
		range_popupcanvas_widget,
			range_min_label_widget,
			range_min_text_widget,
			range_min_import_widget,
			range_min_export_widget,
			range_max_label_widget,
			range_max_text_widget,
			range_max_import_widget,
			range_max_export_widget,
			range_reset_global_widget,
			range_global_values_widget,
			range_symmetric_widget,
			range_allvars_widget,
			range_ok_widget,
			range_cancel_widget;

typedef struct {
	float	min, max;
} Min_Max_Struct;

static Min_Max_Struct global_min_max;
static int	range_popup_done = FALSE, range_popup_result;
static XEvent   range_event;

void 	range_popup_callback( Widget widget, XtPointer client_data, XtPointer call_data);
void 	range_max_export_callback( Widget widget, XtPointer client_data, XtPointer call_data);
void 	range_max_import_callback( Widget widget, XtPointer client_data, XtPointer call_data);
static void range_max_loseown_proc( Widget w, Atom *selection );
void 	range_min_export_callback( Widget widget, XtPointer client_data, XtPointer call_data);
void 	range_min_import_callback( Widget widget, XtPointer client_data, XtPointer call_data);
void 	reset_global_callback( Widget w, XtPointer client_data, XtPointer call_data);
void 	range_symmetric_callback( Widget w, XtPointer client_data, XtPointer call_data);
static void range_min_loseown_proc( Widget w, Atom *selection );
static Boolean range_min_convert_proc( Widget w, Atom *selection, Atom *target, 
			Atom *type_return, XtPointer *value_return,
			unsigned long *length_return, int *format_return );
static Boolean range_max_convert_proc( Widget w, Atom *selection, Atom *target, 
			Atom *type_return, XtPointer *value_return,
			unsigned long *length_return, int *format_return );
static void max_paste_cb( Widget w, XtPointer client_data, Atom *selection, Atom *type, 
	XtPointer value, unsigned long *length, int *format );
static void min_paste_cb( Widget w, XtPointer client_data, Atom *selection, Atom *type, 
	XtPointer value, unsigned long *length, int *format );

/*******************************************************************************/

	int
x_range( float old_min, float old_max, 
	 float global_min, float global_max,
	 float *new_min, float *new_max, 
	 int   *allvars )
{
	int	llx, lly, urx, ury;
	Boolean state;
	char	range_min_string[128], range_max_string[128], 
		global_values_string[128], *tstr;

	snprintf( range_min_string, 127, "%g", old_min );
	snprintf( range_max_string, 127, "%g", old_max );

	*new_min = old_min;
	*new_max = old_max;

	XtVaSetValues( range_min_text_widget, XtNstring, range_min_string, NULL );
	XtVaSetValues( range_max_text_widget, XtNstring, range_max_string, NULL );

	snprintf( global_values_string, 127, "%g to %g", global_min, global_max );
	global_min_max.min = global_min;
	global_min_max.max = global_max;
	XtVaSetValues( range_global_values_widget, 
			XtNlabel, global_values_string, NULL );

	x_get_window_position( &llx, &lly, &urx, &ury );
	XtVaSetValues( range_popup_widget, XtNx, llx + (urx-llx)/3, 
					   XtNy, lly + (ury-lly)/3, NULL );
	XtPopup      ( range_popup_widget, XtGrabExclusive );

	if( options.display_type == PseudoColor )
		x_set_windows_colormap_to_current( range_popup_widget );

	/**********************************************
	 * Main loop for the range popup widget
	 */
	while( ! range_popup_done ) {
		/* An event will cause range_popup_done to become TRUE */
		XtAppNextEvent( x_app_context, &range_event );
		XtDispatchEvent( &range_event );
		}
	range_popup_done = FALSE;

	/**********************************************/

	if( range_popup_result != MESSAGE_CANCEL ) {
		XtVaGetValues( range_min_text_widget, XtNstring, &tstr, NULL );
		sscanf( tstr, "%g", new_min );
		XtVaGetValues( range_max_text_widget, XtNstring, &tstr, NULL );
		sscanf( tstr, "%g", new_max );
		}
	XtPopdown( range_popup_widget );

	XtVaGetValues( range_allvars_widget, XtNstate, &state, NULL );
	if( state == True ) {
		*allvars = TRUE;
		XtVaSetValues( range_allvars_widget, XtNstate, False, NULL );
		}
	else
		*allvars = FALSE;

	return( range_popup_result );
}

	void
x_range_init()
{
	if( options.display_type == TrueColor )
		range_popup_widget = XtVaCreatePopupShell(
			"Set range",
			transientShellWidgetClass,
			topLevel,
			NULL );
	else
		range_popup_widget = XtVaCreatePopupShell(
			"Set range",
			transientShellWidgetClass,
			topLevel,
			NULL );

	range_popupcanvas_widget = XtVaCreateManagedWidget(
		"range_popupcanvas",
		formWidgetClass,
		range_popup_widget,
		XtNborderWidth, 0,
		NULL);

	range_min_label_widget = XtVaCreateManagedWidget(
		"range_min_label",	
		labelWidgetClass,	
		range_popupcanvas_widget,
		XtNlabel, "Minimum:",
		XtNwidth, 80,
		XtNborderWidth, 0,
		NULL );

	range_min_text_widget = XtVaCreateManagedWidget(
		"range_min_text",	
		asciiTextWidgetClass,	
		range_popupcanvas_widget,
		XtNeditType, XawtextEdit,
		XtNfromHoriz, range_min_label_widget,
		XtNlength, 128,
		XtNwidth, MINMAX_TEXT_WIDTH,
		NULL );

	range_min_import_widget = XtVaCreateManagedWidget(
		"Import",
		commandWidgetClass,
		range_popupcanvas_widget,
		XtNfromHoriz, range_min_text_widget,
		NULL);

        XtAddCallback( range_min_import_widget, XtNcallback, 
		range_min_import_callback, (XtPointer)NULL);

	range_min_export_widget = XtVaCreateManagedWidget(
		"Export",
		toggleWidgetClass,
		range_popupcanvas_widget,
		XtNfromHoriz, range_min_import_widget,
		NULL);

        XtAddCallback( range_min_export_widget, XtNcallback, 
		range_min_export_callback, (XtPointer)NULL);

	range_max_label_widget = XtVaCreateManagedWidget(
		"range_max_label",	
		labelWidgetClass,	
		range_popupcanvas_widget,
		XtNlabel, "Maximum:",
		XtNborderWidth, 0,
		XtNwidth, 80,
		XtNfromVert, range_min_label_widget,
		NULL );

	range_max_text_widget = XtVaCreateManagedWidget(
		"range_max_text",	
		asciiTextWidgetClass,	
		range_popupcanvas_widget,
		XtNeditType, XawtextEdit,
		XtNwidth, MINMAX_TEXT_WIDTH,
		XtNfromVert, range_min_text_widget,
		XtNfromHoriz, range_max_label_widget,
		NULL );

	range_max_import_widget = XtVaCreateManagedWidget(
		"Import",
		commandWidgetClass,
		range_popupcanvas_widget,
		XtNfromVert, range_min_import_widget,
		XtNfromHoriz, range_max_text_widget,
		NULL);

        XtAddCallback( range_max_import_widget, XtNcallback, 
		range_max_import_callback, (XtPointer)NULL);

	range_max_export_widget = XtVaCreateManagedWidget(
		"Export",
		toggleWidgetClass,
		range_popupcanvas_widget,
		XtNfromVert, range_min_export_widget,
		XtNfromHoriz, range_max_import_widget,
		NULL);

        XtAddCallback( range_max_export_widget, XtNcallback, 
		range_max_export_callback, (XtPointer)NULL);

	range_symmetric_widget = XtVaCreateManagedWidget(
		"Symmetric about Zero",
		commandWidgetClass,
		range_popupcanvas_widget,
		XtNfromVert, range_max_export_widget,
		NULL);

        XtAddCallback( range_symmetric_widget, XtNcallback, 
		range_symmetric_callback, (XtPointer)&global_min_max);

	range_reset_global_widget = XtVaCreateManagedWidget(
		"Reset to Global Values:",
		commandWidgetClass,
		range_popupcanvas_widget,
		XtNfromVert, range_symmetric_widget,
		NULL);

        XtAddCallback( range_reset_global_widget, XtNcallback, 
		reset_global_callback, (XtPointer)NULL );

	range_global_values_widget = XtVaCreateManagedWidget(
		"range_global_values",
		labelWidgetClass,
		range_popupcanvas_widget,
		XtNborderWidth, 0,
		XtNfromVert, range_symmetric_widget,
		XtNfromHoriz, range_reset_global_widget,
		XtNwidth, 200,
		NULL);

	range_allvars_widget = XtVaCreateManagedWidget(
		"Use this range for all vars",
		toggleWidgetClass,
		range_popupcanvas_widget,
		XtNfromVert, range_global_values_widget,
		NULL);

	range_ok_widget = XtVaCreateManagedWidget(
		"OK",
		commandWidgetClass,
		range_popupcanvas_widget,
		XtNfromVert, range_allvars_widget,
		NULL);

        XtAddCallback( range_ok_widget, XtNcallback, 
		range_popup_callback, (XtPointer)MESSAGE_OK);

	range_cancel_widget = XtVaCreateManagedWidget(
			"Cancel",
			commandWidgetClass,
			range_popupcanvas_widget,
			XtNfromHoriz, range_ok_widget,
			XtNfromVert, range_allvars_widget,
			NULL);

        XtAddCallback( range_cancel_widget, XtNcallback,
			range_popup_callback, (XtPointer)MESSAGE_CANCEL);
}

        void
range_popup_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
	long	l_client_data;

	l_client_data      = (long)client_data;
	range_popup_result = (int)l_client_data;
	range_popup_done   = TRUE;
}

	void
range_min_import_callback( Widget w, XtPointer client_data, XtPointer call_data)
{
	Atom	min_atom;

	min_atom = XInternAtom( XtDisplay(w), "NCVIEW_MIN_RANGE", TRUE );
	if( min_atom == None ) {
		x_error( "Nothing is exporting a minimum." );
		return;
		}
	XtGetSelectionValue( w,
		min_atom,
		XA_STRING,
		min_paste_cb,
		NULL,
		0 );
}


	void
range_max_import_callback( Widget w, XtPointer client_data, XtPointer call_data)
{
	Atom	max_atom;

	max_atom = XInternAtom( XtDisplay(w), "NCVIEW_MAX_RANGE", TRUE );
	if( max_atom == None ) {
		x_error( "Nothing is exporting a maximum." );
		return;
		}
	XtGetSelectionValue( w,
		max_atom,
		XA_STRING,
		max_paste_cb,
		NULL,
		0 );
}

	static void
min_paste_cb( Widget w, XtPointer client_data, Atom *selection, Atom *type, 
	XtPointer value, unsigned long *length, int *format )
{
	if( *length == 0 ) {
		x_error( "No valid exported minimum found.\n" );
		return;
		}
	XtVaSetValues( range_min_text_widget, XtNstring, value, NULL );
}

	static void
max_paste_cb( Widget w, XtPointer client_data, Atom *selection, Atom *type, 
	XtPointer value, unsigned long *length, int *format )
{
	if( *length == 0 ) {
		x_error( "No valid exported maximum found.\n" );
		return;
		}
	XtVaSetValues( range_max_text_widget, XtNstring, value, NULL );
}

	void
range_min_export_callback( Widget w, XtPointer client_data, XtPointer call_data)
{
	Atom	min_atom;

	min_atom = XInternAtom( XtDisplay(w), "NCVIEW_MIN_RANGE", FALSE );
	if( XtOwnSelection( w, min_atom, 0,
		range_min_convert_proc, range_min_loseown_proc,
		NULL ) == False ) {
			fprintf( stderr, "Hey! XtOwnSelection min failed!\n" );
			}
}

	void
range_symmetric_callback( Widget w, XtPointer client_data, XtPointer call_data)
{
	char	tstr[132], *sptr;
	float	new_min, new_max, cur_min, cur_max, biggest;

	XtVaGetValues( range_min_text_widget, XtNstring, &sptr, NULL );
	sscanf( sptr, "%g", &cur_min );
	XtVaGetValues( range_max_text_widget, XtNstring, &sptr, NULL );
	sscanf( sptr, "%g", &cur_max );

	if( fabs(cur_min) > fabs(cur_max) ) 
		biggest = fabs(cur_min);
	else
		biggest = fabs(cur_max);

	new_min = -1.0*biggest;
	new_max = biggest;


	snprintf( tstr, 130, "%g", new_min );
	XtVaSetValues( range_min_text_widget, XtNstring, tstr, NULL );

	snprintf( tstr, 130, "%g", new_max );
	XtVaSetValues( range_max_text_widget, XtNstring, tstr, NULL );
}

	void
reset_global_callback( Widget w, XtPointer client_data, XtPointer call_data)
{
	float	global_min, global_max;
	char	tstr[100];

	global_min = global_min_max.min;
	global_max = global_min_max.max;

	snprintf( tstr, 99, "%g", global_min );
	XtVaSetValues( range_min_text_widget, XtNstring, tstr, NULL );
	snprintf( tstr, 99, "%g", global_max );
	XtVaSetValues( range_max_text_widget, XtNstring, tstr, NULL );
}

	void
range_max_export_callback( Widget w, XtPointer client_data, XtPointer call_data)
{
	Atom	max_atom;

	max_atom = XInternAtom( XtDisplay(w), "NCVIEW_MAX_RANGE", FALSE );
	if( XtOwnSelection( w, max_atom, 0,
		range_max_convert_proc, range_max_loseown_proc,
		NULL ) == False ) {
			fprintf( stderr, "Hey! XtOwnSelection max failed!\n" );
			}
}

	static void
range_min_loseown_proc( Widget w, Atom *selection )
{
	XtVaSetValues( w, XtNstate, FALSE, NULL );
}

	static Boolean
range_min_convert_proc( Widget w, Atom *selection, Atom *target, 
	Atom *type_return, XtPointer *value_return,
	unsigned long *length_return, int *format_return )
{
	char	*widget_str, *tstr;

	XtVaGetValues( range_min_text_widget, XtNstring, &widget_str, NULL );
	tstr = XtMalloc( strlen(widget_str)+1 );
	strcpy( tstr, widget_str );
	*value_return  = tstr;
	*type_return   = XA_STRING;
	*length_return = strlen(*value_return)+1;
	*format_return = 8;
	return(True);
}


	static void
range_max_loseown_proc( Widget w, Atom *selection )
{
	XtVaSetValues( w, XtNstate, FALSE, NULL );
}

	static Boolean
range_max_convert_proc( Widget w, Atom *selection, Atom *target, 
	Atom *type_return, XtPointer *value_return,
	unsigned long *length_return, int *format_return )
{
	char	*widget_str, *tstr;

	XtVaGetValues( range_max_text_widget, XtNstring, &widget_str, NULL );
	tstr = XtMalloc( strlen(widget_str)+1 );
	strcpy( tstr, widget_str );
	*value_return  = tstr;
	*type_return   = XA_STRING;
	*length_return = strlen(*value_return)+1;
	*format_return = 8;
	return(True);
}

