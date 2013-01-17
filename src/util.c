/*
 * Ncview by David W. Pierce.  A visual netCDF file viewer.
 * Copyright (C) 1993 through 2010  David W. Pierce
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

/*******************************************************************************
 * 	util.c
 *
 *	utility routines for ncview
 *
 *	should be independent of both the user interface and the data
 * 	file format.
 *******************************************************************************/

/* This memory check makes things run slow */
/* define CHECK_MEM */

#include "ncview.includes.h"
#include "ncview.defines.h"
#include "ncview.protos.h"

#include "math.h"

/*-------------------*/
#ifdef HAVE_UDUNITS2
#include "udunits2.h"
extern ut_system *unitsys;
#endif 
/*-------------------*/

extern Options   options;
extern NCVar     *variables;
extern ncv_pixel *pixel_transform;
extern FrameStore framestore;

static void handle_time_dim( int fileid, NCVar *v, int dimid );
static int  months_calc_tgran( int fileid, NCDim *d );
static float util_mean( float *x, size_t n, float fill_value );
static float util_mode( float *x, size_t n, float fill_value );
static void contract_data( float *small_data, View *v, float fill_value );
static int equivalent_FDBs( NCVar *v1, NCVar *v2 );
static int data_has_mv( float *data, size_t n, float fill_value );
static void handle_dim_mapping( NCVar *v );
static void handle_dim_mapping_scalar( NCVar *v, char *coord_var_name, char *coord_att );
static void handle_dim_mapping_2d( NCVar *v, char *coord_var_name, char *coord_att, 
	size_t *coord_var_eff_size, int coord_var_neff_dims, char *orig_coord_att,
	int ncid );
static int  determine_lat_lon( char *s_in, int *is_lat, int *is_lon );

