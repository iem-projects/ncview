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

/* define USE_DISK_FONTS */

#include "../ncview.includes.h"
#include "../ncview.defines.h"
#include "../ncview.protos.h"

extern  Options   options;

#include "../ncview.protos.h"
#include "helvR08.h"

typedef unsigned char rlpix;

typedef struct {
        int     llx, lly, urx, ury;
} my_Rect;

/* A single particular character in a font */
struct chr_struct {
        my_Rect         bb;
        int             number;
        unsigned char   *data;
};

typedef struct {
        my_Rect bb;
        int     width, height;
        int     ascent;
        int     descent;
        int     n_chars;
        struct chr_struct *chr;
} my_Font;

typedef struct {
	int             width, height;
	rlpix		*pixels;
	int		cur_x, cur_y;
	rlpix		pen_1;
	my_Font		*font;
} my_Raster;

static my_Raster *cbar_raster;
static my_Font   *cbar_font;

/*-------------------------------------------
 * Prototypes for routines local to this file
 */
static struct chr_struct *set_chr( int index, my_Font *font );
static void do_special( my_Raster *rast, char *s );
static void set_pixel( my_Raster *rast, int x, int y, rlpix color );
static void put_char( my_Raster *rast, char c );
static void emits( my_Raster *rast, char *s );
static my_Raster *init_rast( int nx, int ny );
static void clip( int *val, int lo, int hi );
static void move_to( my_Raster *rast, int x, int y );
static void genlevs( double mindat, double maxdat, int nlevels,
	double *start, int *nlevs, double *step );
static void nlev_from_step( double step, double mindat, double maxdat, int *nlev, double *start );
static void mynormalize( double value, double *mantissa, double *exponent );
static int string_width( char *s, my_Font *font );
static my_Font *font_from_include();

#ifdef USE_DISK_FONTS
static void get_properties(FILE *f, my_Font *font);
static void turn_to_bits( char c, unsigned char *b4, unsigned char *b3, unsigned char *b2, unsigned char *b1 );
static void set_line( char *line, int chr_width, unsigned char *place );
static my_Font *open_font( char *font_name );
static void process_font_file( FILE *f, my_Font *font );
static void get_a_char( FILE *f, char *firstline, my_Font *font, struct chr_struct *chr );
#endif

/*******************************************************************************************/
void cbar_init()
{
	if( options.debug ) fprintf( stderr, "cbar_init: entering\n" );
	/* Get the font info */
	/**  cbar_font   = open_font( "helvR08" );  **/
	cbar_font = font_from_include();

	cbar_raster = NULL;
	if( options.debug ) fprintf( stderr, "cbar_init: exiting\n" );
}

