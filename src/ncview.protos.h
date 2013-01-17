/*
 * Ncview by David W. Pierce.  A visual netCDF file viewer.
 * Copyright (C) 1993-2010 David W. Pierce
 *
 * This program  is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as 
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
 */

#include "stringlist.h"

/******************************************************************************
 * in ncview.c 
 */
void	initialize_misc		    ( void );
Stringlist *parse_options           ( int argc,  char *argv[] );
void 	initialize_file_interface   ( Stringlist *input_files );
void	initialize_display_interface( void );
void	initialize_colormaps	    ( void );
void	init_cmap_from_file	    ( char *dir_name, char *file_name );
void	process_user_input          ( void );
void	quit_app		    ( void );
void	create_default_colormap     ( void );
int	check			    ( int value, int min, int max );
void	print_disclaimer	    ( void );
void	print_no_warranty	    ( void );
void	print_copying	    	    ( void );
void	useage			    ( void );

/******************************************************************************
 * in file.c 
 */
int 	fi_confirm       ( char *name );
int	fi_writable      ( char *name );
int 	fi_initialize    ( char *name, int nfiles );
Stringlist *fi_list_vars ( int fileid );
int	fi_n_dims	 ( int fileid, char *var_name );
size_t	*fi_var_size	 ( int fileid, char *var_name );
void 	fi_get_data      ( NCVar *var, size_t *start_pos, size_t *count, void *data );
void 	fi_close         ( int fileid );
void	determine_file_type( Stringlist *input_files );
Stringlist *fi_scannable_dims( int fileid, char *var_name );
char 	*fi_title        ( int fileid );
char 	*fi_long_var_name( int fileid, char *var_name );
char 	*fi_var_units    ( int fileid, char *var_name );
char 	*fi_dim_units    ( int fileid, char *var_name );
char 	*fi_dim_calendar ( int fileid, char *dim_name );
int 	fi_has_dim_values( int fileid, char *dim_name );
char 	*fi_dim_longname ( int fileid, char *dim_name );
nc_type fi_dim_value     ( NCVar *v, int dim_id, size_t place, double *ret_val_double, char *ret_val_char, 
				int *return_has_bounds, double *return_bounds_min, double *return_bounds_max,
				size_t *complete_ndim_virt_place );
char 	*fi_dim_id_to_name( int fileid, char *var_name, int dim_id );
int 	fi_dim_name_to_id( int fileid, char *var_name, char *dim_name );
size_t 	fi_n_dim_entries ( int fileid, char *dim_name );
void 	fi_fill_aux_data ( int id, char *var_name, FDBlist *fdb );
void 	fi_fill_value	 ( NCVar *var, float *fillval );
int 	fi_recdim_id     ( int fileid );

/******************************************************************************
 * in file_netcdf.c, netcdf specific routines 
 */
char *  netcdf_att_string       ( int fileid, char *var_name );
char *  netcdf_global_att_string( int fileid );
int 	netcdf_fi_confirm	( char *name );
int 	netcdf_fi_writable	( char *name );
int 	netcdf_fi_initialize	( char *name );
Stringlist *netcdf_fi_list_vars	( int fileid );
int	netcdf_fi_n_dims	( int fileid, char *var_name );
size_t	*netcdf_fi_var_size	( int fileid, char *var_name );
void 	netcdf_fi_get_data	( int fileid, char *var_name, size_t *start_pos, 
						size_t *count, float *data, NetCDFOptions *aux_data );
