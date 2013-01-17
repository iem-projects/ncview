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

static Widget
	display_popup_widget[MAX_DISPLAY_POPUPS],
		display_canvas_widget[MAX_DISPLAY_POPUPS],
		display_close_button_widget[MAX_DISPLAY_POPUPS],
		display_text_widget[MAX_DISPLAY_POPUPS];

void 	display_close_callback(Widget w, XtPointer client_data, XtPointer call_data);
static  int get_free_display_popup_index( void );

extern Widget topLevel;

/******************************************************************************/

	void
x_display_info_init()
{
	int	i;

	/* Mark the widgets as unused */
	for( i=0; i<MAX_DISPLAY_POPUPS; i++ ) {
		display_popup_widget[i] = (Widget)(-1);
		}
}

	void
x_display_stuff( char *s, char *var_name )
{
	int	index;
	char	window_title[132];

	snprintf( window_title, 130, "Attributes of \"%s\"", var_name );

	index = get_free_display_popup_index();
	if( index < 0 ) 
		return;

	display_popup_widget[index] = XtVaCreatePopupShell(
		window_title,
		transientShellWidgetClass,
		topLevel,
		NULL );
	
	display_canvas_widget[index] = XtVaCreateManagedWidget(
		"display_canvas",
		formWidgetClass,
		display_popup_widget[index],
		XtNborderWidth, 0,
		NULL);
	
	display_close_button_widget[index] = XtVaCreateManagedWidget(
		"Close",
		commandWidgetClass,
		display_canvas_widget[index],
		NULL);
	
	display_text_widget[index] = XtVaCreateManagedWidget(
		"display_text",
		asciiTextWidgetClass,
		display_canvas_widget[index],
		XtNlength, strlen(s),
		XtNscrollVertical, XawtextScrollWhenNeeded,
		XtNstring, (String)s,
		XtNfromVert, display_close_button_widget[index],
		XtNwrap, XawtextWrapWord,
		XtNwidth, 500,
		XtNheight, 200,
		NULL );

       	 XtAddCallback( display_close_button_widget[index],     
			XtNcallback, 
			display_close_callback, 
			NULL );

	XtPopup( display_popup_widget[index], XtGrabNone );
}

	int
get_free_display_popup_index( void )
{
	int	i;

	for( i=0; i<MAX_DISPLAY_POPUPS; i++ )
		if(display_popup_widget[i] == (Widget)(-1) )
			return( i );

	x_error( "No more popups available!  Close a few first." );
	return( -1 );
}


        void
display_close_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
	int	i;

	i = 0;
	while( (display_close_button_widget[i] != widget) && (i < MAX_DISPLAY_POPUPS) )
		i++;
	if( i == MAX_DISPLAY_POPUPS ) {
		fprintf( stderr, "Internal error!  Can't find widget to close display popup!\n" );
		exit( -1 );
		}

	XtDestroyWidget( display_popup_widget[i] );
	display_popup_widget[i] = (Widget)(-1);
}
