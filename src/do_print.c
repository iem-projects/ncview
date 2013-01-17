/*
 * Ncview by David W. Pierce.  A visual netCDF file viewer.
 * Copyright (C) 1993 through 2010 David W. Pierce
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
 * 6259 Caminito Carrean
 * San Diego, CA   92122
 * pierce@cirrus.ucsd.edu
 */

/*     define DEBUG */

#include "ncview.includes.h"
#include "ncview.defines.h"
#include "ncview.protos.h"

#define PAGE_WIDTH_INCHES	8.5
#define PAGE_HEIGHT_INCHES	11.5
#define PAGE_X_MARGIN		1.5	/* Inches */
#define PAGE_UPPER_Y_MARGIN	2.0	/* Inches */
#define PAGE_LOWER_Y_MARGIN	4.0	/* Inches */

#define FONT_SIZE		11
#define HEADER_FONT_SIZE	16
#define FONT_NAME		"Helvetica"
#define LEADING			3

#define DEFAULT_DEVICE		DEVICE_PRINTER
#define	INCLUDE_OUTLINE		TRUE
#define	INCLUDE_TITLE		TRUE
#define	INCLUDE_AXIS_LABELS	TRUE
#define	INCLUDE_EXTRA_INFO	TRUE
#define	INCLUDE_ID		TRUE
#define TEST_ONLY		FALSE

#define	ID_FONT_SIZE_SCALE	0.7	/* How much smaller ID font size is than regular */

extern View 	*view;
extern Options 	options;

static PrintOptions printopts;

static void print_header( FILE *out_file, float scale, size_t x, size_t y, size_t top_of_image );
static void calc_scale( float *scale, size_t x, size_t y );
static void set_font( FILE *outf, char *name, int size );
static void do_outline( FILE *f, size_t x, size_t y );
static void print_other_info( FILE *out_file, float output_scale, size_t x_size, size_t y_size, 
			size_t center_x, size_t center_y, size_t top_of_image, size_t bot_of_image );

/********************************************************************/

	void
print_init( void )
{
	printopts.page_width    	= PAGE_WIDTH_INCHES; 
	printopts.page_height   	= PAGE_HEIGHT_INCHES;
	printopts.page_x_margin 	= PAGE_X_MARGIN;
	printopts.page_upper_y_margin 	= PAGE_UPPER_Y_MARGIN;
	printopts.page_lower_y_margin 	= PAGE_LOWER_Y_MARGIN;
	printopts.ppi			= 72.0;

	printopts.leading 		= LEADING;
	printopts.font_size		= FONT_SIZE;
	printopts.header_font_size 	= HEADER_FONT_SIZE;

	printopts.output_device		= DEFAULT_DEVICE;
	printopts.include_outline	= INCLUDE_OUTLINE;
	printopts.include_id		= INCLUDE_ID;
	printopts.include_title		= INCLUDE_TITLE;
	printopts.include_axis_labels	= INCLUDE_AXIS_LABELS;
	printopts.include_extra_info	= INCLUDE_EXTRA_INFO;

	printopts.test_only		= TEST_ONLY;

	strcpy( printopts.font_name, FONT_NAME );

	printer_options_init(); /* this initializes the X-windows interface
				   part of the pinter options panel */
}

/*************************************************************************/

	void
