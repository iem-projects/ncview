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

/*****************************************************************************
 *
 *	The file interface to ncview.
 *
 *	All the routines in this file must be provided for whatever
 *	format data file you want to have.  Ideally, all the information 
 *    	about the data file formats should be encapsulated here.
 *
 *****************************************************************************/

#include "ncview.includes.h"
#include "ncview.defines.h"
#include "ncview.protos.h"

#ifdef HAVE_UDUNITS2
#include "utCalendar2_cal.h"
#endif

static int   file_type;
extern NCVar *variables;
extern Options options;

static void fi_get_data_iterate( NCVar *var, size_t *virt_start_pos, size_t *count, void *data );

/************************************************************************************/
/* return TRUE if passed the name of a file which these routines were designed
 * to read, and FALSE otherwise.
 */
	int
fi_confirm( char *name )
{
	return( netcdf_fi_confirm( name ));
}

/************************************************************************************/
/* return TRUE if the passed filename is writable, and false otherwise.
 * It is assumed that the file exists and is readable.
 */
	int
fi_writable( char *name )
{
	if( file_type != FILE_TYPE_NETCDF )
		{
		fprintf( stderr, "?unknown file_type passed to fi_writable: %d\n",
			file_type );
		exit( -1 );
		}
	return( netcdf_fi_writable( name ));
}

/************************************************************************************/
/* Do all file opening and initialization for the passed filename.
 * Return a unique integer ID by which this file will be indicated
 * in the future.  Passed arg 'nfiles' is total number of files that
 * were indicated on the command line; this can be used to make space
 * for some arrays.
 */
	int
fi_initialize( char *name, int nfiles )
{
	int	id;
	Stringlist *var_list;

	if( file_type == FILE_TYPE_NETCDF ) {
		if( options.debug ) 
			fprintf( stderr, "Initializing file %s\n", name );
		id = netcdf_fi_initialize( name );
		}
	else
		{
		fprintf( stderr, "?unknown file_type passed to fi_initialize: %d\n",
			file_type );
		exit( -1 );
		}

	if( options.debug ) 
		fprintf( stderr, "Getting list of variables for file %s\n", name );
	var_list = fi_list_vars( id );
	add_vars_to_list( var_list, id, name, nfiles );
	
	if( options.debug ) 
		fprintf( stderr, "Done initializing file %s\n", name );

	return( id );
}	

/************************************************************************************/
/* Return a list of the names of all the displayable variables in
 * the file.  Whether or not a variable is "displayable" is determined 
 * by the needs of the data, but in any event any displayable variable
 * must have at least 1 scannable dimension and be accessable by these 
 * routines. If the user wouldn't ever be interested in contouring some
 * variable, such as a dimension variable, it shouldn't appear on this list.
 */
	Stringlist *
fi_list_vars( int fileid )
{
	if( file_type != FILE_TYPE_NETCDF )
		{
		fprintf( stderr, "?unknown file_type passed to fi_list_vars: %d\n",
			file_type );
		exit( -1 );
		}
	return( netcdf_fi_list_vars( fileid ));
}

/************************************************************************************
 * Return the "title" of the file, if applicable.  Otherwise, return NULL.
 */
	char *
fi_title( int fileid )
{
	if( file_type != FILE_TYPE_NETCDF )
		{
		fprintf( stderr, "?unknown file_type passed to fi_title: %d\n",
			file_type );
		exit( -1 );
		}
	return( netcdf_title( fileid ));
}

/************************************************************************************
 * Return the 'long name' of a variable, if appropriate.  Otherwise, return NULL.
 */
	char *
fi_long_var_name( int fileid, char *var_name )
{
	if( file_type != FILE_TYPE_NETCDF )
		{
		fprintf( stderr, "?unknown file_type passed to fi_title: %d\n",
			file_type );
		exit( -1 );
		}
	return( netcdf_long_var_name( fileid, var_name ));
}

/************************************************************************************
 * Return the 'units' of a variable, if appropriate.  Otherwise, return NULL.
 */
	char *