/*******************************************************************************************/
void cbar_make( int width, int height, int n_extra_colors, float user_min, float user_max, int transform )
{
	int	i, j, typical_label_width, nlev_target, nlev, sw, ptx, ptx_text, 
		vert_cut, pad_side;
	char	tstr[1024];
	double	val, start, step, xfrac, drange, normval;

	if( options.debug ) fprintf( stderr, "cbar_make: entering with width=%d height=%d n_extra_colors+%d user_min=%f user_max=%f\n",
		width, height, n_extra_colors, user_min, user_max );

	/* See if we ALREADY have a raster ... if so, clear it out
	 * before making the new one
	 */
	if( cbar_raster != NULL ) {
		if( options.debug ) fprintf( stderr, "cbar_make: clearing existing raster\n" );
		free( cbar_raster->pixels );
		free( cbar_raster );
		}

	/* Make our new raster, initialize its font */
	if( options.debug ) fprintf( stderr, "cbar_make: making new rast w=%d h=%d\n", width, height );
	cbar_raster = init_rast( width, height );
	cbar_raster->font = cbar_font;

	vert_cut = 11;	/* number of pixels (vertical) for labels */
	pad_side = 32;  /* number of pixels (horizontal) inward to begin color swatch; makes room for labels */

	/* Blank the image to begin with */
	if( options.debug ) fprintf( stderr, "cbar_make: erasing new raster\n" );
	for( j=0; j<height; j++ )
	for( i=0; i<width;  i++ ) {
		*(cbar_raster->pixels + i + j*width) = 1;
		}

	if( options.debug ) fprintf( stderr, "cbar_make: i loop from %d to %d; j loop from %d to %d\n", pad_side, width-pad_side, 0, height-vert_cut ); 
	for( j=0; j<height-vert_cut; j++ )
	for( i=pad_side; i<width-pad_side; i++ ) {
		normval = (double)(i-pad_side)/(double)(width-2*pad_side);
		if( transform == TRANSFORM_HI )
			normval = normval*normval*normval*normval;
		else if( transform == TRANSFORM_LOW ) {
			normval = sqrt(normval);
			normval = sqrt(normval);
			}

		if( options.invert_colors )
			normval = 1. - normval;

		*(cbar_raster->pixels + i + j*width) = (unsigned char)(normval*(double)options.n_colors + n_extra_colors);
		/* if( options.debug && (j==0)) fprintf( stderr, "cbar_make: value for i=%d is %d\n", i, *(cbar_raster->pixels + i + j*width)); */
		}

	/* Bail if the data is too weird to make labels for */
	if( user_max <= user_min ) {
		if( options.debug ) fprintf( stderr, "cbar_make: bailing since user_max <= user_min\n" );
		return;
		}

	/* Get levels to label */
	typical_label_width = 6*8;
	nlev_target = (int)((double)width/(double)typical_label_width) - 2;
	if( nlev_target < 2 ) {
		if( options.debug ) fprintf( stderr, "cbar_make: premature exit since window too narrow to support labels\n" );
		return;	/* window too narrow to have labels */
		}

	if( options.debug ) fprintf( stderr, "cbar_make: about to call genlevs with target=%d levels\n", nlev_target );
	genlevs( user_min, user_max, nlev_target,
		&start, &nlev, &step );
	if( options.debug ) fprintf( stderr, "cbar_make: back from genlevs, start=%f nlev=%d step=%f\n", start, nlev, step );

	drange = user_max - user_min;

	for( i=0; i<nlev; i++ ) {
		if( options.debug ) fprintf( stderr, "cbar_make: drawing label %d of %d\n", i, nlev );
		val = start + step*i;
		snprintf( tstr, 1020, "%g", val );
		sw = string_width( tstr, cbar_raster->font );
		xfrac = (val-user_min)/drange;
		if( (xfrac>=0.) && (xfrac<=1.)) {
			ptx = pad_side + (int)(xfrac*(double)(width-2*pad_side) + .5);
			ptx_text = ptx - (int)((double)sw * .5 + .5);
			if( ptx_text > 0 ) {
				move_to( cbar_raster, ptx_text, height-1 );
				emits( cbar_raster, tstr );
				}

			/* Draw a little tic mark */
			set_pixel( cbar_raster, ptx, cbar_raster->font->height-1, cbar_raster->pen_1 );
			set_pixel( cbar_raster, ptx, cbar_raster->font->height-2, cbar_raster->pen_1 );
			}
		}

	if( options.debug ) fprintf( stderr, "cbar_make: exiting\n" );
}

/*******************************************************************************************/
/* Returns 0 on success, -1 on failure.  Failure usually means that an expose
 * event was generated before the colorbar was initialized.
 */
int cbar_info( unsigned char **data, int *width, int *height )
{
	if( options.debug ) fprintf( stderr, "cbar_info: entering\n" );

	if( cbar_raster == NULL ) {
		if( options.debug ) fprintf( stderr, "cbar_info: cbar_raster is NULL, premature exit\n" );
		return(-1);
		}

	*data   = (unsigned char *)cbar_raster->pixels;
	*width  = cbar_raster->width;
	*height = cbar_raster->height;

	if( options.debug ) fprintf( stderr, "cbar_info: exiting, width=%d height=%d\n", *width, *height );
	return(0);
}