void	netcdf_fi_close		( int fileid );
int 	netcdf_n_dims 		( int cdfid, char *varname );
char	*netcdf_varindex_to_name( int cdfid, int index );
Stringlist *netcdf_scannable_dims( int fileid, char *var_name );
char 	*netcdf_title           ( int fileid );
char 	*netcdf_long_var_name   ( int fileid, char *var_name );
char 	*netcdf_get_char_att( int fileid, char *var_name, char *att_name );
char 	*netcdf_var_units       ( int fileid, char *var_name );
char 	*netcdf_dim_units       ( int fileid, char *var_name );
int 	netcdf_has_dim_values   ( int fileid, char *dim_name );
char 	*netcdf_dim_longname 	( int fileid, char *dim_name );
nc_type	netcdf_dim_value     	( int fileid, char *dim_name, size_t place, double *ret_val_double, char *ret_val_char, 
				  size_t virt_place, int *has_bounds, double *return_bounds_min, double *return_bounds_max  );
char 	*netcdf_dim_id_to_name  ( int fileid, char *var_name, int dim_id );
int 	netcdf_dim_name_to_id   ( int fileid, char *var_name, char *dim_name );
size_t 	netcdf_n_dim_entries    ( int fileid, char *dim_name );
void 	netcdf_fill_aux_data    ( int id, char *var_name, FDBlist *fdb );
int	netcdf_min_max_option_set( NCVar *var, float *ret_min, float *ret_max );
int	netcdf_min_option_set	( NCVar *var, float *ret_min );
int	netcdf_max_option_set	( NCVar *var, float *ret_max );
void 	netcdf_fill_value	( int file_id, char *var_name, float *v, NetCDFOptions *opts );
int 	netcdf_fi_recdim_id     ( int fileid );
int 	netcdf_dimvar_bounds_id ( int fileid, char *dim_name, int *nvertices );
char 	*netcdf_dim_calendar( int fileid, char *dim_name );
int 	safe_ncvarid( int fileid, char *varname );

/******************************************************************************
 * in util.c, general utility routines
 */
int 	close_enough	   ( float data, float fill );
void 	new_fdblist        ( FDBlist **el );
void 	new_netcdf         ( NetCDFOptions **n );
int	data_to_pixels     ( View *v );
void	add_var_to_list    ( char *var_name, int file_id, char *filename, int nfiles );
NCVar	*get_var	   ( char *var_name );
void	add_to_varlist     ( NCVar **list, NCVar *new_var );
void	init_min_max	   ( NCVar *var );
void	clip_f		   ( float *val, float min, float max );
void	clip_i		   ( int   *val, int   min, int   max );
void 	fill_dim_structs   ( NCVar *v );
void 	expand_data	   ( float *big_data, View *v, size_t array_size );
void 	check_ranges       ( NCVar *var );
char 	*limit_string	   ( char *s );
int 	*gen_overlay       ( View *v, char *overlay_fname );
void 	fmt_time	   ( char *temp_string, size_t temp_string_len, double new_dimval, NCDim *dim, int include_granularity );
int	n_vars_in_list	   ( NCVar *v );
void 	set_blowup_type	   ( int new_type );
int 	n_strings_in_list  ( Stringlist *s );
int 	strncmp_nocase     ( char *s1, char *s2, size_t n );
int 	warn_if_file_exits ( char *fname );
void 	virt_to_actual_place( NCVar *var, size_t *virt_pl, size_t *act_pl, FDBlist **file );
void 	calc_dim_minmaxes   ( void );
void    add_vars_to_list    ( Stringlist *var_list, int id, char *filename, int nfiles );
int     is_scannable        ( NCVar *v, int i );
void 	sl_cat		    ( Stringlist **dest, Stringlist **src );
void 	get_min_max_onestep( NCVar *var, size_t n_other, size_t tstep, float *data, 
					float *min, float *max, int verbose );
void 	cache_scalar_coord_info( NCVar *vars );
int 	count_nslashes	    ( char *s );
Stringlist *get_group_list  ( NCVar *vars );

/******************************************************************************
 * in interface.c 
 */