fi_var_units( int fileid, char *var_name )
{
	if( file_type != FILE_TYPE_NETCDF )
		{
		fprintf( stderr, "?unknown file_type passed to fi_var_units: %d\n",
			file_type );
		exit( -1 );
		}
	return( netcdf_var_units( fileid, var_name ));
}

/************************************************************************************
 * Return the 'calendar' attribution of a dimension, if appropriate.  Otherwise, return NULL.
 */
	char *
fi_dim_calendar( int fileid, char *dim_name )
{
	/* Command line specified calendar OVERRIDES info in the file */
	if( options.calendar != NULL )
		return( options.calendar );

	if( file_type != FILE_TYPE_NETCDF )
		{
		fprintf( stderr, "?unknown file_type passed to fi_dim_calendar: %d\n",
			file_type );
		exit( -1 );
		}
	return( netcdf_dim_calendar( fileid, dim_name ));
}

/************************************************************************************
 * Return the 'units' of a dimension, if appropriate.  Otherwise, return NULL.
 */
	char *
fi_dim_units( int fileid, char *dim_name )
{
	if( file_type != FILE_TYPE_NETCDF )
		{
		fprintf( stderr, "?unknown file_type passed to fi_dim_units: %d\n",
			file_type );
		exit( -1 );
		}
	return( netcdf_dim_units( fileid, dim_name ));
}

/************************************************************************************/
/* Given a file id and the name of a variable, return the number of 
 * dimensions which that variable has.  This reads it directly from
 * the file, not using information in the 'variables' structure.
 */
	int
fi_n_dims( int id, char *var_name )
{
	if( file_type != FILE_TYPE_NETCDF )
		{
		fprintf( stderr, "?unknown file_type passed to fi_n_dims: %d\n",
			file_type );
		exit( -1 );
		}
	return( netcdf_fi_n_dims( id, var_name ));
}

/***********************************************************************************
 * Given a variable name, return a Stringlist of "scannable" dimensions for it.  The
 * definition of "scannable" dimension is rather loose; I'm using the assumption
 * that a scannable dimension must have a minimum number of points along it.
 * This is set in the routine netcdf_scannable_dims.
 */
	Stringlist *
fi_scannable_dims( int fileid, char *var_name )
{
	if( file_type != FILE_TYPE_NETCDF )
		{
		fprintf( stderr, "?unknown file_type passed to fi_scannable_dims: %d\n",
			file_type );
		exit( -1 );
		}
	return( netcdf_scannable_dims( fileid, var_name ));
}

/************************************************************************************
 * Given the file and the name of a variable in it, return an array
 * of the dimension sizes for that variable.  This reads the information
 * directly from the file, not from the 'variables' structure.
 */
	size_t *
fi_var_size( int fileid, char *var_name )
{
	if( file_type != FILE_TYPE_NETCDF )
		{
		fprintf( stderr, "?unknown file_type passed to fi_var_size: %d\n",
			file_type );
		exit( -1 );
		}
	return( netcdf_fi_var_size( fileid, var_name ));
}

/************************************************************************************
 * Given a dimension's id and the name of the variable who owns it,
 * return the name of the dimension.  'Id' here means the index into
 * the size_array of the owning variable.
 */
	char *
fi_dim_id_to_name( int fileid, char *var_name, int dim_id )
{
	if( file_type != FILE_TYPE_NETCDF )
		{
		fprintf( stderr, "?unknown file_type passed to fi_dim_id_to_name: %d\n",
			file_type );
		exit( -1 );
		}
	return( netcdf_dim_id_to_name( fileid, var_name, dim_id ));
}

/************************************************************************************
 * Given a dimension's name and the name of the variable who owns it,
 * return the index where that dimension appears in the size array
 * returned by 'fi_var_size'.  Return -1 if the dimension does not 
 * appear in the variable.
 */
	int
fi_dim_name_to_id( int fileid, char *var_name, char *dim_name )
{
	if( file_type != FILE_TYPE_NETCDF ) {
		fprintf( stderr, "?unknown file_type passed to fi_var_size: %d\n",
			file_type );
		exit( -1 );
		}
	return( netcdf_dim_name_to_id( fileid, var_name, dim_name ));
}