/* Variables local to routines in this file */
static  char    *month_name[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/*******************************************************************************
 * Determine whether the data is "close enough" to the fill value
 */
int
close_enough( float data, float fill )
{
	float	criterion, diff;
	int	retval;

	if( fill == 0.0 )
		criterion = 1.0e-5;
	else if( fill < 0.0 )
		criterion = -1.0e-5*fill;
	else
		criterion = 1.0e-5*fill;

	diff = data - fill;
	if( diff < 0.0 ) 
		diff = -diff;

	if( diff <= criterion )
		retval = 1;
	else
		retval = 0;

 /* printf( "d=%f f=%f c=%f r=%d\n", data, fill, criterion, retval );  */
	return( retval );
}

/******************************************************************************
 * Add the passed NCVar element to the list 
 */
	void
add_to_varlist( NCVar **list, NCVar *new_el )
{
	int	i;
	NCVar	*cursor;

	i = 0;
	if( *list == NULL ) {
		*list = new_el;
		new_el->prev = NULL;
		}
	else
		{
		i = 1;
		cursor = *list;
		while( cursor->next != NULL ) {
			cursor = cursor->next;
			i++;
			}
		cursor->next = new_el;
		new_el->prev = cursor;
		}
}

/******************************************************************************
 * Allocate space for a new NCVar element
 */
	void
new_variable( NCVar **el )
{
	(*el)       = (NCVar *)malloc( sizeof( NCVar ));
	(*el)->next = NULL;
}


/******************************************************************************
 * Allocate space for a new FDBlist element
 */
	void
new_fdblist( FDBlist **el )
{
	NetCDFOptions	*new_netcdf_options;

	(*el)           = (FDBlist *)malloc( sizeof( FDBlist ));
	(*el)->next     = NULL;
	(*el)->filename     = (char *)malloc( MAX_FILE_NAME_LEN );
	(*el)->recdim_units = (char *)malloc( MAX_RECDIM_UNITS_LEN );

#ifdef HAVE_UDUNITS2
	(*el)->ut_unit_ptr  = NULL;
#endif

	strcpy( (*el)->filename, "UNINITIALIZED" );

	new_netcdf( &new_netcdf_options );
	(*el)->aux_data = new_netcdf_options;
}

/******************************************************************************
 * Allocate space for a NetCDFOptions structure.
 */
	void
new_netcdf( NetCDFOptions **n )
{
	(*n) = (NetCDFOptions *)malloc( sizeof( NetCDFOptions ));
	(*n)->valid_range_set  = FALSE;
	(*n)->valid_min_set    = FALSE;
	(*n)->valid_max_set    = FALSE;
	(*n)->scale_factor_set = FALSE;
	(*n)->add_offset_set   = FALSE;

	(*n)->valid_range[0] = 0.0;
	(*n)->valid_range[1] = 0.0;
	(*n)->valid_min      = 0.0;
	(*n)->valid_max      = 0.0;
	(*n)->scale_factor   = 1.0;
	(*n)->add_offset     = 0.0;
}

/******************************************************************************
 * Return 1 if any data value is missing, 0 otherwise
 */
	int
data_has_mv( float *data, size_t n, float fill_value )
{
	size_t i;

	for( i=0; i<n; i++ )
		if( close_enough( data[i], fill_value ))
			return(1);

	return(0);
}

/******************************************************************************
 * Scale the data, replicate it, and convert to a pixel type array.  I'm afraid
 * that for speed, this considers 'ncv_pixel' to be a single byte value.  Make sure
 * to change it if you change the definition of ncv_pixel!  Returns 0 on
 * success, -1 on failure.
 */
	int
data_to_pixels( View *v )
{
	long	i, j, j2;
	size_t	x_size, y_size, new_x_size, new_y_size;
	ncv_pixel pix_val;
	float	data_range, rawdata, data, fill_value, *scaled_data;
	long	blowup, result, orig_minmax_method;
	char	error_message[1024];

	/* Make sure the limits have been set on this variable.
	 * They won't always be because an initial expose event can 
	 * cause this routine to be executed before the min and
	 * maxes are calcuclated.
	 */
	if( ! v->variable->have_set_range )
		return( -1 );

	blowup   = options.blowup;	/* NOTE: can be negative if shrinking data! -N means size is 1/Nth */

	x_size     = *(v->variable->size + v->x_axis_id);
	y_size     = *(v->variable->size + v->y_axis_id);

	view_get_scaled_size( options.blowup, x_size, y_size, &new_x_size, &new_y_size );

	scaled_data   = (float *)malloc( new_x_size*new_y_size*sizeof(float));
	if( scaled_data == NULL ) {
		fprintf( stderr, "ncview: data_to_pixels: can't allocate data expansion array\n" );
		fprintf( stderr, "requested size: %ld bytes\n", new_x_size*new_y_size*sizeof(float) );
		fprintf( stderr, "new_x_size, new_y_size, float_size: %ld %ld %ld\n", 
				new_x_size, new_y_size, sizeof(float) );
		fprintf( stderr, "blowup: %d\n", options.blowup );
		exit( -1 );
		}

	/* If we are doing overlays, implement them */
	if( options.overlay->doit && (options.overlay->overlay != NULL)) {
		for( i=0; i<(x_size*y_size); i++ ) {
			*((float *)v->data + i) = 
			     (float)(1 - *(options.overlay->overlay+i)) * *((float *)v->data + i) +
			     (float)(*(options.overlay->overlay+i)) * v->variable->fill_value;
			}
		}

	fill_value = v->variable->fill_value;

	if( blowup > 0 ) {
		if( options.debug ) printf( "..expanding data, blowup=%ld\n", blowup );
		expand_data( scaled_data, v, new_x_size*new_y_size );
		}
	else
		{
		if( options.debug ) printf( "..contracting data, blowup=%ld\n", blowup );
		contract_data( scaled_data, v, fill_value );
		}

	data_range = v->variable->user_max - v->variable->user_min;

	if( (v->variable->user_max == 0) &&
	    (v->variable->user_min == 0) &&
	    (! options.autoscale) ) {
		in_set_cursor_normal();
		in_button_pressed( BUTTON_PAUSE, MOD_1 );
		if( options.min_max_method == MIN_MAX_METHOD_EXHAUST ) {
	    		snprintf( error_message, 1022, "min and max both 0 for variable %s (checked all data)\nSetting range to (-1,1)", 
								v->variable->name );
			in_error( error_message );
			v->variable->user_max = 1;
			v->variable->user_min = -1;
			v->variable->auto_set_no_range = 1;
			return( data_to_pixels(v) );
			}
	    	snprintf( error_message, 1022, "min and max both 0 for variable %s.\nI can check ALL the data instead of subsampling if that's OK,\nor just cancel viewing this variable.",
	    				v->variable->name );
		result = in_dialog( error_message, NULL, TRUE );
		if( result == MESSAGE_OK ) {
			orig_minmax_method = options.min_max_method;
			options.min_max_method = MIN_MAX_METHOD_EXHAUST;
			init_min_max( v->variable );
			options.min_max_method = orig_minmax_method;
			if( (v->variable->user_max == 0) &&
	    		    (v->variable->user_min == 0) ) {
	    			snprintf( error_message, 1022, "min and max both 0 for variable %s (checked all data)\nSetting range to (-1,1)", 
								v->variable->name );
				in_error( error_message );
				v->variable->user_max = 1;
				v->variable->user_min = -1;
				v->variable->auto_set_no_range = 1;
				return( data_to_pixels(v) );
				}
			else
				return( data_to_pixels(v) );
			}
		else
			{
			if( ! data_has_mv( v->data, x_size*y_size, fill_value ) )
				return( -1 );
			v->variable->user_max = 1;
			}
	    	}

	if( (v->variable->user_max == v->variable->user_min) && (! options.autoscale) ) {
		in_set_cursor_normal();
	    	snprintf( error_message, 1022, "min and max both %g for variable %s",
	    		v->variable->user_min, v->variable->name );
		x_error( error_message );
		if( ! data_has_mv( v->data, x_size*y_size, fill_value ) ) {
			v->variable->user_max += 0.1 * v->variable->user_max;
			v->variable->user_min -= 0.1 * v->variable->user_min;
			v->variable->auto_set_no_range = 1;
			return( data_to_pixels(v) );
			}
		/* If we get here, data is all same, but have a missing value,
		 * so let's go ahead and show it
		 */
		if( v->variable->user_max == 0 )
			v->variable->user_max = 1;
		else if( v->variable->user_max > 0 )
			v->variable->user_min = 0;
		else
			v->variable->user_max = 0;
	    	}

	for( j=0; j<new_y_size; j++ ) {

		if( options.invert_physical )
			j2 = j;
		else
			j2 = new_y_size - j - 1;

		for( i=0; i<new_x_size; i++ ) {
			rawdata =  *(scaled_data + i + j2*new_x_size);
			if( close_enough(rawdata, fill_value) || (rawdata == FILL_FLOAT))
				pix_val = *pixel_transform;
			else
				{
				data = (rawdata - v->variable->user_min) / data_range;
				clip_f( &data, 0.0, .9999 );
				switch( options.transform ) {
					case TRANSFORM_NONE:	break;

					/* This might cause problems.  It is at odds with what
					 * the manual claims--at least for Ultrix--but works, 
					 * whereas what the manual claims works, doesn't!
					 */
					case TRANSFORM_LOW:	data = sqrt( data );  
								data = sqrt( data );
								break;

					case TRANSFORM_HI:	data = data*data*data*data;     break;
					}		
				if( options.invert_colors )
					data = 1. - data;
				pix_val = (ncv_pixel)(data * options.n_colors) + 10;
				if( options.display_type == PseudoColor )
					pix_val = *(pixel_transform+pix_val);
				}
			*(v->pixels + i + j*new_x_size) = pix_val;
			}
		}

	free( scaled_data );
	return( 0 );
}

/******************************************************************************
 * Returns the number of entries in the NCVarlist
 */
	int
n_vars_in_list( NCVar *v )
{
	NCVar *cursor;
	int   i;

	i      = 0;
	cursor = v;
	while( cursor != NULL ) {
		i++;
		cursor = cursor->next;
		}

	return( i );
}

/******************************************************************************
 * Given a list of variable *names*, initialize the variable *structure* and
 * add it to the global list of variables.  Input arg 'nfiles' is the total
 * number of files indicated on the command line, this can be useful for 
 * initializing arrays.
 */
	void
add_vars_to_list( Stringlist *var_list, int id, char *filename, int nfiles )
{
	Stringlist *var;

	if( options.debug )
		fprintf( stderr, "add_vars_to_list: entering, adding vars to list for file %s\n", filename );
	var = var_list;
	while( var != NULL ) {
		if( options.debug ) 
			fprintf( stderr, "adding variable %s to list\n", var->string );
		add_var_to_list( var->string, id, filename, nfiles );
		var = var->next;
		}
	if( options.debug ) 
		fprintf( stderr, "done adding vars for file %s\n", filename );
}

/******************************************************************************
 * For the given variable name, fill out the variable and file structures,
 * and add them into the global variable list.
 */
	void
add_var_to_list( char *var_name, int file_id, char *filename, int nfiles )
{
	NCVar	*var, *new_var;
	int	n_dims, i;
	FDBlist	*new_fdb, *fdb;

	/* make a new file description entry for this var/file combo */
	new_fdblist( &new_fdb );
	new_fdb->id       = file_id;
	new_fdb->var_size = fi_var_size( file_id, var_name );
	if( strlen(filename) > (MAX_FILE_NAME_LEN-1)) {
		fprintf( stderr, "Error, input file name is too long; longest I can handle is %d\nError occurred on file %s\n",
			MAX_FILE_NAME_LEN, filename );
		exit(-1);
		}
	strcpy( new_fdb->filename, filename );

	/* fill out auxilliary (data-file format dependent) information
	 * for the new fdb.
	 */
	fi_fill_aux_data( file_id, var_name, new_fdb );
#ifdef HAVE_UDUNITS2
	new_fdb->ut_unit_ptr = ut_parse( unitsys, new_fdb->recdim_units, UT_ASCII ); /* Will be NULL if there was an error */
#endif

	/* Does this variable already have an entry on the global var list "variables"? */
	var = get_var( var_name );
	if( var == NULL ) {	/* NO -- make a new NCVar structure */
		new_variable( &new_var );
		new_var->name       = (char *)malloc( strlen(var_name)+1 );
		strcpy( new_var->name, var_name );
		n_dims              = fi_n_dims( file_id, var_name );
		new_var->n_dims     = n_dims;
		if( options.debug )
			fprintf( stderr, "adding variable %s with %d dimensions\n",
				var_name, n_dims );
		new_var->first_file = new_fdb;
		new_var->last_file  = new_fdb;
		new_var->global_min = 0.0;
		new_var->global_max = 0.0;
		new_var->user_min   = 0.0;
		new_var->user_max   = 0.0;
		new_var->user_set_blowup   = -99999;
		new_var->auto_set_no_range = 0;
		new_var->have_set_range    = FALSE;
		new_var->size       = fi_var_size( file_id, var_name );
		new_var->fill_value = DEFAULT_FILL_VALUE;
		fi_fill_value( new_var, &(new_var->fill_value) );
		new_fdb->prev       = NULL;
		new_fdb->index      = 0;	/* Since this is the FIRST fdb for this var */

		/* Init the dim mapping info */
		new_var->scalar_dim_map_info = (NCDim_map_info **)malloc( sizeof( NCDim_map_info * ) * MAX_SCALAR_COORDS );
		if( new_var->scalar_dim_map_info == NULL ) {
			fprintf( stderr, "Error allocating space for scalar coordinates\n" );
			exit(-1);
			}
		for( i=0; i<MAX_SCALAR_COORDS; i++ )
			new_var->scalar_dim_map_info[i] = NULL;
		new_var->n_scalar_coords = 0;
		handle_dim_mapping( new_var );	/* needs to be before fill_dim_structs cuz latter access fi_dim_info */

		fill_dim_structs( new_var );
		add_to_varlist  ( &variables, new_var );
		new_var->is_virtual = FALSE;

		}
	else	/* YES -- just add the FDB to the list of files in which 
		 * this variable appears, and accumulate the variable's size.
		 */
		{
		/* Go to the end of the file list and add it there */
		if( options.debug )
			fprintf( stderr, "adding another file with variable %s in it\n",
				var_name );
		if( var->last_file == NULL ) {
			fprintf( stderr, "ncview: add_var_to_list: internal ");
			fprintf( stderr, "inconsistancy; var has no last_file\n" );
			exit( -1 );
			}
		fdb = var->first_file;
		while( fdb->next != NULL )
			fdb = fdb->next;
		fdb->next         = new_fdb;
		new_fdb->prev     = fdb;
		new_fdb->index    = ((FDBlist *)(new_fdb->prev))->index + 1;	/* so index for this fdb is 1 more than index for prev one */
		var->last_file    = new_fdb;
		*(var->size)      += *(new_fdb->var_size);	/* this works b/c you can only concatenate across first (timelike) dim */
		var->is_virtual   = TRUE;
		}
}

/******************************************************************************
 * Go through each variable, and if it has scalar coordinate information,
 * read that in from each file that the var lives in.
 */
	void
cache_scalar_coord_info( NCVar *vars )
{
 	NCVar		*v;
	FDBlist		*tfile;
	int		nfiles, ifile, nsc, isc;
	NCDim_map_info	*dmi;
	float		fval;
	size_t		zeros[MAX_NC_DIMS], ones[MAX_NC_DIMS], n_ts, ii, i_cursor, n_ts_this_file;

	if( options.debug ) printf( "cache_scalar_coord_info: entering\n" );

	/* Allocate space for the timestep_2_fdb array. This points
	 * to the file (FDBlist) associated with EACH TIMESTEP of
	 * the variable
	 */
	v = vars;
	while( v != NULL ) {
		n_ts = v->size[0];	/* total number of timesteps across ALL files */
		if( n_ts > 0 ) {
			if( options.debug )
				printf( "Constructing timestep_2_fdb array for var %s, which has %ld timesteps\n", v->name, n_ts );
			/* One FDBlist pointer for each timestep of the var */
			v->timestep_2_fdb = (FDBlist **)malloc( sizeof( FDBlist *) * n_ts );
			if( v->timestep_2_fdb == NULL ) {
				fprintf( stderr, "Error, failed to allocate space for FDBlist pointer block of length %ld!\n",
						n_ts );
				exit(-1);
				}

			tfile = v->first_file;
			i_cursor = 0L;
			while( tfile != NULL ) {
				/* Set all FDBpointers for the timesteps in THIS file 
				 * to point to this file
				 */
				n_ts_this_file = tfile->var_size[0];
				if( options.debug )
					printf( "%ld timesteps of var %s are in file %s\n", n_ts_this_file, v->name, tfile->filename ); 
				for( ii=0; ii<n_ts_this_file; ii++ ) 
					v->timestep_2_fdb[i_cursor++] = tfile;
				tfile = tfile->next;
				}
			if( i_cursor != n_ts ) {
				fprintf( stderr, "Internal error: in routine cache_scalar_coord_info, got a total length of the unlimited dim in var %s to be %ld, but when setting pointers to the files, there seemd to be only %ld entries\n",
					v->name, n_ts, i_cursor );
				exit(-1);
				}
			}
		v = v->next;
		}

	/* These will be used as the start (zeros) and count (ones)
	 * to get the scalar data
	 */
	for( isc=0; isc<MAX_NC_DIMS; isc++ ) {
		zeros[isc] = 0L;
		ones[isc]  = 1L;
		}

	v = vars;
	while( v != NULL ) {
		nsc = v->n_scalar_coords;
		if( nsc > 0 ) {
			tfile = v->first_file;

			/* How many files does this var live in? */
			nfiles = 0;
			while( tfile != NULL ) {
				nfiles++;
				tfile = tfile->next;
				}
			if( options.debug )
				printf( "Making cache for the %d SCALAR coordinates of variable %s, which lives in %d files\n", nsc, v->name, nfiles );

			/* We hold the values of the scalar coords in the data_cache */
			for( isc=0; isc<nsc; isc++ ) {	
				dmi = v->scalar_dim_map_info[isc];
				dmi->data_cache = (float *)malloc( sizeof(float) * nfiles ); /* one val per FILE (not timestep) */
				if( dmi->data_cache == NULL ) {
					fprintf( stderr, "Error, failed to allocate space for %d scalar coord vals\n", nfiles );
					exit(-1);
					}
				}

			/* Go through each file and read in the vals of all the scalar coords */
			tfile = v->first_file;
			for( ifile=0; ifile<nfiles; ifile++ ) {
				for( isc=0; isc<nsc; isc++ ) {	
					dmi = v->scalar_dim_map_info[isc];
					if( dmi == NULL ) {
						fprintf( stderr, "Coding error, uninitialized pointer to a scalar dim info struct is being used\n" );
						exit(-1);
						}
					netcdf_fi_get_data( tfile->id, dmi->coord_var_name, zeros, ones, &fval, NULL );
					if( options.debug ) printf( "In file %d/%d, value of scalar coord \"%s\" is %f %s\n",
						ifile, nfiles, dmi->coord_var_name, fval, dmi->coord_var_units );
					dmi->data_cache[ifile] = fval;
					}
				tfile = tfile->next;
				}

			/* Now see if all the scalar values are the same */
			for( isc=0; isc<nsc; isc++ ) {	
				dmi = v->scalar_dim_map_info[isc];
				dmi->scalar_all_same = 1;	
				if( nfiles > 1 ) {
					for( ifile=1; ifile<nfiles; ifile++ ) {
						if( dmi->data_cache[ifile] != dmi->data_cache[0] ) 
							dmi->scalar_all_same = 0;
						}
					}
				}
			}

		v = v->next;
		}

	if( options.debug ) printf( "cache_scalar_coord_info: finished\n" );
 }

/******************************************************************************
 * Calculate the min and max values for the passed variable.
 */
	void
init_min_max( NCVar *var )
{
	long	n_other, i, step;
	size_t	n_timesteps;
	float	*data, init_min, init_max;
	int	verbose;

	init_min =  9.9e30;
	init_max = -9.9e30;
	var->global_min = init_min;
	var->global_max = init_max;

	printf( "calculating min and maxes for %s", var->name );

	/* n_other is the number of elements in a single timeslice of the data array */
	n_timesteps = *(var->size);
	n_other     = 1L;
	for( i=1; i<var->n_dims; i++ )
		n_other *= *(var->size+i);

	data = (float *)malloc( n_other * sizeof( float ));
	if( data == NULL ) {
		fprintf( stderr, "ncview: init_min_max_file: failed on malloc of data array\n" );
		exit( -1 );
		}

	/* We always get the min and max of the first, middle, and last time 
	 * entries if they are distinct.
	 */
	verbose = TRUE;
	step    = 0L;
	get_min_max_onestep( var, n_other, step, data, 
			&(var->global_min), &(var->global_max), verbose );
	if( n_timesteps == 1 ) {
		if( verbose )
			printf( "\n" );
		free( data );
		check_ranges( var );
		return;
		}

	step = n_timesteps-1L;
	get_min_max_onestep( var, n_other, step, data, 
			&(var->global_min), &(var->global_max), verbose );
	if( n_timesteps == 2 ) {
		if( verbose )
			printf( "\n" );
		free( data );
		check_ranges( var );
		return;
		}

	step = (n_timesteps-1L)/2L;
	get_min_max_onestep( var, n_other, step, data, 
			&(var->global_min), &(var->global_max), verbose );
	if( n_timesteps == 3 ) {
		if( verbose )
			printf( "\n" );
		free( data );
		check_ranges( var );
		return;
		}

	switch( options.min_max_method ) {
		case MIN_MAX_METHOD_FAST: 
			if( verbose )
				printf( "\n" );
			break;
			
		case MIN_MAX_METHOD_MED:     
			verbose = TRUE;
			step = (n_timesteps-1L)/4L;
			get_min_max_onestep( var, n_other, step, data, 
				&(var->global_min), &(var->global_max), verbose );
			step = (3L*(n_timesteps-1L))/4L;
			get_min_max_onestep( var, n_other, step, data, 
				&(var->global_min), &(var->global_max), verbose );
			if( verbose )
				printf( "\n" );
			break;
				
		case MIN_MAX_METHOD_SLOW:
			verbose = TRUE;
			for( i=2; i<=9; i++ ) { 
				printf( "." );
				step = (i*(n_timesteps-1L))/10L;
				get_min_max_onestep( var, n_other, step, data, 
					&(var->global_min), &(var->global_max), verbose );
				}
			if( verbose )
				printf( "\n" );
			break;
			
		case MIN_MAX_METHOD_EXHAUST:
			verbose = TRUE;
			for( i=1; i<(n_timesteps-2L); i++ ) {
				step = i;
				get_min_max_onestep( var, n_other, step, data, 
					&(var->global_min), &(var->global_max), verbose );
				}
			if( verbose )
				printf( "\n" );
			break;
		}

	if( (var->global_min == init_min) && (var->global_max == init_max) ) {
		var->global_min = 0.0;
		var->global_max = 0.0;
		}
		
	check_ranges( var );
	free( data );
}

/******************************************************************************
 * Try to reconcile the computed and specified (if any) data range
 */
	void
check_ranges( NCVar *var )
{
	float	min, max;
	int	message;
	char	temp_string[ 1024 ];

	if( netcdf_min_max_option_set( var, &min, &max ) ) {
		if( var->global_min < min ) {
			snprintf( temp_string, 1022, "Calculated minimum (%g) is less than\nvalid_range minimum (%g).  Reset\nminimum to valid_range minimum?", var->global_min, min );
			message = in_dialog( temp_string, NULL, TRUE );
			if( message == MESSAGE_OK )
				var->global_min = min;
			}
		if( var->global_max > max ) {
			snprintf( temp_string, 1022, "Calculated maximum (%g) is greater\nthan valid_range maximum (%g). Reset\nmaximum to valid_range maximum?", var->global_max, max );
			message = in_dialog( temp_string, NULL, TRUE );
			if( message == MESSAGE_OK )
				var->global_max = max;
			}
		}

	if( netcdf_min_option_set( var, &min ) ) {
		if( var->global_min < min ) {
			snprintf( temp_string, 1022, "Calculated minimum (%g) is less than\nvalid_min minimum (%g).  Reset\nminimum to valid_min value?", var->global_min, min );
			message = in_dialog( temp_string, NULL, TRUE );
			if( message == MESSAGE_OK )
				var->global_min = min;
			}
		}

	if( netcdf_max_option_set( var, &max ) ) {
		if( var->global_max > max ) {
			snprintf( temp_string, 1022, "Calculated maximum (%g) is greater than\nvalid_max maximum (%g).  Reset\nmaximum to valid_max value?", var->global_max, max );
			message = in_dialog( temp_string, NULL, TRUE );
			if( message == MESSAGE_OK )
				var->global_max = max;
			}
		}

	var->user_min = var->global_min;
	var->user_max = var->global_max;
	var->have_set_range = TRUE;
}

/******************************************************************************
 * get_min_max utility routine; is passed timestep number where want to 
 * determine extrema 
 * Inputs:
 *	n_other : # of entries in a single timelice of the variable
 *	data    : working space that will be overwritten with data values
 *		  of the specified timestep
 */
	void
get_min_max_onestep( NCVar *var, size_t n_other, size_t tstep, float *data, 
					float *min, float *max, int verbose )
{
	size_t	*start, *count, n_time;
	size_t	j;
	int	i;
	float	dat, fill_v;
	
	count  = (size_t *)malloc( var->n_dims * sizeof( size_t ));
	start  = (size_t *)malloc( var->n_dims * sizeof( size_t ));
	fill_v = var->fill_value;

	n_time = *(var->size);
	if( tstep > (n_time-1) )
		tstep = n_time-1;

	*(count) = 1L;
	*(start) = tstep;
	for( i=1; i<var->n_dims; i++ ) {
		*(start+i) = 0L;
		*(count+i) = *(var->size + i);
		}

	if( verbose ) {
		printf( "." );
		fflush( stdout );
		}

	fi_get_data( var, start, count, data );

	for( j=0; j<n_other; j++ ) {
		dat = *(data+j);
		if( dat != dat )
			dat = fill_v;
		if( (! close_enough(dat, fill_v)) && (dat != FILL_FLOAT) )
			{
			if( dat > *max )
				*max = dat;
			if( dat < *min )
				*min = dat;
			}
		}
		
	free( count );
	free( start );
}

/******************************************************************************
 * convert a variable name to a NCVar structure
 */
	NCVar *
get_var( char *var_name )
{
	NCVar	*ret_val;

	ret_val = variables;
	while( ret_val != NULL )
		if( strcmp( var_name, ret_val->name ) == 0 )
			return( ret_val );
		else
			ret_val = ret_val->next;

	return( NULL );
}

/******************************************************************************
 * Clip out of range floats 
 */
	void
clip_f( float *data, float min, float max )
{
	if( *data < min )
		*data = min;
	if( *data > max )
		*data = max;
}

/******************************************************************************
 * Turn a virtual variable 'place' array into a file/place pair.  Which is
 * to say, the virtual size of a variable spans the entries in all the files; 
 * the actual place where the entry for a particular virtual location can
 * be found is in a file/actual_place pair.  This routine does the conversion.
 * Note that this routine is assuming the netCDF convention that ONLY THE
 * FIRST index can be contiguous across files.  The first index is typically
 * the time index in netCDF files.  NOTE! that 'act_pl' must be allocated 
 * before calling this!
 */
	void
virt_to_actual_place( NCVar *var, size_t *virt_pl, size_t *act_pl, FDBlist **file )
{
	FDBlist	*f;
	size_t	v_place, cur_start, cur_end;
	int	i, n_dims;

	f       = var->first_file;
	n_dims  = fi_n_dims( f->id, var->name );
	v_place = *(virt_pl);

	if( v_place >= *(var->size) ) {
		fprintf( stderr, "ncview: virt_to_actual_place: error trying ");
		fprintf( stderr, "to convert the following virtual place to\n" );
		fprintf( stderr, "an actual place for variable %s:\n", var->name );
		for( i=0; i<n_dims; i++ )
			fprintf( stderr, "[%1d]: %ld\n", i, *(virt_pl+i) );
		exit( -1 );
		}

	cur_start = 0L;
	cur_end   = *(f->var_size) - 1L;
	while( v_place > cur_end ) {
		cur_start += *(f->var_size);
		f          = f->next;
		cur_end   += *(f->var_size);
		}

	*file = f;
	*act_pl = v_place - cur_start;

	/* Copy the rest of the indices over */
	for( i=1; i<n_dims; i++ )
		*(act_pl+i) = *(virt_pl+i);
}

/******************************************************************************
 * Initialize the var->dim_map_info table
 */
	static void
handle_dim_mapping( NCVar *v )
{
	int	i, varid, ncid, coord_var_ndims, coord_var_neff_dims;
	size_t	coord_var_eff_size[MAX_NC_DIMS], *coord_var_size; 
	char	*coord_att, *s, orig_coord_att[1024];
	const 	char *delim = " \n\0\t";

	if( options.debug ) printf( "handle_dim_mapping: entering for var %s\n", v->name );

	ncid = v->first_file->id;

	/* dim_map_info itself is never NULL. If the var has no coordinate mappings,
	 * then this will point to an array each entry of which is NULL.
	 */
	v->dim_map_info = (NCDim_map_info **)malloc( sizeof(NCDim_map_info *) * v->n_dims );
	for( i=0; i<v->n_dims; i++ )
		(v->dim_map_info)[i] = (NCDim_map_info *)NULL;

	/* See if this var has a "coordinates" attribute */
	coord_att = netcdf_get_char_att( ncid, v->name, "coordinates" );
	if( coord_att == NULL )
		return;

	if( strlen(coord_att) > 1020 )
		strncpy( orig_coord_att, coord_att, 1020 );
	else
		strcpy( orig_coord_att, coord_att );
	if( options.debug ) printf( "var %s HAS a coordinates attribute: >%s<\n", v->name, coord_att );

	/* Check for blank-delimited strings in the coordinates attribute
	 * that name other vars in the file
	 */
	s = strtok( coord_att, delim );
	while( s != NULL ) {

		/* See if this token "s", which came from the coordinates attribute,
		 * is the name of a variable in the file 
		 */
		varid = safe_ncvarid( ncid, s );
		if( varid != -1 ) {	/* yes, the token "s" matches the name of a var in the file! */

			/* Right now, I'm only going to try to handle either scalar (0d)
			 * or 2-D mapping dims.  Scalar mapping dims just give an 
			 * additional location where the data is valid; for example,
			 * the data might be a 2d field in (lon,lat) and have a 
			 * "height" coordinate that tells the height the data is at.
			 * If the dim is more complicated than that, then we simply
			 * ignore the mapping.  In particular, the test WRF output file
			 * I have has a 3-D mapping dim with time as the first time.
			 * Does WRF move the mapping around over time?  Dunno.  In
			 * any event, we will allow it to handle 2 EFFECTIVE dims,
			 * but otherwise, if the mapping var has more than 2 effective
			 * dims, then forget it.
			 */
			coord_var_ndims = netcdf_n_dims( ncid, s );
			coord_var_size = netcdf_fi_var_size( ncid, s );
			coord_var_neff_dims = 0;
			for( i=0; i<coord_var_ndims; i++ )
				if( coord_var_size[i] > 1 ) {
					coord_var_eff_size[coord_var_neff_dims] = coord_var_size[i];
					coord_var_neff_dims++;
					}

			/* These routines are where we do most of the work
			 */
			if( coord_var_neff_dims == 0 ) 
				handle_dim_mapping_scalar( v, s, coord_att );

			else if( coord_var_neff_dims == 2 ) 
				handle_dim_mapping_2d( v, s, coord_att, coord_var_eff_size, coord_var_neff_dims,
					orig_coord_att, ncid );

			else
				{
				printf( "Note: the coordinates attribute for variable %s is being ignored,\n", v->name );
				printf( "since it specifies a variable (%s) that has %d effective dims (an effective dim has a size greater than 1)\n", 
					s, coord_var_neff_dims );
				printf( "I am not set up to handle cases with coordinate mapping using anything other than 0 or 2 effective dims\n" );
				return;
				}
			}
		else
			{
			/* varid == -1, indicating no var of this name was found in the file */
			if( options.debug )
				printf( "Warning: token \"%s\" appears in a coordinates attribute yet is NOT a var in the file\n", s );
			}

		s = strtok( NULL, delim );
		}
}

/**********************************************************************************************/
	static void
handle_dim_mapping_scalar( NCVar *v, char *coord_var_name, char *coord_att )
{
	NCDim_map_info	*tmi;

	if( v->n_scalar_coords >= MAX_SCALAR_COORDS ) {
		printf( "Note: var %s has exceeded the allowable number of scalar coordinate dimensions, which is %d. Ignoring the rest\n",
			v->name, MAX_SCALAR_COORDS);
		return;
		}

	/* Make a new SCALAR dim map info structure to 
	 * hold our single value
	 */
	tmi = (NCDim_map_info *)malloc( sizeof(NCDim_map_info) );

	/* Copy info to the new scalar dim map structure
	 */
	tmi->var_i_map = v;
	tmi->coord_var_name = (char *)malloc( sizeof(char) * (strlen(coord_var_name) + 1));
	strcpy( tmi->coord_var_name, coord_var_name );
	tmi->coord_var_units = fi_var_units( v->first_file->id, coord_var_name );
	tmi->scalar_all_same = 0;

	/* Add this new scalar dim to the array */
	v->scalar_dim_map_info[v->n_scalar_coords] = tmi;
	(v->n_scalar_coords)++;

	if( options.debug ) printf("Added a new scalar coord to var %s: name=%s count=%d\n",
		v->name, coord_var_name, v->n_scalar_coords );

	/* Note that we CANNOT fill out the data values for this "scalar" coord var 
	 * yet. The reason is because this var might live in multiple files, and
	 * each file could have a different value of the scalar variable. I.e., 
	 * the scalar coord var could be used to essentially add another unlimited
	 * dimension to the variable. To handle this, we must read in the 
	 * scalar values from EACH FILE after we know what all the files this
	 * variable lives in are. That happens in the routine that calls this
	 * one, add_var_to_list
	 */
}

/****************************************************************************************************/
	static void
handle_dim_mapping_2d( NCVar *v, char *coord_var_name, char *coord_att, size_t *coord_var_eff_size,
		int coord_var_neff_dims, char *orig_coord_att, int ncid )
{
	NCDim_map_info	*map_info;
	size_t		totsize, start[MAX_NC_DIMS], count[MAX_NC_DIMS];
	int		i, n_matches, err, is_lat, is_lon, idx_lat_dim, idx_lon_dim,
			must_be_left_of, j;

	/* Make new, uninitialized dim_map_info structure */
	map_info = (NCDim_map_info *)malloc( sizeof(NCDim_map_info) );
	if( map_info == NULL ) {
		fprintf( stderr, "Error, could not allocate space for a dim mapping info structure\n" );
		exit(-1);
		}

	/* Copy over the coordinate attribute for posterity */
	map_info->coord_att = (char *)malloc( sizeof(char)*(strlen(coord_att)+2));
	if( map_info->coord_att == NULL ) {
		fprintf( stderr, "Error, failed to allocate space to copy the coordinate attribute\n" );
		exit(-1);
		}
	strcpy( map_info->coord_att, coord_att );

	/* This is the "variable that I map" */
	map_info->var_i_map = v;

	/* Since "coord_var_name" matches a var name, it must be the coordinate variable name in particular.
	 * The coordinate variable is the var in the file that holds mapping info 
	 */
	map_info->coord_var_name = (char *)malloc( strlen(coord_var_name) + 2 );
	if( map_info->coord_att == NULL ) {
		fprintf( stderr, "Error, failed to allocate space to copy the coordinate variable name\n" );
		exit(-1);
		}
	strcpy( map_info->coord_var_name, coord_var_name );

	if( options.debug ) printf( "Coord var named >%s< is a NON-SCALAR coord used to map a dimension of var %s\n", 
			coord_var_name, v->name );

	/* See how many dims this coord var has */
	map_info->coord_var_ndims = netcdf_n_dims( ncid, coord_var_name );

	/* Get size of the coord var */
	map_info->coord_var_size = netcdf_fi_var_size( ncid, coord_var_name );

	if( options.debug ) {
		printf( "non-scalar Coord var %s has %d dims, here are their sizes: ",
			coord_var_name, map_info->coord_var_ndims );
		for( i=0; i<map_info->coord_var_ndims; i++ )
			printf( "%ld ", map_info->coord_var_size[i] );
		printf( "\n" );
		}

	/* Get array of boolean indicating which dims in the 
	 * base var match the shape of this coord var.  For
	 * instance, if we have a var of shape (10,20,180,360)
	 * and a coord var of shape (180,360) then this indicating
	 * array will be 0,0,1,1.
	 */
	map_info->matching_var_dims  = (int *)   malloc( sizeof(int)    * v->n_dims );
	map_info->index_place_factor = (size_t *)malloc( sizeof(size_t) * v->n_dims );
	for( i=0; i<v->n_dims; i++ ) {
		map_info->matching_var_dims [i] = 0;
		map_info->index_place_factor[i] = 0;
		}

	/* We could have a problem if the dim sizes are repeated instead of unique.
	 * For example, imagine a square data array of size [n,n].  Then we have
	 * a lon mapping array of size [n,n].  We don't want the boolean array
	 * 'matching_var_dims' to end up as [0,1], we want it to end up as [1,1].
	 * In other words, stop a dim in the coord var from matching the same dim
	 * in the original var twice, even if the dim size is repeated.  We do this
	 * by fist finding a match, then requiring the NEXT match to be to the
	 * left of (in the array of the var's dim sizes) the previous match.
	 */
	must_be_left_of = v->n_dims;	/* Start out by setting all the way to right edge */
	for( i=map_info->coord_var_ndims-1; i>=0; i--) {/* Want to find a dim in v that matches size of coord_var dim number i... */
		/*
		fprintf( stderr, "Searching for a dim in var %s that matches dim number %d in %s, which is of size %d\n",
			v->name, i, s, coord_var_eff_size[i] );
		fprintf( stderr, "the match must be to the left of %d\n", must_be_left_of );
		*/
		for( j=must_be_left_of-1; j>=0; j-- ) {	/* ...subject to constraint that match be left of (have lower numerical value then) j */
			if( coord_var_eff_size[i] == v->size[j] ) {
				map_info->matching_var_dims[j] = 1;
				must_be_left_of = j;	/* found a match at j, so NEXT match must be at a lower value of j than this */
				break;
				}
			}
		}

	n_matches = 0;
	for( i=0; i<v->n_dims; i++ )
		n_matches += map_info->matching_var_dims[i];
	if( n_matches != coord_var_neff_dims ) {
		fprintf( stderr, "Warning: did not correctly match mapped dims specified in the coordinates attribute to dims in the variable\n" );
		fprintf( stderr, "Problem encountered on variable \"%s\" which has shape (", v->name );
		for( i=0; i<v->n_dims; i++ ) {
			fprintf( stderr, "%ld", v->size[i] );
			if( i < (v->n_dims-1))
				fprintf( stderr, "," );
			}
		fprintf( stderr, ")\n" );
		fprintf( stderr, "and has coordinates attribute \"%s\"\n", orig_coord_att );
		fprintf( stderr, "The problem is that coordinate var \"%s\" has shape (", coord_var_name );
		for( i=0; i<map_info->coord_var_ndims; i++ ) {
			fprintf( stderr, "%ld", map_info->coord_var_size[i] );
			if( i < (map_info->coord_var_ndims-1))
				fprintf( stderr, "," );
			}
		fprintf( stderr, "), which does not match dimensions in the variable being mapped!\n" );
		fprintf( stderr, "Abandoning coordinate mapping for this variable\n-------------\n" );
		for( i=0; i<v->n_dims; i++ )
			(v->dim_map_info)[i] = (NCDim_map_info *)NULL;
		return;
		}
	if( (n_matches<1) || (n_matches>2)) {
		fprintf( stderr, "(Location B) Error, did not correctly match mapped dims specified in the coordinates attribute to dims in the variable\n" );
		fprintf( stderr, "(Location B) Please send email to dpierce@ucsd.edu letting me know what your coordinates attribute looks like so I can fix this problem.\n" );
		fprintf( stderr, "Problem encountered on variable \"%s\"\n", v->name );
		fprintf( stderr, "which has coordinates attribute \"%s\"\n", orig_coord_att );
		fprintf( stderr, "Abandoning coordinate mapping for this variable\n" );
		for( i=0; i<v->n_dims; i++ )
			(v->dim_map_info)[i] = (NCDim_map_info *)NULL;
		return;
		}

	/* Try to figure out if this dim is 'latitude' like
	 * or 'longitude' like....these are the only options
	 * for now. 
	 */
	err = determine_lat_lon( map_info->coord_var_name, &is_lat, &is_lon );
	if( err != 0 ) {
		/* Abort this process */
		for( i=0; i<v->n_dims; i++ )
			(v->dim_map_info)[i] = (NCDim_map_info *)NULL;
		return;
		}
	idx_lon_dim = -1;
	idx_lat_dim = -1;
	if( is_lon ) {
		if( options.debug ) printf( "Coord var was found to be a LONGITUDE\n" );
		/* Match this coord var to the last one on the right */
		for( i=v->n_dims-1; i>=0; i-- ) {
			if( map_info->matching_var_dims[i] == 1 ) {
				v->dim_map_info[i] = map_info;
				idx_lon_dim = i;
				if( options.debug )
					printf( "In variable \"%s\", dimension \"%s\" is mapped by LONGITUDE-like %d-dimensional variable \"%s\"\n",
					v->name, netcdf_dim_id_to_name( v->first_file->id, v->name, i), 
					map_info->coord_var_ndims, map_info->coord_var_name );
				break;
				}
			}
		/* Now, since we've found the index of the lon dim, the
		 * index of the lat dim must be the other one
		 */
		for( i=0; i<v->n_dims; i++ ) 
			if( (map_info->matching_var_dims[i] == 1) && (i != idx_lon_dim))
				idx_lat_dim = i;
		}
	else if( is_lat ) {
		if( options.debug ) printf( "Coord var was found to be a LATITUDE\n" );
		/* Match this coord var to the first one on the left */
		for( i=0; i<v->n_dims; i++ ) {
			if( map_info->matching_var_dims[i] == 1 ) {
				idx_lat_dim = i;
				v->dim_map_info[i] = map_info;
				if( options.debug )
					printf( "In variable \"%s\", dimension \"%s\" is mapped by LATITUDE-like dimension %d-dimensional variable \"%s\"\n",
					v->name, netcdf_dim_id_to_name( v->first_file->id, v->name, i),
					map_info->coord_var_ndims, map_info->coord_var_name );
				break;
				}
			}
		/* Now, since we've found the index of the lat dim, the
		 * index of the lon dim must be the other one
		 */
		for( i=0; i<v->n_dims; i++ ) 
			if( (map_info->matching_var_dims[i] == 1) && (i != idx_lat_dim))
				idx_lon_dim = i;
		}
	else
		{
		fprintf( stderr, "(Location C)Error, did not correctly match mapped dims specified in the coordinates attribute to dims in the variable\n" );
		fprintf( stderr, "(Location C)Please send email to dpierce@ucsd.edu letting me know what your coordinates attribute looks like so I can fix this problem.\n" );
		exit( -1 );
		}

	/* Read in data from var, store it in cache */
	totsize = 1L;
	for( i=0; i<map_info->coord_var_ndims; i++ ) {
		totsize *= map_info->coord_var_size[i];
		start[i] = 0L;
		count[i] = map_info->coord_var_size[i];
		}
	map_info->data_cache = (float *)malloc( totsize*sizeof(float) );
	if( map_info->data_cache == NULL ) {
		fprintf( stderr, "Error, could not allocate cache for dim map variable %s; total size (bytes): %ld\n", 
			map_info->coord_var_name, totsize*sizeof(float) );
		exit(-1);
		}
	netcdf_fi_get_data( ncid, map_info->coord_var_name, start, count, map_info->data_cache, NULL );

	if( n_matches == 1 ) {
		if( idx_lon_dim == -1 )
			map_info->index_place_factor[idx_lat_dim] = 1L;
		else
			map_info->index_place_factor[idx_lon_dim] = 1L;
		}
	else if( n_matches == 2 ) {
		map_info->index_place_factor[idx_lon_dim] = 1L;
		map_info->index_place_factor[idx_lat_dim] = v->size[ idx_lon_dim ];
		}
	else
		{
		fprintf( stderr, "(Location D)Error, did not correctly match mapped dims specified in the coordinates attribute to dims in the variable\n" );
		fprintf( stderr, "(Location D)Please send email to dpierce@ucsd.edu letting me know what your coordinates attribute looks like so I can fix this problem.\n" );
		exit( -1 );
		}

	/*
	printf( "matching var dims: " );
	for( i=0; i<v->n_dims; i++ )
		printf( "%d ", map_info->matching_var_dims[i] );
	printf( "Index place factor: " );
	for( i=0; i<v->n_dims; i++ )
		printf( "%ld ", map_info->index_place_factor[i] );
	printf( "\n" );
	*/
}

/******************************************************************************
 * Initialize all the fields in the dim structure by reading from the data file
 */
	void
fill_dim_structs( NCVar *v )
{
	int	i, fileid, debug;
	NCDim	*d;
	char	*dim_name, *tmp_units;
	static  int global_id = 0;
	FDBlist	*cursor;

	debug = 0;

	if( debug == 1 ) printf( "fill_dim_structs: entering for var %s, which has %d dims\n", v->name, v->n_dims );

	fileid = v->first_file->id;
	v->dim = (NCDim **)malloc( v->n_dims*sizeof( NCDim *));
	for( i=0; i<v->n_dims; i++ ) {
		dim_name = fi_dim_id_to_name( fileid, v->name, i );
		if( debug == 1 ) printf( "fill_dim_structs: dim %d has name %s and length %ld\n", i, dim_name, v->size[i] );
		if( is_scannable( v, i ) ) {
			*(v->dim + i)	= (NCDim *)malloc( sizeof( NCDim ));
			d            	= *(v->dim+i);
			d->name      	= dim_name;
			d->long_name 	= fi_dim_longname( fileid, dim_name );
			d->have_calc_minmax = 0;
			d->units     	= fi_dim_units   ( fileid, dim_name );
			d->units_change = 0;
			d->size      	= *(v->size+i);
			d->calendar  	= fi_dim_calendar( fileid, dim_name );
			d->global_id 	= ++global_id;
			handle_time_dim( fileid, v, i );
			if( options.debug ) 
				fprintf( stderr, "adding scannable dim to var %s: dimname: %s dimsize: %ld\n", v->name, dim_name, d->size );
			}
		else
			{
			/* Indicate non-scannable dimensions by a NULL */
			*(v->dim + i) = NULL;
			if( options.debug ) 
				fprintf( stderr, "adding non-scannable dim to var %s: dim name: %s size: %ld\n", 
					v->name, fi_dim_id_to_name( fileid, v->name, i), *(v->size+i) );
			}
		}

	/* If this variable lives in more than one file, it might have 
	 * different time units in each one.  Check for this.
	 */
	if( v->is_virtual && (*(v->dim) != NULL) && (v->first_file->next != NULL) ) {
		/* The timelike dimension MUST be the first one! */
		d = *(v->dim+0);
		if( d->timelike ) {
			/* Go through each file and see if it has the same units
			 * as the first file, which is stored in d->units 
			 */
			cursor = v->first_file->next;
			while( cursor != NULL ) {
				tmp_units = fi_dim_units( cursor->id, d->name );
				if( strcmp( d->units, tmp_units ) != 0 ) {
					printf( "** Warning: different time units found in different files.  Trying to compensate...\n" );
					d->units_change = 1;
					}
				}
			}
		}
}

/******************************************************************************
 * Is this a "scannable" dimension -- i.e., accessable by the taperecorder
 * style buttons? Is scannable if: 
 * 	> is unlimited
 *	> or, is size > 1
 */
	int
is_scannable( NCVar *v, int i )
{
	/* The unlimited record dimension is always scannable */
	if( i == 0 )
		return( TRUE );

	if( *(v->size+i) > 1 )
		return( TRUE );
	else
		return( FALSE );
}

/******************************************************************************
 * Return the mode (most common value) of passed array "x".  We assume "x"
 * contains the floating point representation of integers.
 */
	float
util_mode( float *x, size_t n, float fill_value )
{
	long 	i, n_vals;
	double 	sum;
	long 	ival, *count_vals, max_count, *unique_vals;
	int	foundval, j, max_index;
	float	retval;

	count_vals  = (long *)malloc( n*sizeof(long) );
	unique_vals = (long *)malloc( n*sizeof(long) );

	sum = 0.0;
	n_vals = 0;
	for( i=0L; i<n; i++ ) {
		if( close_enough( x[i], fill_value )) {
			free(count_vals);
			free(unique_vals);
			return( fill_value );
			}
		ival = (x[i] > 0.) ? (long)(x[i]+.4) : (long)(x[i]-.4); /* round x[i] to nearest integer */
		foundval = -1;
		for( j=0; j<n_vals; j++ ) {
			if( unique_vals[j] == ival ) {
				foundval = j;
				break;
				}
			}
		if( foundval == -1 ) {
			unique_vals[n_vals] = ival;
			count_vals[n_vals] = 1;
			n_vals++;
			}
		else
			count_vals[foundval]++;
		}

	max_count = -1;
	max_index = -1;
	for( i=0L; i<n_vals; i++ )
		if( count_vals[i] > max_count ) {
			max_count = count_vals[i];
			max_index = i;
			}

	retval = (float)unique_vals[max_index];

	free(count_vals);
	free(unique_vals);

	return( retval );
}

/******************************************************************************/
	float
util_mean( float *x, size_t n, float fill_value )
{
	long i;
	double sum;

	sum = 0.0;
	for( i=0L; i<n; i++ ) {
		if( close_enough( x[i], fill_value ))
			return( fill_value );
		sum += x[i];
		}

	sum = sum / (double)n;
	return( sum );
}

/******************************************************************************/
	int
equivalent_FDBs( NCVar *v1, NCVar *v2 )
{
	FDBlist *f1, *f2;

	f1 = v1->first_file;
	f2 = v2->first_file;
	while( f1 != NULL ) {
		if( f2 == NULL ) {
			return(0); /* files differ */
			}
		if( f1->id != f2->id ) {
			return(0); /* files differ */
			}
		f1 = f1->next;
		f2 = f2->next;
		}

	/* Here f1 == NULL */
	if( f2 != NULL ) {
		return(0); /* files differ */
		}

	return(1);
}

/******************************************************************************/
	void
copy_info_to_identical_dims( NCVar *vsrc, NCDim *dsrc, size_t dim_len )
{
	NCVar	*v;
	int	i, dims_are_same;
	NCDim	*d;
	size_t	j;

	v = variables;
	while( v != NULL ) {
		for( i=0; i<v->n_dims; i++ ) {
			d = *(v->dim+i);
			if( (d != NULL) && (d->have_calc_minmax == 0)) {
				/* See if this dim is same as passed dim */
				dims_are_same = (strcmp( dsrc->name, d->name ) == 0 ) &&
						equivalent_FDBs( vsrc, v );
				if( dims_are_same ) {
					if( options.debug ) 
						fprintf( stderr, "Dim %s (%d) is same as dim %s (%d), copying min&max from former to latter...\n", dsrc->name, dsrc->global_id, d->name, d->global_id );
					d->min = dsrc->min;
					d->max = dsrc->max;
					d->have_calc_minmax = 1;
					d->values = (float *)malloc(dim_len*sizeof(float));
					for( j=0L; j<dim_len; j++ )
						*(d->values + j) = *(dsrc->values + j);
					d->is_lat = dsrc->is_lat;
					d->is_lon = dsrc->is_lon;
					}
				}
			}
		v = v->next;
		}
}

/******************************************************************************
 * Calculate the minimum and maximum values in the dimension structs.  While
 * we are messing with the dims, we also try to determine if they are lat and
 * lon.
 */
	void
calc_dim_minmaxes( void )
{
	int	i, j;
	NCVar	*v;
	NCDim	*d;
	char	temp_str[1024];
	nc_type	type;
	double	temp_double, bounds_max, bounds_min;
	int	has_bounds, name_lat, name_lon, units_lat, units_lon;
	size_t	dim_len;
	size_t	cursor_place[MAX_NC_DIMS];

	v = variables;
	while( v != NULL ) {
		for( i=0; i<v->n_dims; i++ ) {
			d = *(v->dim+i);
			if( (d != NULL) && (d->have_calc_minmax == 0)) {
				if( options.debug ) 
					fprintf( stderr, "...min & maxes for dim %s (%d)...\n", d->name, d->global_id );
				dim_len = *(v->size+i);
				d->values = (float *)malloc(dim_len*sizeof(float));

				for( j=0; j<v->n_dims; j++ ) 
					cursor_place[j] = (int)(*(v->size+j)/2.0);	/* take middle in case 2-d mapped dims apply */

				type = fi_dim_value( v, i, 0L, &temp_double, temp_str, &has_bounds, &bounds_min, 
								&bounds_max, cursor_place );	/* used to get type ONLY */
				if( type == NC_DOUBLE ) {
					for( j=0; j<dim_len; j++ ) {
						cursor_place[i] = j;
						type = fi_dim_value( v, i, j, &temp_double, temp_str, &has_bounds, &bounds_min, &bounds_max, cursor_place );
						*(d->values+j) = (float)temp_double;
						}
					d->min  = *(d->values);
					d->max  = *(d->values + dim_len - 1);
					}
				else
					{
					if( options.debug ) 
						fprintf( stderr, "**Note: non-float dim found; i=%d\n", i );
					d->min  = 1.0;
					d->max  = (float)dim_len;
					for( j=0; j<dim_len; j++ )
						*(d->values+j) = (float)j;
					}
				d->have_calc_minmax = 1;
				
				/* Try to see if the dim is a lat or lon.  Not an exact science by a long shot */
				name_lat  = strncmp_nocase(d->name,  "lat",    3)==0;
				units_lat = strncmp_nocase(d->units, "degree", 6) == 0;
				name_lon  = strncmp_nocase(d->name,  "lon",    3)==0;
				units_lon = strncmp_nocase(d->units, "degree", 6) == 0;
				d->is_lat = ((name_lat || units_lat) && (d->max <  90.01) && (d->min > -90.01));
				d->is_lon = ((name_lon || units_lon) && (d->max < 360.01) && (d->min > -180.01));

				/* There is a funny thing we need to do at this point.  Think about the following case.
				 * We want to look at 3 different files, and they all have a dim named 'lon' in them,
				 * and each is different.  Because this might happen, we can't use the name as an
				 * indication of a unique dimension.  On the other hand, it is very slow to repeatedly
				 * reprocess the same dim over and over, especially if it's the time dim in a series
				 * of virtually concatenated input files.  For that reason, we copy the min and max
				 * values we just found to all identical dims.
				 */
				copy_info_to_identical_dims( v, d, dim_len );
				}
			}
		v = v->next;
		}
}
	
/********************************************************************************
 * Actually do the "shrinking" of the FLOATING POINT (not pixel) data, converting 
 * it to the small version by either finding the most common value in the square,
 * or by averaging over the square.  Remember that our standard for how to 
 * interpret 'options.blowup' is that a value of "-N" means to shrink by a factor
 * of N.  So, blowup == -2 means make it half size, -3 means 1/3 size, etc.
 */
	void
contract_data( float *small_data, View *v, float fill_value )
{
	long 	i, j, n, nx, ny, ii, jj;
	size_t	new_nx, new_ny, idx, ioffset, joffset;
	float 	*tmpv;

	if( options.blowup > 0 ) {
		fprintf( stderr, "internal error, contract_data called with a positive blowup factor!\n" );
		exit(-1);
		}

	n = -options.blowup;
	tmpv = (float *)malloc( n*n * sizeof(float) );
	if( tmpv == NULL ) {
		fprintf( stderr, "internal error, failed to allocate array for calculating reduced means\n" );
		exit( -1 );
		}

	/* Get old and new sizes (new size is smaller in this routine) */
	nx   = *(v->variable->size + v->x_axis_id);
	ny   = *(v->variable->size + v->y_axis_id);
	view_get_scaled_size( options.blowup, nx, ny, &new_nx, &new_ny );

	for( j=0; j<new_ny; j++ )
	for( i=0; i<new_nx; i++ ) {
		for( jj=0; jj<n; jj++ )
		for( ii=0; ii<n; ii++ ) {
			ioffset = i*n + ii;
			joffset = j*n + jj;
			if( ioffset >= nx )
				ioffset = nx-1;
			if( joffset >= ny )
				joffset = ny-1;
			idx = ioffset + joffset*nx;
			tmpv[ii + jj*n] = *((float *)v->data + idx);
			}

		if( options.shrink_method == SHRINK_METHOD_MEAN )
			small_data[i + j*new_nx] = util_mean( tmpv, n*n, fill_value );

		else if( options.shrink_method == SHRINK_METHOD_MODE ) {
			small_data[i + j*new_nx] = util_mode( tmpv, n*n, fill_value );
			}
		else
			{
			fprintf( stderr, "Error in contract_data: unknown value of options.shrink_method!\n" );
			exit( -1 );
			}
		}


	free(tmpv);
}

/******************************************************************************
 * Actually do the "blowup" of the FLOATING POINT (not pixel) data, converting 
 * it to the large version by either interpolation or replication.
 * NOTE this routine is only called when options.blowup > 0!
 */
	void
expand_data( float *big_data, View *v, size_t array_size )
{
	size_t	idx, nxl, nyl, nxb, nyb;
	long	line, il, jl, i2b, j2b;
	int	blowup, offset_xb, offset_yb, miss_base, miss_right, miss_below;
	float	step, final_est, extrap_fact;
	float	base_val, right_val, below_val, val, bupr;
	float	base_x, base_y, del_x, del_y;
	float	est1, est2, frac_x, frac_y;
	float 	fill_val, cval;

	blowup   = options.blowup;

#ifdef CHECK_MEM
	printf( "...CHECK_MEM is on!!\n" );
#endif

	/*--------------------------------------------------------------------------------
	 * See my notebook entry of 2010-08-23. 
	 * In general we draw a distinction between indices that are valid in the
	 * original (little) array, indicazted by a "l" (little) suffix (such as il or jl),
	 * and indices valid in the destination (big) array, which have a suffix of "b".
	 *---------------------------------------------------------------------------------*/
	nxl = *(v->variable->size + v->x_axis_id);	/* # of X entries in the little array */
	nyl = *(v->variable->size + v->y_axis_id);	/* # of Y entries in the little array */
	nxb = nxl*blowup;				/* # of X entries in big array */
	nyb = nyl*blowup;				/* # of Y entries in big array */

	fill_val = v->variable->fill_value;
	
	if( (nxb < blowup) || (nxb*nyb < blowup) ) {
		fprintf( stderr, "ncview: data_to_pixels: too much magnification\n" );
		fprintf( stderr, "nxb=%ld\n", nxb );
		exit( -1 );
		}

	if( (blowup == 1) || (options.blowup_type == BLOWUP_REPLICATE)) { 
		for( jl=0; jl<nyl; jl++ ) {
			for( il=0; il<nxl; il++ )
				for( i2b=0; i2b<blowup; i2b++ ) {
#ifdef CHECK_MEM
					if( il*blowup + jl*nxb*blowup + i2b >= array_size ) { fprintf( stderr, "mem error 001\n" ); exit(-1); }
#endif
					*(big_data + il*blowup + jl*nxb*blowup + i2b) = *((float *)((float *)v->data)+il+jl*nxl);
					}
			for( line=1; line<blowup; line++ )
				for( i2b=0; i2b<nxb; i2b++ ) {
#ifdef CHECK_MEM
					if( i2b + jl*nxb*blowup + line*nxb >= array_size ) { fprintf( stderr, "mem error 002\n" ); exit(-1); }
#endif
					*(big_data + i2b + jl*nxb*blowup + line*nxb) =
						*(big_data + i2b + jl*nxb*blowup);
					}
			}
		} 

	else 	{ /* BLOWUP_BILINEAR */
		bupr = 1.0/(float)blowup;

		/* Offset where we will put the center value into the big array. These are offsets
		 * into the big array.
		 */
		offset_xb = (blowup - 1)/2;
		offset_yb = offset_xb;

		/* Horizontal base lines */
		for( jl=0; jl<nyl; jl++ ) {
			for( il=0; il<nxl-1; il++ ) {
				base_val  = *((float *)v->data + il   + jl*nxl);
				right_val = *((float *)v->data + il+1 + jl*nxl);

				miss_base  = close_enough(base_val,  fill_val);
				miss_right = close_enough(right_val, fill_val);
				if( miss_base ) {
					if( miss_right ) {
						/* BOTH missing */
						step = 0.0;
						val = base_val;		/* missing value */
						}
					else
						{
						/* base missing, but right is there */
						step = 0.0;
						val = right_val;	/* an OK value */
						}
					}
				else if( miss_right ) {
					/* ONLY right is missing, checked for both missing above */
					val = base_val;
					step = 0.0;
					}
				else
					{
					/* NEITHER missing */
					step = (right_val-base_val)*bupr;
					val = base_val;
					}

				for( i2b=0; i2b < blowup; i2b++ ) {
#ifdef CHECK_MEM
					if( il*blowup+i2b+offset_xb + jl*blowup*nxb + offset_yb*nxb >= array_size ) { fprintf( stderr, "mem error 003\n" ); exit(-1); }
#endif
					*(big_data + il*blowup+i2b+offset_xb + jl*blowup*nxb + offset_yb*nxb ) = val;
					val += step;
					}
				}
			/* Fill in the last center value on the right, which was left unfilled by the above alg */
#ifdef CHECK_MEM
			if( (nxl-1)*blowup+offset_xb + jl*blowup*nxb + offset_yb*nxb >= array_size ) { fprintf( stderr, "mem error 004\n" ); exit(-1); }
#endif
			*(big_data + (nxl-1)*blowup+offset_xb + jl*blowup*nxb + offset_yb*nxb ) = *((float *)v->data + (nxl-1) + jl*nxl);
			}

		/* Vertical base lines */
		for( jl=0; jl<nyl-1; jl++ ) 
		for( il=0; il<nxl;   il++ ) {
			base_val  = *((float *)v->data + il + jl*nxl);
			below_val = *((float *)v->data + il + (jl+1)*nxl);

			miss_base  = close_enough(base_val,  fill_val);
			miss_below = close_enough(below_val, fill_val);

			if( miss_base ) {
				if( miss_below ) {
					/* BOTH missing */
					step = 0.0;
					val = base_val;		/* missing value */
					}
				else
					{
					/* base missing, but below is there */
					step = 0.0;
					val = below_val;	/* an OK value */
					}
				}
			else if( miss_below ) {
				/* ONLY below is missing, checked for both missing above */
				val = base_val;
				step = 0.0;
				}
			else
				{
				/* NEITHER missing */
				step = (below_val-base_val)*bupr;
				val = base_val;
				}

			for( j2b=0; j2b < blowup; j2b++ ) {
#ifdef CHECK_MEM
			if( il*blowup+offset_xb + jl*blowup*nxb + (j2b+offset_yb)*nxb >= array_size ) { fprintf( stderr, "mem error 005\n" ); exit(-1); }
#endif
				*(big_data + il*blowup+offset_xb + jl*blowup*nxb + (j2b+offset_yb)*nxb ) = val;
				val += step;
				}
			}
		/* Fill in the last center value along the top, which was left unfilled by the above alg */
		for( il=0; il<nxl; il++ ) {
#ifdef CHECK_MEM
			if( il*blowup+offset_xb + (nyl-1)*blowup*nxb + offset_yb*nxb >= array_size ) { fprintf( stderr, "mem error 006\n" ); exit(-1); }
#endif
			*(big_data + il*blowup+offset_xb + (nyl-1)*blowup*nxb + offset_yb*nxb) = *((float *)v->data + il + (nyl-1)*nxl);
			}

		/* Now, fill in the interior of the interior squares by 
		 * interpolating from the horizontal and vertical
		 * base lines.
		 */
		for( jl=0; jl<nyl-1; jl++ )
		for( il=0; il<nxl-1; il++ ) {
			for( j2b=1; j2b<blowup; j2b++ )
			for( i2b=1; i2b<blowup; i2b++ ) {
				frac_x = (float)i2b*bupr;
				frac_y = (float)j2b*bupr;

				base_x    = *(big_data +  il   *blowup+offset_xb + jl*blowup*nxb +(j2b+offset_yb)*nxb);
				right_val = *(big_data + (il+1)*blowup+offset_xb + jl*blowup*nxb+ (j2b+offset_yb)*nxb);
				base_y    = *(big_data + il*blowup+i2b+offset_xb +  jl   *blowup*nxb + offset_yb*nxb);
				below_val = *(big_data + il*blowup+i2b+offset_xb + (jl+1)*blowup*nxb + offset_yb*nxb);

				if( close_enough(base_x,    fill_val) || 
				    close_enough(right_val, fill_val) || 
				    (il == nxl-1) )
					del_x = 0.0;
				else
					del_x  = right_val - base_x;
				if( close_enough(base_y,    fill_val) || 
				    close_enough(below_val, fill_val) || 
				    (jl == nyl-1) )
					del_y = 0.0;
				else
					del_y  = below_val - base_y;
				est1 = frac_x*del_x + base_x;
				est2 = frac_y*del_y + base_y;

				if( close_enough( est1, fill_val )) {
					if( close_enough( est2, fill_val ))
						final_est = fill_val;
					else
						final_est = est2;
					}
				else if( close_enough( est2, fill_val ))
					final_est = est1;
				else
					final_est = (est1 + est2)*.5;

#ifdef CHECK_MEM
				if( il*blowup+i2b+offset_xb + jl*blowup*nxb + (j2b+offset_yb)*nxb >= array_size ) { fprintf( stderr, "mem error 007\n" ); exit(-1); }
#endif
				*(big_data + il*blowup+i2b+offset_xb + jl*blowup*nxb + (j2b+offset_yb)*nxb ) = final_est;
				}
			}

		/* It is a tricky and undetermined question as to whether we want to allow
		 * extrema on the boundaries.  As a complete and total hack, we use only 
		 * some fraction of the linear projection when extrapolating out to the 
		 * edges.  If this is set to 1, then full linear extrapolation is used;
		 * if set to 0, no extrapolation is done.
		 */
		extrap_fact = 0.2;

		/* Fill in right hand side by extrapolating the gradient from the interior square fill.
		 * This goes from y=the first center point to y=the last center point.
		 */
		il = nxl-1;
		for( j2b=0; j2b<=blowup*(nyl-1); j2b++ ) {
			idx = il*blowup+offset_xb + (j2b+offset_yb)*nxb;	
			step = (*(big_data + idx - 1) - *(big_data + idx - 2));
			val  = *(big_data + idx) + step;
			for( i2b=1; i2b<(blowup-offset_xb+1); i2b++ ) {
#ifdef CHECK_MEM
				if( idx + i2b >= array_size ) { fprintf( stderr, "mem error 008\n" ); exit(-1); }
#endif
				*(big_data + idx + i2b) = val;
				val += step*extrap_fact;
				}
			}

		/* Fill in left hand side */
		il = 0;
		for( j2b=0; j2b<=blowup*(nyl-1); j2b++ ) {
			idx = il*blowup+offset_xb + (j2b+offset_yb)*nxb;
			step = (*(big_data + idx + 2) - *(big_data + idx + 1));
			val  = *(big_data + idx) - step;
			for( i2b=1; i2b<=(blowup-1)/2; i2b++ ) {
#ifdef CHECK_MEM
				if( idx - i2b >= array_size ) { fprintf( stderr, "mem error 009\n" ); exit(-1); }
#endif
				*(big_data + idx - i2b) = val;
				val -= step*extrap_fact;
				}
			}

		/* Fill in bottom */
		jl = 0;
		for( i2b=0; i2b<=blowup*(nxl-1); i2b++ ) {
			idx = i2b+offset_xb + jl*blowup*nxb + offset_yb*nxb;
			step = (*(big_data + idx + 2*nxb) - *(big_data + idx + nxb));   /* big(,y+2) - big(,y+1) */
			val  = *(big_data + idx) - step;
			for( j2b=1; j2b<=(blowup-1)/2; j2b++ ) {
#ifdef CHECK_MEM
				if( idx - j2b*nxb >= array_size ) { fprintf( stderr, "mem error 010\n" ); exit(-1); }
#endif
				*(big_data + idx - j2b*nxb) = val;
				val -= step*extrap_fact;
				}
			}

		/* Fill in top */
		jl = nyl-1;
		for( i2b=0; i2b<blowup*(nxl-1); i2b++ ) {
			idx = i2b+offset_xb + jl*blowup*nxb + offset_yb*nxb;
			step = (*(big_data + idx - nxb) - *(big_data + idx - 2*nxb));  /* big(,y-1) - big(,y-2) */
			val  = *(big_data + idx) + step;
			for( j2b=1; j2b<=blowup/2; j2b++ ) {
#ifdef CHECK_MEM
				if( idx + j2b*nxb >= array_size ) { fprintf( stderr, "mem error 011\n" ); exit(-1); }
#endif
				*(big_data + idx + j2b*nxb) = val;
				val += step*extrap_fact;
				}
			}

		/* Still have to fill in the four corners at this point.   Because of the
		 * extrapolation issue noted above, we take a simple approach.  Just fill
		 * in the corner blocks with the center data value.
		 */

		/* Lower left corner */
		il = 0;
		jl = 0;
		cval = *((float *)v->data + il + jl*nxl);          /* Data value in lower left corner */
		if( ! close_enough( cval, fill_val )) {
			/* Fill in lower left corner */
			for( j2b=0; j2b<=offset_yb; j2b++ )
			for( i2b=0; i2b<=offset_xb; i2b++ ) {
#ifdef CHECK_MEM
				if( i2b + j2b*nxb >= array_size ) { fprintf( stderr, "mem error 012\n" ); exit(-1); }
#endif
				*(big_data + i2b + j2b*nxb) = cval;
				}
			}
			
		/* Lower right corner */
		il = nxl - 1;
		jl = 0;
		cval = *((float *)v->data + il + jl*nxl);          /* Data value in lower left corner */
		if( ! close_enough( cval, fill_val )) {
			/* Fill in lower right corner */
			for( j2b=0; j2b<=offset_yb; j2b++ )
			for( i2b=offset_xb; i2b<blowup; i2b++ ) {
#ifdef CHECK_MEM
				if( il*blowup + i2b + j2b*nxb >= array_size ) { fprintf( stderr, "mem error 013\n" ); exit(-1); }
#endif
				*(big_data + il*blowup + i2b + j2b*nxb) = cval;
				}
			}

		/* Upper right corner */
		il = nxl - 1;
		jl = nyl - 1;
		cval = *((float *)v->data + il + jl*nxl);          /* Data value in lower left corner */
		if( ! close_enough( cval, fill_val )) {
			/* Fill in upper right corner */
			for( j2b=offset_yb; j2b<blowup; j2b++ )
			for( i2b=offset_xb; i2b<blowup; i2b++ ) {
#ifdef CHECK_MEM
				if( il*blowup + i2b + jl*blowup*nxb + j2b*nxb >= array_size ) { fprintf( stderr, "mem error 014\n" ); exit(-1); }
#endif
				*(big_data + il*blowup + i2b + jl*blowup*nxb + j2b*nxb) = cval;
				}
			}

		/* Upper left corner */
		il = 0;
		jl = nyl - 1;
		cval = *((float *)v->data + il + jl*nxl);          /* Data value in lower left corner */
		if( ! close_enough( cval, fill_val )) {
			/* Fill in upper left corner */
			for( j2b=offset_yb; j2b<blowup; j2b++ )
			for( i2b=0; i2b<=offset_xb; i2b++ ) {
#ifdef CHECK_MEM
				if(  il*blowup + i2b + jl*blowup*nxb + j2b*nxb >= array_size ) { fprintf( stderr, "mem error 015\n" ); exit(-1); }
#endif
				*(big_data + il*blowup + i2b + jl*blowup*nxb + j2b*nxb) = cval;
				}
			}

		/* Paint missing value blocks */
		for( jl=0; jl<nyl; jl++ )
		for( il=0; il<nxl; il++ ) {
			base_val  = *((float *)v->data + il   + jl*nxl);
			if( close_enough( base_val, fill_val )) {
				for( j2b=0; j2b<blowup; j2b++ )
				for( i2b=0; i2b<blowup; i2b++ ) {
#ifdef CHECK_MEM
					if( il*blowup+i2b + jl*nxb*blowup + j2b*nxb >= array_size ) { fprintf( stderr, "mem error 016\n" ); exit(-1); }
#endif
					*(big_data + il*blowup+i2b + jl*nxb*blowup + j2b*nxb ) = base_val;
					}
				}
			}

		}	/* end of BLOWUP_BILINEAR case */
}

/******************************************************************************
 * Set the style of blowup we want to do.
 */
	void
set_blowup_type( int new_type )
{
	if( new_type == BLOWUP_REPLICATE ) 
		in_set_label( LABEL_BLOWUP_TYPE, "Repl"   );
	else
		in_set_label( LABEL_BLOWUP_TYPE, "Bi-lin" );

	options.blowup_type = new_type;
}

/******************************************************************************
 * If we allowed strings of arbitrary length, some of the widgets
 * would crash when trying to display them.
 */
	char *
limit_string( char *s )
{
	int	i;

	i = strlen(s)-1;
	while( *(s+i) == ' ' )
		i--;
	*(s+i+1) = '\0';

	if( strlen(s) > MAX_DISPLAYED_STRING_LENGTH )
		*(s+MAX_DISPLAYED_STRING_LENGTH) = '\0';

	return( s );
}

/******************************************************************************
 * If we try to print to an already existing file, then warn the user
 * before clobbering it.
 */
	int
warn_if_file_exits( char *fname )
{
	int	retval;
	FILE	*f;
	char	message[1024];

	if( (f = fopen(fname, "r")) == NULL )
		return( MESSAGE_OK );
	fclose(f);

	snprintf( message, 1022, "OK to overwrite existing file %s?\n", fname );
	retval = in_dialog( message, NULL, TRUE );
	return( retval );
}

/******************************************************************************/

	static void
handle_time_dim( int fileid, NCVar *v, int dimid )
{
	NCDim   *d;

	d = *(v->dim+dimid);

	if( udu_utistime( d->name, d->units ) ) {
		d->timelike = 1;
		d->time_std = TSTD_UDUNITS;
		d->tgran    = udu_calc_tgran( fileid, v, dimid );
		}
	else if( epic_istime0( fileid, v, d )) {
		d->timelike = 1;
		d->time_std = TSTD_EPIC_0;
		d->tgran    = epic_calc_tgran( fileid, d );
		}
	else if( (d->units != NULL) &&
		 (strlen(d->units) >= 5) &&
		 (strncasecmp( d->units, "month", 5 ) == 0 ))  {
		d->timelike = 1;
		d->time_std = TSTD_MONTHS;
		d->tgran    = months_calc_tgran( fileid, d );
		}
	else
		d->timelike = 0;
}

/******************************************************************************/
	static int
months_calc_tgran( int fileid, NCDim *d )
{
	char	temp_string[128];
	float	delta, v0, v1;
	int	type, has_bounds;
	double	temp_double, bounds_min, bounds_max;

	if( d->size < 2 ) {
		return( TGRAN_DAY );
		}

	type = netcdf_dim_value( fileid, d->name, 0L, &temp_double, temp_string, 0L, &has_bounds, &bounds_min, &bounds_max );
	if( type == NC_DOUBLE )
		v0 = (float)temp_double;
	else
		{
		fprintf( stderr, "Note: can't calculate time granularity, unrecognized timevar type (%d)\n",
			type );
		return( TGRAN_DAY );
		}

	type = netcdf_dim_value( fileid, d->name, 1L, &temp_double, temp_string, 1L, &has_bounds, &bounds_min, &bounds_max );
	if( type == NC_DOUBLE )
		v1 = (float)temp_double;
	else
		{
		fprintf( stderr, "Note: can't calculate time granularity, unrecognized timevar type (%d)\n",
			type );
		return( TGRAN_DAY );
		}
	
	delta = v1 - v0;

	if( delta > 11.5 ) 
		return( TGRAN_YEAR );
	if( delta > .95 ) 
		return( TGRAN_MONTH );
	if( delta > .03 ) 
		return( TGRAN_DAY );

	return( TGRAN_MIN );
}

/******************************************************************************/
void fmt_time( char *temp_string, size_t temp_string_len, double new_dimval, NCDim *dim, int include_granularity )
{
	int 	year, month, day;

	if( ! dim->timelike ) {
		fprintf( stderr, "ncview: internal error: fmt_time called on non-timelike axis!\n");
		fprintf( stderr, "dim name: %s\n", dim->name );
		exit( -1 );
		}

	if( dim->time_std == TSTD_UDUNITS ) 
		udu_fmt_time( temp_string, temp_string_len, new_dimval, dim, include_granularity );

	else if( dim->time_std == TSTD_EPIC_0 ) 
		epic_fmt_time( temp_string, temp_string_len, new_dimval, dim );

	else if( dim->time_std == TSTD_MONTHS ) {
		/* Format for months standard */
		year  = (int)( (new_dimval-1.0) / 12.0 );
		month = (int)( (new_dimval-1.0) - year*12 + .01 );
		month = (month < 0) ? 0 : month;
		month = (month > 11) ? 11 : month;
		day   =
		   (int)( ((new_dimval-1.0) - year*12 - month) * 30.0) + 1;
		snprintf( temp_string, temp_string_len-1, "%s %2d %4d", month_name[month],
				day, year+1 );
		}

	else
		{
		fprintf( stderr, "Internal error: uncaught value of tim_std=%d\n", dim->time_std );
		exit( -1 );
		}
}

/*********************************************************************************************
 * like strncmp, but ignoring case
 */
	int
strncmp_nocase( char *s1, char *s2, size_t n )
{
	char 	*s1_lc, *s2_lc;
	int	i, retval;

	if( (s1==NULL) || (s2==NULL))
		return(-1);

	s1_lc = (char *)malloc(strlen(s1)+1);
	s2_lc = (char *)malloc(strlen(s2)+1);

	for( i=0; i<strlen(s1); i++ )
		s1_lc[i] = tolower(s1[i]);
	s1_lc[i] = '\0';
	for( i=0; i<strlen(s2); i++ )
		s2_lc[i] = tolower(s2[i]);
	s2_lc[i] = '\0';

	retval = strncmp( s1_lc, s2_lc, n );

	free(s1_lc);
	free(s2_lc);

	return(retval);
}

/**************************************************************************************************
 * Determine if the passed string names a lat or if the string names a lon.
 * If we figure out either lat or lon, returns 0 (success).
 * If we cannot figure either lat or lon, returns 1 (error).
 */
int determine_lat_lon( char *s_in, int *is_lat, int *is_lon )
{
	static  int have_given_warning = 0;
	char	*s;
	size_t	n, i;

	/* Get lower case version of input name */
	n = strlen(s_in);
	s = (char *)malloc( sizeof(char) * (n+2));

	for( i=0; i<n; i++ )
		s[i] = tolower( s_in[i] );

	*is_lat = 0;
	*is_lon = 0;

	if( strncasecmp( "lat", s, 3 ) == 0 ) {
		*is_lat = 1;
		free( s );
		return(0);
		}

	if( strncasecmp( "lon", s, 3 ) == 0 ) {
		*is_lon = 1;
		free( s );
		return(0);
		}

	if( strstr( s, "lat" ) != NULL ) {
		*is_lat = 1;
		free( s );
		return(0);
		}

	if( strstr( s, "lon" ) != NULL ) {
		*is_lon = 1;
		free( s );
		return(0);
		}

	if( (s[0] == 'x') || (s[0] == 'X') ) {
		*is_lon = 1;
		free( s );
		return(0);
		}

	if( (s[0] == 'y') || (s[0] == 'Y') ) {
		*is_lat = 1;
		free( s );
		return(0);
		}

	if( (have_given_warning == 0) && options.debug ) {
		have_given_warning = 1;
		fprintf( stderr, "Warning, cannot figure out whether coordinate variable \"%s\" is a latitude or a longitude, just based on its name\n", s_in );
		fprintf( stderr, "Please name it either Latitude or Longitude, as appropriate, or send email to dpierce@ucsd.edu if you have a case that does not fit this description so I can fix it.\n----------------\n" );
		}

	return(1);	/* error return */
}

/*******************************************************************************************
 * Returns the number of forward slashes in a string
 */
int count_nslashes( char *s ) 
{
	int	i, nslash;

	nslash = 0;
	for( i=0; i<strlen(s); i++ ) 
		if( s[i] == '/' )
			nslash++;

	return( nslash );
}

/*******************************************************************************************
 * Given a list of variables, this returns a stringlist of unique group names. If ANY var
 * lives in the root group, then the return list includes "/". If no var lives in the root
 * group, then the list does NOT include "/".
 */
Stringlist *get_group_list( NCVar *vars ) 
{
	Stringlist	*retval = NULL, *tg;
	NCVar		*cursor;
	char		group_name[ MAX_NC_NAME*20 ];	/* Assume no more than 20 levels of groups */
	int		i, ierr, n_so_far, foundit;

	cursor = vars;
	while( cursor != NULL ) {

		ierr = unpack_groupname( cursor->name, -1, group_name );	/* -1 means get full group name */

		/* Only add to list if not already there */
		n_so_far = stringlist_len( retval );
		tg = retval;
		foundit = 0;
		while( tg != NULL ) {
			if( strcmp( tg->string, group_name ) == 0 ) {
				foundit = 1;
				break;
				}
			tg = tg->next;
			}
		if( foundit == 0 )
			ierr = stringlist_add_string( &retval, group_name, NULL, SLTYPE_NULL );

		cursor = cursor->next;
		}

	return( retval );
}

/*******************************************************************************************
 * Given a varname string of format: groupname0/groupname1/groupnameN/varname
 *
 * and an integer ig: 0...N this returns groupname correspoinding to the integer ig
 * (NOTE: counting starts at 0, so if ig==0 then the first group name is returned)
 *
 * If ig == -1, then the full groupname without the varname is returned:
 * I.e., "groupname0/groupname1/groupnameN". If the var does NOT have any
 * forward slashes, it lives in the root group, and "/" is returned.
 *
 * If ig == -2, then ONLY the varname is returned. I.e., "varname"
 *
 * groupname must already be allocated upon entry
 *
 * Returns 0 on success, -1 on error
 */
int unpack_groupname( char *varname, int ig, char *groupname ) 
{
	int	i, i0, i1, idx_slash[MAX_NC_NAME], nslash;
	char	ts[MAX_NC_NAME];

	/* Get indices of the slashes */
	nslash = 0;
	for( i=0; i<strlen(varname); i++ ) {
		if( varname[i] == '/' ) {
			idx_slash[nslash] = i;
			nslash++;
			}
		}

	if( nslash == 0 ) {
		if (ig == -2 ) {
			/* Asked for varname only */
			strcpy( groupname, varname );
			return(0);
			}
		else
			{
			/* If no slashes in the var name, must live in root group */
			strcpy( groupname, "/" );
			return( 0 );
			}
		}

	if( ig > (nslash+1) ) {
		fprintf( stderr, "Error in unpack_groupname: varname: >%s< group to find (starting at 0)=%d invalid group to find (not this many groups in the varname)\n",
			varname, ig );
		exit(-1);
		}

	strcpy( ts, varname );

	if( ig == -2 ) {
		strcpy( groupname, ts+idx_slash[nslash-1]+1 );
		return( 0 );
		}

	if( ig == -1 ) {
		ts[ idx_slash[nslash-1] ] = '\0';
		strcpy( groupname, ts );
		return( 0 );
		}

	if( ig == 0 ) 
		i0 = 0;
	else
		i0 = idx_slash[ig-1] + 1;
	i1 = idx_slash[ig];
	ts[i1] = '\0';

	strcpy( groupname, ts+i0 );

	return( 0 );
}

