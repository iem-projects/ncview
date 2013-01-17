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


#include "ncview.includes.h"
#include "ncview.defines.h"
#include "ncview.protos.h"

/* These are the arrays of coastline and outline data build into ncview
 * (i.e, they are NOT required to be loaded from an external directory)
 */
#include "overlay_coasts_p08deg.h"
#include "overlay_coasts_p8deg.h"
#include "overlay_usa.h"

/* Number and order of these must match the defines given in ncview.defines.h!
 * They are used as the labels for the radio buttons
 */
char *my_overlay_names[] = { "None",  
                        "0.8 degree coastlines",
		        "0.08 degree coastlines",
			"USA states",
			"custom" };

extern View  	*view;
extern Options  options;

static int	my_current_overlay;

static int 	gen_xform( float value, int n, float *dimvals );
static int 	*gen_overlay_internal( View *v, float *data, long n );
static void 	do_overlay_inner( View *v, float *data, long nvals, int suppress_screen_changes );
static void 	overlay_find_closest_pt( size_t point_number, float locx, float locy, float *xvals, float *yvals, size_t nx, size_t ny,
			size_t *idxx, size_t *idxy );

/*====================================================================================
 * This routine is only called when the state of the overlay is being changed
 */
	void
do_overlay( int n, char *custom_filename, int suppress_screen_changes )
{
	if( view == NULL ) {
		x_error( "You must select a variable before turning on overlays" );
		return;
		}

	/* Free space for previous overlay */
	if( options.overlay->doit && (options.overlay->overlay != NULL ))
		free( options.overlay->overlay );

	switch(n) {
		
		case OVERLAY_NONE:
			options.overlay->doit = FALSE;
			if( ! suppress_screen_changes ) {
				view->data_status = VDS_INVALID;
				invalidate_all_saveframes();
				change_view( 0, FRAMES );
				}
			break;

		case OVERLAY_P8DEG:
			do_overlay_inner( view, overlay_coasts_p8deg, n_overlay_coasts_p8deg,
					suppress_screen_changes );
			break;

		case OVERLAY_P08DEG:
			do_overlay_inner( view, overlay_coasts_p08deg, n_overlay_coasts_p08deg,
					suppress_screen_changes );
			break;

		case OVERLAY_USA:
			do_overlay_inner( view, overlay_usa, n_overlay_usa,
					suppress_screen_changes );
			break;

		case OVERLAY_CUSTOM:
			if( (custom_filename == NULL) || (strlen(custom_filename) == 0)) {
				in_error( "Specified custom overlay filename is not a valid filename!\n" );
				return;
				}
			options.overlay->overlay = gen_overlay( view, custom_filename ); 
			if( options.overlay->overlay != NULL ) {
				options.overlay->doit = TRUE;
				if( ! suppress_screen_changes ) {
					invalidate_all_saveframes();
					change_view( 0, FRAMES );
					}
				}
			break;

		default:
			fprintf( stderr, "Error, do_overlay called with an unknown index = %d\n", n );
			exit(-1);
		}

	my_current_overlay = n;
}

/*=========================================================================================
 * NOTE: 'nvals' is the total number of data values in array data.  Since there are
 * two data values per location, nvals is TWICE the number of locations.
 */
	void
do_overlay_inner( View *v, float *data, long nvals, int suppress_screen_changes )
{
	options.overlay->overlay = gen_overlay_internal( v, data, nvals );
	if( options.overlay->overlay != NULL ) {
		options.overlay->doit = TRUE;
		if( ! suppress_screen_changes ) {
			invalidate_all_saveframes();
			change_view( 0, FRAMES );
			}
		}
}

/*=========================================================================================
 * This is called just once, when ncview starts up.  In particular,
 * it is NOT called every time we start a new overlay.
 */
	void
overlay_init()
{
	my_current_overlay       = OVERLAY_NONE;
	options.overlay->overlay = NULL;
	options.overlay->doit    = FALSE;
}

/*======================================================================================
 * NOTE: overlay_base_dir must already be allocated to length 'n'
 */
	void
determine_overlay_base_dir( char *overlay_base_dir, int n )
{
	char	*dir;

	dir = (char *)getenv( "NCVIEWBASE" );
	if( dir == NULL ) {
#ifdef NCVIEW_LIB_DIR
		if( strlen(NCVIEW_LIB_DIR) >= n ) {
			fprintf( stderr, "Error, routine determine_overlay_base_dir, string NCVIEW_LIB_DIR too long! Max=%d\n", n );
			exit(-1);
			}
		strcpy( overlay_base_dir, NCVIEW_LIB_DIR );
#else
		strcpy( overlay_base_dir, "." );
#endif
		}
	else
		{
		if( strlen(dir) >= n ) {
			fprintf( stderr, "Error, routine determine_overlay_base_dir, length of dir is too long! Max=%d\n", n );
			exit(-1);
			}
		strcpy( overlay_base_dir, dir );
		}
}

