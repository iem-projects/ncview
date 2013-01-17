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

/*
	This provides an interface to the udunits package, version 2
	(http://www.unidata.ucar.edu/packages/udunits/index.html)
	that can be bypassed if desired.
*/

#include "ncview.includes.h"
#include "ncview.defines.h"
#include "ncview.protos.h"
#include "math.h"

typedef struct {
	char	*name;
	void	*next;
} UniqList;

static int		valid_udunits_pkg;
static int		is_unique( char *units_name );
static UniqList 	*uniq = NULL;

/******************************************************************************/
#ifdef HAVE_UDUNITS2

ut_system	*unitsys;

#include "udunits2.h"
#include "utCalendar2_cal.h"

/******************************************************************************/
void udu_utinit( char *path )
{
	/* Turn annoying "override" errors off */
	ut_set_error_message_handler( ut_ignore );

	/* Init udunits-2 lib */
	unitsys = ut_read_xml( path );

	/* Turn errors back on */
	ut_set_error_message_handler( ut_write_to_stderr );

	if( unitsys != NULL )
		valid_udunits_pkg = 1;
	else
		{
		valid_udunits_pkg = 0;
		fprintf( stderr, "Note: Udunits-2 library could not be initialized; no units conversion will be attmpted.\n" );
		fprintf( stderr, "(To fix, put the path of the units file into environmental variable UDUNITS2_XML_PATH)\n");
		}
}

/******************************************************************************/
int udu_utistime( char *dimname, char *unitstr )
{
	int		ierr;
	static int	have_initted=0;
	ut_unit		*unit;
	static ut_unit	*time_unit_with_origin;

	if( (unitstr == NULL) || (!valid_udunits_pkg))
		return(0);

	if( (unit = ut_parse( unitsys, unitstr, UT_ASCII)) == NULL ) {
		/* can't parse unit spec */
		if( is_unique(unitstr) ) 
			fprintf( stderr, "Note: udunits: unknown units for %s: \"%s\"\n",
				dimname, unitstr );
		return( 0 );
		}

	if(!have_initted) {
		if( (time_unit_with_origin = ut_parse( unitsys, "seconds since 1901-01-01", UT_ASCII)) == NULL ) {
			fprintf( stderr, "Internal error: could not ut_parse time origin test string\n" );
			valid_udunits_pkg = 0;
			return(0);
			}
		have_initted = 1;
		}

	ierr = ut_are_convertible( unit, time_unit_with_origin );
	ut_free( unit );

	return( ierr != 0 );   /* Oddly, returns a non-zero number if ARE convertible */
}

/******************************************************************************/
int udu_calc_tgran( int fileid, NCVar *v, int dimid )
{
	ut_unit		*unit, *seconds;
	int		ii, retval;
	int		verbose, has_bounds;
	double		tval0_user, tval0_sec, tval1_user, tval1_sec, delta_sec, bound_min, bound_max;
	char		cval0[1024], cval1[1024];
	nc_type 	rettype;
	size_t		cursor_place[MAX_NC_DIMS];
	cv_converter	*convert_units_to_sec;

	NCDim *d;
	d = *(v->dim+dimid);

	/* Not enough data to analyze */
	if( d->size < 3 )
		return(TGRAN_SEC);

	verbose = 0;

	if( ! valid_udunits_pkg )
		return( 0 );

	if( (unit = ut_parse( unitsys, d->units, UT_ASCII )) == NULL ) {
		fprintf( stderr, "internal error: udu_calc_tgran with invalid unit string: %s\n",
			d->units );
		exit( -1 );
		}

	if( (seconds = ut_parse( unitsys, "seconds", UT_ASCII )) == NULL ) {
		fprintf( stderr, "internal error: udu_calc_tgran can't parse seconds unit string!\n" );
		exit( -1 );
		}
	
	/* Get converter to convert from "units" to "seconds" */
	if( ut_are_convertible( unit, seconds ) == 0 ) {
		/* Units are not convertible */
		return( TGRAN_SEC );
		}
	if( (convert_units_to_sec = ut_get_converter( unit, seconds )) == NULL ) {
		/* This shouldn't happen */
		return( TGRAN_SEC );
		}

	/* Get a delta time to analyze */
	for( ii=0L; ii<v->n_dims; ii++ )
		cursor_place[ii] = (int)((*(v->size+ii))/2.0);
	rettype = fi_dim_value( v, dimid, 1L, &tval0_user, cval0, &has_bounds, &bound_min, &bound_max, cursor_place );
	rettype = fi_dim_value( v, dimid, 2L, &tval1_user, cval1, &has_bounds, &bound_min, &bound_max, cursor_place );

	/* Convert time vals from user units to seconds */
	tval0_sec = cv_convert_double( convert_units_to_sec, tval0_user );
	tval1_sec = cv_convert_double( convert_units_to_sec, tval1_user );

	/* Free our units converter storage */
	cv_free( convert_units_to_sec );

	delta_sec = fabs(tval1_sec - tval0_sec);
	
	if( verbose ) 
		printf( "units: %s  t1: %lf t2: %lf delta-sec: %lf\n", d->units, tval0_user, tval1_user, delta_sec );

	if( delta_sec < 57. ) {
		if(verbose)
			printf("data is TGRAN_SEC\n");
		retval = TGRAN_SEC;
		}
	else if( delta_sec < 3590. ) {
		if(verbose)
			printf("data is TGRAN_MIN\n");
		retval = TGRAN_MIN;
		}
	else if( delta_sec < 86300. ) {
		if(verbose)
			printf("data is TGRAN_HOUR\n");
		retval = TGRAN_HOUR;
		}
	else if( delta_sec < 86395.*28. ) {
		if(verbose)
			printf("data is TGRAN_DAY\n");
		retval = TGRAN_DAY;
		}
	else if(  delta_sec < 86395.*365. ) {
		if(verbose)
			printf("data is TGRAN_MONTH\n");
		retval = TGRAN_MONTH;
		}
	else
		{
		if(verbose)
			printf("data is TGRAN_YEAR\n");
		retval = TGRAN_YEAR;
		}

	return( retval );
}