/*******************************************************************************************/
static struct chr_struct *set_chr( int index, my_Font *font )
{
	struct chr_struct *mc;
	int	i;

	if( font == NULL ) {
		fprintf( stderr, "?empty font passed to set_chr\n" );
		exit( -1 );
		}

	i  = 0;
	mc = font->chr;
	while( mc != NULL ) 
		{
		mc = font->chr+i;
		if( mc->number == index )
			return( mc );
		if( ++i == font->n_chars )
			{
			if( index != 10 )
				{
				fprintf( stderr, "bogus character passed to set_chr; id=%d\n", index );
				fprintf( stderr, "nchars=%d\n", font->n_chars );
				exit( -1 );
				}
			return( set_chr( 32, font) );
			}
		}

	fprintf( stderr, "map for character %d not found\n", index );
	return( set_chr( 32, font) );
}

/*******************************************************************************************
 * All this following code is for when you want to read a font from disk.
 * However, in ncview itself, I don't do this because you don't know if the
 * disk font will be available. Instead, I use defined arrays. But it's 
 * here for future use or debugging.
 */
#ifdef USE_DISK_FONTS
/*******************************************************************************************/

/*******************************************************************************************/
static void process_font_file( FILE *f, my_Font *font )
{
	char	line[132];
	int	i = 0;

	get_properties(f, font);

	fgets( line, 132, f );
	if( strncmp( line, "CHARS", 5 ) == 0 )
		sscanf( line, "%*s %d", &(font->n_chars) );
	else
		{
		fprintf( stderr, "?no `CHARS' line\n" );
		exit( -1 );
		}

	/* allocate space for the characters */
	font->chr = (struct chr_struct *)malloc( 
				font->n_chars*sizeof( struct chr_struct ));

	while( fgets(line, 132, f ) != NULL )
		{
		if( strncmp( line, "ENDFONT", 7 ) == 0 )
			return;
		get_a_char( f, line, font, font->chr+i );
		i++;
		}
}

/*************************************************************************************/
static void get_a_char( FILE *f, char *firstline, my_Font *font, struct chr_struct *chr )
{
	char	input_line[132];
	int	i, top_y;

	/* Make space for the character data */
	chr->data = (unsigned char *)malloc
		( font->width*font->height*sizeof( unsigned char ));
	if( chr->data == NULL)
		{
		fprintf( stderr, "error on allocation of char data\n" );
		exit( -1 );
		}

	for( i=0; i<font->width*font->height; i++ )
		*(chr->data+i) = 0;

	do
		fgets( input_line, 132, f );
	while
		((strncmp( input_line, "ENCODING", 8 ) != 0) && (input_line != NULL));

	if( input_line == NULL )
		{
		fprintf( stderr, "unexpected end of font file in get_char\n" );
		exit( -1 );
		}

	sscanf( input_line, "%*s %d", &(chr->number) );

	do
		fgets( input_line, 132, f );
	while
		((strncmp( input_line, "BBX", 3 ) != 0) && (input_line != NULL));

	if( input_line == NULL )
		{
		fprintf( stderr, "unexpected end of font file in get_char\n" );
		exit( -1 );
		}

	sscanf( input_line, "%*s %d %d %d %d", 
		&(chr->bb.urx),
		&(chr->bb.ury),
		&(chr->bb.llx),
		&(chr->bb.lly) );


	do
		fgets( input_line, 132, f );
	while
		((strncmp( input_line, "BITMAP", 6 ) != 0) && (input_line != NULL));

	if( input_line == NULL )
		{
		fprintf( stderr, "unexpected end of font file in get_char\n" );
		exit( -1 );
		}

	top_y = font->bb.ury - chr->bb.ury - (chr->bb.lly - font->bb.lly);
	for( i=0; i<chr->bb.ury; i++)
		{
		fgets( input_line, 132, f );
		set_line( input_line, chr->bb.urx, 
				chr->data+(i+top_y)*font->width );
		}

	/* get "endchar" string */
	fgets( input_line, 132, f );
	if( strncmp( input_line, "ENDCHAR", 7 ) != 0 )
		{
		fprintf( stderr, "?no `ENDCHAR' token found\n" );
		exit( -1 );
		}
}	