/******************************************************************************
 * This is the version when 2-D mapping is being used for the X and/or
 * Y coordinate
 * NOTE: 'nvals' is the total number of data values in array data.  Since there are
 * two data values per location, nvals is TWICE the number of locations.
 */
	void
gen_overlay_internal_mapped( View *v, float *data, long nvals, int *overlay )
{
	NCDim	*dim_x, *dim_y;
	size_t	ii, jj, kk, x_size, y_size, cursor_place[MAX_NC_DIMS];
	float	x, y, *dimval_x_2d, *dimval_y_2d;
	nc_type	dimval_type;
	double	tval, bnds_min, bnds_max;
	char	cval[1024];
	int	has_bnds;

	dim_x = *(v->variable->dim + v->x_axis_id);
	dim_y = *(v->variable->dim + v->y_axis_id);

	x_size = *(v->variable->size + v->x_axis_id);
	y_size = *(v->variable->size + v->y_axis_id);

	/* Allocate space for 2d arrays that hold mapped X and Y coord vals */
	dimval_x_2d = (float *)malloc( x_size*y_size*sizeof(float) );
	dimval_y_2d = (float *)malloc( x_size*y_size*sizeof(float) );

	if( (dimval_x_2d==NULL) || (dimval_y_2d==NULL) ) {
		in_error( "Malloc of overlay (distance) field failed\n" );
		}
	for( ii=0; ii<v->variable->n_dims; ii++ )
		cursor_place[ii] = v->var_place[ii];

	/* Step 1. Get temporary arrays that hold full 2-D X and Y values */
	for( jj=0; jj<y_size; jj++ )
	for( ii=0; ii<x_size; ii++ ) {
		cursor_place[ v->x_axis_id ] = ii;
		cursor_place[ v->y_axis_id ] = jj;

		/* Get X value */
		dimval_type = fi_dim_value( v->variable, v->x_axis_id, ii, &tval, cval,
			&has_bnds, &bnds_min, &bnds_max, cursor_place );
		if( dimval_type == NC_DOUBLE )
			dimval_x_2d[ii + jj*x_size] = tval;
		else
			dimval_x_2d[ii + jj*x_size] = dim_x->values[ii];

		/* Get Y value */
		dimval_type = fi_dim_value( v->variable, v->y_axis_id, ii, &tval, cval,
			&has_bnds, &bnds_min, &bnds_max, cursor_place );
		if( dimval_type == NC_DOUBLE )
			dimval_y_2d[ii + jj*x_size] = tval;
		else
			dimval_y_2d[ii + jj*x_size] = dim_y->values[jj];
		}

	/* Step 2. For each point specified in the overlay file, get the CLOSEST
	 * point in the 2-D X and Y arrays.
	 */
	for( kk=0; kk<nvals; kk+=2 ) {
		x = data[kk];
		y = data[kk+1];

		/* printf( "pt %ld / %ld (x,y)=(%f,%f)\n", kk, nvals, x, y ); */
		overlay_find_closest_pt( kk, x, y, dimval_x_2d, dimval_y_2d, x_size, y_size,
			&ii, &jj );
		*(overlay + jj*x_size + ii) = 1;
		}

	free(dimval_x_2d);
	free(dimval_y_2d);
	printf( "\n" );
}

/******************************************************************************
 * Generate an overlay from data in an overlay file.  There are 'nvals'
 * valid values in array 'data' (for a shoreline, for example).  data[0] is the
 * first X coordinate, data[1] is the first Y coordinate, data[2] is the
 * second X coordinate, etc.
 */
	int *