/******************************************************************************/
void udu_fmt_time( char *temp_string, size_t temp_string_len, double new_dimval, NCDim *dim, int include_granularity )
{
	static  ut_unit	*dataunits=NULL;
	int	year, month, day, hour, minute, debug;
	double	second;
	static  char last_units[1024];
	static	char months[12][4] = { "Jan\0", "Feb\0", "Mar\0", "Apr\0",
				       "May\0", "Jun\0", "Jul\0", "Aug\0",
				       "Sep\0", "Oct\0", "Nov\0", "Dec\0"};

	debug = 0;
	if( debug ) fprintf( stderr, "udu_fmt_time: entering with dim=%s, units=%s, value=%f\n",
		dim->name, dim->units, new_dimval );

	if( ! valid_udunits_pkg ) {
		snprintf( temp_string, temp_string_len-1, "%lg", new_dimval );
		return;
		}

	if( (strlen(dim->units) > 1023) || strcmp(dim->units,last_units) != 0 ) {
		if( dataunits != NULL )	/* see if left over from previous invocation */
			ut_free( dataunits );
		if( (dataunits = ut_parse( unitsys, dim->units, UT_ASCII )) == NULL ) {
			fprintf( stderr, "internal error: udu_fmt_time can't parse data unit string!\n" );
			fprintf( stderr, "problematic units: \"%s\"\n", dim->units );
			fprintf( stderr, "dim->name: %s  dim->timelike: %d\n", dim->name, dim->timelike );
			exit( -1 );
			}
		strncpy( last_units, dim->units, 1023 );
		}

	if( utCalendar2_cal( new_dimval, dataunits, &year, &month, &day, &hour, 
				&minute, &second, dim->calendar ) != 0 ) {
		fprintf( stderr, "internal error: udu_fmt_time can't convert to calendar value!\n");
		fprintf( stderr, "units: >%s<\n", dim->units );
		exit( -1 );
		}
	
	if( include_granularity ) {
		switch( dim->tgran ) {
			
			case TGRAN_YEAR:
			case TGRAN_MONTH:
			case TGRAN_DAY:
				snprintf( temp_string, temp_string_len-1, "%1d-%s-%04d", day, months[month-1], year );
				break;

			case TGRAN_HOUR:
			case TGRAN_MIN:
				snprintf( temp_string, temp_string_len-1, "%1d-%s-%04d %02d:%02d", day, 
						months[month-1], year, hour, minute );
				break;

			default:
				snprintf( temp_string, temp_string_len-1, "%1d-%s-%04d %02d:%02d:%02.0f",
					day, months[month-1], year, hour, minute, second );
			}
		}
	else
		{
		snprintf( temp_string, temp_string_len-1, "%1d-%s-%04d %02d:%02d:%02.0f",
				day, months[month-1], year, hour, minute, second );
		}
}

/******************************************************************************/
	static int 
is_unique( char *units )
{
	UniqList	*ul, *ul_new, *prev_ul;

	if( uniq == NULL ) {
		ul_new = (UniqList *)malloc( sizeof(UniqList) );
		ul_new->name = (char *)malloc(strlen(units)+1);
		strcpy( ul_new->name, units );
		ul_new->next = NULL;
		uniq = ul_new;
		return( TRUE );
		}
	
	ul = uniq;
	while( ul != NULL ) {
		if( strcmp( ul->name, units ) == 0 ) 
			return( FALSE );
		prev_ul = ul;
		ul = ul->next;
		}

	ul_new = (UniqList *)malloc( sizeof(UniqList) );
	ul_new->name = (char *)malloc(strlen(units)+1);
	strcpy( ul_new->name, units );
	ul_new->next = NULL;
	prev_ul->next = ul_new;
	return( TRUE );
}

/******************************************************************************/
#else

void udu_utinit( char *path )
{
	;
}

int udu_utistime( char *dimname, char *units )
{
	return( 0 );
}

int udu_calc_tgran( int fileid, NCVar *v, int dimid )
{
	return( 0 );
}

void udu_fmt_time( char *temp_string, size_t temp_string_len, double new_dimval, NCDim *dim, int include_granularity )
{
	snprintf( temp_string, temp_string_len-1, "%g", new_dimval );
}

#endif
