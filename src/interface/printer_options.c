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

/*    define DEBUG */

#include "../ncview.includes.h"
#include "../ncview.defines.h"
#include "../ncview.protos.h"

#define PO_MAR_TEXT_WIDTH 	40
#define FIRST_LABEL_WIDTH	80
#define FONTNAME_WIDTH		100

extern Widget		topLevel;
extern XtAppContext	x_app_context;
extern Options		options;

static Widget
	po_popup_widget,
		po_popupcanvas_widget,
			po_outdev_box_widget,
				po_outdev_label_widget,
				po_outdev_printer_widget,
				po_outdev_file_widget,
				po_outf_label_widget,
				po_outf_text_widget,
			po_margin_box_widget,
				po_margin_label_widget,
				po_xmar_label_widget,
				po_xmar_text_widget,
				po_ymar_top_label_widget,
				po_ymar_top_text_widget,
				po_ymar_bot_label_widget,
				po_ymar_bot_text_widget,
				po_margin_endlabel_widget,
			po_font_box_widget,
				po_font_label_widget,
				po_font_name_label_widget,
				po_font_name_text_widget,
				po_font_headsize_label_widget,
				po_font_headsize_text_widget,
				po_font_size_label_widget,
				po_font_size_text_widget,
				po_font_endlabel_widget,
			po_include_box_widget,
				po_include_label_widget,
				po_include_outline_widget,
				po_include_title_widget,
				po_include_axis_labels_widget,
				po_include_extra_info_widget,
				po_include_id_widget,
				po_test_only_widget,
			po_action_box_widget,
				po_ok_widget,
				po_cancel_widget;

static int po_popup_done = FALSE, po_popup_result;

static XEvent	po_event;

void po_popup_callback		( Widget widget, XtPointer client_data, XtPointer call_data);
void po_outdev_printer_callback ( Widget widget, XtPointer client_data, XtPointer call_data);
void po_outdev_file_callback	( Widget widget, XtPointer client_data, XtPointer call_data);

/*****************************************************************************/

	int