gen_overlay_internal( View *v, float *data, long nvals )
{
	NCDim	*dim_x, *dim_y;
	size_t	x_size, y_size, ii;
	int	*overlay, x_is_mapped, y_is_mapped;
	float	x, y;
	long	i, j;

	dim_x = *(v->variable->dim + v->x_axis_id);
	dim_y = *(v->variable->dim + v->y_axis_id);

	x_size = *(v->variable->size + v->x_axis_id);
	y_size = *(v->variable->size + v->y_axis_id);

	overlay = (int *)malloc( x_size*y_size*sizeof(int) );
	if( overlay == NULL ) {
		in_error( "Malloc of overlay field failed\n" );
		return( NULL );
		}
	for( ii=0; ii<x_size*y_size; ii++ )
		*(overlay+ii) = 0;

	x_is_mapped = (v->variable->dim_map_info[ v->x_axis_id ] != NULL);
	y_is_mapped = (v->variable->dim_map_info[ v->y_axis_id ] != NULL);
	if( x_is_mapped || y_is_mapped ) {
		printf( "Please wait, calculating overlays for mapped coordinates is time-consuming" );
		gen_overlay_internal_mapped( v, data, nvals, overlay );
		}
	else
		{
		for( ii=0; ii<nvals; ii+=2 ) {
			x = data[ii];
			y = data[ii+1];

			i = gen_xform( x, x_size, dim_x->values );
			if( i == -2 ) 
				return( NULL );
			j = gen_xform( y, y_size, dim_y->values );
			if( j == -2 ) 
				return( NULL );
			if( (i > 0) && (j > 0)) 
				*(overlay + j*x_size + i) = 1;
			}
		}

	return( overlay );
}

/******************************************************************************
 * Generate an overlay from data in an overlay file.
 */
	int *
gen_overlay( View *v, char *overlay_fname )
{
	FILE	*f;
	char	err_mess[1024], line[80], *id_string="NCVIEW-OVERLAY";
	float	x, y, version;
	long	i, j;
	size_t	x_size, y_size;
	int	*overlay;
	NCDim	*dim_x, *dim_y;

	/* Open the overlay file */
	if( (f = fopen(overlay_fname, "r")) == NULL ) {
		snprintf( err_mess, 1024, "Error: can't open overlay file named \"%s\"\n", 
			overlay_fname );
		in_error( err_mess );
		return( NULL );
		}

	/* Make sure it is a valid overlay file
	 */
	if( fgets(line, 80, f) == NULL ) {
		snprintf( err_mess, 1024, "Error trying to read overlay file named \"%s\"\n",
			overlay_fname );
		in_error( err_mess );
		return( NULL );
		}
	for( i=0; i<strlen(id_string); i++ )
		if( line[i] != id_string[i] ) {
			snprintf( err_mess, 1024, "Error trying to read overlay file named \"%s\"\nFile does not start with \"%s version-num\"\n", 
				overlay_fname, id_string );
			in_error( err_mess );
			return( NULL );
			}
	sscanf( line, "%*s %f", &version );
	if( (version < 0.95) || (version > 1.05)) {
		snprintf( err_mess, 1024, "Error, overlay file has unknown version number: %f\nI am set up for version 1.0\n", version );
		in_error( err_mess );
		return( NULL );
		}

	dim_x = *(v->variable->dim + v->x_axis_id);
	dim_y = *(v->variable->dim + v->y_axis_id);

	x_size = *(v->variable->size + v->x_axis_id);
	y_size = *(v->variable->size + v->y_axis_id);

	overlay = (int *)malloc( x_size*y_size*sizeof(int) );
	if( overlay == NULL ) {
		in_error( "Malloc of overlay field failed\n" );
		return( NULL );
		}
	for( i=0; i<x_size*y_size; i++ )
		*(overlay+i) = 0;

	/* Read in the overlay file -- skip lines with first char of #, 
	 * they are comments.
	 */
	while( fgets(line, 80, f) != NULL ) 
		if( line[0] != '#' ) {
			sscanf( line, "%f %f", &x, &y );
			i = gen_xform( x, x_size, dim_x->values );
			if( i == -2 ) 
				return( NULL );
			j = gen_xform( y, y_size, dim_y->values );
			if( j == -2 ) 
				return( NULL );
			if( (i > 0) && (j > 0)) 
				*(overlay + j*x_size + i) = 1;
			}

	return( overlay );
}

/******************************************************************************
 * Given the (dimensional) value from the overlay file, convert it to 
 * the nearest index along the proper dimension that the point corresponds to.
 * 'n' is the length of array dimvals.
 *
 * For example, 'value' might be 160.0, and dimvals might go from 0.0 to 359.0
 * by 1.0, in which case n=360.  Then, the returned value is the location in
 * array dimvals that is closest to 160.0, in this case, it will be 160.
 */
	int
