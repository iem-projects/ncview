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

/* 
Implements a standard group of radio buttons.  Why doesn't Xaw include this?? 

David W. Pierce
1-20-2006
*/

#include "../ncview.includes.h"
#include "../ncview.defines.h"
#include "../ncview.protos.h"

#include "RadioWidget.h"

static void  	button_click_callback(Widget w, XtPointer client_data, XtPointer call_data );
static void  	set_button_on_inner ( RadioWidget rw, long butnum );
static void  	set_button_off_inner( RadioWidget rw, long butnum );

/*
static void 	RadioWidget_dump( RadioWidget rw );
*/

/**************************************************************************************************************/
RadioWidget RadioWidget_init( Widget parent, Widget widget_fromvert, 
		long nbut, long first_on, char **labels, 
		Pixmap *pixmap_off, Pixmap *pixmap_on,
		XtCallbackProc set_callback, XtCallbackProc unset_callback )
{
	long 		i;
	Widget 		hbox_widget, label_widget;
	RadioWidget 	retval;

	retval = (RadioWidget)malloc( sizeof(RadioWidgetStruct) );

	if( nbut == 0 ) {
		fprintf( stderr, "Error, RadioWidget_init called with zero buttons!\n" );
		exit(-1);
		}
	retval->nbut = nbut;

	if( (first_on < 0) || first_on >= nbut ) {
		fprintf( stderr, "Error, RadioWidget_init called with bad value for first_on: %ld\n", first_on );
		exit(-1);
		}

	if( labels == NULL ) {
		fprintf( stderr, "Error, RadioWidget_init called with labels == NULL!\n" );
		exit(-1);
		}
	for( i=0; i<nbut; i++ ) {
		if( labels[i] == NULL ) {
			fprintf( stderr, "Error, RadioWidget_init called with labels[%ld] == NULL!\n", i );
			exit(-1);
			}
		}

	retval->button_widget  = (Widget *)malloc( nbut * sizeof(Widget));
	retval->set_callback   = set_callback;
	retval->unset_callback = unset_callback;

	retval->pixmap_on  = pixmap_on;
	retval->pixmap_off = pixmap_off;

	/* Make the box that holds us 
	 */
	retval->widget = XtVaCreateManagedWidget(
		"RadioWidget",	
		boxWidgetClass,	
		parent,
		XtNorientation, XtorientVertical,
		XtNfromVert, widget_fromvert,
		XtNborderWidth, 0,
		NULL );

	for( i=0; i<nbut; i++ ) {

		hbox_widget = XtVaCreateManagedWidget(
			"hbox",
			boxWidgetClass,
			retval->widget,
			XtNorientation, XtorientHorizontal,
			XtNborderWidth, 0,
			XtNinternalHeight, 0,
			XtNvSpace, 0,
			NULL );

			retval->button_widget[i] = XtVaCreateManagedWidget(
				"radioButton",
				commandWidgetClass,
				hbox_widget,
				/* XtNlabel, "X", */
				XtNborderWidth, 0,
				XtNbitmap, *pixmap_off, 
				XtNinternalHeight, 0,
				NULL );

			XtAddCallback( retval->button_widget[i], XtNcallback, button_click_callback, (XtPointer)retval );

			label_widget = XtVaCreateManagedWidget(
				"label",
				labelWidgetClass,
				hbox_widget,
				XtNlabel, labels[i],
				XtNborderWidth, 0,
				XtNinternalHeight, 0,
				NULL );
		}

	set_button_on_inner( retval, first_on );
	retval->current_set = first_on;

	return( retval );
}

/**************************************************************************************************************/
void set_button_off_inner( RadioWidget rw, long butnum )
{
	XtVaSetValues( rw->button_widget[butnum], XtNbitmap, *(rw->pixmap_off), NULL );
}

/**************************************************************************************************************/
void set_button_on_inner( RadioWidget rw, long butnum )
{
	XtVaSetValues( rw->button_widget[butnum], XtNbitmap, *(rw->pixmap_on), NULL );
}

/**************************************************************************************************************/
void button_click_callback( Widget w, XtPointer client_data, XtPointer call_data )
{
	long 		i, butno;
	RadioWidget 	rw;

	rw = (RadioWidget)client_data;

	/* Find which button was pressed 
	 */
	butno = -1;
	for( i=0; i<rw->nbut; i++ )
		if( w == rw->button_widget[i] ) {
			butno = i;
			break;
			}

	if( butno == -1 ) {
		fprintf( stderr, "Error, could not find which radio button was pushed!\n" );
		exit( -1 );
		}

	/* When we set a button on, we must first clear the currently set button */
	set_button_off_inner( rw, rw->current_set );
	if( rw->unset_callback != NULL ) {
		(rw->unset_callback)( w, (XtPointer)rw->current_set, call_data );
		}

	set_button_on_inner( rw, butno );
	if( rw->set_callback != NULL ) 
		(rw->set_callback)( w, (XtPointer)butno, call_data );

	rw->current_set = butno;
}

/**************************************************************************************************/
/*
static void RadioWidget_dump( RadioWidget rw )
{
	printf( "-------------\n" );
	printf( "This radio widget has %ld buttons\n", rw->nbut );
	printf( "Curerntly set: %ld\n", rw->current_set );
}
*/

/**************************************************************************************************/
long RadioWidget_query_current( RadioWidget rw )
{
	return( rw->current_set );
}

/**************************************************************************************************/
/* Note this doesn't call the callbacks.  Should it?
 */
void RadioWidget_set_current( RadioWidget rw, long butno )
{
	/* When we set a button on, we must first clear the currently set button */
	set_button_off_inner( rw, rw->current_set );
	set_button_on_inner ( rw, butno );

	rw->current_set = butno;
}