/********************************************************************************************/
static void set_line( char *line, int chr_width, unsigned char *place )
{
	int	i, j;
	unsigned char b1, b2, b3, b4;

	j=0;
	i=0;
	while( (! isspace(line[i]) ) && ( line[i] != '\0' ))
		{
		turn_to_bits( line[i], &b4, &b3, &b2, &b1 );
		if( j++ < chr_width )
			*place     = b4;
		if( j++ < chr_width )
			*(place+1) = b3;
		if( j++ < chr_width )
			*(place+2) = b2;
		if( j++ < chr_width )
			*(place+3) = b1;
		place += 4;
		i++;
		}
}

/********************************************************************************************/
static void turn_to_bits( char c, unsigned char *b4, unsigned char *b3, unsigned char *b2, unsigned char *b1 )
{
	switch( c )
		{
		case 'f':
		case 'F':
			*b4=1; *b3=1; *b2=1; *b1=1; break;

		case 'e':
		case 'E':
			*b4=1; *b3=1; *b2=1; *b1=0; break;

		case 'd':
		case 'D':
			*b4=1; *b3=1; *b2=0; *b1=1; break;

		case 'c':
		case 'C':
			*b4=1; *b3=1; *b2=0; *b1=0; break;

		case 'b':
		case 'B':
			*b4=1; *b3=0; *b2=1; *b1=1; break;

		case 'a':
		case 'A':
			*b4=1; *b3=0; *b2=1; *b1=0; break;

		case '9':
			*b4=1; *b3=0; *b2=0; *b1=1; break;

		case '8':
			*b4=1; *b3=0; *b2=0; *b1=0; break;

		case '7':
			*b4=0; *b3=1; *b2=1; *b1=1; break;

		case '6':
			*b4=0; *b3=1; *b2=1; *b1=0; break;

		case '5':
			*b4=0; *b3=1; *b2=0; *b1=1; break;

		case '4':
			*b4=0; *b3=1; *b2=0; *b1=0; break;

		case '3':
			*b4=0; *b3=0; *b2=1; *b1=1; break;

		case '2':
			*b4=0; *b3=0; *b2=1; *b1=0; break;

		case '1':
			*b4=0; *b3=0; *b2=0; *b1=1; break;

		case '0':
			*b4=0; *b3=0; *b2=0; *b1=0; break;
			
		default:
			fprintf( stderr, "can't turn %c into bits\n", c);
			exit( -1 );
			break;
		}
}

/***************************************************************************************/
static void get_properties(FILE *f, my_Font *font)
{
	char	line[132];

	while( fgets(line,132,f) != NULL )
		{
		if( strncmp( line, "FONTBOUNDINGBOX", 15) == 0 )
			{
			sscanf( line, "%*s %d %d %d %d", 
				&(font->bb.urx),
				&(font->bb.ury),
				&(font->bb.llx),
				&(font->bb.lly) );
			font->width  = font->bb.urx;
			font->height = font->bb.ury;
			}

		else if( strncmp( line, "FONT_ASCENT", 11 ) == 0 )
			sscanf( line, "%*s %d", &(font->ascent) );

		else if( strncmp( line, "FONT_DESCENT", 12 ) == 0 )
			sscanf( line, "%*s %d", &(font->descent) );

		else if( strncmp( line, "ENDPROPERTIES", 13 ) == 0 )
			return;
		}
	
	fprintf( stderr, "unexpected end of font file\n" );
	exit( -1 );
}

