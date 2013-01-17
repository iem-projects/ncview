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

static String fallback_resources[] = {
	"Ncview*foreground:      azure",
	"Ncview*background:      SteelBlue",
	"Ncview*PlotXY_canvas*background: LightSteelBlue",
	"Ncview*labelWidth:	555",
	"Ncview*blowupDefaultSize:  300",
	"Ncview*dimLabelWidth:	95",
	"Ncview*varLabelWidth:	114",
	"Ncview*nVarsPerRow:	4",
	"Ncview*quit*label:	Quit",
	"Ncview*font:			-*-helvetica-*-r-*-*-14-*-*-*-*-*-*-*",
	"Ncview*ccontour*borderWidth: 	0",
	"Ncview*label2.justify:	Left",
	"Ncview*label3.justify:	Left",
	"Ncview*label4.justify:	Left",
	"Ncview*label5.justify:	Left",
	"Ncview*label2*borderWidth:	0",
	"Ncview*label3*borderWidth:	0",
	"Ncview*label4*borderWidth:	0",
	"Ncview*label5*borderWidth:	0",
	"Ncview*varlist_label*borderWidth:	0",
	"Ncview*label_row.borderWidth:		0",
	"Ncview*label_name.borderWidth:		0",
	"Ncview*label_dimension.borderWidth:	0",
	"Ncview*label_min.borderWidth:		0",
	"Ncview*label_cur.borderWidth:		0",
	"Ncview*label_max.borderWidth:		0",
	"Ncview*label_units.borderWidth:	0",
	"Ncview*ydimselectbox.borderWidth:	0",
	"Ncview*ydimlist_label.borderWidth:	0",
	"Ncview*xdimselectbox.borderWidth:	0",
	"Ncview*xdimlist_label.borderWidth:	0",
	NULL };

/*

!
! Modifications to standard button presses.  My convention
! is that mouse button3 indicates a reversal of standard procedure,
! whereas holding down the control key while pressing a mouse button 
! indicates an accelerated version of the standard procedure.  You 
! might prefer something different.  '_mod2' versions are
! the accelerated version of the standard routines, '_mod3'
! are backwards versions of the standards, and '_mod4' are
! accelerated backwards versions of the standard routines.  
! '_mod1' are the standard versions themselves.  
!
! backup actions
Ncview*cmap.translations:   #augment <Btn3Down>,<Btn3Up>: cmap_mod3()
Ncview*Range.translations:  #augment <Btn3Down>,<Btn3Up>: range_mod3()
!
! accelerated actions
Ncview*reverse.translations:     #override Ctrl<Btn1Down>,<Btn1Up>: reverse_mod2()
Ncview*back.translations:        #override Ctrl<Btn1Down>,<Btn1Up>: back_mod2()
Ncview*forward.translations:     #override Ctrl<Btn1Down>,<Btn1Up>: forward_mod2()
Ncview*fastforward.translations: #override Ctrl<Btn1Down>,<Btn1Up>: fastforward_mod2()
!
! Accelerated, backwards, and accelerated backwards actions
Ncview*blowup.translations: #override Ctrl<Btn3Down>,<Btn3Up>: blowup_mod4()\n\
				<Btn3Down>,<Btn3Up>: blowup_mod3()\n\
				Ctrl<Btn1Down>,<Btn1Up>: blowup_mod2()
! Add our own redraw routine
!*ccontour.translations:	   #override <Expose>: redraw_ccontour()
!
Ncview*scrollspeed.translations: #augment <Btn1Down>:MoveThumb() NotifyThumb() 
!
! Various actions you can do in the color contour window.
Ncview*ccontour.translations:	   #augment\n\
				Ctrl<Btn1Up>: do_set_min_from_curdata()\n\
				<Btn1Up>: do_plot_xy()\n\
				Ctrl<Btn1Motion>: do_set_min_from_curdata()\n\
				Ctrl<Btn3Up>: do_set_max_from_curdata()\n\
				Ctrl<Btn3Motion>: do_set_max_from_curdata()\n\
				<Btn2Up>: do_set_dataedit_place()\n\
				<Btn2Motion>: do_set_dataedit_place()
*/