gen_xform( float value, int n, float *dimvals )
{
	float	min_dist, dist;
	int	i, min_place;

	min_dist  = 1.0e35;
	min_place = 0;

	/* See if off ends of dimvalues ... remember that it can be reversed */
	if( *dimvals > *(dimvals+n-1) ) {
		/* reversed */
		if( value > *dimvals )
			return( -1 );
		if( value < *(dimvals+n-1) )
			return( -1 );
		}
	else
		{
		if( value < *dimvals )
			return( -1 );
		if( value > *(dimvals+n-1) )
			return( -1 );
		}
	
	for( i=0; i<n; i++ ) {
		dist = fabs(*(dimvals+i) - value);
		if( dist < min_dist ) {
			min_dist  = dist;
			min_place = i;
			}
		}

	return( min_place );
}

/****************************************************************************************/
	char **
overlay_names( void )
{
	return( my_overlay_names );
}

/****************************************************************************************/
	int
overlay_current( void )
{
	return( my_current_overlay );
}

/****************************************************************************************/
	int
overlay_n_overlays( void )
{
	return( OVERLAY_N_OVERLAYS );
}

/****************************************************************************************/
	int
overlay_custom_n( void )
{
	return( OVERLAY_CUSTOM );
}

/****************************************************************************************
 * Since distances on a sphere are monotonic with no local minima or maxima, we simply
 * follow the downhill direction until we reach a minimum.
 * Indexing:
 *
 *	0 1 2
 * 	3 4 5
 * 	6 7 8
 * |
 * |
 * (0,0) -----
 *
 */
	void
