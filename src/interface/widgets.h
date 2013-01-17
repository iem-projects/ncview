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


static Widget
	error_popup_widget = NULL,
		error_popupcanvas_widget,
			error_popupdialog_widget,
	dimsel_popup_widget,
		dimsel_popupcanvas_widget,
			dimsel_ok_button_widget,
			dimsel_cancel_button_widget,
	ccontourpanel_widget,
		ccontour_form_widget,
			ccontour_info1_widget,
			ccontour_viewport_widget,
                		ccontour_widget,
			ccontour_info2_widget,
	commandcanvas_widget,
                buttonbox_widget,
			label1_widget,
			label2_widget,
			label3_widget,
			label4_widget,
			label5_widget,
                        quit_button_widget,
			restart_button_widget,
                        reverse_button_widget,
                        backwards_button_widget,
                        pause_button_widget,
                        forward_button_widget,
                        fastforward_button_widget,
			edit_button_widget,
			info_button_widget,
			scrollspeed_label_widget,
			scrollspeed_widget,
			options_button_widget,
		optionbox_widget,
                        cmap_button_widget,
                        invert_button_widget,
                        invert_color_button_widget,
                        blowup_widget,
			transform_widget,
			dimset_widget,
			range_widget,
			blowup_type_widget,
			print_button_widget,
		varsel_form_widget,
			*var_selection_widget,	/* the boxes with N vars per box */
			varlist_label_widget,
			*varlist_widget,	/* The buttons that select a var */
			varsel_menu_widget,	/* Only if using menu-style var selection */
		labels_row_widget,
			lr_dim_widget,
			lr_name_widget,
			lr_min_widget,
			lr_cur_widget,
			lr_max_widget,
			lr_units_widget,
		*diminfo_row_widget = NULL,
			*diminfo_dim_widget = NULL,
			*diminfo_name_widget = NULL,
			*diminfo_min_widget = NULL,
			*diminfo_cur_widget = NULL,
			*diminfo_max_widget = NULL,
			*diminfo_units_widget = NULL,

		xdim_selection_widget,
			xdimlist_label_widget,
			*xdimlist_widget = NULL,

		ydim_selection_widget,
			ydimlist_label_widget,
			*ydimlist_widget = NULL,

		scandim_selection_widget,
			scandimlist_label_widget,
			*scandimlist_widget = NULL;
		