void 	in_change_current	( char *dim_name, int modifier );
void 	in_change_dat		( size_t index, float new_val );
void 	in_display_stuff	( char *s, char *var_name );
void 	in_set_edit_place	( size_t index, int x, int y, int nx, int ny );
void 	in_indicate_active_var  ( char *var_name );
void 	in_indicate_active_dim  ( int dimension, char *dim_name );
void 	in_parse_args		( int *p_argc, char **argv );
void 	in_initialize		( void );
void 	in_set_label		( int label_id, char *string );
void 	in_button_pressed	( int button_id, int modifier );
void	in_process_user_input	( void );
void	in_draw_2d_field 	( unsigned char *data, size_t width, size_t height,
	size_t timestep );
void	in_create_colormap	( char *name, ncv_pixel r[256], ncv_pixel g[256], ncv_pixel b[256] );
char	*in_install_next_colormap( int do_widgets_flag );
int	in_set_2d_size   	( size_t width, size_t height );
void 	in_variable_selected	( char *var_name );
void	in_set_sensitive	( int button_id, int state );
void	in_make_dim_buttons	( Stringlist *dim_list );
void	in_clear_dim_buttons	( void );
void	in_error		( char *message );
int	in_dialog		( char *message, char *ret_string, int want_cancel_button );
void 	in_var_set_sensitive	( char *var_name, int sensitivity );
void 	in_fill_dim_info	( NCDim *d, int please_flip );
void	in_set_cur_dim_value	( char *name, char *string );
void 	in_set_cursor_busy	( void );
void 	in_set_cursor_normal	( void );
int 	in_set_scan_dims	( Stringlist *dim_list, char *x_axis, char *y_axis, Stringlist **new_dim_list );
void	in_change_min		( char *label );
void 	in_flush		( void );
void 	report_position		( int x, int y, unsigned int button_mask );
int	in_popup_XY_graph	( size_t n, int dimindex, double *xvals, double *yvals, char *x_axis_title,
				char *y_axis_title, char *title, char *legend, 
				Stringlist *scannable_dims );
void 	in_query_pointer_position( int *x, int *y );
void	in_popup_2d_window	( void );
void	in_popdown_2d_window	( void );
void 	in_timer_clear		( void );
int	in_report_auto_overlay  ( void );
void 	in_timer_set            ( XtTimerCallbackProc procedure, XtPointer arg, unsigned long delay_millisec );
char    *in_install_prev_colormap( int do_widgets );
void 	in_data_edit_dump	( void );

/******************************************************************************
 * in do_buttons.c
 */
int 	which_button_pressed( void );
void 	do_range 	  ( int modifier );
void 	do_quit		  ( int modifier );
void 	do_data_edit	  ( int modifier );
void 	do_info		  ( int modifier );
void 	do_options        ( int modifier );
void 	do_dimset         ( int modifier );
void	do_restart        ( int modifier );
void	do_rewind         ( int modifier );
void	do_backwards      ( int modifier );
void	do_pause          ( int modifier );
void	do_forward        ( int modifier );
void	do_fastforward    ( int modifier );
void	do_colormap_sel   ( int modifier );
void	do_invert_physical( int modifier );
void	do_invert_colormap( int modifier );
void	do_set_minimum    ( int modifier );
void	do_set_maximum    ( int modifier );
void	do_blowup	  ( int modifier );
void	do_transform	  ( int modifier );
void	do_blowup_type	  ( int modifier );

/******************************************************************************
 * in x_interface.c 
 */
void	x_error                 ( char *message );
void 	x_set_windows_colormap_to_current( Widget w );
void 	x_parse_args( int *p_argc, char **argv );
void	x_initialize  		( void );
void    x_process_user_input    ( void );
void 	x_set_label		( int id, char *string );
void	x_set_speed_proc	( Widget scrollbar, XtPointer client_data, XtPointer position );
void	x_draw_2d_field		( unsigned char *data, size_t width, size_t height,
	size_t timestep );