/************************************************************************************/
/* Fill out a pointer to the data for the passed variable from the
 * indicated file.  The data is assumed to be multi-dimensional
 * (it has to be at least 2 dimensions, else it isn't displayable!)
 * and starts at the position given by start_pos[0...N-1], with
 * counts of count[0...N-1].  The pointer MUST ALREADY point to 
 * storage large enough to hold the data!  Note that this routine
 * translates the 'start' place from a virtual location to an 
 * actual location for you, so you don't have to worry about that.
 * I.e., if you have a variable spread out over many files, you just
 * index it as if it were in one file and let the translation routine
 * take care of figuring out where it actually is.
 */
	void
fi_get_data( NCVar *var, size_t *virt_start_pos, size_t *count, void *data )
{
	size_t	*act_start_pos;
	FDBlist	*file;

	/* Check to see if we should loop over the timelike indices
	 */
	if( (var->is_virtual == TRUE) && (count[0] > 1) ) {
		fi_get_data_iterate( var, virt_start_pos, count, data );
		return;
		}
		
	act_start_pos = (size_t *)malloc(var->n_dims * sizeof(size_t));
	if( act_start_pos == NULL ) {
		fprintf( stderr, "error allocating space for act_start_pos\n" );
		fprintf( stderr, "in routine fi_get_data\n" );
		exit( -1 );
		}
	virt_to_actual_place( var, virt_start_pos, act_start_pos, &file );

	if( file_type == FILE_TYPE_NETCDF )
		netcdf_fi_get_data( file->id, var->name, act_start_pos, 
			  count, data, (NetCDFOptions *)var->first_file->aux_data );
	else
		{
		fprintf( stderr, "?unknown file_type passed to fi_get_data: %d\n",
			file_type );
		exit( -1 );
		}

	free( act_start_pos );
}

/*****************************************************************************
 * This is called when a variable lives in multiple files AND we
 * want data from more than one file.  We must iterate over the files.
 */
	void
fi_get_data_iterate( NCVar *var, size_t *virt_start_pos, size_t *count, void *data )
{
	size_t	it, *act_start_pos, start2[20], count2[20], prod_lower_dims;
	FDBlist	*file;
	int	i;

	act_start_pos = (size_t *)malloc(var->n_dims * sizeof(size_t));
	if( act_start_pos == NULL ) {
		fprintf( stderr, "error allocating space for act_start_pos\n" );
		fprintf( stderr, "in routine fi_get_data\n" );
		exit( -1 );
		}

	prod_lower_dims = 1L;
	for( i=1; i<var->n_dims; i++ ) {
		start2[i] = virt_start_pos[i];
		count2[i] = count[i];
		prod_lower_dims *= count[i];
		}

	count2[0] = 1L;
	for( it=virt_start_pos[0]; it<(virt_start_pos[0]+count[0]); it++ ) {
		start2[0] = it;
		virt_to_actual_place( var, start2, act_start_pos, &file );
		if( file_type == FILE_TYPE_NETCDF )
			netcdf_fi_get_data( file->id, var->name, act_start_pos, 
				  count2, ((float *)data)+it*prod_lower_dims, 
				  	(NetCDFOptions *)var->first_file->aux_data );
		else
			{
			fprintf( stderr, "?unknown file_type passed to fi_get_data: %d\n",
				file_type );
			exit( -1 );
			}
		}

	free( act_start_pos );
}

/************************************************************************************
 * Close the relevant file 
 */
	void
fi_close( int fileid )
{
	if( file_type == FILE_TYPE_NETCDF )
		netcdf_fi_close( fileid );
	else
		{
		fprintf( stderr, "?unknown file_type passed to fi_close: %d\n",
			file_type );
		exit( -1 );
		}
}

/*************************************************************************************
 * Does this dimension have a longname?  If so, return it.  Otherwise, NULL.
 */
	char *
fi_dim_longname( int fileid, char *dim_name )
{
	if( file_type != FILE_TYPE_NETCDF )
		{
		fprintf( stderr, "?unknown file_type passed to fi_has_dim_values: %d\n",
			file_type );
		exit( -1 );
		}
	return( netcdf_dim_longname( fileid, dim_name ) );
}