do_print( void )
{
	long	i, j;
	size_t	x_size, y_size, scaled_x_size, scaled_y_size, top_of_image, bot_of_image, 
		center_x, center_y, left_of_image, right_of_image;
	char	outfname[1024], tstr[1500];
	int     outfid;
	FILE	*outf;
	float	output_scale;
	int	r, g, b, n_print;
	ncv_pixel pix;

#ifdef DEBUG
	fprintf( stderr, "entering do_print()\n" );
#endif
	x_size = *(view->variable->size + view->x_axis_id);
	y_size = *(view->variable->size + view->y_axis_id);
	view_get_scaled_size( options.blowup, x_size, y_size, &scaled_x_size, &scaled_y_size );

	snprintf( printopts.out_file_name, 1024, "ncview.%s.ps", view->variable->name ); 
	if( printer_options( &printopts ) == MESSAGE_CANCEL )
		return;

	if( printopts.output_device == DEVICE_PRINTER ) {
	    strcpy( printopts.out_file_name, "/tmp/ncview.XXXXXX" );
	    outfid = mkstemp( printopts.out_file_name );
	    if (outfid == -1) {
		snprintf( tstr, 1499, "Error opening temporary file for output!\n" );
		in_error( tstr );
		return;
	    }
	    if( (outf = fopen(printopts.out_file_name, "a" )) == NULL ) {
		snprintf( tstr, 1499, "Error opening file %s for output!\n",
			 printopts.out_file_name );
		in_error( tstr );
		return;
	    }
	    close(outfid);
	}
	else {
	    if( warn_if_file_exits( printopts.out_file_name ) == MESSAGE_CANCEL )
		return;
	    
	    if( (outf = fopen(printopts.out_file_name, "w" )) == NULL ) {
		snprintf( tstr, 1499, "Error opening file %s for output!\n",
			 outfname );
		in_error( tstr );
		return;
	    }
	}
	
	in_set_cursor_busy();
	calc_scale( &output_scale, scaled_x_size, scaled_y_size );

	/* These are all in absolute points in the default coordinate system */
	top_of_image   = (size_t)(printopts.page_height*printopts.ppi -
				printopts.page_upper_y_margin*printopts.ppi);
	bot_of_image   = top_of_image - (long)((float)scaled_y_size*output_scale);
	left_of_image  = (size_t)(printopts.page_x_margin*printopts.ppi);
	right_of_image = left_of_image + (long)((float)scaled_x_size*output_scale);
	center_y       = (top_of_image + bot_of_image)/2;
	center_x       = (size_t)(((float)left_of_image + (float)right_of_image)/2.0);

	print_header( outf, output_scale, scaled_x_size, scaled_y_size, top_of_image );

	/***** dump out the color image *****/
	if( ! printopts.test_only ) {
		view_draw( FALSE, FALSE ); /* Don't allow saveframes -- force reload of image data */
		n_print = 0;
		for( j=0; j<scaled_y_size; j++ ) {
			for( i=0; i<scaled_x_size; i++ ) {
				pix = *(view->pixels + j*scaled_x_size + i);
				pix_to_rgb( pix, &r, &g, &b );
				fprintf( outf, "%02x%02x%02x", (r>>8), (g>>8), (b>>8)); 
				n_print += 6;
				if( n_print > 70 ) {
					fprintf( outf, "\n" );
					n_print = 0;
					}
				}
			}
		fprintf( outf, "\n\n" );
		}
	
	/* Outline the color contour with lines */
	if( printopts.include_outline ) 
		do_outline( outf, scaled_x_size, scaled_y_size );

	fprintf( outf, "\n\ngrestore\n" );

	print_other_info( outf, output_scale, scaled_x_size, scaled_y_size, center_x, center_y, 
			top_of_image, bot_of_image );

#ifdef DEBUG
	fprintf( stderr, "exiting do_print()\n" );
#endif
}

/*************************************************************************/

	static void
