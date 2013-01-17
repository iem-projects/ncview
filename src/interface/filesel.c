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
 	Function to pop up a file-selection widget.

	David W Pierce
	May 1, 1997
*/
#include "../ncview.includes.h"
#include "../ncview.defines.h"
#include "../ncview.protos.h"

#define PATH_AND_FILENAME_WIDTH 400

extern Widget           topLevel;
extern XtAppContext     x_app_context;

static int fs_popup_done = FALSE, fs_popup_result;

static XEvent   po_event;

static char	retval_buf[2048];

static Widget
		fs_popup_widget,
			fs_popupcanvas_widget,
				fs_viewport_widget,
				fs_list_widget,
				fs_rhpanel_widget,
				fs_pathname_label_widget,
				fs_pathname_text_widget,
				fs_filename_label_widget,
				fs_filename_text_widget,
				fs_options_widget,
					fs_ok_widget,
					fs_cancel_widget;

static 	int fs_popup( Stringlist *strings );
void 	fs_popup_callback( Widget widget, XtPointer client_data, XtPointer call_data);
void	fs_single_click(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void	fs_double_click(Widget w, XButtonEvent *e, String *p, Cardinal *n );
static int fs_is_a_directory( char *name );
static void fs_get_file_dir_list( Stringlist **files_and_dirs );
static String *stringlist_to_Xawlist( Stringlist *strings );
static char *fs_cwd( void );

/*********************************************************************/
	int
fs_popup( Stringlist *strings )
{
	int	llx, lly, urx, ury;

	static XtActionsRec new_actions[] = {
		{"fs_double_click",	(XtActionProc)fs_double_click },
		{"fs_single_click",	(XtActionProc)fs_single_click }};

	fs_popup_widget = XtVaCreatePopupShell(
		"File Selection",
		transientShellWidgetClass,
		topLevel,
		NULL );

	fs_popupcanvas_widget = XtVaCreateManagedWidget(
		"fs_popupcanvas",
		formWidgetClass,
		fs_popup_widget,
		NULL );

	fs_viewport_widget = XtVaCreateManagedWidget(
		"fs_viewport",
		viewportWidgetClass,
		fs_popupcanvas_widget,
		XtNallowVert, True,
		XtNuseRight, True,
		XtNheight, 250,
		XtNwidth, 500,
		NULL );

	fs_list_widget = XtVaCreateManagedWidget(
		"fs_list",
		listWidgetClass,
		fs_viewport_widget,
		XtNlist, stringlist_to_Xawlist( strings ),
		XtNforceColumns, True,
		XtNdefaultColumns, 1,
		NULL );

	XtAugmentTranslations( fs_list_widget,
		XtParseTranslationTable(
			"<Btn1Down>,<Btn1Up>: fs_single_click()\n<Btn1Up>(2): fs_double_click()" ));

	fs_rhpanel_widget  = XtVaCreateManagedWidget(
		"fs_rhpanel",
		formWidgetClass,
		fs_popupcanvas_widget,
		XtNorientation, XtorientVertical,
		XtNfromVert, fs_viewport_widget,
		XtNborderWidth, 0,
		NULL );

	fs_pathname_label_widget = XtVaCreateManagedWidget(
		"fs_pathname_label",
		labelWidgetClass,
		fs_rhpanel_widget,
		XtNlabel, "Path:",
		XtNborderWidth, 0,
		XtNwidth, 80,
		NULL );

	fs_pathname_text_widget = XtVaCreateManagedWidget(
		"fs_pathname_text",
		asciiTextWidgetClass,
		fs_rhpanel_widget,
		XtNeditType, XawtextEdit,
		XtNfromHoriz, fs_pathname_label_widget,
		XtNlength, 800,
		XtNwidth, PATH_AND_FILENAME_WIDTH,
		XtNstring, fs_cwd(),
		NULL );

	fs_filename_label_widget = XtVaCreateManagedWidget(
		"fs_filename",
		labelWidgetClass,
		fs_rhpanel_widget,
		XtNlabel, "Filename:",
		XtNfromVert, fs_pathname_label_widget,
		XtNborderWidth, 0,
		XtNwidth, 80,
		NULL );

	fs_filename_text_widget = XtVaCreateManagedWidget(
		"fs_filename_text",
		asciiTextWidgetClass,
		fs_rhpanel_widget,
		XtNeditType, XawtextEdit,
		XtNfromHoriz, fs_filename_label_widget,
		XtNfromVert, fs_pathname_label_widget,
		XtNlength, 800,
		XtNwidth, PATH_AND_FILENAME_WIDTH,
		NULL );

	fs_options_widget  = XtVaCreateManagedWidget(
		"fs_options",
		boxWidgetClass,
		fs_rhpanel_widget,
		XtNorientation, XtorientHorizontal,
		XtNfromVert, fs_filename_label_widget,
		NULL );

	fs_ok_widget = XtVaCreateManagedWidget(
		"fs_ok",
		commandWidgetClass,
		fs_options_widget,
		XtNlabel, "OK",
		NULL );

       	XtAddCallback( fs_ok_widget, XtNcallback, 
		fs_popup_callback, (XtPointer)MESSAGE_OK );

	fs_cancel_widget = XtVaCreateManagedWidget(
		"fs_cancel",
		commandWidgetClass,
		fs_options_widget,
		XtNfromHoriz, fs_ok_widget,
		XtNlabel, "Cancel",
		NULL );

       	XtAddCallback( fs_cancel_widget, XtNcallback, 
		fs_popup_callback, (XtPointer)MESSAGE_CANCEL );

	XtAppAddActions( x_app_context, new_actions, XtNumber( new_actions ));

	x_get_window_position( &llx, &lly, &urx, &ury );
	XtVaSetValues( fs_popup_widget, XtNx, llx+10, XtNy, 
			lly+10, NULL );

	XtPopup( fs_popup_widget, XtGrabExclusive );
	while( ! fs_popup_done ) {
		XtAppNextEvent( x_app_context, &po_event );
		XtDispatchEvent( &po_event );
		}

	XtDestroyWidget( fs_popup_widget );
	fs_popup_done = FALSE;

	return( fs_popup_result );
}

/*********************************************************************/
	void	
fs_single_click(Widget w, XButtonEvent *e, String *p, Cardinal *n )
{
	XawListReturnStruct	*highlited_entry;

	highlited_entry = XawListShowCurrent( w );
	if( highlited_entry->list_index == XAW_LIST_NONE )
		return;

	XtVaSetValues( fs_filename_text_widget, XtNstring, 
			highlited_entry->string, NULL );
}

/*********************************************************************/
	void	
fs_double_click(Widget w, XButtonEvent *e, String *p, Cardinal *n )
{
	XawListReturnStruct	*highlited_entry;
	Stringlist		*files_and_dirs;
	int			ierr;

	highlited_entry = XawListShowCurrent( w );
	if( highlited_entry->list_index == XAW_LIST_NONE )
		return;

	/* If the selected item is a directory, then chdir to
	 * that directory and list it.  Otherwise, must mean
	 * that we select this file.
	 */
	if( fs_is_a_directory( highlited_entry->string )) {
		ierr = chdir( highlited_entry->string );
		fs_get_file_dir_list( &files_and_dirs );
		XawListChange( fs_list_widget, 
			stringlist_to_Xawlist( files_and_dirs ),
			0, 0, False );
		XtVaSetValues( fs_pathname_text_widget, 
			XtNstring, fs_cwd(), NULL );
		}
	else
		{
		XtVaSetValues( fs_filename_text_widget, XtNvalue, highlited_entry->string, NULL );
		fs_popup_done = TRUE;
		}
}

/*********************************************************************/

        void
fs_popup_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
	long	l_client_data;

	l_client_data   = (long)client_data;
	fs_popup_result = (int)l_client_data;
	fs_popup_done   = TRUE;
}