/**************************************************************************************
 * May ALTER the value of dimval if warranted!!
 */
void fi_dim_value_convert( double *dimval, FDBlist *file, NCVar *var, NCDim *d )
{
#ifdef HAVE_UDUNITS2
	double converted_dimval;
	int	year0, month0, hour0, min0, day0, err;
	double	sec0;

	if( (file->recdim_units 	   == NULL) ||
	    (var->first_file->recdim_units == NULL) ||
	    (var->first_file->ut_unit_ptr  == NULL) ||
	    (file->ut_unit_ptr 		   == NULL) ||
	    (! d->timelike )                        ||
	    (strcmp(file->recdim_units,var->first_file->recdim_units) == 0) ) 
	    	return;

	/* Convert the dim value to a date using the units given 
	 * in the file that this dim value came from
	 */
	err = utCalendar2_cal( *dimval, file->ut_unit_ptr, 
		&year0, &month0, &day0, &hour0, &min0, &sec0, d->calendar );
	if( err == 0 ) {
		err = utInvCalendar2_cal( year0, month0, day0, hour0, min0, sec0, 
			var->first_file->ut_unit_ptr, &converted_dimval,
			d->calendar );
		if( err == 0 ) 
			*dimval = converted_dimval;
		}
#endif
}

/*************************************************************************************
 * Return the value of a dimension at a specific point.  Returns the type
 * of the dimension value, which is either NC_DOUBLE or NC_CHAR.  Make sure
 * to allocate space for at least a 1024 character string in the return_value!
 * It will never be larger than that.  Takes a virtual place, and converts 
 * it to an actual place before determining the value.
 */
	nc_type
fi_dim_value( NCVar *var, int dim_id, size_t virt_place, double *return_val_double, 
	char *return_val_char, int *return_has_bounds, double *return_bounds_min, 
	double *return_bounds_max, size_t *complete_ndim_virt_place )
{
	size_t	actual_place, *virt_start_pos, *act_start_pos;
	FDBlist	*file;
	int	i;
	char	*dim_name;
	nc_type	ret_val;
	NCDim	*d;
	size_t	idx_map;
	NCDim_map_info	*dmi;

if(1==0){
printf( "Data cache vals for var %s:\n", var->name );
for( i=0; i<var->n_dims; i++ ) {
	printf( "Dim %d (%s): ", i, var->dim[i]->name );
	if( var->dim_map_info[i] == NULL ) 
		printf( "NULL\n" );
	else
		printf( "(%s) %f %f %f\n", 
			var->dim_map_info[i]->coord_var_name,
			var->dim_map_info[i]->data_cache[0], var->dim_map_info[i]->data_cache[10],
			var->dim_map_info[i]->data_cache[100] );
	}
}

	/* See if this dim value is actually 2-d mapped */
	dmi = var->dim_map_info[dim_id];
	if( dmi != NULL ) {
		/* It IS 2-d mapped, calculate entry in data cache where val is */
		idx_map = 0L;
		for( i=0; i<var->n_dims; i++ ) {
			idx_map += complete_ndim_virt_place[i] * dmi->index_place_factor[i];
/*printf( "dimidx=%d  place=%ld  factor=%ld  idx_so_far=%ld\n", i, complete_ndim_virt_place[i], dmi->index_place_factor[i], idx_map );*/
			}
		*return_val_double = dmi->data_cache[idx_map];
/*printf( "mapped, dim=%s loc=%ld  val=%lf\n", var->dim[dim_id]->name, idx_map, *return_val_double );*/
		return( NC_DOUBLE );
		}

	act_start_pos  = (size_t *)malloc(var->n_dims * sizeof(size_t));
	if( act_start_pos == NULL ) {
		fprintf( stderr, "error allocating space for act_start_pos\n" );
		fprintf( stderr, "in routine fi_dim_value\n" );
		exit( -1 );
		}
	virt_start_pos = (size_t *)malloc(var->n_dims * sizeof(size_t));
	if( virt_start_pos == NULL ) {
		fprintf( stderr, "error allocating space for virt_start_pos\n" );
		fprintf( stderr, "in routine fi_dim_value\n" );
		exit( -1 );
		}

	for( i=0; i<var->n_dims; i++ )
		*(virt_start_pos+i) = 0L;
	*(virt_start_pos+dim_id) = virt_place;

	virt_to_actual_place( var, virt_start_pos, act_start_pos, &file );

	actual_place = *(act_start_pos+dim_id);

	d = (*(var->dim+dim_id));
	dim_name  = d->name;
	if( file_type == FILE_TYPE_NETCDF )
		ret_val = netcdf_dim_value( file->id, dim_name, actual_place, 
				return_val_double, return_val_char, virt_place,
				return_has_bounds, return_bounds_min, return_bounds_max );
	else
		{
		fprintf( stderr, "?unknown file_type passed to fi_dim_value: %d\n",
			file_type );
		exit( -1 );
		}
	free( act_start_pos );
	free( virt_start_pos );

#ifdef HAVE_UDUNITS2
	/* Now we have to figure out if we need to change units on the
	 * returned value...This will happen with timelike dimensions that
	 * have a different units string in each file.
	 */
	if( ret_val != NC_CHAR) {
		fi_dim_value_convert( return_val_double, file, var, d );
		fi_dim_value_convert( return_bounds_min, file, var, d );
		fi_dim_value_convert( return_bounds_max, file, var, d );
		}
#endif

	return( ret_val );
}