print_other_info( FILE *outf, float output_scale, size_t x_size, size_t y_size, 
			size_t center_x, size_t center_y, 
			size_t top_of_image, size_t bot_of_image )
{
	char 	*units, *x_dim_name, *x_dim_longname,
		*y_dim_name, *y_dim_longname, *x_units, *y_units,
		tstr[1500], tstr2[1000], *main_long_name, *main_units,
		*dim_name, *dim_longname;
	FDBlist	*fdb;
	NCDim	*d;
	int	i, type, has_bounds;
	size_t	*actual_place;
	time_t	sec_since_1970;
	double	temp_double, bound_min, bound_max;
	FILE	*f_dummy;

#ifdef DEBUG
	fprintf( stderr, "print_other_info: entering\n" );
#endif
	x_dim_name     = (*(view->variable->dim + view->x_axis_id))->name;
	x_dim_longname = fi_dim_longname( view->variable->first_file->id, x_dim_name );
	x_units        = fi_dim_units( view->variable->first_file->id, x_dim_name );

	y_dim_name     = (*(view->variable->dim + view->y_axis_id))->name;
	y_dim_longname = fi_dim_longname( view->variable->first_file->id, y_dim_name );
	y_units        = fi_dim_units( view->variable->first_file->id, y_dim_name );

	main_long_name = fi_long_var_name( view->variable->first_file->id, 
			view->variable->name );
	if( main_long_name == NULL )
		main_long_name = view->variable->name;
	main_units     = fi_var_units( view->variable->first_file->id, view->variable->name );

	/***** Main variable name and units ******/
	if( printopts.include_title ) {
		snprintf( tstr, 1499, "%s", main_long_name );
		if( main_units != NULL ) {
			strcat( tstr, " (" );
			strcat( tstr, main_units );
			strcat( tstr, ")" );
			}

		/* move to the center, then half the string's width */
		set_font( outf, printopts.font_name, printopts.header_font_size );
		fprintf( outf, "%ld %ld moveto\n", 
				center_x,
				top_of_image+printopts.font_size );
		fprintf( outf, "(%s) stringwidth pop -0.5 mul 0 rmoveto\n", tstr );
		fprintf( outf, "(%s) show\n", tstr );
		}

	/***** X axis title *****/
	if( printopts.include_axis_labels ) {
		set_font( outf, printopts.font_name, printopts.font_size );
		strcpy( tstr, x_dim_longname );
		if( x_units != NULL ) {
			strcat( tstr, " (" );
			strcat( tstr, x_units );
			strcat( tstr, ")" );
			}
		fprintf( outf, "%ld %ld moveto\n", 
			center_x, bot_of_image-(long)(1.5*(float)printopts.font_size) );
		fprintf( outf, "(%s) stringwidth pop -0.5 mul 0 rmoveto\n", tstr );
		fprintf( outf, "(%s) show\n", tstr );

		/***** Y axis title *****/
		set_font( outf, printopts.font_name, printopts.font_size );
		strcpy( tstr, y_dim_longname );
		if( y_units != NULL ) {
			strcat( tstr, " (" );
			strcat( tstr, y_units );
			strcat( tstr, ")" );
			}
		fprintf( outf, "%ld %ld moveto\n", 
			center_x - (long)((float)x_size*output_scale/2.0),
			center_y );
		fprintf( outf, "gsave 90 rotate 0 %d rmoveto\n", 
				(int)((float)printopts.font_size*output_scale) );
		fprintf( outf, "(%s) stringwidth pop -0.5 mul 0 rmoveto\n", tstr );
		fprintf( outf, "(%s) show grestore\n", tstr );
		}

	/***************** Other information *******************/
	if( printopts.include_extra_info ) {
		set_font( outf, printopts.font_name, printopts.font_size );
		fprintf( outf, "%ld %ld moveto\n", (long)(printopts.page_x_margin*printopts.ppi),
			bot_of_image - 4*printopts.font_size );

		/**** File title ***/
		if( fi_title( view->variable->first_file->id ) != NULL ) {
			fprintf( outf, "gsave (%s) show grestore\n", 
				fi_title( view->variable->first_file->id ) );
			fprintf( outf, "0 %d rmoveto\n", 
				-(printopts.leading+printopts.font_size) );
			}

		/*** Range of data ***/
		snprintf( tstr, 1499, "Range of %s: %g to %g %s", main_long_name, 
			view->variable->user_min, view->variable->user_max, main_units );
		fprintf( outf, "gsave (%s) show grestore\n", tstr );
		fprintf( outf, "0 %d rmoveto\n", -(printopts.leading+printopts.font_size) );

		/*** Range of X axis ***/
		d = *(view->variable->dim + view->x_axis_id);
		if( x_units == NULL )
			snprintf( tstr, 1499, "Range of %s: %g to %g", 
				x_dim_longname, d->min, d->max);
		else
			snprintf( tstr, 1499, "Range of %s: %g to %g %s", 
				x_dim_longname, d->min, d->max, x_units );
		fprintf( outf, "gsave (%s) show grestore\n", tstr );
		fprintf( outf, "0 %d rmoveto\n", -(printopts.leading+printopts.font_size) );

		/*** Range of Y axis ***/
		d = *(view->variable->dim + view->y_axis_id);
		if( options.invert_physical ) 
			snprintf( tstr, 1499, "Range of %s: %g to %g", 
				y_dim_longname, d->max, d->min );
		else
			snprintf( tstr, 1499, "Range of %s: %g to %g", 
				y_dim_longname, d->min, d->max );
		if( y_units != NULL ) {
			strcat( tstr, " " );
			strcat( tstr, y_units );
			}
		fprintf( outf, "gsave (%s) show grestore\n", tstr );
		fprintf( outf, "0 %d rmoveto\n", -(printopts.leading+printopts.font_size) );

		/*** Values of other dimensions ***/
		for(i=0; i<view->variable->n_dims; i++)
			if( (i != view->x_axis_id) && 
			    (i != view->y_axis_id) &&
			    (*(view->variable->dim+i) != NULL)) {
				dim_name     = (*(view->variable->dim + i))->name;
				dim_longname = fi_dim_longname( view->variable->first_file->id, dim_name );
				units        = fi_dim_units( view->variable->first_file->id, dim_name );
				type         = fi_dim_value( view->variable, i, *(view->var_place+i),
							&temp_double, tstr2, &has_bounds, &bound_min, &bound_max, view->var_place );
				if( type == NC_DOUBLE )
					snprintf( tstr, 1499, "Current %s: %lg", dim_longname, temp_double );
				else
					snprintf( tstr, 1499, "Current %s: %s", dim_longname,
						tstr2 );
				if( units != NULL ) {
					strcat( tstr, " " );
					strcat( tstr, units );
					}
				fprintf( outf, "gsave (%s) show grestore\n", tstr );
				fprintf( outf, "0 %d rmoveto\n", -(printopts.leading+printopts.font_size) );
				}
			
		/*** Name of file ***/
		tstr[0] = '\0';
		actual_place = (size_t *)malloc( sizeof(size_t)*20 );
		virt_to_actual_place( view->variable, view->var_place, actual_place, &fdb );
		if( (fi_recdim_id( view->variable->first_file->id ) != view->x_axis_id ) &&
		    (fi_recdim_id( view->variable->first_file->id ) != view->y_axis_id)) 
			snprintf( tstr, 1499, "Frame %ld in ", 
				*(actual_place + view->scan_axis_id)+1 );
		strcat( tstr, "File " );
		strcat( tstr, fdb->filename );
		fprintf( outf, "gsave (%s) show grestore\n", tstr );
		fprintf( outf, "0 %d rmoveto\n", -(printopts.leading+printopts.font_size) );
		}

	if( printopts.include_id ) {
		sec_since_1970 = time(NULL);
		snprintf( tstr, 1499, "%s %s", getlogin(), ctime(&sec_since_1970) );
		/* Make the id font a bit smaller */
		set_font( outf, printopts.font_name, 
				(int)((float)printopts.font_size*ID_FONT_SIZE_SCALE) );
		fprintf( outf, "gsave %ld %ld translate 0 0 moveto\n", 
			center_x + (long)((float)x_size*output_scale/2.0) 
						+ printopts.font_size + printopts.leading,
			bot_of_image );
		fprintf( outf, "90 rotate (%s) show grestore\n", tstr );
		}

	/****** All done! *****/
	fprintf( outf, "\n\nshowpage\n" );
	fclose( outf );
	if( printopts.output_device == DEVICE_PRINTER ) {
		/* Before executing the command, ensure that the file name exists ... helps
		 * to prevent problems if a strange file name is specified, such as "out.ps ; rm -r ."
		 */
		if( (f_dummy = fopen( printopts.out_file_name, "r" )) == NULL ) {
			fprintf( stderr, "Error, could not open file \"%s\" for reading, which is a prerequisite to printing it\n",
				printopts.out_file_name );
			exit( -1 );
			}
		fclose( f_dummy );
		snprintf( tstr, 1499, "lpr \"%s\"\n", printopts.out_file_name );
		system( tstr );
		unlink( printopts.out_file_name );
		}

	fprintf( stdout, "" );
	fflush( stdout );
	in_set_cursor_normal();
#ifdef DEBUG
	fprintf( stderr, "print_other_info: exiting\n" );
#endif
}

	static void