printer_options( PrintOptions *po )
{
	int	llx, lly, urx, ury;
	char	tstr[1024], *tstr2;
	Boolean t_or_f;
	long	l_output_device;

#ifdef DEBUG
	fprintf(stderr, "entering printer_options\n" );
#endif

	/******* Set current values of everything *******/

	/*** Output Device ***/
	l_output_device = (long)(po->output_device);
	XawToggleSetCurrent( po_outdev_printer_widget, (XtPointer)l_output_device );
	if( po->output_device == DEVICE_PRINTER )
		XtVaSetValues( po_outf_text_widget, XtNsensitive, FALSE, NULL );
	XtVaSetValues( po_outf_text_widget, XtNstring, po->out_file_name, NULL );

	/*** Margins ***/
	snprintf( tstr, 1022, "%g", po->page_x_margin );
	XtVaSetValues( po_xmar_text_widget, XtNstring, tstr, NULL );

	snprintf( tstr, 1022, "%g", po->page_upper_y_margin );
	XtVaSetValues( po_ymar_top_text_widget, XtNstring, tstr, NULL );

	snprintf( tstr, 1022, "%g", po->page_lower_y_margin );
	XtVaSetValues( po_ymar_bot_text_widget, XtNstring, tstr, NULL );

	/*** Fonts ***/
	XtVaSetValues( po_font_name_text_widget, XtNstring, po->font_name, NULL );

	snprintf( tstr, 1022, "%d", po->header_font_size );
	XtVaSetValues( po_font_headsize_text_widget, XtNstring, tstr, NULL );

	snprintf( tstr, 1022, "%d", po->font_size );
	XtVaSetValues( po_font_size_text_widget, XtNstring, tstr, NULL );

	/*** Includes ***/
	XtVaSetValues( po_include_title_widget,       XtNstate, po->include_title,       NULL );
	XtVaSetValues( po_include_axis_labels_widget, XtNstate, po->include_axis_labels, NULL );
	XtVaSetValues( po_include_extra_info_widget,  XtNstate, po->include_extra_info,  NULL );
	XtVaSetValues( po_include_outline_widget,     XtNstate, po->include_outline,     NULL );
	XtVaSetValues( po_include_id_widget,          XtNstate, po->include_id,          NULL );
	XtVaSetValues( po_test_only_widget,           XtNstate, po->test_only,           NULL );

#ifdef DEBUG
	fprintf(stderr, "printer_options: popping up window\n" );
#endif
	/********** Pop up the window **********/
	x_get_window_position( &llx, &lly, &urx, &ury );
	XtVaSetValues( po_popup_widget, XtNx, llx+10, XtNy, (ury+lly)/2, NULL );
	XtPopup      ( po_popup_widget, XtGrabExclusive );

	if( options.display_type == PseudoColor )
		x_set_windows_colormap_to_current( po_popup_widget );

	/**********************************************
	 * Main loop for the printer options popup widget
	 */
	while( ! po_popup_done ) {
		/* An event will cause range_popup_done to become TRUE */
		XtAppNextEvent( x_app_context, &po_event );
		XtDispatchEvent( &po_event );
		}
	po_popup_done = FALSE;

	/**********************************************/

#ifdef DEBUG
	fprintf(stderr, "printer_options: done with popup window\n" );
#endif

	if( po_popup_result != MESSAGE_CANCEL ) {
		/* get the values in the widges, put them back into the printer
		 * options structure.
		 */

		/*** Margins ***/
		XtVaGetValues( po_xmar_text_widget, XtNstring, &tstr2, NULL );
		sscanf( tstr2, "%g", &(po->page_x_margin) );
		XtVaGetValues( po_ymar_top_text_widget, XtNstring, &tstr2, NULL );
		sscanf( tstr2, "%g", &(po->page_upper_y_margin) );
		XtVaGetValues( po_ymar_bot_text_widget, XtNstring, &tstr2, NULL );
		sscanf( tstr2, "%g", &(po->page_lower_y_margin) );

		/*** Fonts ***/
		XtVaGetValues( po_font_name_text_widget, XtNstring, &tstr2, NULL );
		strcpy( po->font_name, tstr2 );
		XtVaGetValues( po_font_size_text_widget, XtNstring, &tstr2, NULL );
		sscanf( tstr2, "%d", &(po->font_size) );
		XtVaGetValues( po_font_headsize_text_widget, XtNstring, &tstr2, NULL );
		sscanf( tstr2, "%d", &(po->header_font_size) );

		/*** Device ***/
		l_output_device = (long)XawToggleGetCurrent(po_outdev_printer_widget); 
		po->output_device = (int)l_output_device;
		XtVaGetValues( po_outf_text_widget, XtNstring, &tstr2, NULL );
		strcpy( po->out_file_name, tstr2 );

		/*** Includes ***/
		XtVaGetValues( po_include_title_widget,       XtNstate, &t_or_f, NULL );
		po->include_title       = ( (t_or_f == TRUE) ? 1 : 0 );

		XtVaGetValues( po_include_axis_labels_widget, XtNstate, &t_or_f, NULL );
		po->include_axis_labels = ( (t_or_f == TRUE) ? 1 : 0 );

		XtVaGetValues( po_include_extra_info_widget,  XtNstate, &t_or_f, NULL );
		po->include_extra_info  = ( (t_or_f == TRUE) ? 1 : 0 );

		XtVaGetValues( po_include_outline_widget,     XtNstate, &t_or_f, NULL );
		po->include_outline     = ( (t_or_f == TRUE) ? 1 : 0 );

		XtVaGetValues( po_include_id_widget,          XtNstate, &t_or_f, NULL );
		po->include_id          = ( (t_or_f == TRUE) ? 1 : 0 );

		XtVaGetValues( po_test_only_widget,           XtNstate, &t_or_f, NULL );
		po->test_only           = ( (t_or_f == TRUE) ? 1 : 0 );
		}
	XtPopdown( po_popup_widget );
#ifdef DEBUG
	fprintf(stderr, "printer_options: exiting\n" );
#endif
	return( po_popup_result );
}