/*************************************************************************************
 * Does this data file have *values* for the dimensions?
 */
	int
fi_has_dim_values( int fileid, char *dim_name )
{
	if( file_type != FILE_TYPE_NETCDF )
		{
		fprintf( stderr, "?unknown file_type passed to fi_has_dim_values: %d\n",
			file_type );
		exit( -1 );
		}
	return( netcdf_has_dim_values( fileid, dim_name ) );
}

/*************************************************************************************
 * File utility routines; things below this line shouldn't have to be changed 
 * for different data file formats.
 */

	void
determine_file_type( Stringlist *input_files )
{
	int		ierr;
	struct stat 	buf;

	if( input_files == NULL ) {
		fprintf( stderr, "ncview: takes at least one file name as argument\n" );
		useage();
		exit( -1 );
		}

	if( netcdf_fi_confirm( input_files->string ) )
		file_type = FILE_TYPE_NETCDF;
	else
		{
		ierr = stat( input_files->string, &buf );
		if( ierr == 0 ) {
			fprintf( stderr, "ncview: can't recognize format of input file %s\n",
				input_files->string );
			exit( -1 );
			}
		else
			{
			fprintf( stderr, "ncview: can't open file %s",
				input_files->string );
			perror(" ");
			exit( -1 );
			}
		}
}

/************************************************************************************
 * Fill out the data structure which holds information unique to each data file
 * format.
 */
	void
fi_fill_aux_data( int id, char *var_name, FDBlist *fdb )
{
	if( file_type == FILE_TYPE_NETCDF )
		netcdf_fill_aux_data( id, var_name, fdb );
	else
		{
		fprintf( stderr, "?unknown file_type passed to fi_has_dim_values: %d\n",
			file_type );
		exit( -1 );
		}
}

/*******************************************************************************
 * If the file format we are currently using defines a "fill value" (i.e.,
 * a special data value which indicates out-of-domain or never-written data)
 * then set the value to that fill value.  Otherwise, don't change it.
 */
	void
fi_fill_value( NCVar *var, float *fill_value )
{
	if( file_type == FILE_TYPE_NETCDF )
		netcdf_fill_value( var->first_file->id, var->name, 
				fill_value, (NetCDFOptions *)var->first_file->aux_data );
	else
		{
		fprintf( stderr, "?unknown file_type passed to fi_fill_value: %d\n",
			file_type );
		exit( -1 );
		}
}

	int 	
fi_recdim_id( int fileid )
{
	return( netcdf_fi_recdim_id( fileid ));
}