/*********************************************************************
 * Returns either MESSAGE_OK or MESSAGE_CANCEL; "filename" is set
 * to a valid, fully qualified pathname if returned value is MESSAGE_OK
 * "init_dir" is the initial directory to be in.
 */
	int
file_select( char **filename, char *init_dir )
{
	int		retval, ierr;
	Stringlist	*files_and_dirs;
	char		*tstr, orig_dir[1024];

	/* Save the directory we are in, so we can go back at
	 * the end.
	 */
	if( getcwd(orig_dir, 1024) == NULL ) {
		x_error( "Sorry, current directory name\nis too long to handle!" );
		return(MESSAGE_CANCEL);
		}

	ierr = chdir( init_dir );

	fs_get_file_dir_list( &files_and_dirs );
	retval = fs_popup( files_and_dirs );

	ierr = chdir( orig_dir );

	/* Get the final selected filename if not MESSAGE_CANCEL */
	if( retval != MESSAGE_CANCEL ) {
		XtVaGetValues( fs_pathname_text_widget, XtNstring, 
			&tstr, NULL );
		strcpy( retval_buf, tstr );
		XtVaGetValues( fs_filename_text_widget, XtNstring, 
			&tstr, NULL );
		/* If the user typed in a fully-qualified pathname,
		 * then don't overlay the pathname onto it again.
		 */
		if( *(tstr) == '/' )
			strcpy( retval_buf, tstr );
		else
			{
			strcat( retval_buf, "/" );
			strcat( retval_buf, tstr );
			}
		*filename = retval_buf;
		}
	return( retval );
}