/*===============================================================================================*/
	void
printer_options_init()
{

	po_popup_widget = XtVaCreatePopupShell(
		"Printer Options",
		transientShellWidgetClass,
		topLevel,
		NULL );

	po_popupcanvas_widget = XtVaCreateManagedWidget(
		"po_popupcanvas",
		formWidgetClass,
		po_popup_widget,
		NULL);

	po_outdev_box_widget = XtVaCreateManagedWidget(
		"po_outdev_box_widget",	
		boxWidgetClass,	
		po_popupcanvas_widget,
		XtNorientation, XtorientHorizontal,
		NULL );

		po_outdev_label_widget = XtVaCreateManagedWidget(
			"po_outdev_label",	
			labelWidgetClass,	
			po_outdev_box_widget,
			XtNlabel, "Device:",
			XtNwidth, FIRST_LABEL_WIDTH,
			XtNborderWidth, 0,
			NULL );
	
		po_outdev_printer_widget = XtVaCreateManagedWidget(
			"po_outdev_printer_widget",	
			toggleWidgetClass,	
			po_outdev_box_widget,
			XtNlabel, "Printer",
			XtNfromHoriz, po_outdev_label_widget,
			XtNradioData, DEVICE_PRINTER,
			NULL );
	
       		 XtAddCallback( po_outdev_printer_widget, XtNcallback,
			po_outdev_printer_callback, (XtPointer)NULL );
	
		po_outdev_file_widget = XtVaCreateManagedWidget(
			"po_outdev_file_widget",	
			toggleWidgetClass,	
			po_outdev_box_widget,
			XtNlabel, "File",
			XtNradioGroup, po_outdev_printer_widget,
			XtNfromHoriz, po_outdev_printer_widget,
			XtNradioData, DEVICE_FILE,
			NULL );
	
       	 	XtAddCallback( po_outdev_file_widget, XtNcallback,
			po_outdev_file_callback, (XtPointer)NULL );
	
		po_outf_label_widget = XtVaCreateManagedWidget(
			"po_outf_label",	
			labelWidgetClass,	
			po_outdev_box_widget,
			XtNlabel, "Filename:",
			XtNsensitive, False,
			XtNborderWidth, 0,
			NULL );
	
		po_outf_text_widget = XtVaCreateManagedWidget(
			"po_outf_text",	
			asciiTextWidgetClass,	
			po_outdev_box_widget,
			XtNeditType, XawtextEdit,
			XtNfromHoriz, po_outf_label_widget,
			XtNlength, 128,
			XtNborderWidth, 3,
			XtNwidth, 200,
			NULL );

	po_margin_box_widget = XtVaCreateManagedWidget(
		"po_margin_box_widget",	
		boxWidgetClass,	
		po_popupcanvas_widget,
		XtNorientation, XtorientHorizontal,
		XtNfromVert, po_outdev_box_widget,
		NULL );

		po_margin_label_widget = XtVaCreateManagedWidget(
			"po_margin_label",	
			labelWidgetClass,	
			po_margin_box_widget,
			XtNlabel, "Margins:",
			XtNwidth, FIRST_LABEL_WIDTH,
			XtNborderWidth, 0,
			NULL );

		po_xmar_label_widget = XtVaCreateManagedWidget(
			"po_xmargin_label",	
			labelWidgetClass,	
			po_margin_box_widget,
			XtNlabel, "X:",
			XtNborderWidth, 0,
			NULL );

		po_xmar_text_widget = XtVaCreateManagedWidget(
			"po_xmargin_text",	
			asciiTextWidgetClass,	
			po_margin_box_widget,
			XtNeditType, XawtextEdit,
			XtNlength, 128,
			XtNwidth, PO_MAR_TEXT_WIDTH,
			NULL );
	
		po_ymar_top_label_widget = XtVaCreateManagedWidget(
			"po_ymargin_top_label",	
			labelWidgetClass,	
			po_margin_box_widget,
			XtNlabel, "Y top:",
			XtNborderWidth, 0,
			NULL );
	
		po_ymar_top_text_widget = XtVaCreateManagedWidget(
			"po_ymargin_top_text",	
			asciiTextWidgetClass,	
			po_margin_box_widget,
			XtNeditType, XawtextEdit,
			XtNlength, 128,
			XtNwidth, PO_MAR_TEXT_WIDTH,
			NULL );

		po_ymar_bot_label_widget = XtVaCreateManagedWidget(
			"po_ymargin_bot_label",	
			labelWidgetClass,	
			po_margin_box_widget,
			XtNlabel, "Y bottom:",
			XtNborderWidth, 0,
			NULL );
	
		po_ymar_bot_text_widget = XtVaCreateManagedWidget(
			"po_ymargin_bot_text",	
			asciiTextWidgetClass,	
			po_margin_box_widget,
			XtNeditType, XawtextEdit,
			XtNlength, 128,
			XtNwidth, PO_MAR_TEXT_WIDTH,
			NULL );

		po_margin_endlabel_widget = XtVaCreateManagedWidget(
			"po_margin_label",	
			labelWidgetClass,	
			po_margin_box_widget,
			XtNlabel, "(inches)",
			XtNborderWidth, 0,
			NULL );

	po_font_box_widget = XtVaCreateManagedWidget(
		"po_font_box_widget",	
		boxWidgetClass,	
		po_popupcanvas_widget,
		XtNorientation, XtorientHorizontal,
		XtNfromVert, po_margin_box_widget,
		NULL );

		po_font_label_widget = XtVaCreateManagedWidget(
			"po_font_label",	
			labelWidgetClass,	
			po_font_box_widget,
			XtNlabel, "Fonts:",
			XtNwidth, FIRST_LABEL_WIDTH,
			XtNborderWidth, 0,
			NULL );

		po_font_name_label_widget = XtVaCreateManagedWidget(
			"po_font_name_label",	
			labelWidgetClass,	
			po_font_box_widget,
			XtNlabel, "Name:",
			XtNborderWidth, 0,
			NULL );

		po_font_name_text_widget = XtVaCreateManagedWidget(
			"po_font_name_text",	
			asciiTextWidgetClass,	
			po_font_box_widget,
			XtNeditType, XawtextEdit,
			XtNlength, 128,
			XtNwidth, FONTNAME_WIDTH,
			NULL );

		po_font_size_label_widget = XtVaCreateManagedWidget(
			"po_font_size_label",	
			labelWidgetClass,	
			po_font_box_widget,
			XtNlabel, "Size:",
			XtNborderWidth, 0,
			NULL );

		po_font_size_text_widget = XtVaCreateManagedWidget(
			"po_font_size_text",	
			asciiTextWidgetClass,	
			po_font_box_widget,
			XtNeditType, XawtextEdit,
			XtNlength, 128,
			XtNwidth, PO_MAR_TEXT_WIDTH,
			NULL );
	
		po_font_headsize_label_widget = XtVaCreateManagedWidget(
			"po_font_headsize_label",	
			labelWidgetClass,	
			po_font_box_widget,
			XtNlabel, "Header Size:",
			XtNborderWidth, 0,
			NULL );

		po_font_headsize_text_widget = XtVaCreateManagedWidget(
			"po_font_headsize_text",	
			asciiTextWidgetClass,	
			po_font_box_widget,
			XtNeditType, XawtextEdit,
			XtNlength, 128,
			XtNwidth, PO_MAR_TEXT_WIDTH,
			NULL );
	
		po_font_endlabel_widget = XtVaCreateManagedWidget(
			"po_font_label",	
			labelWidgetClass,	
			po_font_box_widget,
			XtNlabel, "(points)",
			XtNborderWidth, 0,
			NULL );

	po_include_box_widget = XtVaCreateManagedWidget(
		"po_include_box_widget",	
		boxWidgetClass,	
		po_popupcanvas_widget,
		XtNorientation, XtorientHorizontal,
		XtNfromVert, po_font_box_widget,
		NULL );
	
		po_include_label_widget = XtVaCreateManagedWidget(
			"po_include_label",	
			labelWidgetClass,	
			po_include_box_widget,
			XtNlabel, "Options:",
			XtNborderWidth, 0,
			XtNwidth, FIRST_LABEL_WIDTH,
			NULL );

		po_include_title_widget = XtVaCreateManagedWidget(
			"po_include_title_widget",	
			toggleWidgetClass,	
			po_include_box_widget,
			XtNlabel, "Title",
			NULL );

		po_include_axis_labels_widget = XtVaCreateManagedWidget(
			"po_include_axis_labels_widget",	
			toggleWidgetClass,	
			po_include_box_widget,
			XtNlabel, "Axis Labels",
			NULL );

		po_include_extra_info_widget = XtVaCreateManagedWidget(
			"po_include_extra_info_widget",	
			toggleWidgetClass,	
			po_include_box_widget,
			XtNlabel, "Extra Info",
			NULL );

		po_include_outline_widget = XtVaCreateManagedWidget(
			"po_include_outline_widget",	
			toggleWidgetClass,	
			po_include_box_widget,
			XtNlabel, "Outline",
			NULL );

		po_include_id_widget = XtVaCreateManagedWidget(
			"po_include_id_widget",	
			toggleWidgetClass,	
			po_include_box_widget,
			XtNlabel, "ID",
			NULL );

		po_test_only_widget = XtVaCreateManagedWidget(
			"po_test_only_widget",	
			toggleWidgetClass,	
			po_include_box_widget,
			XtNlabel, "No Image",
			NULL );

	po_action_box_widget = XtVaCreateManagedWidget(
		"po_action_box_widget",	
		boxWidgetClass,	
		po_popupcanvas_widget,
		XtNorientation, XtorientHorizontal,
		XtNfromVert, po_include_box_widget,
		NULL );
	
		po_ok_widget = XtVaCreateManagedWidget(
			"OK",	
			commandWidgetClass,	
			po_action_box_widget,
			NULL );
	
       	 	XtAddCallback( po_ok_widget, XtNcallback, 
			po_popup_callback, (XtPointer)MESSAGE_OK );
	
		po_cancel_widget = XtVaCreateManagedWidget(
			"Cancel",	
			commandWidgetClass,	
			po_action_box_widget,
			NULL );

       	 	XtAddCallback( po_cancel_widget, XtNcallback, 
			po_popup_callback, (XtPointer)MESSAGE_CANCEL );

}