void	x_set_2d_size 		( size_t width, size_t height );
void    x_indicate_active_var   ( char *var_name );
void    *x_create_default_colormap( void );
void 	x_add_to_cmap_list	( char *name, Colormap new_colormap );
void	x_set_sensitive		( int button_id, int state );
void	x_clear_dim_buttons   	( void );
void 	x_make_dim_buttons    	( Stringlist *dim_list );
void 	x_var_set_sensitive     ( char *var_name, int sensitivity );
int 	x_make_dim_info		( Stringlist *dim_list );
void 	x_clear_dim_info	( void );
void	x_init_dim_info		( Stringlist *dim_list );
void 	x_fill_dim_info		( NCDim *d, int please_flip );
void	x_set_cur_dim_value     ( char *name, char *string );
int	x_set_scan_dims     ( Stringlist *dim_list, char *x_axis, char *y_axis, Stringlist **new_dim_list );
void 	x_set_cursor_busy	( void );
void 	x_set_cursor_normal	( void );
void 	x_flush			( void );
void    x_indicate_active_dim   ( int dimension, char *dim_name );
void 	x_query_pointer_position( int *x, int *y );
void	pix_to_rgb		( ncv_pixel pix, int *r, int *g, int *b );
void	x_get_window_position   ( int *llx, int *lly, int *urx, int *ury );
void 	x_set_var_sensitivity( char *varname, int sens );
void	x_popup_2d_window	( void );
void	x_popdown_2d_window	( void );
void 	x_force_set_invert_state( int state );
int	x_report_auto_overlay   ( void );
int 	x_report_widget_width   ( Widget *w );
void 	x_set_colormap_form_width( int width );
void 	x_draw_colorbar		( void );
void 	x_create_colorbar       ( float user_min, float user_max, int transform );
void    x_timer_clear           ( void );
void    x_timer_set             ( XtTimerCallbackProc procedure, XtPointer client_arg, unsigned long delay_millisec );
void    x_indicate_active_var   ( char *var_name );
int     x_dialog                ( char *message, char *ret_string, int want_cancel_button );

/**********************************************************************
 * in range.c
 */
int 	x_range( float old_min, float old_max, float global_min, 
		float global_max, float *new_min, float *new_max,
		int *allvars );
void 	x_range_init();
void 	x_plot_range_init();

/******************************************************************************
 * in view.c 
 */
int 	set_scan_variable    ( NCVar *var );
void 	set_scan_view        ( size_t scan_place );
int 	change_view          ( int delta, int interpretation );
int	view_draw            ( int allow_saveframes_useage, int force_range_to_frame );
void 	view_change_cur_dim  ( char *dim_name, int modifier );
void	view_forward         ( void );
void	view_backward        ( void );
void	view_change_blowup   ( int delta, int redraw_flag, int view_var_is_valid );
void	init_saveframes	     ( void );
void 	redraw_dimension_info( void );
void 	redraw_ccontour      ( void );
void	view_check_new_data  ( int unused );
void	view_report_position ( int x, int y, unsigned int button_mask );
void 	view_report_position_vals( float xval, float yval, int plot_index );
void 	plot_XY              ( void );
void 	set_dataedit_place   ( void );
void    view_data_edit_dump  ( void );
void 	set_min_from_curdata ( void );
void 	set_max_from_curdata ( void );
void	beep		     ( void );
void    invalidate_all_saveframes( void );
void	view_set_XY_plot_axis( String );
void	view_plot_XY_fmt_x_val( float val, int dimindex, char *s, size_t slen );
void 	view_change_dat	     ( size_t index, float new_val );
void	view_get_scaled_size ( int blowup, size_t old_nx, size_t old_ny, size_t *new_nx, size_t *new_ny );
void 	view_change_transform( int delta );
void 	view_recompute_colorbar( void );
void    view_set_range_frame ( void );
void    view_set_range       ( void );
void    view_set_scan_dims   ( void );
void 	view_data_edit       ( void );
void 	view_information     ( void );

/******************************************************************************
 * in overlay.c
 */