overlay_find_closest_pt_inner( size_t point_number, size_t init_guess_idxx, size_t init_guess_idxy, 
	float locx, float locy, float *xvals, float *yvals, size_t nx, size_t ny, size_t *idxx, size_t *idxy )
{
	float	dist[9], dx, dy, mindist, tdist[9], prev_d4;
	long	i, j, nsteps, xx;
	long	have_calc[9], index, minloc, ox, oy, isrc, jsrc, idx_src, idx_dest, debug;
	long	curx, cury, i2use, j2use, i0, j0;
	int	n_wrapped_x, n_wrapped_y;
	
	debug  = 0;
	nsteps = 0L;
	n_wrapped_x = 0;
	n_wrapped_y = 0;

	if( debug ) {
		i0 = 68;
		j0 = 46;
		for( j=j0; j<j0+5; j++ ) {
			for( i=i0; i<i0+5; i++ ) {
				index = i + j*nx;
				printf( "(%3ld,%3ld)=(%f,%f) ", i, j, xvals[index], yvals[index] );
				}
			printf( "\n" );
			}
		}

	curx = init_guess_idxx;
	cury = init_guess_idxy;

	for( i=0; i<9; i++ )
		have_calc[i] = 0;

	minloc = -1;
	while( minloc != 4 ) {
		nsteps++;
		mindist = 1.e31;
		for( j=-1; j<=1; j++ )
		for( i=-1; i<=1; i++ ) {
			index = (i+1) + (-j+1)*3;
			if( ! have_calc[index] ) {
				if( (curx == 0) && (i == -1))
					i2use = nx-1;
				else if( (curx == nx-1) && (i == 1))
					i2use = 0;
				else
					i2use = curx + i;

				if( (cury == 0) && (j == -1))
					j2use = ny-1;
				else if( (cury == ny-1) && (j == 1))
					j2use = 0;
				else
					j2use = cury + j;
				xx = i2use + j2use*nx;
				dx = locx - xvals[xx];
				if( dx != dx ) {
					fprintf( stderr, "Error, bad dx  locx=%f  index_xx=%ld  xvals[index]=%f\n", 
						locx, xx, xvals[xx] );
					exit(-1);
					}
				dy = locy - yvals[xx];
				if( dy != dy ) {
					fprintf( stderr, "Error, bad dy  locy=%f  index_xx=%ld  yvals[index]=%f\n", 
						locy, xx, yvals[xx] );
					exit(-1);
					}
				dist[index] = dx*dx + dy*dy;	/* don't bother with square root, takes time and won't affect minimum finding */
				}
			if( debug ) 
				printf( "calculating for index=%ld, i2use,j2use=%ld,%ld  xx=%ld  dx,dy=%f,%f  dist=%f\n", 
					index, i2use, j2use, xx, dx, dy, dist[index] );
				
			if( dist[index] < mindist ) {
				mindist = dist[index];
				minloc  = index;
				}
			}

		if( debug ) {
			printf( "cur loc=(%ld,%ld)  minloc=%ld\n", curx, cury, minloc );
			for( j=2; j>=0; j-- ) {
				for( i=0; i<3; i++ ) {
					index = i + (2-j)*3;
					if( index == minloc )
						printf( ">%f< ", dist[index] );
					else
						printf( " %f  ", dist[index] );
					}
				printf( "\n" );
				}
				
			}

		prev_d4 = dist[4];

		/* Now do the shifting towards the downhill direction 
		 *
		 *	0 1 2
		 * 	3 4 5
		 * 	6 7 8
		 * |
		 * |
		 * (0,0) -----
		 */
		for( i=0; i<9; i++ )
			have_calc[i] = 0;
		switch( minloc ) {
 			case 0: ox=-1; oy=1;  break;
			case 1: ox=0;  oy=1;  break;
			case 2: ox=1;  oy=1;  break;
			case 3: ox=-1; oy=0;  break;
			case 4: ox=0;  oy=0;  break;
			case 5: ox=1;  oy=0;  break;
			case 6: ox=-1; oy=-1; break;
			case 7: ox=0;  oy=-1; break;
			case 8: ox=1;  oy=-1; break;
			default:
				fprintf( stderr, "coding error in overlay.c, minloc=%ld\n", minloc );
				exit(-1);
			}

		/* Update current x,y position */
		if( (curx == 0) && (ox == -1)) {
			n_wrapped_x++;
			curx = nx-1;
			}
		else if( (curx == nx-1) && (ox == 1)) {
			n_wrapped_x++;
			curx = 0;
			}
		else
			curx = curx + ox;

		if( (cury == 0) && (oy == -1)) {
			n_wrapped_y++;
			cury = ny-1;
			}
		else if( (cury == ny-1) && (oy == 1)) {
			cury = 0;
			n_wrapped_y++;
			}
		else
			cury = cury + oy;
		if( (curx<0) || (cury<0) || (curx>=nx) || (cury>=ny)) {
			fprintf( stderr, "Error, alg fails, cursor off array; cursor=(%ld,%ld)  nx=%ld  ny=%ld\n", curx, cury, nx, ny);
			exit(-1);
			}
		if( (n_wrapped_x > 2) || (n_wrapped_y > 2)) {
			/* Give up */
			*idxx = 0;
			*idxy = 0;
			return;
			}

		/* Shift the distance array appropriately */
		for( i=-1; i<=1; i++ )
		for( j=-1; j<=1; j++ ) {
			idx_dest = (i+1) + (-j+1)*3;
			isrc = i + ox;
			jsrc = j + oy;
			if( (isrc>=-1) && (isrc<=1) && (jsrc>=-1) && (jsrc<=1)) {
				idx_src = (isrc+1) + (-jsrc+1)*3;
				if( (idx_src<0) || (idx_src>8)) {
					fprintf( stderr, "Error, alg failed! overlay.c while shifting\n" );
					exit(-1);
					}
				tdist[idx_dest] = dist[idx_src];
				have_calc[idx_dest] = 1;
				}
			}

		for( i=0; i<9; i++ )
			dist[i] = tdist[i];

		if( dist[4] > prev_d4 ) {
			fprintf( stderr, "error, alg fails; prev center dist=%f  new=%f  should always be getting closer!\n",
				prev_d4, dist[4] );
			exit(-1);
			}
		}

	*idxx = curx;
	*idxy = cury;

	/* printf( "nsteps=%ld\n", nsteps ); */
}

/******************************************************************************************
 * Given a location (locx, locy) and an array of values (xvals(nx,ny), yvals(nx,ny)), this
 * calculates the index into the xvals and yvals array such that the distance between
 * (locx,locy) and (xvals(idxx,idxy), yvals(idxx,idxy)) is minimized.
 */
 	void
overlay_find_closest_pt( size_t point_number, float locx, float locy, float *xvals, float *yvals, size_t nx, size_t ny,
		size_t *idxx, size_t *idxy )
{
	size_t	init_guess_idxx, init_guess_idxy;
	static	int have_been_here_before = 0;
	static	size_t last_idxx, last_idxy;

	if( have_been_here_before ) {
		init_guess_idxx = last_idxx;
		init_guess_idxy = last_idxy;
		}
	else
		{
		init_guess_idxx = nx/2;
		init_guess_idxy = ny/2;
		}

	overlay_find_closest_pt_inner( point_number, init_guess_idxx, init_guess_idxy, locx, locy, xvals, yvals,
		nx, ny, idxx, idxy );

	have_been_here_before = 1;
	last_idxx = *idxx;
	last_idxy = *idxy;
}