set_font( FILE *outf, char *name, int size )
{
	fprintf( outf, "/%s findfont\n", name );
	fprintf( outf, "%d scalefont setfont\n", size );
}

	static void
calc_scale( float *scale, size_t x, size_t y )
{
	size_t	page_width, page_height;
	float	scale_x, scale_y;

	page_width = (printopts.page_width-2.0*printopts.page_x_margin)*printopts.ppi;
	page_height =   (printopts.page_height -
			   (printopts.page_upper_y_margin + printopts.page_lower_y_margin)
			)*printopts.ppi;

	scale_x = page_width / (float)x;
	scale_y = page_height / (float)y;

	*scale = (scale_x < scale_y) ? scale_x : scale_y;
}

	static void
do_outline( FILE *f, size_t x, size_t y )
{
	fprintf( f, "newpath\n" );
	fprintf( f, "0 0 moveto\n" );
	fprintf( f, "0 %ld lineto\n", -y );
	fprintf( f, "%ld %ld lineto\n", x, -y );
	fprintf( f, "%ld 0 lineto\n", x );
	fprintf( f, "0 0 lineto\n" );
	fprintf( f, "closepath stroke\n" );
}

	static void
print_header( FILE *f, float scale, size_t x, size_t y, size_t top_of_image )
{
	fprintf( f, "%%!\n" );
	fprintf( f, "/picstr %ld string def\n", x*3 );
	fprintf( f, "gsave\n" );

	/* This sets the position of the output image on the page */
	fprintf( f, "%ld %ld translate\n", 
			(long)(printopts.page_x_margin*printopts.ppi), top_of_image );

	/* This sets the size of the image */
	fprintf( f, "%f %f scale\n", scale, scale );

	if( printopts.test_only ) {
		fprintf( f, "newpath\n" );
		fprintf( f, "0 0 moveto\n" );
		fprintf( f, "0 %ld lineto\n", -y );
		fprintf( f, "%ld %ld lineto\n", x, -y );
		fprintf( f, "%ld 0 lineto\n", x );
		fprintf( f, "0 0 lineto\n" );
		fprintf( f, "%ld %ld lineto\n", x, -y );
		fprintf( f, "0 %ld moveto\n", -y );
		fprintf( f, "%ld 0 lineto\n", x );
		fprintf( f, "closepath stroke\n" );
		}
	else
		{
		fprintf( f, "%ld %ld 8\n", x, y );
		fprintf( f, "[1 0 0 -1 0 1]\n" );
		fprintf( f, "{currentfile picstr readhexstring pop}\n" );
		fprintf( f, "false 3\n" );
		fprintf( f, "colorimage\n\n" );
		}
}