void 	do_overlay		( int n, char *custom_filename, int suppress_screen_changes );
char 	**overlay_names		( void );
int 	overlay_current		( void );
int 	overlay_n_overlays	( void );
void 	determine_overlay_base_dir( char *overlay_base_dir, int n );
int 	overlay_custom_n	( void );

/******************************************************************************
 * in set_options.c
 */
void 	options_set_overlay_filename( char *fn );
void 	set_options_init();
void	set_options( void );
Stringlist *get_persistent_X_state();

/******************************************************************************
 * in filesel.c
 */
int 	file_select( char **filename, char *orig_dir );
int 	overlay_select_filename( void );
void 	overlay_init( void );
void 	fs_list_dir( Stringlist **files, Stringlist **dirs );

/******************************************************************************
 * in plot_xy.c
 */
int 	x_popup_XY_graph( long n, int dimindex, double *xvals, double *yvals, char *x_axis_title,
		char *y_axis_title, char *title, char *legend, Stringlist *scannable_dims );
void 	plot_xy_init();
void 	unlock_plot( void );
void 	close_all_XY_plots();
 
/******************************************************************************
 * in plot_range.c
 */
int 	x_plot_range( float old_min, float old_max, float *new_min, float *new_max,
	 		int *autoscale );

/******************************************************************************
 * in udu.c
 */
void 	udu_utinit( char *path );
int 	udu_utistime( char *dimname, char *units );
int 	udu_calc_tgran( int fileid, NCVar *v, int dimid );
void 	udu_fmt_time( char *temp_string, size_t temp_string_len, double new_dimval, NCDim *dim, int include_granularity );

/******************************************************************************
 * in epic_time.c
 */
void epic_fmt_time( char *temp_string, size_t temp_string_len, double new_dimval, NCDim *dim );
int  epic_istime0( int fileid, NCVar *v, NCDim *d );
int  epic_calc_tgran( int fileid, NCDim *d );

/******************************************************************************
 * in do_print.c
 */
void 	print_init	( void );
void 	do_print	( void );

/******************************************************************************
 * in x_dataedit.c
 */
void x_dataedit( char **text, int nx );
void x_set_edit_place( long index, int x, int y, int nx, int ny );

/******************************************************************************
 * in x_display_info
 */
void x_display_info_init();
void x_display_stuff( char *s, char *var_name );

/******************************************************************************
 * in printer_options
 */
void 	printer_options_init();
int 	printer_options( PrintOptions *po );

/******************************************************************************
 * in interface/cbar.c
 */
void cbar_init();
int  cbar_info( unsigned char **data, int *width, int *height );
void cbar_make( int width, int height, int n_extra_colors, float user_min, float user_max, int transform );
char *cbar_data();

/******************************************************************************
 * in interface/make_tc_data.c
 */
void make_tc_data( unsigned char *data, long width, long height, XColor *color_list, unsigned char *tc_data );

/******************************************************************************
 * in interface/colormap_funcs.c
 */
Cmaplist *dup_whole_cmaplist( Cmaplist *src_list );
int 	x_seen_colormap_name( char *name );
void 	x_colormap_info( Cmaplist *cmlist, int idx, char **name, int *enabled, XColor **color_list );
int 	x_n_colormaps( Cmaplist *cmlist );
void 	x_create_colormap( char *name, unsigned char r[256], unsigned char g[256], unsigned char b[256] );
char 	*x_change_colormap( int delta, int do_widgets_flag );
void 	x_check_legal_colormap_loaded();
void 	delete_cmaplist( Cmaplist *cml );
int 	colormap_options_to_stringlist( Stringlist **sl );

/******************************************************************************
 * in handle_rc_file.c
 */
int 	write_state_to_file( Stringlist *state_to_save );
int 	read_state_from_file( Stringlist **state );
Stringlist *get_persistent_state();

/******************************************************************************
 * in file interface/util.c
 */
unsigned char interp( int i, int range_i, unsigned char *mat, int n_entries );