/*===============================================================================================*/
        void
po_popup_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
	long	l_po_popup_result;

#ifdef DEBUG
	fprintf( stderr, "entering po_popup_callback\n" );
#endif
	l_po_popup_result = (long)client_data;
	po_popup_result   = (int)l_po_popup_result;
	po_popup_done     = TRUE;
#ifdef DEBUG
	fprintf( stderr, "exiting po_popup_callback\n" );
#endif
}

/*===============================================================================================*/
        void
po_outdev_printer_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
#ifdef DEBUG
	fprintf( stderr, "entering po_outdev_printer_callback\n" );
#endif
	XtVaSetValues( po_outf_label_widget, XtNsensitive, False, NULL );
	XtVaSetValues( po_outf_text_widget, XtNsensitive, False, NULL );
#ifdef DEBUG
	fprintf( stderr, "exiting po_outdev_printer_callback\n" );
#endif
}

/*===============================================================================================*/
        void
po_outdev_file_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
#ifdef DEBUG
	fprintf( stderr, "entering po_outdev_file_callback\n" );
#endif
	XtVaSetValues( po_outf_label_widget, XtNsensitive, True, NULL );
	XtVaSetValues( po_outf_text_widget, XtNsensitive, True, NULL );
#ifdef DEBUG
	fprintf( stderr, "exiting po_outdev_file_callback\n" );
#endif
}