/*******************************************************************************************/
static my_Font *open_font( char *font_name )
{
	FILE	*font_file;
	char	filename[132], *font_dir, *getenv(), *DEFAULT_FONT_DIR=".";
	my_Font	*ret_val;

	snprintf( filename, 130, "%s/%s.bdf", DEFAULT_FONT_DIR, font_name );
	if( (font_file = fopen( filename, "r" )) == NULL )
		{
		font_dir = getenv( "TITLE_FONT_DIR" );
		if( font_dir == NULL ) 
			{
			fprintf(stderr, "can't open font file %s\n", filename );
			fprintf(stderr, "and environmental variable TITLE_FONT_DIR not set.\n" );
			exit( -1 );
			}
		else
			{
			snprintf( filename, 130, "%s/%s.bdf",
					font_dir, font_name );
			font_file = fopen( filename, "r" );
			if( font_file == NULL )
				{
				fprintf(stderr, "can't open font file %s\n", filename );
				exit( -1 );
				}
			}
		}

	if( (ret_val = (my_Font *)malloc( sizeof( my_Font ))) == NULL )
		{
		fprintf( stderr, "malloc failed on font allocation\n" );
		exit( -1 );
		}
	process_font_file( font_file, ret_val );
	fclose( font_file );

	return( ret_val );
}
/*******************************************************************************************/
/* This is the endif for USE_DISK_FONTS */
/*******************************************************************************************/
#endif

/************************************************************************************/
/* Can take special characters \sup{} and \sub{} */
static void emits( my_Raster *rast, char *s )
{
	int	i;

	for( i=0; i<strlen(s); i++ )
		{
		if( *(s+i) == '\\' )
			do_special( rast, s+i );
		else
			put_char( rast, *(s+i) );
		}
}

/************************************************************************************/
static void do_special( my_Raster *rast, char *s )
{
	int	offset, i, j, end_special;
	char	*s2;

	if( strncmp( s, "\\sup{", 5 ) == 0 )
		{
		offset = rast->font->height * .3;
		rast->cur_y -= offset;
		s2 = s + 5;
		i  = 0;
		while( *(s2+i) != '}' )
			{
			put_char( rast, *(s2+i) );
			i++;
			}
		end_special = 5 + i;
		rast->cur_y += offset;
		for( j=end_special; j<strlen(s); j++ )
			*(s+j-end_special) = *(s+j);
		*(s+j-end_special) = '\0';
		}

	else 	/* default case, just dump it straight */
		{
		for( i=0; i<strlen(s); i++ )
			put_char( rast, *(s+i) );
		}
}

/**********************************************************************************/
static void put_char( my_Raster *rast, char c )
{
	int	i, j, offset_y;
	struct 	chr_struct *chr;
	int	spacing;

	if( rast == NULL ) {
		fprintf( stderr, "null rast passed to put_char\n" );
		exit( -1 );
		}

	if( rast->font == NULL ) {
		fprintf( stderr, "null font passed to put_char\n" );
		exit( -1 );
		}

	offset_y = rast->font->bb.ury + rast->font->bb.lly;
	chr = set_chr( (int)c, rast->font );

	for( j=0; j < rast->font->height; j++ )
	for( i=0; i < rast->font->width;  i++ ) {
		if( *(chr->data+i+j*rast->font->width) == 1 )
			set_pixel( rast, rast->cur_x+i, rast->cur_y+j-offset_y,
						rast->pen_1 );
		}
		
	/* spaces should be at least as wide as an 'f' */
	spacing = 1;
	if( rast->font->height > 15 )
		spacing++;
	if( rast->font->height > 22 )
		spacing++;
	if( c == ' ' )
		{
		chr = set_chr( 'f', rast->font );
		rast->cur_x += chr->bb.urx + spacing;
		}
	else
		rast->cur_x += chr->bb.urx + spacing;
}

/**********************************************************************************/
static void set_pixel( my_Raster *rast, int x, int y, rlpix color )
{
	*(rast->pixels+x+y*rast->width) = color;
}