/*********************************************************************/
	static void
fs_get_file_dir_list( Stringlist **files_and_dirs )
{
	Stringlist	*files, *dirs;

	files = NULL;
	dirs  = NULL;
	*files_and_dirs = NULL;

	fs_list_dir( &files, &dirs );

	stringlist_cat( files_and_dirs, &dirs  );
	stringlist_cat( files_and_dirs, &files );
}

/*********************************************************************
 * This is system dependent.  What it does is put a list of the contents
 * of the current working directory into two Stringlists, one of which
 * contains filenames and the other, directory names.
 */
	void
fs_list_dir( Stringlist **files, Stringlist **dirs )
{
	DIR	*cwd;
	struct dirent	*dir_entry;
	char	*tchar;
	size_t	slen;

	cwd = opendir( "." );
	while( (dir_entry = readdir( cwd )) != NULL ) {
		if( (strcmp( dir_entry->d_name, ".") != 0) &&
		    (strcmp( dir_entry->d_name, "..") != 0)) {
			if( fs_is_a_directory( dir_entry->d_name )) {
				slen = strlen(dir_entry->d_name) + 6;	/* add space for NULL and trailing slash */
				tchar = (char *)malloc( sizeof(char) * slen );
				snprintf( tchar, slen-1, "%s/", dir_entry->d_name );
				stringlist_add_string_ordered( dirs, tchar, NULL, SLTYPE_NULL );
				free( tchar );
				}
			else 
				stringlist_add_string_ordered( files, dir_entry->d_name, NULL, SLTYPE_NULL );
			}
		}

	stringlist_add_string_ordered( dirs, "../", NULL, SLTYPE_NULL );
	stringlist_add_string_ordered( dirs, "./", NULL, SLTYPE_NULL );
}

/*********************************************************************/
	static int
fs_is_a_directory( char *name )
{
	struct stat	status;

	stat( name, &status );
	if( S_ISDIR( status.st_mode ) )
		return( TRUE );
	else
		return( FALSE );
}

/*********************************************************************/
	static char *
fs_cwd()
{
	char	*s;
	static char buf[2048];

	s = getcwd( buf, 2048 );
	return( buf );
}

/***************************************************************/
	static String *
stringlist_to_Xawlist( Stringlist *strings )
{
	int	n, i;
	String  *s;
	Stringlist *sl;

	n  = stringlist_len(strings);
	s = (String *)malloc( sizeof(String *) * (n+1) );
	sl = strings;
	i = 0;
	while( sl != NULL ) {
		*(s+i) = (char *)malloc( strlen( sl->string )+1 );
		strcpy( *(s+i++), sl->string );
		sl = sl->next;
		}
	*(s+i) = NULL;
	return( s );
}

