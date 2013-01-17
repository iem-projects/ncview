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

/* You can 'set' the data to a specified value using the dataedit window --
 * this tells whether or not that mode is currently engaged.
 */
#define	DE_SET_MODE_OFF	0
#define	DE_SET_MODE_ON	1

static Widget
	dataedit_popup_widget = NULL,
		dataedit_popupcanvas_widget,
			dataedit_viewport_widget,
				dataedit_list_widget,
			dataedit_dump_widget,
			dataedit_done_widget,
			dataedit_set_widget,
			dataedit_set_value_widget;

static int	dataedit_popup_done = FALSE;
static int	dataedit_set_mode   = DE_SET_MODE_OFF;
static XEvent	de_event;

void 	dataedit_dump_callback(Widget w, XtPointer client_data, XtPointer call_data );
void 	dataedit_done_callback(Widget w, XtPointer client_data, XtPointer call_data );
void 	dataedit_callback(Widget w, XtPointer client_data, XtPointer call_data );
void 	dataedit_set_callback(Widget w, XtPointer client_data, XtPointer call_data );

extern Widget 		topLevel;
extern XtAppContext 	x_app_context;

/*****************************************************************/

	void
x_dataedit( char **text, int nx )
{
	dataedit_popup_widget = XtVaCreatePopupShell(
		"Data Edit Popup",
		transientShellWidgetClass,
		topLevel,
		NULL );

	dataedit_popupcanvas_widget = XtVaCreateManagedWidget(
		"dataedit_popupcanvas",
		boxWidgetClass,
		dataedit_popup_widget,
		NULL);

	dataedit_viewport_widget = XtVaCreateManagedWidget(
		"Data Viewport",
		viewportWidgetClass,
		dataedit_popupcanvas_widget,
		XtNwidth, 400,
		XtNheight, 300,
		XtNallowHoriz, True,
		XtNallowVert,  True,
		NULL );

	dataedit_list_widget = XtVaCreateManagedWidget(
		"Data Edit",
		listWidgetClass,
		dataedit_viewport_widget,
		XtNlist, text,
		XtNdefaultColumns, nx,
		XtNforceColumns, True,
		NULL );

	dataedit_dump_widget = XtVaCreateManagedWidget(
		"Dump Data",
		commandWidgetClass,
		dataedit_popupcanvas_widget,
		XtNfromVert, dataedit_list_widget,
		NULL);

	dataedit_done_widget = XtVaCreateManagedWidget(
		"Done",
		commandWidgetClass,
		dataedit_popupcanvas_widget,
		XtNfromVert, dataedit_list_widget,
		XtNfromHoriz, dataedit_dump_widget,
		NULL);

	dataedit_set_widget = XtVaCreateManagedWidget(
		"Set",
		toggleWidgetClass,
		dataedit_popupcanvas_widget,
		XtNfromVert, dataedit_list_widget,
		XtNfromHoriz, dataedit_done_widget,
		NULL);

	dataedit_set_value_widget = XtVaCreateManagedWidget(
		"dataedit_set_value",
		dialogWidgetClass,
		dataedit_popupcanvas_widget,
		XtNfromVert, dataedit_list_widget,
		XtNfromHoriz, dataedit_set_widget,
		XtNvalue, "0.0",
		XtNlabel, "Value to set to:",
		NULL);

        XtAddCallback( dataedit_dump_widget, XtNcallback, 
			dataedit_dump_callback, (XtPointer)MESSAGE_OK );
        XtAddCallback( dataedit_done_widget, XtNcallback, 
			dataedit_done_callback, (XtPointer)MESSAGE_OK );
        XtAddCallback( dataedit_set_widget, XtNcallback, 
			dataedit_set_callback, (XtPointer)MESSAGE_OK );
        XtAddCallback( dataedit_list_widget, XtNcallback, 
			dataedit_callback, (XtPointer)text );

	XtPopup      ( dataedit_popup_widget, XtGrabNone );

	/* This mini main loop just handles the dataedit popup widget */
	dataedit_popup_done = FALSE;
	while( ! dataedit_popup_done ) {
		XtAppNextEvent( x_app_context, &de_event );
		XtDispatchEvent( &de_event );
		}

	XtPopdown( dataedit_popup_widget );

	XtDestroyWidget( dataedit_dump_widget        );
	XtDestroyWidget( dataedit_done_widget        );
	XtDestroyWidget( dataedit_list_widget        );
	XtDestroyWidget( dataedit_popupcanvas_widget );
	XtDestroyWidget( dataedit_popup_widget       );
	dataedit_popup_widget = NULL;
}
	
	void
x_set_edit_place( long index, int x, int y, int nx, int ny )
{
	Widget	horiz_scroll, vert_scroll;
	float	shown_horiz, shown_vert;
	Dimension list_nx, list_ny;
	int	element_dx, element_dy, cx, cy;

	if( dataedit_popup_widget == NULL )
		return;

	XawListHighlight( dataedit_list_widget, index );
	horiz_scroll = XtNameToWidget( dataedit_viewport_widget, "horizontal" );
	vert_scroll  = XtNameToWidget( dataedit_viewport_widget, "vertical"   );
	XtVaGetValues( horiz_scroll, XtNshown,  &shown_horiz, NULL );
	XtVaGetValues( vert_scroll, XtNshown,  &shown_vert, NULL );

	XtVaGetValues( dataedit_list_widget, XtNwidth, &list_nx, XtNheight, &list_ny, NULL );
	element_dx = list_nx / nx;
	element_dy = list_ny / ny;
	cx = (x-2)*element_dx;
	cy = (y-5)*element_dy;

	XawViewportSetCoordinates( dataedit_viewport_widget, cx, cy );
}

        void
dataedit_callback(widget, client_data, call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
	XawListReturnStruct	*list;
	int			message, n_fields;
	char			line[132];
	float			new_val, dummy;
	char			**list_text;
	size_t			index;

	list_text = (char **)client_data;
	list      = XawListShowCurrent( dataedit_list_widget );

	if( dataedit_set_mode == DE_SET_MODE_ON ) {
		strcpy( line, 
			XawDialogGetValueString( dataedit_set_value_widget) );
		}
	else
		{
		strcpy( line, list->string );
		message = x_dialog( "", line, TRUE );
		if( message != MESSAGE_OK )
			return;
		}
	n_fields = sscanf( line, "%f %f", &new_val, &dummy );
	if( n_fields != 1 ) 
		return;

	index = (size_t)list->list_index;
	in_change_dat( index, new_val );
	strcpy( *(list_text+list->list_index), line );
	XawListChange( dataedit_list_widget, list_text, 0, 0, True );
	XawListHighlight( dataedit_list_widget, list->list_index );
}

        void
dataedit_dump_callback(widget, client_data, call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
	in_data_edit_dump();
}

        void
dataedit_done_callback(widget, client_data, call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
	dataedit_popup_done   = TRUE;
}

        void
dataedit_set_callback(widget, client_data, call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
	if(dataedit_set_mode == DE_SET_MODE_OFF )
		dataedit_set_mode = DE_SET_MODE_ON;
	else
		dataedit_set_mode = DE_SET_MODE_OFF;
}
