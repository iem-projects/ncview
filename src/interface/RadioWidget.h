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

NOTE NOTE NOTE that a RadioWidget is NOT A Widget!  If you need a 'real'
Widget, use RadioWidget->widget

*/

#include "../ncview.includes.h"

typedef struct {

	Widget		widget;		/* My top level box widget */
	long		nbut;
	Widget		*button_widget;
	XtCallbackProc	set_callback, unset_callback;
	Pixmap		*pixmap_on, *pixmap_off;
	long		current_set;

} RadioWidgetStruct, *RadioWidget;

RadioWidget RadioWidget_init( Widget parent, Widget fromvert, long nbut, long first_on, char **labels, Pixmap *pixmap_off, Pixmap *pixmap_on, XtCallbackProc set_callback, XtCallbackProc unset_callback );

long 	RadioWidget_query_current( RadioWidget rw );
void 	RadioWidget_set_current  ( RadioWidget rw, long butno );