/**********************************************************************************/
static my_Raster *init_rast( int nx, int ny )
{
	my_Raster       *ret_val;

	ret_val = (my_Raster *)malloc( sizeof( my_Raster ));
	if( ret_val == NULL ) {
		fprintf( stderr, "failed allocation of rast structure\n" );
		exit( -1 );
		}
	ret_val->pixels = (rlpix *)malloc( nx*ny*sizeof(rlpix) );
	if( ret_val->pixels == NULL ) {
		fprintf( stderr, "failed on allocation of pixels\n" );
		fprintf( stderr, "requested size: %d\n", nx*ny );
		exit( -1 );
		}

	ret_val->pen_1 = 0;

	ret_val->font = NULL;

	ret_val->width  = nx;
	ret_val->height = ny;

	return( ret_val );
}

/**********************************************************************************/
static void clip( int *val, int lo, int hi )
{
	if( *val < lo )
		*val = lo;
	else if( *val > hi )
		*val = hi;
}

/**********************************************************************************/
static void move_to( my_Raster *rast, int x, int y )
{
	clip( &x, 0, rast->width );
	clip( &y, 0, rast->height );
	rast->cur_x = x;
	rast->cur_y = y;
}

/*========================================================================================*/
static void mynormalize( double value, double *mantissa, double *exponent )
{
	double q;

	if( value == 0.0 ) {
	        q         = 0.;
	        *mantissa = 0.;
	        *exponent = 0.;
		}
	else 
		{
	        q        = log10(value);
	        *exponent = (double)((int)q);
	        *mantissa = value/pow(10.0, *exponent);
		}

	if( q < 0.0) {
	        *exponent = *exponent - 1.0;
	        *mantissa = *mantissa * 10.;
		}
}

/*=========================================================================================*/
static void nlev_from_step( double step, double mindat, double maxdat, int *nlev, double *start )
{
	double	cursor;
	int	n0, n1;

	/* Go to multiple of step that is <= mindat */
	n0 = (int)(maxdat/step);
	cursor = (double)n0 * step;
	while( cursor > mindat ) {
		n0--;
		cursor = (double)n0 * step;
		}

	/* Go to multiple of step that is >= mindat */
	n1 = (int)(mindat/step);
	cursor = (double)n1 * step;
	while( cursor < maxdat ) {
		n1++;
		cursor = (double)n1 * step;
		}

	*nlev = n1 - n0 + 1;
	*start = (double)n0 * step;
}

/*=========================================================================================*/
static void genlevs( double mindat, double maxdat, int nlevels,
	double *start, int *nlevs, double *step )
{
	int	*trial, ntrial, i, n1, n2;
	double	range, trialstep, mant, expon, fact;
	int	found;
	double	start1, start2, step1, step2;

	if( maxdat > 9.e35 ) {
		printf( "warning: invalid maximum data value for colorbar: %lf.  Not setting colorbar\n", maxdat );
		*start = mindat;
		*nlevs = 10;
		*step = 1;
		return;
		}

	/* ------------------------------------------------
	 * This holds the possible intervals which the step
	 * between contours can be.  It is normalized.
	 * ------------------------------------------------*/
	ntrial = 4;
	trial  = (int *)malloc( sizeof(int)*ntrial );
	trial[0] = 1; trial[1] = 2; trial[2] = 5; trial[3] = 10;

	range     = maxdat-mindat;
	trialstep = range/(double)(nlevels-1);

	/*-----------------------------------------------
 	 * Normalize trialstep to the range [1.0,9.99999]
	 *-----------------------------------------------*/
	mynormalize( trialstep, &mant, &expon);

	fact = pow(10.0, expon);

	*step  = -1.0;
	i      = 0;
	found  = FALSE;
	while( (i < ntrial-1) && (! found)) {
		if( (mant >= trial[i]) && (mant <= trial[i+1]) ) {
			found = TRUE;
			/* -----------------------------------------
			 * Add 2 because we add levels on either end
			 * to completely enclose the range of data.
			 *-----------------------------------------*/
			step1 = trial[i  ]*fact;
			step2 = trial[i+1]*fact;
			nlev_from_step( step1, mindat, maxdat, &n1, &start1 );
			nlev_from_step( step2, mindat, maxdat, &n2, &start2 );
			if (abs(n1-nlevels) <= abs(n2-nlevels)) {
				*step  = step1;
				*nlevs = n1;
				*start = start1;
				}
			else 
				{
				*step  = step2;
				*nlevs = n2;
				*start = start2;
				}
			}
		i++;
		}

	if( *step == -1.0) {
		fprintf( stderr, "*** genlevs: step =-1, failed\n" );
		fprintf( stderr, "*** genlevs: Input data: mindat=%lf maxdat=%lf nlevels=%d\n", mindat, maxdat, nlevels );
		fprintf( stderr, "*** trialstep=%lf, mant=%lf, exp=%lf\n",trialstep,mant,expon);
		exit( -1 );
		}
}

/*====================================================================================*/
static int string_width( char *s, my_Font *font )
{
	int	i, ret_val=0;
	struct	chr_struct *chr;
	int	spacing;

	if( s == NULL )
		return( 0 );

	if( font == NULL )
		{
		fprintf( stderr, "null font passed to string_width\n" );
		exit( -1 );
		}

	for( i=0; i<strlen(s); i++ )
		{
		if( *(s+i) == '\\' )
			{
			while( *(s+i) != '}' )
				i++;
			}
		else
			{
			chr = set_chr( (int)(*(s+i)), font );
			spacing = 1;
			if( font->height > 15 )
				spacing++;
			if( font->height > 22 )
				spacing++;
			if( *(s+i) == ' ' )
				chr = set_chr( 'f', font );
			ret_val += chr->bb.urx + spacing;
			}
		}

	return( ret_val );
}

/******************************************************************************************/
static my_Font *font_from_include()
{
	int	i, j, wh;
	my_Font	*retval;
	struct chr_struct *chr;

	retval = (my_Font *)malloc( sizeof(my_Font) );

	retval->bb.llx = font_helvR08_bb[0];
	retval->bb.lly = font_helvR08_bb[1];
	retval->bb.urx = font_helvR08_bb[2];
	retval->bb.ury = font_helvR08_bb[3];

	retval->width  = font_helvR08_widthheight[0];
	retval->height = font_helvR08_widthheight[1];
	retval->ascent  = font_helvR08_ascentdescent[0];
	retval->descent = font_helvR08_ascentdescent[1];
	retval->n_chars = font_helvR08_nchars[0];

	retval->chr = (struct chr_struct *)malloc(
		retval->n_chars*sizeof( struct chr_struct ));
	
	for( i=0; i<retval->n_chars; i++ ) {
		chr = retval->chr + i;
		chr->number = font_helvR08_char_number[i];
		}
	
	for( i=0; i<retval->n_chars; i++ ) {
		chr = retval->chr + i;
		chr->bb.llx = font_helvR08_char_bbllx[i];
		}
	
	for( i=0; i<retval->n_chars; i++ ) {
		chr = retval->chr + i;
		chr->bb.lly = font_helvR08_char_bblly[i];
		}
	
	for( i=0; i<retval->n_chars; i++ ) {
		chr = retval->chr + i;
		chr->bb.urx = font_helvR08_char_bburx[i];
		}
	
	for( i=0; i<retval->n_chars; i++ ) {
		chr = retval->chr + i;
		chr->bb.ury = font_helvR08_char_bbury[i];
		}

	wh = retval->width*retval->height;
	for( i=0; i<retval->n_chars; i++ ) {
		chr = retval->chr + i;
		chr->data = (unsigned char *)malloc( sizeof(unsigned char)*wh );
		for( j=0; j<wh; j++ ) {
			chr->data[j] = font_helvR08_char_data[j + i*wh];
			}
		}

	return( retval );
}

